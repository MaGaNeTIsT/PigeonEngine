// ShaderCompiler
// Parameter driven shader compiler: one compile description json in, N variant
// shader artifacts plus one build manifest out. The tool never scans directories,
// never watches files and never caches results - scheduling belongs to the caller.
//
// Usage: ShaderCompiler.exe <CompileJson>

#include "MCJsonParser.h"
#include "MCDxcCompiler.h"
#include "MCLogger.h"

#include <Config/EngineShaderConfig.h>

#include <document.h>
#include <error/en.h>
#include <prettywriter.h>
#include <stringbuffer.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <iomanip>


// Reserved variant name used by the variant compiled without any define override.
static constexpr const char* BaseVariantName = "Base";

static constexpr const char* DefaultManifestExtn = ".Manifest.json";
static constexpr const char* DefaultReflectionExtn = ".Reflection.json";
static constexpr const char* DefaultLogFileName = "ShaderCompiler.log";

enum class MCShaderStageType : uint8_t
{
	ShaderStage_VS,
	ShaderStage_PS,
	ShaderStage_CS,
};

struct MCVariantDesc
{
	std::string			Name			= "";
	std::vector<MC::MCDefine>	Defines			= {};
	std::vector<MC::MCReplace>	Replaces		= {};
	bool				bIsVariant		= true;
};

struct MCCompileArgs
{
	std::string			RootDir			= "";
	std::string			JsonFilePath		= "";
	std::string			Stage			= "";
	std::string			OutputDir		= "";
	std::string			ShaderIncludeDir	= "";
	std::string			LogPath			= "";
	std::string			ManifestFilePath	= "";
	std::vector<MCVariantDesc>	Variants		= {};
	bool				bDx11			= false;
	bool				bSm66			= false;
	bool				bDebug			= false;
	bool				bDumpHlsl		= false;
};

struct MCVariantResult
{
	std::string			Name			= "";
	std::string			DefHash			= "";
	std::string			RplHash			= "";
	std::string			BinaryFile		= "";
	std::string			ReflectionFile		= "";
	std::string			PdbFile			= "";
	std::string			Errors			= "";
	std::vector<MC::MCDefine>	Defines			= {};
	bool				bSuccess		= false;
};

// ---------------------------------------------------------------------------
// Command line
// ---------------------------------------------------------------------------
static void PrintUsage() noexcept
{
	std::cout <<
		"Usage: ShaderCompiler.exe <CompileJson>\n"
		"  <CompileJson>   Compile description json, fields:\n"
		"                    RootDir, JsonFilePath, Stage, OutputDir, ShaderIncludeDir\n"
		"                    Dx11, Sm66, Debug, DumpHlsl, LogPath, ManifestFilePath\n"
		"                    Variants: [ { Name, IsVariant, Defines, Replaces } ]\n";
}

static bool IsUsageRequested(int InArgc, char** InArgv) noexcept
{
	if (InArgc != 2)
	{
		return false;
	}
	const std::string Argument = InArgv[1];
	return ((Argument == "--help") || (Argument == "-h"));
}

static bool ParseCommandLine(int InArgc, char** InArgv, std::string& OutCompileJsonPath) noexcept
{
	if (InArgc != 2)
	{
		return false;
	}
	OutCompileJsonPath = InArgv[1];
	return (!OutCompileJsonPath.empty());
}

// ---------------------------------------------------------------------------
// File helpers
// ---------------------------------------------------------------------------
static bool ReadTextFile(const std::string& InPath, std::string& OutText) noexcept
{
	std::ifstream FileStream(InPath, std::ios::binary);
	if (!FileStream.is_open())
	{
		return false;
	}
	std::ostringstream StrStream;
	StrStream << FileStream.rdbuf();
	OutText = StrStream.str();
	return true;
}

static bool WriteTextFile(const std::string& InPath, const std::string& InText) noexcept
{
	std::ofstream FileStream(InPath, std::ios::binary);
	if (!FileStream.is_open())
	{
		return false;
	}
	FileStream << InText;
	return true;
}

static bool WriteBinaryFile(const std::string& InPath, const std::vector<uint8_t>& InBytes) noexcept
{
	std::ofstream FileStream(InPath, std::ios::binary);
	if (!FileStream.is_open())
	{
		return false;
	}
	FileStream.write(reinterpret_cast<const char*>(InBytes.data()), static_cast<std::streamsize>(InBytes.size()));
	return true;
}

// ---------------------------------------------------------------------------
// Path helpers - every relative path of the compile json is resolved against
// the current working directory.
// ---------------------------------------------------------------------------
static std::string NormalizeDir(const std::string& InDir) noexcept
{
	if (InDir.empty())
	{
		return "";
	}
	std::error_code ErrorCode;
	std::filesystem::path AbsoluteDir = std::filesystem::absolute(InDir, ErrorCode);
	if (ErrorCode)
	{
		return "";
	}
	std::string DirStr = AbsoluteDir.generic_string();
	if ((!DirStr.empty()) && (DirStr.back() != '/'))
	{
		DirStr += "/";
	}
	return DirStr;
}

static std::string ResolveFilePath(const std::string& InPath) noexcept
{
	std::error_code ErrorCode;
	std::filesystem::path AbsolutePath = std::filesystem::absolute(InPath, ErrorCode);
	if (ErrorCode)
	{
		return "";
	}
	return AbsolutePath.generic_string();
}

static std::string ParentDirOf(const std::string& InFilePath) noexcept
{
	std::filesystem::path ParentPath = std::filesystem::path(InFilePath).parent_path();
	if (ParentPath.empty())
	{
		return "";
	}
	std::string DirStr = ParentPath.generic_string();
	if ((!DirStr.empty()) && (DirStr.back() != '/'))
	{
		DirStr += "/";
	}
	return DirStr;
}

static bool EnsureDirectory(const std::string& InDir, std::string& OutError) noexcept
{
	if (InDir.empty())
	{
		return true;
	}
	std::error_code ErrorCode;
	std::filesystem::create_directories(InDir, ErrorCode);
	if (ErrorCode)
	{
		OutError = "cannot create directory: " + InDir;
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Stage / target helpers
// ---------------------------------------------------------------------------
static bool ParseShaderStage(const std::string& InStageStr, MCShaderStageType& OutStage) noexcept
{
	if (InStageStr == "VS")
	{
		OutStage = MCShaderStageType::ShaderStage_VS;
		return true;
	}
	if (InStageStr == "PS")
	{
		OutStage = MCShaderStageType::ShaderStage_PS;
		return true;
	}
	if (InStageStr == "CS")
	{
		OutStage = MCShaderStageType::ShaderStage_CS;
		return true;
	}
	return false;
}

static std::string StageSuffixOf(MCShaderStageType InStage) noexcept
{
	switch (InStage)
	{
		case MCShaderStageType::ShaderStage_VS:
		{
			return MC_SHADER_VS_SUFFIX;
		}
		case MCShaderStageType::ShaderStage_PS:
		{
			return MC_SHADER_PS_SUFFIX;
		}
		case MCShaderStageType::ShaderStage_CS:
		{
			return MC_SHADER_CS_SUFFIX;
		}
	}
	return "";
}

static std::string ShaderModelOf(const MCCompileArgs& InArgs) noexcept
{
	if (InArgs.bDx11)
	{
		return "5_0";
	}
	if (InArgs.bSm66)
	{
		return "6_6";
	}
	return "6_0";
}

static std::string ProfileOf(MCShaderStageType InStage, const MCCompileArgs& InArgs) noexcept
{
	std::string PrefixStr = "";
	switch (InStage)
	{
		case MCShaderStageType::ShaderStage_VS:
		{
			PrefixStr = "vs";
			break;
		}
		case MCShaderStageType::ShaderStage_PS:
		{
			PrefixStr = "ps";
			break;
		}
		case MCShaderStageType::ShaderStage_CS:
		{
			PrefixStr = "cs";
			break;
		}
	}
	return PrefixStr + "_" + ShaderModelOf(InArgs);
}

static std::string BinaryExtnOf(const MCCompileArgs& InArgs) noexcept
{
	return InArgs.bDx11 ? ".dxbc" : ".dxil";
}

static std::string TargetDescOf(const MCCompileArgs& InArgs) noexcept
{
	if (InArgs.bDx11)
	{
		return "DX11 (DXBC/SM5.0)";
	}
	if (InArgs.bSm66)
	{
		return "DX12 (DXIL/SM6.6)";
	}
	return "DX12 (DXIL/SM6.0)";
}

// ---------------------------------------------------------------------------
// Build identity helpers - the hashes only tag the manifest and the log, the
// reflection file name is decided by the compile description itself.
// ---------------------------------------------------------------------------
static uint32_t HashFnv1a32(const std::string& InText) noexcept
{
	uint32_t HashValue = 2166136261u;
	for (char Ch : InText)
	{
		HashValue ^= static_cast<uint32_t>(static_cast<uint8_t>(Ch));
		HashValue *= 16777619u;
	}
	return HashValue;
}

static std::string MakeHashHex(uint32_t InHashValue) noexcept
{
	std::ostringstream StrStream;
	StrStream << std::hex << std::setw(8) << std::setfill('0') << InHashValue;
	return StrStream.str();
}

static std::string MakeDefinesHash(const std::vector<MC::MCDefine>& InDefines) noexcept
{
	std::vector<std::string> Lines;
	Lines.reserve(InDefines.size());
	for (const MC::MCDefine& Def : InDefines)
	{
		Lines.emplace_back(Def.Name + "=" + std::to_string(Def.Value));
	}
	std::sort(Lines.begin(), Lines.end());

	std::string JoinedStr = "";
	for (const std::string& Line : Lines)
	{
		if (!JoinedStr.empty())
		{
			JoinedStr += ";";
		}
		JoinedStr += Line;
	}
	return MakeHashHex(HashFnv1a32(JoinedStr));
}

static std::string MakeReplacesHash(const std::vector<MC::MCReplace>& InReplaces) noexcept
{
	std::vector<std::string> Lines;
	Lines.reserve(InReplaces.size());
	for (const MC::MCReplace& Rpl : InReplaces)
	{
		Lines.emplace_back(Rpl.Name + "=" + Rpl.Element);
	}
	std::sort(Lines.begin(), Lines.end());

	std::string JoinedStr = "";
	for (const std::string& Line : Lines)
	{
		if (!JoinedStr.empty())
		{
			JoinedStr += ";";
		}
		JoinedStr += Line;
	}
	return MakeHashHex(HashFnv1a32(JoinedStr));
}

// ---------------------------------------------------------------------------
// Compile json parsing
// ---------------------------------------------------------------------------
static bool ReadJsonString(const rapidjson::Value& InJson, const char* InMemberName, std::string& OutValue) noexcept
{
	if (InJson.HasMember(InMemberName) && InJson[InMemberName].IsString())
	{
		OutValue = InJson[InMemberName].GetString();
		return true;
	}
	return false;
}

static bool ReadJsonBool(const rapidjson::Value& InJson, const char* InMemberName, bool& OutValue) noexcept
{
	if (InJson.HasMember(InMemberName) && InJson[InMemberName].IsBool())
	{
		OutValue = InJson[InMemberName].GetBool();
		return true;
	}
	return false;
}

static bool ReadJsonUint(const rapidjson::Value& InJson, const char* InMemberName, uint32_t& OutValue) noexcept
{
	if (!InJson.HasMember(InMemberName))
	{
		return false;
	}
	if (InJson[InMemberName].IsUint())
	{
		OutValue = InJson[InMemberName].GetUint();
		return true;
	}
	if (InJson[InMemberName].IsInt())
	{
		const int IntValue = InJson[InMemberName].GetInt();
		if (IntValue < 0)
		{
			return false;
		}
		OutValue = static_cast<uint32_t>(IntValue);
		return true;
	}
	return false;
}

static bool IsValidVariantName(const std::string& InName) noexcept
{
	if (InName.empty())
	{
		return false;
	}
	for (char Ch : InName)
	{
		const bool bIsDigit = (Ch >= '0') && (Ch <= '9');
		const bool bIsUpper = (Ch >= 'A') && (Ch <= 'Z');
		const bool bIsLower = (Ch >= 'a') && (Ch <= 'z');
		if (!(bIsDigit || bIsUpper || bIsLower || (Ch == '_')))
		{
			return false;
		}
	}
	return true;
}

static bool ParseVariantDefines(const rapidjson::Value& InJson, std::vector<MC::MCDefine>& OutDefines, std::string& OutError) noexcept
{
	const char* DefinesTxt = "Defines";
	const char* DefineNameTxt = "Name";
	const char* DefineValueTxt = "Value";

	if (!InJson.HasMember(DefinesTxt))
	{
		return true;
	}
	if (!InJson[DefinesTxt].IsArray())
	{
		OutError = "'Defines' must be an array";
		return false;
	}
	for (const auto& DefElem : InJson[DefinesTxt].GetArray())
	{
		if (!DefElem.IsObject())
		{
			OutError = "'Defines' entries must be objects";
			return false;
		}
		std::string DefName = "";
		if (!ReadJsonString(DefElem, DefineNameTxt, DefName) || DefName.empty())
		{
			OutError = "'Defines' entry missing valid 'Name'";
			return false;
		}
		int DefValue = 1;
		if (DefElem.HasMember(DefineValueTxt))
		{
			if (!DefElem[DefineValueTxt].IsInt())
			{
				OutError = "'Defines' entry '" + DefName + "' has invalid 'Value' (int expected)";
				return false;
			}
			DefValue = DefElem[DefineValueTxt].GetInt();
		}
		MC::MCDefine Def;
		Def.Name = DefName;
		Def.Value = DefValue;
		OutDefines.emplace_back(std::move(Def));
	}
	return true;
}

static bool ParseVariantReplaces(const rapidjson::Value& InJson, std::vector<MC::MCReplace>& OutReplaces, std::string& OutError) noexcept
{
	const char* ReplacesTxt = "Replaces";
	const char* ReplaceNameTxt = "Name";
	const char* ReplaceElementTxt = "Element";

	if (!InJson.HasMember(ReplacesTxt))
	{
		return true;
	}
	if (!InJson[ReplacesTxt].IsArray())
	{
		OutError = "'Replaces' must be an array";
		return false;
	}
	for (const auto& RplElem : InJson[ReplacesTxt].GetArray())
	{
		if (!RplElem.IsObject())
		{
			OutError = "'Replaces' entries must be objects";
			return false;
		}
		std::string RplName = "";
		if (!ReadJsonString(RplElem, ReplaceNameTxt, RplName) || RplName.empty())
		{
			OutError = "'Replaces' entry missing valid 'Name'";
			return false;
		}
		std::string RplElement = "";
		if (!ReadJsonString(RplElem, ReplaceElementTxt, RplElement))
		{
			OutError = "'Replaces' entry '" + RplName + "' missing valid 'Element'";
			return false;
		}
		MC::MCReplace Rpl;
		Rpl.Name = RplName;
		Rpl.Element = RplElement;
		OutReplaces.emplace_back(std::move(Rpl));
	}
	return true;
}

static bool ParseVariantDesc(const rapidjson::Value& InJson, MCVariantDesc& OutVariant, std::string& OutError) noexcept
{
	if (!InJson.IsObject())
	{
		OutError = "'Variants' entries must be objects";
		return false;
	}
	if (!ReadJsonString(InJson, "Name", OutVariant.Name) || OutVariant.Name.empty())
	{
		OutError = "'Variants' entry missing valid 'Name'";
		return false;
	}
	if (!IsValidVariantName(OutVariant.Name))
	{
		OutError = "'Variants' entry name '" + OutVariant.Name + "' must be file name safe ([A-Za-z0-9_])";
		return false;
	}
	ReadJsonBool(InJson, "IsVariant", OutVariant.bIsVariant);
	if (!ParseVariantDefines(InJson, OutVariant.Defines, OutError))
	{
		return false;
	}
	if (!ParseVariantReplaces(InJson, OutVariant.Replaces, OutError))
	{
		return false;
	}

	if (!OutVariant.bIsVariant)
	{
		if (OutVariant.Name != BaseVariantName)
		{
			OutError = "the variant without defines must be named '" + std::string(BaseVariantName) + "', got '" + OutVariant.Name + "'";
			return false;
		}
		if ((!OutVariant.Defines.empty()) || (!OutVariant.Replaces.empty()))
		{
			OutError = "the variant '" + OutVariant.Name + "' must not carry 'Defines' / 'Replaces'";
			return false;
		}
	}
	else
	{
		if (OutVariant.Name == BaseVariantName)
		{
			OutError = "'" + std::string(BaseVariantName) + "' is reserved for the variant without defines";
			return false;
		}
		if (OutVariant.Defines.empty() && OutVariant.Replaces.empty())
		{
			MC::MCLogger& MCLog = MC::MCLogger::Get();
			MCLog.Warning("variant '" + OutVariant.Name + "' carries neither 'Defines' nor 'Replaces', it is compiled as a duplicate of '" + std::string(BaseVariantName) + "'");
		}
	}
	return true;
}

static bool ParseCompileArgs(const rapidjson::Document& InDoc, MCCompileArgs& OutArgs, std::string& OutError) noexcept
{
	const char* VariantsTxt = "Variants";

	if (!InDoc.IsObject())
	{
		OutError = "compile json must be an object";
		return false;
	}
	ReadJsonString(InDoc, "RootDir", OutArgs.RootDir);
	ReadJsonString(InDoc, "JsonFilePath", OutArgs.JsonFilePath);
	ReadJsonString(InDoc, "Stage", OutArgs.Stage);
	ReadJsonString(InDoc, "OutputDir", OutArgs.OutputDir);
	ReadJsonString(InDoc, "ShaderIncludeDir", OutArgs.ShaderIncludeDir);
	ReadJsonString(InDoc, "LogPath", OutArgs.LogPath);
	ReadJsonString(InDoc, "ManifestFilePath", OutArgs.ManifestFilePath);
	ReadJsonBool(InDoc, "Dx11", OutArgs.bDx11);
	ReadJsonBool(InDoc, "Sm66", OutArgs.bSm66);
	ReadJsonBool(InDoc, "Debug", OutArgs.bDebug);
	ReadJsonBool(InDoc, "DumpHlsl", OutArgs.bDumpHlsl);

	if (InDoc.HasMember(VariantsTxt))
	{
		if (!InDoc[VariantsTxt].IsArray())
		{
			OutError = "'Variants' must be an array";
			return false;
		}
		for (const auto& VariantElem : InDoc[VariantsTxt].GetArray())
		{
			MCVariantDesc Variant;
			if (!ParseVariantDesc(VariantElem, Variant, OutError))
			{
				return false;
			}
			OutArgs.Variants.emplace_back(std::move(Variant));
		}
		if (OutArgs.Variants.empty())
		{
			OutError = "'Variants' must not be empty, omit the field to compile '" + std::string(BaseVariantName) + "' only";
			return false;
		}
	}
	else
	{
		MCVariantDesc BaseVariant;
		BaseVariant.Name = BaseVariantName;
		BaseVariant.bIsVariant = false;
		OutArgs.Variants.emplace_back(std::move(BaseVariant));
	}
	return true;
}

static bool ValidateCompileArgs(MCCompileArgs& InOutArgs, std::string& OutError) noexcept
{
	if (InOutArgs.RootDir.empty())
	{
		OutError = "missing 'RootDir'";
		return false;
	}
	if (InOutArgs.JsonFilePath.empty())
	{
		OutError = "missing 'JsonFilePath'";
		return false;
	}
	if (InOutArgs.Stage.empty())
	{
		OutError = "missing 'Stage'";
		return false;
	}
	if (InOutArgs.OutputDir.empty())
	{
		OutError = "missing 'OutputDir'";
		return false;
	}
	if (std::filesystem::path(InOutArgs.JsonFilePath).is_absolute())
	{
		OutError = "'JsonFilePath' must be relative to 'RootDir'";
		return false;
	}

	MCShaderStageType StageType = MCShaderStageType::ShaderStage_VS;
	if (!ParseShaderStage(InOutArgs.Stage, StageType))
	{
		OutError = "'Stage' must be one of \"VS\" / \"PS\" / \"CS\", got '" + InOutArgs.Stage + "'";
		return false;
	}
	if (InOutArgs.bDx11 && InOutArgs.bSm66)
	{
		MC::MCLogger& MCLog = MC::MCLogger::Get();
		MCLog.Warning("'Dx11' and 'Sm66' are mutually exclusive, 'Dx11' wins");
	}

	const std::string RootDirStr = NormalizeDir(InOutArgs.RootDir);
	if (RootDirStr.empty())
	{
		OutError = "cannot resolve 'RootDir': " + InOutArgs.RootDir;
		return false;
	}
	InOutArgs.RootDir = RootDirStr;

	const std::string EntryJsonPath = InOutArgs.RootDir + InOutArgs.JsonFilePath;
	std::error_code ErrorCode;
	if (!std::filesystem::is_regular_file(EntryJsonPath, ErrorCode))
	{
		OutError = "entry json not found: " + EntryJsonPath;
		return false;
	}

	const std::string OutputDirStr = NormalizeDir(InOutArgs.OutputDir);
	if (OutputDirStr.empty())
	{
		OutError = "cannot resolve 'OutputDir': " + InOutArgs.OutputDir;
		return false;
	}
	InOutArgs.OutputDir = OutputDirStr;
	if (!EnsureDirectory(InOutArgs.OutputDir, OutError))
	{
		return false;
	}

	if (InOutArgs.ShaderIncludeDir.empty())
	{
		InOutArgs.ShaderIncludeDir = InOutArgs.RootDir;
	}
	else
	{
		const std::string IncludeDirStr = NormalizeDir(InOutArgs.ShaderIncludeDir);
		if (IncludeDirStr.empty())
		{
			OutError = "cannot resolve 'ShaderIncludeDir': " + InOutArgs.ShaderIncludeDir;
			return false;
		}
		InOutArgs.ShaderIncludeDir = IncludeDirStr;
	}

	if (InOutArgs.LogPath.empty())
	{
		InOutArgs.LogPath = InOutArgs.OutputDir + DefaultLogFileName;
	}
	else
	{
		const std::string LogPathStr = ResolveFilePath(InOutArgs.LogPath);
		if (LogPathStr.empty())
		{
			OutError = "cannot resolve 'LogPath': " + InOutArgs.LogPath;
			return false;
		}
		InOutArgs.LogPath = LogPathStr;
		if (!EnsureDirectory(ParentDirOf(InOutArgs.LogPath), OutError))
		{
			return false;
		}
	}

	if (!InOutArgs.ManifestFilePath.empty())
	{
		const std::string ManifestPathStr = ResolveFilePath(InOutArgs.ManifestFilePath);
		if (ManifestPathStr.empty())
		{
			OutError = "cannot resolve 'ManifestFilePath': " + InOutArgs.ManifestFilePath;
			return false;
		}
		InOutArgs.ManifestFilePath = ManifestPathStr;
		if (!EnsureDirectory(ParentDirOf(InOutArgs.ManifestFilePath), OutError))
		{
			return false;
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// Entry json probe - the only fields the tool reads from the shader json, they
// drive the artifact naming and the compiler entry point.
// ---------------------------------------------------------------------------
static bool ProbeEntryJson(const std::string& InFullJsonPath, std::string& OutShaderName, std::string& OutEntryName, std::string& OutError) noexcept
{
	std::string JsonText = "";
	if (!ReadTextFile(InFullJsonPath, JsonText))
	{
		OutError = "cannot read entry json: " + InFullJsonPath;
		return false;
	}

	rapidjson::Document Doc;
	Doc.Parse(JsonText.c_str());
	if (Doc.HasParseError())
	{
		OutError = std::string("entry json parse error: ") + rapidjson::GetParseError_En(Doc.GetParseError());
		return false;
	}
	if (!(Doc.HasMember("Name") && Doc["Name"].IsString() && (Doc["Name"].GetStringLength() > 0)))
	{
		OutError = "entry json missing valid 'Name': " + InFullJsonPath;
		return false;
	}
	if (!(Doc.HasMember("Entry") && Doc["Entry"].IsString() && (Doc["Entry"].GetStringLength() > 0)))
	{
		OutError = "entry json missing valid 'Entry': " + InFullJsonPath;
		return false;
	}

	OutShaderName = Doc["Name"].GetString();
	OutEntryName = Doc["Entry"].GetString();
	return true;
}

static std::string ResolveManifestPath(const MCCompileArgs& InArgs, const std::string& InShaderName, MCShaderStageType InStage) noexcept
{
	if (!InArgs.ManifestFilePath.empty())
	{
		return InArgs.ManifestFilePath;
	}
	// The stage token carries no separator of its own, so the underscore here is the only
	// one: <ShaderName>_<Token>, and the binary is <ShaderName>_<Variant>_<Token>.
	return InArgs.OutputDir + InShaderName + "_" + StageSuffixOf(InStage) + DefaultManifestExtn;
}

// ---------------------------------------------------------------------------
// Reflection layout - the constant buffer layout reported by the compiler
// reflection is written back into the reflection json. Zero means "unknown"
// and CompilerCulled means "not present in the compiled shader".
// ---------------------------------------------------------------------------
static const MC::MCDxcCbLayout* FindCbLayout(const std::vector<MC::MCDxcCbLayout>& InLayouts, const std::string& InCbName) noexcept
{
	for (const MC::MCDxcCbLayout& Layout : InLayouts)
	{
		if (Layout.Name == InCbName)
		{
			return &Layout;
		}
	}
	return nullptr;
}

static const MC::MCDxcCbVariable* FindCbVariable(const MC::MCDxcCbLayout& InLayout, const std::string& InVariableName) noexcept
{
	for (const MC::MCDxcCbVariable& Variable : InLayout.Variables)
	{
		if (Variable.Name == InVariableName)
		{
			return &Variable;
		}
	}
	return nullptr;
}

static void SetJsonUint(rapidjson::Value& InOutJson, const char* InMemberName, uint32_t InValue, rapidjson::Document::AllocatorType& InAllocator) noexcept
{
	if (InOutJson.HasMember(InMemberName))
	{
		InOutJson[InMemberName].SetUint(static_cast<unsigned>(InValue));
	}
	else
	{
		rapidjson::Value MemberNameValue(InMemberName, InAllocator);
		rapidjson::Value MemberValue(static_cast<unsigned>(InValue));
		InOutJson.AddMember(MemberNameValue, MemberValue, InAllocator);
	}
}

static void SetJsonBool(rapidjson::Value& InOutJson, const char* InMemberName, bool InValue, rapidjson::Document::AllocatorType& InAllocator) noexcept
{
	if (InOutJson.HasMember(InMemberName))
	{
		InOutJson[InMemberName].SetBool(InValue);
	}
	else
	{
		rapidjson::Value MemberNameValue(InMemberName, InAllocator);
		rapidjson::Value MemberValue(InValue);
		InOutJson.AddMember(MemberNameValue, MemberValue, InAllocator);
	}
}

static const MC::MCDxcBinding* FindBinding(const std::vector<MC::MCDxcBinding>& InBindings, MC::MCDxcBindingKind InKind, uint32_t InBindPoint) noexcept
{
	for (const MC::MCDxcBinding& Binding : InBindings)
	{
		if ((Binding.Kind == InKind) && (Binding.BindPoint == InBindPoint))
		{
			return &Binding;
		}
	}
	return nullptr;
}

// Non cbuffer sections: every entry is looked up in the compiler reported binding list by kind and slot
// (the compiler strips resource names in optimized builds, so the slot is the authority).
static void WriteReflectionBindingCulled(const std::vector<MC::MCDxcBinding>& InBindings, rapidjson::Value& InOutSection, MC::MCDxcBindingKind InKind, const char* InEntryLabel, rapidjson::Document::AllocatorType& InAllocator) noexcept
{
	MC::MCLogger& MCLog = MC::MCLogger::Get();
	for (auto& EntryJson : InOutSection.GetArray())
	{
		if (!EntryJson.IsObject())
		{
			continue;
		}
		std::string EntryName = "";
		ReadJsonString(EntryJson, "Name", EntryName);
		uint32_t EntrySlot = 0u;
		ReadJsonUint(EntryJson, "Slot", EntrySlot);

		const MC::MCDxcBinding* Binding = FindBinding(InBindings, InKind, EntrySlot);
		if (Binding == nullptr)
		{
			MCLog.Warning(std::string(InEntryLabel) + " '" + EntryName + "' (slot " + std::to_string(EntrySlot) + ") is not present in the compiled shader, it is marked as culled");
		}
		else if ((!Binding->Name.empty()) && (!EntryName.empty()) && (Binding->Name != EntryName))
		{
			MCLog.Warning(std::string(InEntryLabel) + " slot " + std::to_string(EntrySlot) + " is bound to '" + Binding->Name + "' in the compiled shader but named '" + EntryName + "' in the reflection");
		}
		SetJsonBool(EntryJson, "CompilerCulled", (Binding == nullptr), InAllocator);
	}
}

static bool WriteReflectionLayout(const std::string& InReflectionPath, const MC::MCDxcResult& InResult, std::string& OutError) noexcept
{
	MC::MCLogger& MCLog = MC::MCLogger::Get();

	std::string ReflectionText = "";
	if (!ReadTextFile(InReflectionPath, ReflectionText))
	{
		OutError = "cannot read reflection: " + InReflectionPath;
		return false;
	}

	rapidjson::Document ReflectionDoc;
	ReflectionDoc.Parse(ReflectionText.c_str());
	if (ReflectionDoc.HasParseError())
	{
		OutError = std::string("reflection parse error: ") + rapidjson::GetParseError_En(ReflectionDoc.GetParseError());
		return false;
	}
	auto& ReflectionAllocator = ReflectionDoc.GetAllocator();

	if (ReflectionDoc.HasMember("CBuffers") && ReflectionDoc["CBuffers"].IsArray())
	{
		for (auto& CbJson : ReflectionDoc["CBuffers"].GetArray())
		{
			if (!CbJson.IsObject())
			{
				continue;
			}
			std::string CbName = "";
			ReadJsonString(CbJson, "Name", CbName);

			const MC::MCDxcCbLayout* CbLayout = FindCbLayout(InResult.CBuffers, CbName);
			if (CbLayout == nullptr)
			{
				MCLog.Warning("CBuffer '" + CbName + "' is not present in the compiled shader, its fields are marked as culled");
			}
			SetJsonUint(CbJson, "ByteSize", (CbLayout != nullptr) ? CbLayout->ByteSize : 0u, ReflectionAllocator);
			SetJsonBool(CbJson, "CompilerCulled", (CbLayout == nullptr), ReflectionAllocator);

			// Root constant buffers are pushed as root 32 bit constants, their shader visible size must
			// fit the root constant budget. A culled one has no layout, nothing is pushed for it.
			bool bRootConstant = false;
			ReadJsonBool(CbJson, "RootConstant", bRootConstant);
			if (bRootConstant && (CbLayout != nullptr))
			{
				MCLog.Info("CBuffer '" + CbName + "' RootConstant ByteSize = " + std::to_string(CbLayout->ByteSize) + " / " + std::to_string(MC_ROOT_CONSTANT_CB_MAX_BYTES));
				if (CbLayout->ByteSize > static_cast<uint32_t>(MC_ROOT_CONSTANT_CB_MAX_BYTES))
				{
					OutError = "CBuffer '" + CbName + "' is marked 'RootConstant' but its ByteSize = " + std::to_string(CbLayout->ByteSize) + " exceeds the root constant limit of " + std::to_string(MC_ROOT_CONSTANT_CB_MAX_BYTES) + " bytes";
					return false;
				}
			}

			if (!(CbJson.HasMember("Fields") && CbJson["Fields"].IsArray()))
			{
				continue;
			}
			for (auto& FieldJson : CbJson["Fields"].GetArray())
			{
				if (!FieldJson.IsObject())
				{
					continue;
				}
				std::string FieldName = "";
				ReadJsonString(FieldJson, "Name", FieldName);

				const MC::MCDxcCbVariable* Variable = (CbLayout != nullptr) ? FindCbVariable(*CbLayout, FieldName) : nullptr;
				if ((CbLayout != nullptr) && (Variable == nullptr))
				{
					MCLog.Warning("CBuffer '" + CbName + "' field '" + FieldName + "' is not present in the compiled shader, it is marked as culled");
				}
				SetJsonUint(FieldJson, "ByteOffset", (Variable != nullptr) ? Variable->ByteOffset : 0u, ReflectionAllocator);
				SetJsonUint(FieldJson, "ContentBytes", (Variable != nullptr) ? Variable->ContentBytes : 0u, ReflectionAllocator);
				SetJsonUint(FieldJson, "ArrayStride", (Variable != nullptr) ? Variable->ArrayStride : 0u, ReflectionAllocator);
				SetJsonBool(FieldJson, "CompilerCulled", (Variable == nullptr), ReflectionAllocator);
			}
		}
	}

	// Every non cbuffer section: mark what the compiler removed from the compiled shader.
	struct MCBindingSection
	{
		const char*					SectionName;
		const char*					EntryLabel;
		MC::MCDxcBindingKind		Kind;
	};
	static const MCBindingSection BindingSections[] =
	{
		{ "Textures",		"Texture",		MC::MCDxcBindingKind::Binding_Srv },
		{ "Resources",		"Resource",		MC::MCDxcBindingKind::Binding_Srv },
		{ "RWTextures",		"RWTexture",	MC::MCDxcBindingKind::Binding_Uav },
		{ "RWResources",	"RWResource",	MC::MCDxcBindingKind::Binding_Uav },
		{ "Samplers",		"Sampler",		MC::MCDxcBindingKind::Binding_Sampler },
	};
	for (const MCBindingSection& Section : BindingSections)
	{
		if (ReflectionDoc.HasMember(Section.SectionName) && ReflectionDoc[Section.SectionName].IsArray())
		{
			WriteReflectionBindingCulled(InResult.Bindings, ReflectionDoc[Section.SectionName], Section.Kind, Section.EntryLabel, ReflectionAllocator);
		}
	}

	rapidjson::StringBuffer StrBuff;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> StrBuffWriter(StrBuff);
	StrBuffWriter.SetIndent(' ', 4);
	ReflectionDoc.Accept(StrBuffWriter);

	if (!WriteTextFile(InReflectionPath, StrBuff.GetString()))
	{
		OutError = "cannot write reflection: " + InReflectionPath;
		return false;
	}
	return true;
}

static bool CheckCBufferByteSizeBudget(const std::vector<MC::MCDxcCbLayout>& InLayouts, std::string& OutError) noexcept
{
	MC::MCLogger& MCLog = MC::MCLogger::Get();
	for (const MC::MCDxcCbLayout& Layout : InLayouts)
	{
		MCLog.Info("CBuffer '" + Layout.Name + "' ByteSize = " + std::to_string(Layout.ByteSize) + " / " + std::to_string(MC_MATERIAL_CB_MAX_BYTES));
		if (Layout.ByteSize > static_cast<uint32_t>(MC_MATERIAL_CB_MAX_BYTES))
		{
			OutError = "CBuffer '" + Layout.Name + "' ByteSize = " + std::to_string(Layout.ByteSize) + " exceeds the limit of " + std::to_string(MC_MATERIAL_CB_MAX_BYTES) + " bytes";
			return false;
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// One variant: parse the json chain, compile the hlsl, write the artifacts.
// ---------------------------------------------------------------------------
static std::vector<std::string> CollectIncludeDirs(const MCCompileArgs& InArgs) noexcept
{
	std::vector<std::string> IncludeDirs;
	IncludeDirs.emplace_back(InArgs.ShaderIncludeDir);

	const std::string JsonDirStr = ParentDirOf(InArgs.RootDir + InArgs.JsonFilePath);
	if ((!JsonDirStr.empty()) && (JsonDirStr != InArgs.ShaderIncludeDir))
	{
		IncludeDirs.emplace_back(JsonDirStr);
	}
	return IncludeDirs;
}

static std::vector<MC::MCCompileParams::TDefine> MakeCompileDefines(const std::vector<MC::MCDefine>& InDefines) noexcept
{
	std::vector<MC::MCCompileParams::TDefine> CompileDefines;
	CompileDefines.reserve(InDefines.size());
	for (const MC::MCDefine& Def : InDefines)
	{
		CompileDefines.emplace_back(Def.Name, std::to_string(Def.Value));
	}
	return CompileDefines;
}

static bool CompileShaderVariant(
	const MCCompileArgs&	InArgs,
	MCShaderStageType		InStage,
	const std::string&		InShaderName,
	const std::string&		InEntryName,
	const MCVariantDesc&	InVariant,
	MC::MCDxcCompiler&		InDxcCompiler,
	MCVariantResult&		OutResult
) noexcept
{
	MC::MCLogger& MCLog = MC::MCLogger::Get();

	const std::string StageSuffixStr = StageSuffixOf(InStage);
	const std::string BaseName = InShaderName + "_" + InVariant.Name + "_" + StageSuffixStr;

	OutResult.Name = InVariant.Name;
	OutResult.DefHash = MakeDefinesHash(InVariant.Defines);
	OutResult.RplHash = MakeReplacesHash(InVariant.Replaces);

	MC::MCLoadParam LoadParam;
	LoadParam.RootDir = InArgs.RootDir;
	LoadParam.JsonFilePath = InArgs.JsonFilePath;
	LoadParam.Stage = InArgs.Stage;
	LoadParam.OutDir = InArgs.OutputDir;
	LoadParam.OutFileName = BaseName + DefaultReflectionExtn;
	LoadParam.DefHash = OutResult.DefHash;
	LoadParam.RplHash = OutResult.RplHash;
	LoadParam.bSM66 = (InArgs.bSm66 && (!InArgs.bDx11));

	std::vector<MC::MCDefine> FinalDefines = InVariant.Defines;
	std::string HlslSource = "";
	MCLog.Stage("ParseShader", BaseName);
	if (!MC::ParseShader(LoadParam, FinalDefines, InVariant.Replaces, HlslSource))
	{
		OutResult.Errors = "ParseShader failed, see log for details";
		return false;
	}
	OutResult.Defines = FinalDefines;

	if (InArgs.bDumpHlsl)
	{
		const std::string DumpPath = InArgs.OutputDir + BaseName + ".hlsl";
		if (!WriteTextFile(DumpPath, HlslSource))
		{
			MCLog.Warning("cannot dump hlsl: " + DumpPath);
		}
	}

	MC::MCCompileParams CompileParams;
	CompileParams.HlslSource = HlslSource;
	CompileParams.SourceName = BaseName;
	CompileParams.Profile = ProfileOf(InStage, InArgs);
	CompileParams.EntryName = InEntryName;
	CompileParams.IncludeDirs = CollectIncludeDirs(InArgs);
	CompileParams.Defines = MakeCompileDefines(FinalDefines);
	CompileParams.bDebug = InArgs.bDebug;
	CompileParams.bDx11 = InArgs.bDx11;

	MCLog.Stage("CompileShader", BaseName + " (" + CompileParams.Profile + ")");
	MC::MCDxcResult DxcResult;
	InDxcCompiler.Compile(CompileParams, DxcResult);
	if (!DxcResult.Errors.empty())
	{
		MCLog.Error(DxcResult.Errors);
	}
	if (!DxcResult.Success)
	{
		OutResult.Errors = DxcResult.Errors.empty() ? std::string("shader compile failed") : DxcResult.Errors;
		return false;
	}

	std::string StrError = "";
	const std::string ReflectionPath = InArgs.OutputDir + LoadParam.OutFileName;
	if (!WriteReflectionLayout(ReflectionPath, DxcResult, StrError))
	{
		OutResult.Errors = StrError;
		return false;
	}
	if (!CheckCBufferByteSizeBudget(DxcResult.CBuffers, StrError))
	{
		OutResult.Errors = StrError;
		return false;
	}
	OutResult.ReflectionFile = LoadParam.OutFileName;

	const std::string BinaryFile = BaseName + BinaryExtnOf(InArgs);
	if (!WriteBinaryFile(InArgs.OutputDir + BinaryFile, DxcResult.DxilBytes))
	{
		OutResult.Errors = "cannot write binary: " + InArgs.OutputDir + BinaryFile;
		return false;
	}
	OutResult.BinaryFile = BinaryFile;

	if (InArgs.bDebug && (!DxcResult.PdbBytes.empty()))
	{
		const std::string PdbFile = BaseName + ".pdb";
		if (WriteBinaryFile(InArgs.OutputDir + PdbFile, DxcResult.PdbBytes))
		{
			OutResult.PdbFile = PdbFile;
		}
		else
		{
			MCLog.Warning("cannot write pdb: " + InArgs.OutputDir + PdbFile);
		}
	}

	OutResult.bSuccess = true;
	return true;
}

// ---------------------------------------------------------------------------
// Build manifest - the tool's whole result, written even when variants failed.
// ---------------------------------------------------------------------------
static bool WriteManifest(
	const MCCompileArgs&				InArgs,
	MCShaderStageType					InStage,
	const std::string&					InShaderName,
	const std::string&					InEntryName,
	const std::vector<MCVariantResult>&	InResults,
	const std::string&					InManifestPath
) noexcept
{
	rapidjson::Document ManifestDoc;
	ManifestDoc.SetObject();
	auto& ManifestAllocator = ManifestDoc.GetAllocator();

	ManifestDoc.AddMember("Name", rapidjson::Value(InShaderName.c_str(), ManifestAllocator), ManifestAllocator);
	ManifestDoc.AddMember("Entry", rapidjson::Value(InEntryName.c_str(), ManifestAllocator), ManifestAllocator);
	ManifestDoc.AddMember("Stage", rapidjson::Value(InArgs.Stage.c_str(), ManifestAllocator), ManifestAllocator);
	ManifestDoc.AddMember("StageSuffix", rapidjson::Value(StageSuffixOf(InStage).c_str(), ManifestAllocator), ManifestAllocator);
	ManifestDoc.AddMember("ShaderModel", rapidjson::Value(ShaderModelOf(InArgs).c_str(), ManifestAllocator), ManifestAllocator);
	ManifestDoc.AddMember("EntryJson", rapidjson::Value(InArgs.JsonFilePath.c_str(), ManifestAllocator), ManifestAllocator);

	rapidjson::Value VariantArray(rapidjson::kArrayType);
	for (int VariantIdx = 0, NumVariants = static_cast<int>(InResults.size()); VariantIdx < NumVariants; ++VariantIdx)
	{
		const MCVariantResult& Result = InResults[VariantIdx];

		rapidjson::Value VariantObj(rapidjson::kObjectType);
		VariantObj.AddMember("Index", VariantIdx, ManifestAllocator);
		VariantObj.AddMember("Name", rapidjson::Value(Result.Name.c_str(), ManifestAllocator), ManifestAllocator);
		VariantObj.AddMember("DefHash", rapidjson::Value(Result.DefHash.c_str(), ManifestAllocator), ManifestAllocator);
		VariantObj.AddMember("RplHash", rapidjson::Value(Result.RplHash.c_str(), ManifestAllocator), ManifestAllocator);

		rapidjson::Value DefineArray(rapidjson::kArrayType);
		for (const MC::MCDefine& Def : Result.Defines)
		{
			rapidjson::Value DefineObj(rapidjson::kObjectType);
			DefineObj.AddMember("Name", rapidjson::Value(Def.Name.c_str(), ManifestAllocator), ManifestAllocator);
			DefineObj.AddMember("Value", Def.Value, ManifestAllocator);
			DefineArray.PushBack(DefineObj, ManifestAllocator);
		}
		VariantObj.AddMember("Defines", DefineArray, ManifestAllocator);

		VariantObj.AddMember("Binary", rapidjson::Value(Result.BinaryFile.c_str(), ManifestAllocator), ManifestAllocator);
		VariantObj.AddMember("Reflection", rapidjson::Value(Result.ReflectionFile.c_str(), ManifestAllocator), ManifestAllocator);
		VariantObj.AddMember("Pdb", rapidjson::Value(Result.PdbFile.c_str(), ManifestAllocator), ManifestAllocator);
		VariantObj.AddMember("Success", Result.bSuccess, ManifestAllocator);
		VariantObj.AddMember("Errors", rapidjson::Value(Result.Errors.c_str(), ManifestAllocator), ManifestAllocator);

		VariantArray.PushBack(VariantObj, ManifestAllocator);
	}
	ManifestDoc.AddMember("Variants", VariantArray, ManifestAllocator);

	rapidjson::StringBuffer StrBuff;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> StrBuffWriter(StrBuff);
	StrBuffWriter.SetIndent(' ', 4);
	ManifestDoc.Accept(StrBuffWriter);

	return WriteTextFile(InManifestPath, StrBuff.GetString());
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int InArgc, char** InArgv)
{
	if (IsUsageRequested(InArgc, InArgv))
	{
		PrintUsage();
		return 0;
	}

	std::string CompileJsonPath = "";
	if (!ParseCommandLine(InArgc, InArgv, CompileJsonPath))
	{
		PrintUsage();
		return 1;
	}

	MC::MCLogger& MCLog = MC::MCLogger::Get();

	std::string CompileJsonText = "";
	if (!ReadTextFile(CompileJsonPath, CompileJsonText))
	{
		MCLog.Error("cannot read compile json: " + CompileJsonPath);
		return 1;
	}

	rapidjson::Document CompileDoc;
	CompileDoc.Parse(CompileJsonText.c_str());
	if (CompileDoc.HasParseError())
	{
		MCLog.Error(std::string("compile json parse error: ") + rapidjson::GetParseError_En(CompileDoc.GetParseError()));
		return 1;
	}

	MCCompileArgs Args;
	std::string StrError = "";
	if (!ParseCompileArgs(CompileDoc, Args, StrError))
	{
		MCLog.Error("compile json invalid: " + StrError);
		return 1;
	}
	if (!ValidateCompileArgs(Args, StrError))
	{
		MCLog.Error("compile json invalid: " + StrError);
		return 1;
	}

	MCShaderStageType StageType = MCShaderStageType::ShaderStage_VS;
	ParseShaderStage(Args.Stage, StageType);

	MCLog.Init(Args.LogPath);
	MCLog.Info("ShaderCompiler started");
	MCLog.Info("Compile Json: " + ResolveFilePath(CompileJsonPath));
	MCLog.Info("Root Directory: " + Args.RootDir);
	MCLog.Info("Entry Json: " + Args.RootDir + Args.JsonFilePath);
	MCLog.Info("Output Directory: " + Args.OutputDir);
	MCLog.Info("Stage: " + Args.Stage + " (" + StageSuffixOf(StageType) + ")");
	MCLog.Info("Target: " + TargetDescOf(Args));
	MCLog.Info("Debug: " + std::string(Args.bDebug ? "ON" : "OFF") + ", Dump HLSL: " + std::string(Args.bDumpHlsl ? "ON" : "OFF"));
	MCLog.Info("Variants: " + std::to_string(Args.Variants.size()));

	std::string ShaderName = "";
	std::string EntryName = "";
	if (!ProbeEntryJson(Args.RootDir + Args.JsonFilePath, ShaderName, EntryName, StrError))
	{
		MCLog.Error(StrError);
		MCLog.Shutdown();
		return 1;
	}
	MCLog.Info("Shader: " + ShaderName + ", Entry: " + EntryName);

	MC::MCDxcCompiler DxcCompiler;
	if (!DxcCompiler.Init(StrError))
	{
		MCLog.Error("DXC init failed: " + StrError);
		MCLog.Shutdown();
		return 1;
	}

	const std::string ManifestPath = ResolveManifestPath(Args, ShaderName, StageType);
	MCLog.Info("Manifest: " + ManifestPath);

	bool bAllOk = true;
	std::vector<MCVariantResult> VariantResults;
	VariantResults.reserve(Args.Variants.size());
	for (int VariantIdx = 0, NumVariants = static_cast<int>(Args.Variants.size()); VariantIdx < NumVariants; ++VariantIdx)
	{
		const MCVariantDesc& Variant = Args.Variants[VariantIdx];
		MCLog.Stage("CompileVariant", Variant.Name + " #" + std::to_string(VariantIdx));

		MCVariantResult VariantResult;
		if (!CompileShaderVariant(Args, StageType, ShaderName, EntryName, Variant, DxcCompiler, VariantResult))
		{
			bAllOk = false;
			MCLog.Error("variant '" + Variant.Name + "' failed: " + VariantResult.Errors);
		}
		else
		{
			MCLog.Info("variant '" + Variant.Name + "' -> " + VariantResult.BinaryFile);
		}
		VariantResults.emplace_back(std::move(VariantResult));
	}

	MCLog.Stage("WriteManifest", ManifestPath);
	if (!WriteManifest(Args, StageType, ShaderName, EntryName, VariantResults, ManifestPath))
	{
		bAllOk = false;
		MCLog.Error("cannot write manifest: " + ManifestPath);
	}

	DxcCompiler.Shutdown();
	MCLog.Info("ShaderCompiler finished");
	MCLog.Shutdown();
	return (bAllOk ? 0 : 1);
}
