#include "MCJsonParser.h"
#include "MCLogger.h"
#include <prettywriter.h>
#include <stringbuffer.h>
#include <iomanip>
#include <error/en.h>
#include <fstream>
#include <sstream>
#include <set>
#include <regex>


namespace MC
{

	// ---------------------------------------------------------------------------
	// Helpers
	// ---------------------------------------------------------------------------

	static bool ReadFile(const std::string& InPath, std::string& OutContent)
	{
		MCLogger& MCLog = MCLogger::Get();
		std::ifstream FileStream(InPath);
		if (!FileStream.is_open())
		{
			MCLog.Error("Cannot open: " + InPath);
			return false;
		}
		std::ostringstream StrStream;
		StrStream << FileStream.rdbuf();
		OutContent = StrStream.str();
		return true;
	}

	static bool LoadHlsl(const std::string& InHlslFilePath, std::string& OutContent)
	{
		MCLogger& MCLog = MCLogger::Get();
		if ((InHlslFilePath.size() > 5) && (InHlslFilePath.substr(InHlslFilePath.size() - 5) == ".hlsl"))
		{
			return ReadFile(InHlslFilePath, OutContent);
		}
		MCLog.Error("Invalid HLSL file extension (expected .hlsl): " + InHlslFilePath);
		return false;
	}

	static bool ParseDoc(const std::string& InRaw, rapidjson::Document& OutDoc)
	{
		MCLogger& MCLog = MCLogger::Get();
		OutDoc.Parse(InRaw.c_str());
		if (OutDoc.HasParseError())
		{
			MCLog.Error(std::string("JSON parse error: ") + rapidjson::GetParseError_En(OutDoc.GetParseError()));
			return false;
		}
		return true;
	}

	static std::set<std::string> ExtractTemplatePlaceholders(const std::string& InSource)
	{
		std::set<std::string> Keys;
		std::regex Pattern(R"(\{[ \t]*([A-Za-z_][A-Za-z0-9_]*)[ \t]*\})");
		auto Begin = std::sregex_iterator(InSource.begin(), InSource.end(), Pattern);
		auto End = std::sregex_iterator();
		for (auto It = Begin; It != End; ++It)
		{
			Keys.insert((*It)[1].str());
		}
		return Keys;
	}

	static void ReplaceTemplateStringRecursive(std::string& InOutStr, const std::map<std::string, std::string>& InReplaceRules, int InMaxDepth = 16)
	{
		if (InMaxDepth <= 0)
		{
			MCLogger::Get().Error("ReplaceTemplateStringRecursive over max depth, shader may compile with wrong codes");
			return;
		}

		bool bAnyReplaced = false;
		for (const auto& [Key, Value] : InReplaceRules)
		{
			size_t Pos = 0;
			while ((Pos = InOutStr.find(Key, Pos)) != std::string::npos)
			{
				InOutStr.replace(Pos, Key.size(), Value);
				Pos += Value.size();
				bAnyReplaced = true;
			}
		}

		if (bAnyReplaced)
		{
			ReplaceTemplateStringRecursive(InOutStr, InReplaceRules, InMaxDepth - 1);
		}
	}

	static void ReplaceTemplatePlaceholders(std::string& InOutStr, const std::map<std::string, std::string>& InReplaceRules, int InMaxDepth = 16)
	{
		if (InMaxDepth <= 0)
		{
			MCLogger::Get().Error("ReplaceTemplatePlaceholders over max depth, shader may compile with wrong codes");
			return;
		}

		std::regex Pattern(R"(\{[ \t]*([A-Za-z_][A-Za-z0-9_]*)[ \t]*\})");

		std::string Result;
		bool bAnyReplaced = false;
		size_t LastPos = 0;

		auto Begin = std::sregex_iterator(InOutStr.begin(), InOutStr.end(), Pattern);
		auto End = std::sregex_iterator();

		for (auto It = Begin; It != End; ++It)
		{
			std::smatch Match = *It;
			std::string Key = Match[1].str();

			auto RuleIt = InReplaceRules.find(Key);
			if (RuleIt == InReplaceRules.end())
			{
				continue;
			}

			if (Result.empty())
			{
				Result.reserve(InOutStr.size());
			}
			Result.append(InOutStr, LastPos, Match.position() - LastPos);
			Result.append(RuleIt->second);                // whole {  KEY  } -> value
			LastPos = Match.position() + Match.length();
			bAnyReplaced = true;
		}

		if (!bAnyReplaced)
		{
			return;
		}

		Result.append(InOutStr, LastPos, InOutStr.length() - LastPos);
		InOutStr = std::move(Result);

		ReplaceTemplatePlaceholders(InOutStr, InReplaceRules, InMaxDepth - 1);
	}

	// Comment banners are line based: a raw newline inside a value terminates the '//' comment and
	// turns the rest of the text into real shader code, so such values are escaped when commented.
	static std::string EscapeCommentText(const std::string& InText)
	{
		std::string Result = "";
		Result.reserve(InText.size());
		for (char Ch : InText)
		{
			if (Ch == '\n')
			{
				Result += "\\n";
			}
			else if (Ch == '\r')
			{
				Result += "\\r";
			}
			else if (Ch == '\t')
			{
				Result += "\\t";
			}
			else
			{
				Result += Ch;
			}
		}
		return Result;
	}

	// ---------------------------------------------------------------------------
	// Condition helpers
	// ---------------------------------------------------------------------------

	static bool ParseConditionOpStr(const std::string& InStr, MCConditionOpType& OutOp)
	{
		if (InStr == "==")
		{
			OutOp = MCConditionOpType::Equal;
			return true;
		}
		if (InStr == "!=")
		{
			OutOp = MCConditionOpType::NotEqual;
			return true;
		}
		if (InStr == ">")
		{
			OutOp = MCConditionOpType::Greater;
			return true;
		}
		if (InStr == ">=")
		{
			OutOp = MCConditionOpType::GreaterEqual;
			return true;
		}
		if (InStr == "<")
		{
			OutOp = MCConditionOpType::Less;
			return true;
		}
		if (InStr == "<=")
		{
			OutOp = MCConditionOpType::LessEqual;
			return true;
		}
		return false;
	}

	static bool ParseConditionOpEnum(MCConditionOpType InOp, std::string& OutStr)
	{
		switch (InOp)
		{
			case MCConditionOpType::Equal:
			{
				OutStr = "==";
				return true;
			}
			case MCConditionOpType::NotEqual:
			{
				OutStr = "!=";
				return true;
			}
			case MCConditionOpType::Greater:
			{
				OutStr = ">";
				return true;
			}
			case MCConditionOpType::GreaterEqual:
			{
				OutStr = ">=";
				return true;
			}
			case MCConditionOpType::Less:
			{
				OutStr = "<";
				return true;
			}
			case MCConditionOpType::LessEqual:
			{
				OutStr = "<=";
				return true;
			}
		}
		return false;
	}

	static bool EvaluateCondition(MCConditionOpType InOp, int InLhs, int InRhs)
	{
		switch (InOp)
		{
			case MCConditionOpType::Equal:
			{
				return (InLhs == InRhs);
			}
			case MCConditionOpType::NotEqual:
			{
				return (InLhs != InRhs);
			}
			case MCConditionOpType::Greater:
			{
				return (InLhs > InRhs);
			}
			case MCConditionOpType::GreaterEqual:
			{
				return (InLhs >= InRhs);
			}
			case MCConditionOpType::Less:
			{
				return (InLhs < InRhs);
			}
			case MCConditionOpType::LessEqual:
			{
				return (InLhs <= InRhs);
			}
		}
		return false;
	}

	static bool ParseConditionField(const rapidjson::Value& InJson, MCConditionField& OutField)
	{
		const char* DefineNameTxt = "Define";
		const char* DefineOpTxt = "DefineOp";
		const char* DefineRefTxt = "DefineRef";

		MCLogger& MCLog = MCLogger::Get();

		if (InJson.HasMember(DefineNameTxt) && InJson[DefineNameTxt].IsString())
		{
			OutField.DefineName = InJson[DefineNameTxt].GetString();
			if (InJson.HasMember(DefineOpTxt) && InJson[DefineOpTxt].IsString())
			{
				if (!ParseConditionOpStr(InJson[DefineOpTxt].GetString(), OutField.Op))
				{
					MCLog.Error(std::string("ParseConditionField unknown condition op '") + InJson[DefineOpTxt].GetString() + "' for Define '" + OutField.DefineName + "'");
					return false;
				}
			}
			OutField.Ref = (InJson.HasMember(DefineRefTxt) && InJson[DefineRefTxt].IsInt()) ? (InJson[DefineRefTxt].GetInt()) : 0;
		}

		MCLog.Verbose("ParseConditionField Define='" + OutField.DefineName + "' op=" + (OutField.DefineName.empty() ? "none" : std::to_string(static_cast<int>(OutField.Op))) + " ref=" + std::to_string(OutField.Ref));
		return true;
	}

	static bool CheckConditionField(const MCConditionField& InField, const std::vector<MCDefine>& InDefs)
	{
		if (InField.DefineName.empty())
		{
			return true;		// unconditional
		}

		MCLogger& MCLog = MCLogger::Get();

		bool		bFound = false;
		int			DefineVal = 0;			// No defines mean the value is 0
		for (const auto& Def : InDefs)
		{
			if (Def.Name == InField.DefineName)
			{
				DefineVal = Def.Value;
				bFound = true;
				MCLog.Verbose("CheckConditionField found condition name: " + Def.Name + ", value = " + std::to_string(DefineVal));
				break;
			}
		}
		if (!bFound)
		{
			MCLog.Info("CheckConditionField condition define not found: " + InField.DefineName + ", fallback to zero value");
		}
		if (!EvaluateCondition(InField.Op, DefineVal, InField.Ref))
		{
			std::string OpStr;
			ParseConditionOpEnum(InField.Op, OpStr);
			MCLog.Verbose("CheckConditionField condition not met for define '" + InField.DefineName + "' ref value = " + std::to_string(InField.Ref) + " op = '" + OpStr + "' does not match define value = " + std::to_string(DefineVal));
			return false;
		}
		return true;
	}

	static void SplitVariableFormatStr(const std::string& InStr, std::string& OutPrefix, std::string& OutSuffix)
	{
		auto It = std::find_if(InStr.begin(), InStr.end(), [](char InChr) { return ((InChr >= '0') && (InChr <= '9')); });
		OutPrefix.assign(InStr.begin(), It);
		OutSuffix.assign(It, InStr.end());
	}

	static bool ParseVariableFormatPrefixEnum(MCVarFormatPrefixType InFormat, std::string& OutStr)
	{
		switch (InFormat)
		{
			case MCVarFormatPrefixType::Var_Int:
			{
				OutStr = "int";
				return true;
			}
			case MCVarFormatPrefixType::Var_Uint:
			{
				OutStr = "uint";
				return true;
			}
			case MCVarFormatPrefixType::Var_Float:
			{
				OutStr = "float";
				return true;
			}
		}
		return false;
	}

	static bool ParseVariableFormatSuffixEnum(MCVarFormatSuffixType InFormat, std::string& OutStr)
	{
		switch (InFormat)
		{
			case MCVarFormatSuffixType::Var_1:
			{
				OutStr = "";
				return true;
			}
			case MCVarFormatSuffixType::Var_2:
			{
				OutStr = "2";
				return true;
			}
			case MCVarFormatSuffixType::Var_3:
			{
				OutStr = "3";
				return true;
			}
			case MCVarFormatSuffixType::Var_4:
			{
				OutStr = "4";
				return true;
			}
			case MCVarFormatSuffixType::Var_2x2:
			{
				OutStr = "2x2";
				return true;
			}
			case MCVarFormatSuffixType::Var_3x2:
			{
				OutStr = "3x2";
				return true;
			}
			case MCVarFormatSuffixType::Var_2x3:
			{
				OutStr = "2x3";
				return true;
			}
			case MCVarFormatSuffixType::Var_3x3:
			{
				OutStr = "3x3";
				return true;
			}
			case MCVarFormatSuffixType::Var_4x2:
			{
				OutStr = "4x2";
				return true;
			}
			case MCVarFormatSuffixType::Var_2x4:
			{
				OutStr = "2x4";
				return true;
			}
			case MCVarFormatSuffixType::Var_4x3:
			{
				OutStr = "4x3";
				return true;
			}
			case MCVarFormatSuffixType::Var_3x4:
			{
				OutStr = "3x4";
				return true;
			}
			case MCVarFormatSuffixType::Var_4x4:
			{
				OutStr = "4x4";
				return true;
			}
		}
		return false;
	}

	static bool ParseVariableFormatSuffixElems(MCVarFormatSuffixType InFormat, int& OutNumElems)
	{
		switch (InFormat)
		{
			case MCVarFormatSuffixType::Var_1:
			{
				OutNumElems = 1;
				return true;
			}
			case MCVarFormatSuffixType::Var_2:
			{
				OutNumElems = 2;
				return true;
			}
			case MCVarFormatSuffixType::Var_3:
			{
				OutNumElems = 3;
				return true;
			}
			case MCVarFormatSuffixType::Var_4:
			{
				OutNumElems = 4;
				return true;
			}
			case MCVarFormatSuffixType::Var_2x2:
			{
				OutNumElems = 4;
				return true;
			}
			case MCVarFormatSuffixType::Var_3x2:
			{
				OutNumElems = 6;
				return true;
			}
			case MCVarFormatSuffixType::Var_2x3:
			{
				OutNumElems = 6;
				return true;
			}
			case MCVarFormatSuffixType::Var_3x3:
			{
				OutNumElems = 9;
				return true;
			}
			case MCVarFormatSuffixType::Var_4x2:
			{
				OutNumElems = 8;
				return true;
			}
			case MCVarFormatSuffixType::Var_2x4:
			{
				OutNumElems = 8;
				return true;
			}
			case MCVarFormatSuffixType::Var_4x3:
			{
				OutNumElems = 12;
				return true;
			}
			case MCVarFormatSuffixType::Var_3x4:
			{
				OutNumElems = 12;
				return true;
			}
			case MCVarFormatSuffixType::Var_4x4:
			{
				OutNumElems = 16;
				return true;
			}
		}
		return false;
	}

	static bool ParseVariableFormatPrefixStr(const std::string& InStr, MCVarFormatPrefixType& OutFormat)
	{
		if (InStr == "int")
		{
			OutFormat = MCVarFormatPrefixType::Var_Int;
			return true;
		}
		if (InStr == "uint")
		{
			OutFormat = MCVarFormatPrefixType::Var_Uint;
			return true;
		}
		if (InStr == "float")
		{
			OutFormat = MCVarFormatPrefixType::Var_Float;
			return true;
		}
		return false;
	}

	static bool ParseVariableFormatSuffixStr(const std::string& InStr, MCVarFormatSuffixType& OutFormat)
	{
		if (InStr == "")
		{
			OutFormat = MCVarFormatSuffixType::Var_1;
			return true;
		}
		if (InStr == "2")
		{
			OutFormat = MCVarFormatSuffixType::Var_2;
			return true;
		}
		if (InStr == "3")
		{
			OutFormat = MCVarFormatSuffixType::Var_3;
			return true;
		}
		if (InStr == "4")
		{
			OutFormat = MCVarFormatSuffixType::Var_4;
			return true;
		}
		if (InStr == "2x2")
		{
			OutFormat = MCVarFormatSuffixType::Var_2x2;
			return true;
		}
		if (InStr == "3x2")
		{
			OutFormat = MCVarFormatSuffixType::Var_3x2;
			return true;
		}
		if (InStr == "2x3")
		{
			OutFormat = MCVarFormatSuffixType::Var_2x3;
			return true;
		}
		if (InStr == "3x3")
		{
			OutFormat = MCVarFormatSuffixType::Var_3x3;
			return true;
		}
		if (InStr == "4x2")
		{
			OutFormat = MCVarFormatSuffixType::Var_4x2;
			return true;
		}
		if (InStr == "2x4")
		{
			OutFormat = MCVarFormatSuffixType::Var_2x4;
			return true;
		}
		if (InStr == "4x3")
		{
			OutFormat = MCVarFormatSuffixType::Var_4x3;
			return true;
		}
		if (InStr == "3x4")
		{
			OutFormat = MCVarFormatSuffixType::Var_3x4;
			return true;
		}
		if (InStr == "4x4")
		{
			OutFormat = MCVarFormatSuffixType::Var_4x4;
			return true;
		}
		return false;
	}

	union PrefixVariableToUint32
	{
		int					_IntVal;
		unsigned int		_UintVal;
		float				_FloatVal;
		uint32_t			StorageVal;
	};

	static bool DefaultVariableFormatValue(MCVarFormatPrefixType InFormat, uint32_t& OutVal)
	{
		switch (InFormat)
		{
			case MCVarFormatPrefixType::Var_Int:
			{
				PrefixVariableToUint32 DefaultVal;
				DefaultVal._IntVal = 0;
				OutVal = DefaultVal.StorageVal;
				return true;
			}
			case MCVarFormatPrefixType::Var_Uint:
			{
				PrefixVariableToUint32 DefaultVal;
				DefaultVal._UintVal = 0u;
				OutVal = DefaultVal.StorageVal;
				return true;
			}
			case MCVarFormatPrefixType::Var_Float:
			{
				PrefixVariableToUint32 DefaultVal;
				DefaultVal._FloatVal = 0.0f;
				OutVal = DefaultVal.StorageVal;
				return true;
			}
		}
		return false;
	}

	//	Example 1 JSON:
	//	{
	//		"Semantic"			:		"TEXCOORD",
	//		"SemanticIdx"		:		0,
	//		"Format"			:		"float2"
	//		"Slot"				:		0
	//	}

	//	Example 2 JSON:
	//	{
	//		"Name"				:		"UVStream",
	//		"Semantic"			:		"TEXCOORD",
	//		"SemanticIdx"		:		1,
	//		"Format"			:		"float4",
	//		"Slot"				:		0
	//	}

	//	Example 3 JSON:
	//	{
	//		"Semantic"			:		"TEXCOORD",
	//		"SemanticIdx"		:		2,
	//		"Format"			:		"float2",
	//		"Slot"				:		1
	//		"Define"			:		"USED_CUSTOM_DATA"
	//	}

	//	Example 4 JSON:
	//	{
	//		"Semantic"			:		"TEXCOORD",
	//		"SemanticIdx"		:		3,
	//		"Format"			:		"float2",
	//		"Slot"				:		2
	//		"Define"			:		"NUM_TEXCOORD",
	//		"DefineOp"			:		">=",
	//		"DefineRef"			:		0
	//	}

	// Parse a single input entry from JSON into an MCInputTemplate.
	static bool ParseVFInputTemplate(const rapidjson::Value& InJson, int InSlot, MCVFInputTemplate& OutTmpl)
	{
		const char* NameTxt = "Name";
		const char* SemanticTxt = "Semantic";
		const char* SemanticIdxTxt = "SemanticIdx";
		const char* FormatTxt = "Format";
		const char* SlotTxt = "Slot";

		MCLogger& MCLog = MCLogger::Get();

		if (!(InJson.HasMember(NameTxt) && InJson[NameTxt].IsString()))
		{
			MCLog.Error("ParseVFInputTemplate input template contains invalid name");
			return false;
		}
		OutTmpl.Input.Name = InJson[NameTxt].GetString();
		if (!(InJson.HasMember(SemanticTxt) && InJson[SemanticTxt].IsString()) ||
			!(InJson.HasMember(FormatTxt) && InJson[FormatTxt].IsString()) ||
			!InJson.HasMember(SemanticIdxTxt) ||
			((InSlot < 0) && (!InJson.HasMember(SlotTxt))))
		{
			if ((InSlot < 0) && (!InJson.HasMember(SlotTxt)))
			{
				MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' input template missing 'Semantic' or 'Format' or 'SemanticIdx' or 'Slot'");
			}
			else
			{
				MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' input template missing 'Semantic' or 'Format' or 'SemanticIdx'");
			}
			return false;
		}
		OutTmpl.Input.Semantic = InJson[SemanticTxt].GetString();
		{
			int SemanticIdx = InJson[SemanticIdxTxt].GetInt();
			if (SemanticIdx < 0 || SemanticIdx > 15)
			{
				MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' input template contains invalid semantic index number: " + std::to_string(SemanticIdx));
				return false;
			}
			OutTmpl.Input.SemanticIdx = SemanticIdx;
		}
		std::string StrPrefix, StrSuffix;
		SplitVariableFormatStr(InJson[FormatTxt].GetString(), StrPrefix, StrSuffix);
		if (StrPrefix == "color")
		{
			MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' can not use 'color' input stream format");
			return false;
		}
		if (!ParseVariableFormatPrefixStr(StrPrefix, OutTmpl.Input.Format.Prefix))
		{
			MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' input template FormatPrefix = " + StrPrefix + " not valid");
			return false;
		}
		if (!ParseVariableFormatSuffixStr(StrSuffix, OutTmpl.Input.Format.Suffix))
		{
			MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' input template FormatSuffix = " + StrSuffix + " not valid");
			return false;
		}
		OutTmpl.Input.Slot = (InJson.HasMember(SlotTxt)) ? (InJson[SlotTxt].GetInt()) : InSlot;
		if ((OutTmpl.Input.Slot < 0) || (OutTmpl.Input.Slot > 15))
		{
			MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' input template contains invalid slot number: " + std::to_string(OutTmpl.Input.Slot));
			return false;
		}

		// Define + op + ref
		if (!ParseConditionField(InJson, OutTmpl.Condition))
		{
			return false;
		}

		return true;
	}

	static bool ExpandVFInputTemplate(const MCVFInputTemplate& InTmpl, const std::vector<MCDefine>& InDefs, MCVFInput& OutInput)
	{
		if (!CheckConditionField(InTmpl.Condition, InDefs))
		{
			return false;
		}
		OutInput = InTmpl.Input;
		return true;
	}

	static bool ParseVariableInputTemplate(const rapidjson::Value& InJson, MCVariableInputTemplate& OutTmpl)
	{
		const char* NameTxt = "Name";
		const char* FormatTxt = "Format";
		const char* ArrayElementTxt = "ArrayElement";
		const char* DefaultTxt = "Default";
		const char* ExposureTxt = "Exposure";

		MCLogger& MCLog = MCLogger::Get();

		if (!(InJson.HasMember(NameTxt) && InJson[NameTxt].IsString()))
		{
			MCLog.Error("ParseVariableInputTemplate input template missing valid 'Name'");
			return false;
		}
		OutTmpl.Input.Name = InJson[NameTxt].GetString();
		if (!(InJson.HasMember(FormatTxt) && InJson[FormatTxt].IsString()))
		{
			MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' input template missing valid 'Format'");
			return false;
		}
		std::string StrPrefix, StrSuffix;
		SplitVariableFormatStr(InJson[FormatTxt].GetString(), StrPrefix, StrSuffix);
		OutTmpl.Input.bColor = false;
		if (StrPrefix == "color")
		{
			OutTmpl.Input.bColor = true;
			if (!ParseVariableFormatPrefixEnum(MCVarFormatPrefixType::Var_Float, StrPrefix))
			{
				MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' something wrong in color check phase");
				return false;
			}
		}
		if (!ParseVariableFormatPrefixStr(StrPrefix, OutTmpl.Input.Format.Prefix))
		{
			MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' input template FormatPrefix = " + StrPrefix + " not valid");
			return false;
		}
		if (!ParseVariableFormatSuffixStr(StrSuffix, OutTmpl.Input.Format.Suffix))
		{
			MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' input template FormatSuffix = " + StrSuffix + " not valid");
			return false;
		}
		OutTmpl.Input.bArray = false;
		if (InJson.HasMember(ArrayElementTxt))
		{
			int ArrayElems = InJson[ArrayElementTxt].GetInt();
			if (ArrayElems > 0)
			{
				OutTmpl.Input.bArray = true;
				OutTmpl.Input.ArrayNumElems = ArrayElems;
			}
		}
		uint32_t InitVal;
		if (!DefaultVariableFormatValue(OutTmpl.Input.Format.Prefix, InitVal))
		{
			MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' something wrong in get default value phase");
			return false;
		}
		int FormatElems;
		if (!ParseVariableFormatSuffixElems(OutTmpl.Input.Format.Suffix, FormatElems))
		{
			MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' something wrong in get suffix elements phase");
			return false;
		}
		if (InJson.HasMember(DefaultTxt) && InJson[DefaultTxt].IsArray())
		{
			OutTmpl.Input.Default.clear();
			for (const auto& DefaultVal : InJson[DefaultTxt].GetArray())
			{
				PrefixVariableToUint32 ValRef;
				switch (OutTmpl.Input.Format.Prefix)
				{
					case MCVarFormatPrefixType::Var_Int:
					{
						ValRef._IntVal = DefaultVal.GetInt();
						break;
					}
					case MCVarFormatPrefixType::Var_Uint:
					{
						ValRef._UintVal = DefaultVal.GetUint();
						break;
					}
					case MCVarFormatPrefixType::Var_Float:
					{
						ValRef._FloatVal = DefaultVal.GetFloat();
						break;
					}
					default:
					{
						MCLog.Info("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' '" + StrPrefix + "' idx='" + std::to_string(OutTmpl.Input.Default.size()) + "' default value does not match MCVarFormatPrefixType so use default value instead, plz check");
						ValRef.StorageVal = InitVal;
						break;
					}
				}
				OutTmpl.Input.Default.push_back(ValRef.StorageVal);
			}
			for (size_t StartPos = OutTmpl.Input.Default.size(), PosSize = static_cast<size_t>(FormatElems * (OutTmpl.Input.bArray ? OutTmpl.Input.ArrayNumElems : 1)); StartPos < PosSize; ++StartPos)
			{
				OutTmpl.Input.Default.push_back(InitVal);
			}
		}
		else
		{
			MCLog.Info("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' does not contain valid 'Default' field so use default value instead");
			OutTmpl.Input.Default.resize(static_cast<size_t>(FormatElems * (OutTmpl.Input.bArray ? OutTmpl.Input.ArrayNumElems : 1)));
			for (auto& Elem : OutTmpl.Input.Default)
			{
				Elem = InitVal;
			}
		}
		OutTmpl.Input.bExposure = false;
		if (InJson.HasMember(ExposureTxt) && InJson[ExposureTxt].IsBool() && InJson[ExposureTxt].GetBool())
		{
			OutTmpl.Input.bExposure = true;
		}

		if (!ParseConditionField(InJson, OutTmpl.Condition))
		{
			return false;
		}

		return true;
	}

	static bool ExpandVariableInputTemplate(const MCVariableInputTemplate& InTmpl, const std::vector<MCDefine>& InDefs, MCVariableInput& OutInput)
	{
		if (!CheckConditionField(InTmpl.Condition, InDefs))
		{
			return false;
		}
		OutInput = InTmpl.Input;
		return true;
	}

	static bool ParseConstantBuffer(const rapidjson::Value& InJson, const std::vector<MCDefine>& InDefs, int InSlot, MCConstantBuffer& OutCB)
	{
		const char* NameTxt = "Name";
		const char* FieldsTxt = "Fields";
		const char* SlotTxt = "Slot";
		const char* RootConstantTxt = "RootConstant";

		MCLogger& MCLog = MCLogger::Get();

		if (!(InJson.HasMember(NameTxt) && InJson[NameTxt].IsString()))
		{
			MCLog.Error("ParseConstantBuffer does not contain valid 'Name'");
			return false;
		}
		OutCB.Name = InJson[NameTxt].GetString();
		if (!(InJson.HasMember(FieldsTxt) && InJson[FieldsTxt].IsArray()))
		{
			MCLog.Error("ParseConstantBuffer '" + OutCB.Name + "' does not contain valid 'Fields'");
			return false;
		}
		if ((InSlot < 0) && (!InJson.HasMember(SlotTxt)))
		{
			MCLog.Error("ParseConstantBuffer '" + OutCB.Name + "' does not contain valid 'Slot'");
			return false;
		}
		OutCB.Slot = (InJson.HasMember(SlotTxt)) ? (InJson[SlotTxt].GetInt()) : InSlot;
		if ((OutCB.Slot < 0) || (OutCB.Slot > 13))
		{
			MCLog.Error("ParseConstantBuffer '" + OutCB.Name + "' slot=" + std::to_string(OutCB.Slot) + " out of range [0, 13]");
			return false;
		}
		OutCB.bRootConstant = false;
		if (InJson.HasMember(RootConstantTxt) && InJson[RootConstantTxt].IsBool() && InJson[RootConstantTxt].GetBool())
		{
			OutCB.bRootConstant = true;
			MCLog.Info("ParseConstantBuffer '" + OutCB.Name + "' is marked 'RootConstant'");
		}
		for (const auto& Field : InJson[FieldsTxt].GetArray())
		{
			MCVariableInputTemplate InputTmpl{};
			if (!ParseVariableInputTemplate(Field, InputTmpl))
			{
				return false;
			}
			MCVariableInput Input{};
			if (ExpandVariableInputTemplate(InputTmpl, InDefs, Input))
			{
				OutCB.Fields.emplace_back(std::move(Input));
			}
		}
		return (!(OutCB.Fields.empty()));
	}

	// ---------------------------------------------------------------------------
	// VF InputStream
	// ---------------------------------------------------------------------------

	//	Example JSON:
	//	{
	//		"Name": "MainStream",
	//		"Inputs": [
	//			{ "Semantic": "POSITION",  "SemanticIdx": 0, "Format": "float3" },
	//			{ "Semantic": "NORMAL",    "SemanticIdx": 0, "Format": "float3" },
	//			{ "Semantic": "TEXCOORD",  "SemanticIdx": 0, "Format": "float2", "Slot": 1 },
	//			{ "Semantic": "TEXCOORD",  "SemanticIdx": 1, "Format": "float2", "Slot": 1, "Define": "NUM_TEXCOORD", "DefineOp": ">=", "DefineRef": 2 }
	//		]
	//	}

	static bool ParseVFInputStream(const rapidjson::Value& InJson, const std::vector<MCDefine>& InDefs, int InSlot, MCVFInputStream& OutStream)
	{
		const char* NameTxt = "Name";
		const char* InputsTxt = "Inputs";

		MCLogger& MCLog = MCLogger::Get();

		if (!(InJson.HasMember(NameTxt) && InJson[NameTxt].IsString()))
		{
			MCLog.Error("ParseVFInputStream does not contain valid 'Name'");
			return false;
		}
		OutStream.Name = InJson[NameTxt].GetString();
		if (!(InJson.HasMember(InputsTxt) && InJson[InputsTxt].IsArray()))
		{
			MCLog.Error("ParseVFInputStream '" + OutStream.Name + "' does not contain valid 'Inputs'");
			return false;
		}
		for (const auto& Elem : InJson[InputsTxt].GetArray())
		{
			MCVFInputTemplate InputTmpl{};
			if (!ParseVFInputTemplate(Elem, InSlot, InputTmpl))
			{
				return false;
			}
			MCVFInput Input{};
			if (ExpandVFInputTemplate(InputTmpl, InDefs, Input))
			{
				OutStream.Inputs.emplace_back(std::move(Input));
			}
		}
		return (!(OutStream.Inputs.empty()));
	}

	// ---------------------------------------------------------------------------
	// Texture Sampler & Texture enum <-> string helpers
	// ---------------------------------------------------------------------------

	static bool ParseTextureSamplerFilterStr(const std::string& InStr, MCTextureSamplerFilterType& OutFilter)
	{
		if (InStr == "Point")
		{
			OutFilter = MCTextureSamplerFilterType::Tex_Filter_Point;
			return true;
		}
		if (InStr == "MinMagLinear")
		{
			OutFilter = MCTextureSamplerFilterType::Tex_Filter_MinMagLinear;
			return true;
		}
		if (InStr == "MinMagMipLinear")
		{
			OutFilter = MCTextureSamplerFilterType::Tex_Filter_MinMagMipLinear;
			return true;
		}
		if (InStr == "Anisotropic")
		{
			OutFilter = MCTextureSamplerFilterType::Tex_Filter_Anisotropic;
			return true;
		}
		return false;
	}

	static bool ParseTextureSamplerFilterEnum(MCTextureSamplerFilterType InFilter, std::string& OutStr)
	{
		switch (InFilter)
		{
			case MCTextureSamplerFilterType::Tex_Filter_Point:
			{
				OutStr = "Point";
				return true;
			}
			case MCTextureSamplerFilterType::Tex_Filter_MinMagLinear:
			{
				OutStr = "MinMagLinear";
				return true;
			}
			case MCTextureSamplerFilterType::Tex_Filter_MinMagMipLinear:
			{
				OutStr = "MinMagMipLinear";
				return true;
			}
			case MCTextureSamplerFilterType::Tex_Filter_Anisotropic:
			{
				OutStr = "Anisotropic";
				return true;
			}
		}
		return false;
	}

	static bool ParseTextureSamplerAddressStr(const std::string& InStr, MCTexSamplerAddressModeType& OutMode)
	{
		if (InStr == "Wrap")
		{
			OutMode = MCTexSamplerAddressModeType::Tex_Sampler_Wrap;
			return true;
		}
		if (InStr == "Mirror")
		{
			OutMode = MCTexSamplerAddressModeType::Tex_Sampler_Mirror;
			return true;
		}
		if (InStr == "Clamp")
		{
			OutMode = MCTexSamplerAddressModeType::Tex_Sampler_Clamp;
			return true;
		}
		if (InStr == "Border")
		{
			OutMode = MCTexSamplerAddressModeType::Tex_Sampler_Border;
			return true;
		}
		if (InStr == "MirrorOnce")
		{
			OutMode = MCTexSamplerAddressModeType::Tex_Sampler_MirrorOnce;
			return true;
		}
		return false;
	}

	static bool ParseTextureSamplerAddressEnum(MCTexSamplerAddressModeType InMode, std::string& OutStr)
	{
		switch (InMode)
		{
			case MCTexSamplerAddressModeType::Tex_Sampler_Wrap:
			{
				OutStr = "Wrap";
				return true;
			}
			case MCTexSamplerAddressModeType::Tex_Sampler_Mirror:
			{
				OutStr = "Mirror";
				return true;
			}
			case MCTexSamplerAddressModeType::Tex_Sampler_Clamp:
			{
				OutStr = "Clamp";
				return true;
			}
			case MCTexSamplerAddressModeType::Tex_Sampler_Border:
			{
				OutStr = "Border";
				return true;
			}
			case MCTexSamplerAddressModeType::Tex_Sampler_MirrorOnce:
			{
				OutStr = "MirrorOnce";
				return true;
			}
		}
		return false;
	}

	static bool ParseTextureComparisonFuncStr(const std::string& InStr, MCTextureSamplerComparisonFuncType& OutFunc)
	{
		if (InStr == "Never")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Never;
			return true;
		}
		if (InStr == "Less")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Less;
			return true;
		}
		if (InStr == "Equal")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Equal;
			return true;
		}
		if (InStr == "LessEqual")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_LessEqual;
			return true;
		}
		if (InStr == "Greater")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Greater;
			return true;
		}
		if (InStr == "NotEqual")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_NotEqual;
			return true;
		}
		if (InStr == "GreaterEqual")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_GreaterEqual;
			return true;
		}
		if (InStr == "Always")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Always;
			return true;
		}
		return false;
	}

	static bool ParseTextureComparisonFuncEnum(MCTextureSamplerComparisonFuncType InFunc, std::string& OutStr)
	{
		switch (InFunc)
		{
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Never:
			{
				OutStr = "Never";
				return true;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Less:
			{
				OutStr = "Less";
				return true;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Equal:
			{
				OutStr = "Equal";
				return true;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_LessEqual:
			{
				OutStr = "LessEqual";
				return true;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Greater:
			{
				OutStr = "Greater";
				return true;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_NotEqual:
			{
				OutStr = "NotEqual";
				return true;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_GreaterEqual:
			{
				OutStr = "GreaterEqual";
				return true;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Always:
			{
				OutStr = "Always";
				return true;
			}
		}
		return false;
	}

	static bool ParseTextureTypeStr(const std::string& InStr, MCTextureType& OutType)
	{
		if (InStr == "1D")
		{
			OutType = MCTextureType::Tex_1D;
			return true;
		}
		if (InStr == "2D")
		{
			OutType = MCTextureType::Tex_2D;
			return true;
		}
		if (InStr == "3D")
		{
			OutType = MCTextureType::Tex_3D;
			return true;
		}
		if (InStr == "1DArray")
		{
			OutType = MCTextureType::Tex_1DArray;
			return true;
		}
		if (InStr == "2DArray")
		{
			OutType = MCTextureType::Tex_2DArray;
			return true;
		}
		if (InStr == "3DArray")
		{
			OutType = MCTextureType::Tex_3DArray;
			return true;
		}
		if (InStr == "Cube")
		{
			OutType = MCTextureType::Tex_Cube;
			return true;
		}
		if (InStr == "CubeArray")
		{
			OutType = MCTextureType::Tex_CubeArray;
			return true;
		}
		return false;
	}

	static bool ParseTextureTypeEnum(MCTextureType InType, std::string& OutStr)
	{
		switch (InType)
		{
			case MCTextureType::Tex_1D:
			{
				OutStr = "1D";
				return true;
			}
			case MCTextureType::Tex_2D:
			{
				OutStr = "2D";
				return true;
			}
			case MCTextureType::Tex_3D:
			{
				OutStr = "3D";
				return true;
			}
			case MCTextureType::Tex_1DArray:
			{
				OutStr = "1DArray";
				return true;
			}
			case MCTextureType::Tex_2DArray:
			{
				OutStr = "2DArray";
				return true;
			}
			case MCTextureType::Tex_3DArray:
			{
				OutStr = "3DArray";
				return true;
			}
			case MCTextureType::Tex_Cube:
			{
				OutStr = "Cube";
				return true;
			}
			case MCTextureType::Tex_CubeArray:
			{
				OutStr = "CubeArray";
				return true;
			}
		}
		return false;
	}

	// ---------------------------------------------------------------------------
	// Texture Sampler Template
	// ---------------------------------------------------------------------------

	//	Example JSON:
	//	{
	//		"Name"				:		"DefaultSampler",
	//		"Filter"			:		"Anisotropic",
	//		"AddressU"			:		"Clamp",
	//		"AddressV"			:		"Clamp",
	//		"AddressW"			:		"Clamp",
	//		"ComparisonFunc"	:		"Never",
	//		"BorderColor"		:		[0.0, 0.0, 0.0, 0.0],
	//		"MinLOD"			:		0.0,
	//		"MaxLOD"			:		1024.0,
	//		"LODBias"			:		0.0,
	//		"Anisotropy"		:		2,
	//		"Slot"				:		0,
	//		"Define"			:		"SOME_DEFINE",
	//		"DefineOp"			:		">=",
	//		"DefineRef"			:		1
	//	}

	static bool ParseTextureSamplerTemplate(const rapidjson::Value& InJson, int InSlot, MCTextureSamplerTemplate& OutTmpl)
	{
		const char* NameTxt = "Name";
		const char* FilterTxt = "Filter";
		const char* AddressUTxt = "AddressU";
		const char* AddressVTxt = "AddressV";
		const char* AddressWTxt = "AddressW";
		const char* ComparisonFuncTxt = "ComparisonFunc";
		const char* BorderColorTxt = "BorderColor";
		const char* MinLODTxt = "MinLOD";
		const char* MaxLODTxt = "MaxLOD";
		const char* LODBiasTxt = "LODBias";
		const char* AnisotropyTxt = "Anisotropy";
		const char* SlotTxt = "Slot";
		const char* ExposureTxt = "Exposure";

		MCLogger& MCLog = MCLogger::Get();

		if (!(InJson.HasMember(NameTxt) && InJson[NameTxt].IsString()))
		{
			MCLog.Error("ParseTextureSamplerTemplate sampler template missing valid 'Name'");
			return false;
		}
		OutTmpl.Sampler.Name = InJson[NameTxt].GetString();
		if ((InSlot < 0) && (!(InJson.HasMember(SlotTxt))))
		{
			MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' does not contain valid 'Slot'");
			return false;
		}
		OutTmpl.Sampler.Slot = (InJson.HasMember(SlotTxt)) ? (InJson[SlotTxt].GetInt()) : InSlot;
		if ((OutTmpl.Sampler.Slot < 0) || (OutTmpl.Sampler.Slot > 15))
		{
			MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' sampler template Slot = " + std::to_string(OutTmpl.Sampler.Slot) + " out of range [0, 15]");
			return false;
		}
		if (!(InJson.HasMember(FilterTxt) && InJson[FilterTxt].IsString()))
		{
			MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' sampler template missing valid 'Filter'");
			return false;
		}
		if (!ParseTextureSamplerFilterStr(InJson[FilterTxt].GetString(), OutTmpl.Sampler.Filter))
		{
			MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' sampler template Filter = '" + std::string(InJson[FilterTxt].GetString()) + "' not valid");
			return false;
		}
		if (InJson.HasMember(AddressUTxt) && InJson[AddressUTxt].IsString())
		{
			if (!ParseTextureSamplerAddressStr(InJson[AddressUTxt].GetString(), OutTmpl.Sampler.AddressU))
			{
				MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' sampler template AddressU = '" + std::string(InJson[AddressUTxt].GetString()) + "' not valid");
				return false;
			}
		}
		else
		{
			OutTmpl.Sampler.AddressU = MCTexSamplerAddressModeType::Tex_Sampler_Clamp;
			MCLog.Info("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' does not contain 'AddressU' so use default Clamp");
		}
		if (InJson.HasMember(AddressVTxt) && InJson[AddressVTxt].IsString())
		{
			if (!ParseTextureSamplerAddressStr(InJson[AddressVTxt].GetString(), OutTmpl.Sampler.AddressV))
			{
				MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' sampler template AddressV = '" + std::string(InJson[AddressVTxt].GetString()) + "' not valid");
				return false;
			}
		}
		else
		{
			OutTmpl.Sampler.AddressV = MCTexSamplerAddressModeType::Tex_Sampler_Clamp;
			MCLog.Info("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' does not contain 'AddressV' so use default Clamp");
		}
		if (InJson.HasMember(AddressWTxt) && InJson[AddressWTxt].IsString())
		{
			if (!ParseTextureSamplerAddressStr(InJson[AddressWTxt].GetString(), OutTmpl.Sampler.AddressW))
			{
				MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' sampler template AddressW = '" + std::string(InJson[AddressWTxt].GetString()) + "' not valid");
				return false;
			}
		}
		else
		{
			OutTmpl.Sampler.AddressW = MCTexSamplerAddressModeType::Tex_Sampler_Clamp;
			MCLog.Info("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' does not contain 'AddressW' so use default Clamp");
		}
		if (InJson.HasMember(ComparisonFuncTxt) && InJson[ComparisonFuncTxt].IsString())
		{
			if (!ParseTextureComparisonFuncStr(InJson[ComparisonFuncTxt].GetString(), OutTmpl.Sampler.ComparisonFunc))
			{
				MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' sampler template ComparisonFunc = '" + std::string(InJson[ComparisonFuncTxt].GetString()) + "' not valid");
				return false;
			}
		}
		else
		{
			MCLog.Info("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' does not contain 'ComparisonFunc' so use default Never");
		}
		if (InJson.HasMember(BorderColorTxt) && InJson[BorderColorTxt].IsArray())
		{
			int Idx = 0;
			for (const auto& Elem : InJson[BorderColorTxt].GetArray())
			{
				if (Idx < 4)
				{
					OutTmpl.Sampler.BorderColor[Idx] = Elem.GetFloat();
				}
				++Idx;
			}
			for (; Idx < 4; ++Idx)
			{
				OutTmpl.Sampler.BorderColor[Idx] = 0.0f;
			}
		}
		else
		{
			MCLog.Info("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' does not contain valid 'BorderColor' so use default [0, 0, 0, 0]");
		}
		if (InJson.HasMember(MinLODTxt) && InJson[MinLODTxt].IsNumber())
		{
			OutTmpl.Sampler.MinLOD = InJson[MinLODTxt].GetFloat();
		}
		else
		{
			MCLog.Info("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' does not contain valid 'MinLOD' so use default 0.0");
		}
		if (InJson.HasMember(MaxLODTxt) && InJson[MaxLODTxt].IsNumber())
		{
			OutTmpl.Sampler.MaxLOD = InJson[MaxLODTxt].GetFloat();
		}
		else
		{
			OutTmpl.Sampler.MaxLOD = 1024.0f;
			MCLog.Info("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' does not contain valid 'MaxLOD' so use default 1024.0");
		}
		if (InJson.HasMember(LODBiasTxt) && InJson[LODBiasTxt].IsNumber())
		{
			OutTmpl.Sampler.LODBias = InJson[LODBiasTxt].GetFloat();
		}
		else
		{
			MCLog.Info("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' does not contain valid 'LODBias' so use default 0.0");
		}
		if (OutTmpl.Sampler.Filter == MCTextureSamplerFilterType::Tex_Filter_Anisotropic)
		{
			if (InJson.HasMember(AnisotropyTxt) && InJson[AnisotropyTxt].IsInt())
			{
				OutTmpl.Sampler.Anisotropy = InJson[AnisotropyTxt].GetInt();
			}
			else
			{
				OutTmpl.Sampler.Anisotropy = 2;
				MCLog.Info("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' Filter is Anisotropic but does not contain valid 'Anisotropy' so use default 2");
			}
		}
		else
		{
			OutTmpl.Sampler.Anisotropy = 1;
		}
		OutTmpl.Sampler.bExposure = false;
		if (InJson.HasMember(ExposureTxt) && InJson[ExposureTxt].IsBool() && InJson[ExposureTxt].GetBool())
		{
			OutTmpl.Sampler.bExposure = true;
		}

		if (!ParseConditionField(InJson, OutTmpl.Condition))
		{
			return false;
		}

		return true;
	}

	static bool ExpandTextureSamplerTemplate(const MCTextureSamplerTemplate& InTmpl, const std::vector<MCDefine>& InDefs, MCTextureSampler& OutSampler)
	{
		if (!CheckConditionField(InTmpl.Condition, InDefs))
		{
			return false;
		}
		OutSampler = InTmpl.Sampler;
		return true;
	}

	// ---------------------------------------------------------------------------
	// Texture Template
	// ---------------------------------------------------------------------------

	//	Example JSON:
	//	{
	//		"Name"			:		"AlbedoTex",
	//		"SamplerName"	:		"DefaultSampler",
	//		"Type"			:		"2D",
	//		"Define"		:		"USE_ALBEDO",
	//		"DefineOp"		:		">=",
	//		"DefineRef"		:		1
	//	}

	static bool ParseTextureTemplate(bool InIsOutput, const rapidjson::Value& InJson, int InSlot, MCTextureTemplate& OutTmpl)
	{
		const char* NameTxt = "Name";
		const char* SamplerNameTxt = "SamplerName";
		const char* FormatTxt = "Format";
		const char* TypeTxt = "Type";
		const char* SlotTxt = "Slot";
		const char* ExposureTxt = "Exposure";

		MCLogger& MCLog = MCLogger::Get();

		if (!(InJson.HasMember(NameTxt) && InJson[NameTxt].IsString()))
		{
			MCLog.Error("ParseTextureTemplate texture template missing valid 'Name'");
			return false;
		}
		OutTmpl.Texture.Name = InJson[NameTxt].GetString();
		if ((InSlot < 0) && (!(InJson.HasMember(SlotTxt))))
		{
			MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture template missing valid 'Slot'");
			return false;
		}
		OutTmpl.Texture.Slot = (InJson.HasMember(SlotTxt)) ? (InJson[SlotTxt].GetInt()) : InSlot;
		if ((OutTmpl.Texture.Slot < 0) || (OutTmpl.Texture.Slot > (InIsOutput ? 15 : 127)))
		{
			MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture template Slot = " + std::to_string(OutTmpl.Texture.Slot) + " out of range [0, " + std::to_string(InIsOutput ? 15 : 127) + "]");
			return false;
		}
		OutTmpl.Texture.SamplerName = "DefaultSampler";
		if (InJson.HasMember(SamplerNameTxt) && InJson[SamplerNameTxt].IsString())
		{
			OutTmpl.Texture.SamplerName = InJson[SamplerNameTxt].GetString();
		}
		else
		{
			MCLog.Info("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' does not contain valid 'SamplerName' so use default 'DefaultSampler'");
		}
		std::string FormatStr = "float4";
		if (!(InJson.HasMember(FormatTxt) && InJson[FormatTxt].IsString()))
		{
			MCLog.Info("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' does not contain valid 'Format' so use default 'float4'");
		}
		else
		{
			FormatStr = InJson[FormatTxt].GetString();
			std::string PrefixStr, SuffixStr;
			SplitVariableFormatStr(FormatStr, PrefixStr, SuffixStr);
			if (!ParseVariableFormatPrefixStr(PrefixStr, OutTmpl.Texture.Format.Prefix))
			{
				MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture format '" + FormatStr + "' prefix '" + PrefixStr + "' invalid");
				return false;
			}
			if (!ParseVariableFormatSuffixStr(SuffixStr, OutTmpl.Texture.Format.Suffix))
			{
				MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture format '" + FormatStr + "' suffix '" + SuffixStr + "' invalid");
				return false;
			}
		}
		if (!(InJson.HasMember(TypeTxt) && InJson[TypeTxt].IsString()))
		{
			MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture template missing valid 'Type'");
			return false;
		}
		if (!ParseTextureTypeStr(InJson[TypeTxt].GetString(), OutTmpl.Texture.Type))
		{
			MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture template Type = '" + std::string(InJson[TypeTxt].GetString()) + "' not valid");
			return false;
		}
		const bool bIsOutputType = (OutTmpl.Texture.Type == MCTextureType::Tex_1D) ||
			(OutTmpl.Texture.Type == MCTextureType::Tex_1DArray) ||
			(OutTmpl.Texture.Type == MCTextureType::Tex_2D) ||
			(OutTmpl.Texture.Type == MCTextureType::Tex_2DArray) ||
			(OutTmpl.Texture.Type == MCTextureType::Tex_3D);
		if (InIsOutput && (!bIsOutputType))
		{
			std::string TypeStr = "";
			ParseTextureTypeEnum(OutTmpl.Texture.Type, TypeStr);
			MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture type '" + TypeStr + "' can not be bound as output (RW) resource");
			return false;
		}
		OutTmpl.Texture.bExposure = false;
		if (InJson.HasMember(ExposureTxt) && InJson[ExposureTxt].IsBool() && InJson[ExposureTxt].GetBool())
		{
			OutTmpl.Texture.bExposure = true;
		}

		if (!ParseConditionField(InJson, OutTmpl.Condition))
		{
			return false;
		}

		return true;
	}

	static bool ExpandTextureTemplate(const MCTextureTemplate& InTmpl, const std::vector<MCDefine>& InDefs, MCTexture& OutTexture)
	{
		if (!CheckConditionField(InTmpl.Condition, InDefs))
		{
			return false;
		}
		OutTexture = InTmpl.Texture;
		return true;
	}

	// ---------------------------------------------------------------------------
	// Shader Resource enum <-> string helpers
	// ---------------------------------------------------------------------------

	static bool ParseShaderResourceTypeStr(const std::string& InStr, MCShaderResourceType& OutType)
	{
		if (InStr == "Buffer")
		{
			OutType = MCShaderResourceType::Buff_Buffer;
			return true;
		}
		if (InStr == "ByteAddressBuffer")
		{
			OutType = MCShaderResourceType::Buff_ByteAddressBuffer;
			return true;
		}
		if (InStr == "StructuredBuffer")
		{
			OutType = MCShaderResourceType::Buff_StructuredBuffer;
			return true;
		}
		return false;
	}

	static bool ParseShaderResourceTypeEnum(MCShaderResourceType InType, std::string& OutStr)
	{
		switch (InType)
		{
			case MCShaderResourceType::Buff_Buffer:
			{
				OutStr = "Buffer";
				return true;
			}
			case MCShaderResourceType::Buff_ByteAddressBuffer:
			{
				OutStr = "ByteAddressBuffer";
				return true;
			}
			case MCShaderResourceType::Buff_StructuredBuffer:
			{
				OutStr = "StructuredBuffer";
				return true;
			}
		}
		return false;
	}

	// ---------------------------------------------------------------------------
	// Shader Resource Template
	// ---------------------------------------------------------------------------

	//	Example JSON - Buffer<float4>:
	//	{ "Name": "ColorBuffer", "Type": "Buffer", "ElementType": "float4", "Slot": 0 }
	//
	//	Example JSON - ByteAddressBuffer:
	//	{ "Name": "RawData", "Type": "ByteAddressBuffer", "Slot": 2 }
	//
	//	Example JSON - StructuredBuffer<uint>:
	//	{ "Name": "IndexBuffer", "Type": "StructuredBuffer", "ElementType": "uint", "Slot": 3 }
	//
	//	Example JSON - StructuredBuffer with struct (fields support Define for conditional inclusion):
	//	{
	//		"Name": "LightBuffer",
	//		"Type": "StructuredBuffer",
	//		"Slot": 4,
	//		"Struct": [
	//			{ "Name": "Direction",  "Format": "float3" },
	//			{ "Name": "Intensity",  "Format": "float",  "Define": "USE_INTENSITY" },
	//			{ "Name": "Color",      "Format": "color4", "Default": [1, 1, 1, 1] }
	//		]
	//	}

	static bool ParseShaderResourceTemplate(bool InIsOutput, const rapidjson::Value& InJson, int InSlot, const std::vector<MCDefine>& InDefs, MCShaderResourceTemplate& OutTmpl)
	{
		const char* NameTxt = "Name";
		const char* TypeTxt = "Type";
		const char* ElementTypeTxt = "ElementType";
		const char* StructTxt = "Struct";
		const char* SlotTxt = "Slot";
		const char* ExposureTxt = "Exposure";

		MCLogger& MCLog = MCLogger::Get();

		if (!(InJson.HasMember(NameTxt) && InJson[NameTxt].IsString()))
		{
			MCLog.Error("ParseShaderResourceTemplate resource template missing valid 'Name'");
			return false;
		}
		OutTmpl.Resource.Name = InJson[NameTxt].GetString();
		if ((InSlot < 0) && (!(InJson.HasMember(SlotTxt))))
		{
			MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' does not contain valid 'Slot'");
			return false;
		}
		OutTmpl.Resource.Slot = (InJson.HasMember(SlotTxt)) ? (InJson[SlotTxt].GetInt()) : InSlot;
		if ((OutTmpl.Resource.Slot < 0) || (OutTmpl.Resource.Slot > (InIsOutput ? 15 : 127)))
		{
			MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' resource template Slot = " + std::to_string(OutTmpl.Resource.Slot) + " out of range [0, " + std::to_string(InIsOutput ? 15 : 127) + "]");
			return false;
		}
		if (!(InJson.HasMember(TypeTxt) && InJson[TypeTxt].IsString()))
		{
			MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' resource template missing valid 'Type'");
			return false;
		}
		if (!ParseShaderResourceTypeStr(InJson[TypeTxt].GetString(), OutTmpl.Resource.Type))
		{
			MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' resource template Type = '" + std::string(InJson[TypeTxt].GetString()) + "' not valid");
			return false;
		}
		if (OutTmpl.Resource.Type == MCShaderResourceType::Buff_Buffer)
		{
			if (!(InJson.HasMember(ElementTypeTxt) && InJson[ElementTypeTxt].IsString()))
			{
				MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' Buffer type requires 'ElementType'");
				return false;
			}
			std::string StrPrefix, StrSuffix;
			SplitVariableFormatStr(InJson[ElementTypeTxt].GetString(), StrPrefix, StrSuffix);
			if (!ParseVariableFormatPrefixStr(StrPrefix, OutTmpl.Resource.ElemFmt.Prefix))
			{
				MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' ElementType prefix = '" + StrPrefix + "' not valid");
				return false;
			}
			if (!ParseVariableFormatSuffixStr(StrSuffix, OutTmpl.Resource.ElemFmt.Suffix))
			{
				MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' ElementType suffix = '" + StrSuffix + "' not valid");
				return false;
			}
			OutTmpl.Resource.bHasStruct = false;
		}
		else if (OutTmpl.Resource.Type == MCShaderResourceType::Buff_StructuredBuffer)
		{
			bool bHasStruct = (InJson.HasMember(StructTxt) && InJson[StructTxt].IsArray());
			bool bHasElementType = (InJson.HasMember(ElementTypeTxt) && InJson[ElementTypeTxt].IsString());
			if (bHasStruct)
			{
				if (bHasElementType)
				{
					MCLog.Info("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' both 'Struct' and 'ElementType' provided, 'Struct' takes precedence, ignoring 'ElementType'");
				}
				for (const auto& Field : InJson[StructTxt].GetArray())
				{
					MCVariableInputTemplate InputTmpl{};
					if (!ParseVariableInputTemplate(Field, InputTmpl))
					{
						return false;
					}
					MCVariableInput Input{};
					if (ExpandVariableInputTemplate(InputTmpl, InDefs, Input))
					{
						OutTmpl.Resource.StructFields.emplace_back(std::move(Input));
					}
				}
				if (OutTmpl.Resource.StructFields.empty())
				{
					MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' 'Struct' array is empty or all fields filtered by conditions");
					return false;
				}
				OutTmpl.Resource.bHasStruct = true;
			}
			else if (bHasElementType)
			{
				std::string StrPrefix, StrSuffix;
				SplitVariableFormatStr(InJson[ElementTypeTxt].GetString(), StrPrefix, StrSuffix);
				if (!ParseVariableFormatPrefixStr(StrPrefix, OutTmpl.Resource.ElemFmt.Prefix))
				{
					MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' ElementType prefix = '" + StrPrefix + "' not valid");
					return false;
				}
				if (!ParseVariableFormatSuffixStr(StrSuffix, OutTmpl.Resource.ElemFmt.Suffix))
				{
					MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' ElementType suffix = '" + StrSuffix + "' not valid");
					return false;
				}
				OutTmpl.Resource.bHasStruct = false;
			}
			else
			{
				MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' StructuredBuffer requires either 'Struct' or 'ElementType'");
				return false;
			}
		}
		// ByteAddressBuffer: no extra fields needed
		OutTmpl.Resource.bExposure = false;
		if (InJson.HasMember(ExposureTxt) && InJson[ExposureTxt].IsBool() && InJson[ExposureTxt].GetBool())
		{
			OutTmpl.Resource.bExposure = true;
		}

		if (!ParseConditionField(InJson, OutTmpl.Condition))
		{
			return false;
		}

		return true;
	}

	static bool ExpandShaderResourceTemplate(const MCShaderResourceTemplate& InTmpl, const std::vector<MCDefine>& InDefs, MCShaderResource& OutResource)
	{
		if (!CheckConditionField(InTmpl.Condition, InDefs))
		{
			return false;
		}
		OutResource = InTmpl.Resource;
		return true;
	}

	static bool ParseShaderBindings(const rapidjson::Value& InJson, const std::vector<MCDefine>& InDefs, const std::string& InDebugJsonFilePath, MCShaderBindings& OutBindings)
	{
		const char* ConstantBuffersTxt = "ConstantBuffers";
		const char* SamplersTxt = "Samplers";
		const char* TexturesTxt = "Textures";
		const char* ResourcesTxt = "Resources";
		const char* RWTexturesTxt = "RWTextures";
		const char* RWResourcesTxt = "RWResources";

		MCLogger& MCLog = MCLogger::Get();

		// ConstantBuffers
		if (InJson.HasMember(ConstantBuffersTxt) && InJson[ConstantBuffersTxt].IsArray())
		{
			for (const auto& Elem : InJson[ConstantBuffersTxt].GetArray())
			{
				MCConstantBuffer CB{};
				if (!ParseConstantBuffer(Elem, InDefs, -1, CB))
				{
					return false;
				}
				MCLog.Verbose("ParseShaderBindings ConstantBuffer '" + CB.Name + "' slot=" + std::to_string(CB.Slot) + " fields=" + std::to_string(CB.Fields.size()));
				OutBindings.CBuffers.emplace_back(std::move(CB));
			}
		}
		else
		{
			MCLog.Info("ParseShaderBindings no 'ConstantBuffers' section found");
		}

		// Samplers (TextureSamplers)
		if (InJson.HasMember(SamplersTxt) && InJson[SamplersTxt].IsArray())
		{
			for (const auto& Elem : InJson[SamplersTxt].GetArray())
			{
				MCTextureSamplerTemplate Tmpl{};
				if (!ParseTextureSamplerTemplate(Elem, -1, Tmpl))
				{
					return false;
				}
				MCTextureSampler Sampler{};
				if (!ExpandTextureSamplerTemplate(Tmpl, InDefs, Sampler))
				{
					return false;
				}
				std::string FilterStr;
				ParseTextureSamplerFilterEnum(Sampler.Filter, FilterStr);
				MCLog.Verbose("ParseShaderBindings TextureSampler '" + Sampler.Name + "' slot=" + std::to_string(Sampler.Slot) + " filter=" + FilterStr);
				OutBindings.TexSamplers.emplace_back(std::move(Sampler));
			}
		}
		else
		{
			MCLog.Info("ParseShaderBindings no 'Samplers' section found");
		}

		auto ProcessTex = [&InJson, &InDefs, &MCLog](const char* InTxt, bool InIsOutput, const std::string& InPrefixStr, std::vector<MCTexture>& Outer)->bool
		{
			if (InJson.HasMember(InTxt) && InJson[InTxt].IsArray())
			{
				for (const auto& Elem : InJson[InTxt].GetArray())
				{
					MCTextureTemplate Tmpl{};
					if (!ParseTextureTemplate(InIsOutput, Elem, -1, Tmpl))
					{
						return false;
					}
					MCTexture Tex{};
					if (!ExpandTextureTemplate(Tmpl, InDefs, Tex))
					{
						return false;
					}
					std::string TypeStr;
					ParseTextureTypeEnum(Tex.Type, TypeStr);
					MCLog.Verbose("ParseShaderBindings " + InPrefixStr + "Texture '" + Tex.Name + "' type=" + TypeStr + " sampler=" + Tex.SamplerName);
					Outer.emplace_back(std::move(Tex));
				}
			}
			else
			{
				MCLog.Info("ParseShaderBindings no '" + InPrefixStr + "Textures' section found");
			}
			return true;
		};

		auto ProcessRes = [&InJson, &InDefs, &MCLog](const char* InTxt, bool InIsOutput, const std::string& InPrefixStr, std::vector<MCShaderResource>& Outer)->bool
		{
			if (InJson.HasMember(InTxt) && InJson[InTxt].IsArray())
			{
				for (const auto& Elem : InJson[InTxt].GetArray())
				{
					MCShaderResourceTemplate Tmpl{};
					if (!ParseShaderResourceTemplate(InIsOutput, Elem, -1, InDefs, Tmpl))
					{
						return false;
					}
					MCShaderResource Res{};
					if (!ExpandShaderResourceTemplate(Tmpl, InDefs, Res))
					{
						return false;
					}
					std::string TypeStr;
					ParseShaderResourceTypeEnum(Res.Type, TypeStr);
					MCLog.Verbose("ParseShaderBindings " + InPrefixStr + "ShaderResource '" + Res.Name + "' slot=" + std::to_string(Res.Slot) + " type=" + TypeStr);
					Outer.emplace_back(std::move(Res));
				}
			}
			else
			{
				MCLog.Info("ParseShaderBindings no '" + InPrefixStr + "Resources' section found");
			}
			return true;
		};

		// Textures
		if (!ProcessTex(TexturesTxt, false, "", OutBindings.Textures))
		{
			return false;
		}

		// Resources (ShaderResources)
		if (!ProcessRes(ResourcesTxt, false, "", OutBindings.Resources))
		{
			return false;
		}

		// RWTextures
		if (!ProcessTex(RWTexturesTxt, true, "RW", OutBindings.RWTextures))
		{
			return false;
		}

		// RWResources
		if (!ProcessRes(RWResourcesTxt, true, "RW", OutBindings.RWResources))
		{
			return false;
		}

		int TotalCount = static_cast<int>(
			OutBindings.CBuffers.size() +
			OutBindings.TexSamplers.size() +
			OutBindings.Textures.size() +
			OutBindings.Resources.size() +
			OutBindings.RWTextures.size() +
			OutBindings.RWResources.size());
		MCLog.Info("ParseShaderBindings '" + InDebugJsonFilePath + "' total bindings parsed: " + std::to_string(TotalCount));

		return (TotalCount > 0);
	}

	static bool ParseDefines(const std::string& InDebugJsonFilePath, const rapidjson::Document& InDoc, std::vector<MCDefine>& OutDefines)
	{
		const char* DefinesTxt = "Defines";
		const char* DefineNameTxt = "Name";
		const char* DefineValueTxt = "Value";

		MCLogger& MCLog = MCLogger::Get();

		int NumValidDefines = 0;

		if (InDoc.HasMember(DefinesTxt))
		{
			for (const auto& DefElem : InDoc[DefinesTxt].GetArray())
			{
				std::string DefName = "";
				if (DefElem.HasMember(DefineNameTxt) && DefElem[DefineNameTxt].IsString())
				{
					DefName = DefElem[DefineNameTxt].GetString();
				}
				if (DefName.empty())
				{
					MCLog.Info("ParseDefines invalid define entry in JSON: " + InDebugJsonFilePath + " contains empty/invalid define name: " + DefName);
				}
				else
				{
					int DefValue = 1;
					if (DefElem.HasMember(DefineValueTxt) && DefElem[DefineValueTxt].IsInt())
					{
						DefValue = DefElem[DefineValueTxt].GetInt();
					}
					MCDefine Def;
					Def.Name = DefName;
					Def.Value = DefValue;
					OutDefines.emplace_back(std::move(Def));
					NumValidDefines++;
				}
			}
		}
		else
		{
			MCLog.Info("ParseDefines no 'Defines' section found in JSON: " + InDebugJsonFilePath);
		}

		return (NumValidDefines > 0);
	}

	static bool ParseReplaces(const std::string& InDebugJsonFilePath, const rapidjson::Document& InDoc, std::vector<MCReplace>& OutReplaces)
	{
		const char* ReplacesTxt = "Replaces";
		const char* ReplaceNameTxt = "Name";
		const char* ReplaceElementTxt = "Element";

		MCLogger& MCLog = MCLogger::Get();

		int NumValidReplaces = 0;

		if (InDoc.HasMember(ReplacesTxt))
		{
			for (const auto& RplElem : InDoc[ReplacesTxt].GetArray())
			{
				std::string RplName = "";
				if (RplElem.HasMember(ReplaceNameTxt) && RplElem[ReplaceNameTxt].IsString())
				{
					RplName = RplElem[ReplaceNameTxt].GetString();
				}
				if (RplName.empty())
				{
					MCLog.Info("ParseReplaces invalid replace entry in JSON: " + InDebugJsonFilePath + " contains empty/invalid replace name: " + RplName);
				}
				else
				{
					std::string RplElems = "";
					if (RplElem.HasMember(ReplaceElementTxt) && RplElem[ReplaceElementTxt].IsString())
					{
						RplElems = RplElem[ReplaceElementTxt].GetString();
					}
					MCReplace Rpl;
					Rpl.Name = RplName;
					Rpl.Element = RplElems;
					OutReplaces.emplace_back(std::move(Rpl));
					NumValidReplaces++;
				}
			}
		}
		else
		{
			MCLog.Info("ParseReplaces no 'Replaces' section found in JSON: " + InDebugJsonFilePath);
		}

		return (NumValidReplaces > 0);
	}

	enum class MCParseHeaderState : uint8_t
	{
		Parse_None,
		Parse_Define_Only,
		Parse_Replace_Only,
		Parse_All
	};

	static bool ParseShaderComponent(const std::string& InRootDir, const std::string& InDebugJsonFilePath, const rapidjson::Document& InDoc, MCShaderComponent& OutShaderComponent, MCFinalDefine& InOutTotalDefines, MCFinalReplace& InOutTotalReplaces, MCParseHeaderState InState)
	{
		MCLogger& MCLog = MCLogger::Get();

		if ((InState == MCParseHeaderState::Parse_Define_Only) || (InState == MCParseHeaderState::Parse_All))
		{
			std::vector<MCDefine> TmpDefs;
			if (ParseDefines(InDebugJsonFilePath, InDoc, TmpDefs))
			{
				for (const auto& Def : TmpDefs)
				{
					InOutTotalDefines.DefineMappings[Def.Name] = Def.Value;
				}
			}
		}

		if ((InState == MCParseHeaderState::Parse_Replace_Only) || (InState == MCParseHeaderState::Parse_All))
		{
			std::vector<MCReplace> TmpRpls;
			if (ParseReplaces(InDebugJsonFilePath, InDoc, TmpRpls))
			{
				for (const auto& Rpl : TmpRpls)
				{
					InOutTotalReplaces.ReplaceMappings[Rpl.Name] = Rpl.Element;
				}
			}
		}

		if (InState != MCParseHeaderState::Parse_None)
		{
			return true;
		}

		const char* TemplateTxt = "Template";

		if (!(InDoc.HasMember(TemplateTxt) && InDoc[TemplateTxt].IsString()))
		{
			MCLog.Error("ParseShaderComponent no valid 'Template' section found in JSON: " + InDebugJsonFilePath);
			return false;
		}
		OutShaderComponent.Template.clear();
		const std::string TemplateHlsl = InDoc[TemplateTxt].GetString();
		if (!LoadHlsl(InRootDir + TemplateHlsl, OutShaderComponent.Template))
		{
			MCLog.Error("ParseShaderComponent failed to load template HLSL file: " + InRootDir + TemplateHlsl + " in JSON: " + InDebugJsonFilePath);
			return false;
		}
		OutShaderComponent.Defines.clear();
		ParseDefines(InDebugJsonFilePath, InDoc, OutShaderComponent.Defines);
		OutShaderComponent.Replaces.clear();
		ParseReplaces(InDebugJsonFilePath, InDoc, OutShaderComponent.Replaces);
		return true;
	}

	static bool ParseShaderBase(const std::string& InRootDir, const std::string& InDebugJsonFilePath, const rapidjson::Document& InDoc, MCShaderBase& OutShaderBase, MCFinalDefine& InOutTotalDefines, MCFinalReplace& InOutTotalReplaces, MCParseHeaderState InState)
	{
		MCLogger& MCLog = MCLogger::Get();
		if (!ParseShaderComponent(InRootDir, InDebugJsonFilePath, InDoc, OutShaderBase.ShaderComponent, InOutTotalDefines, InOutTotalReplaces, InState))
		{
			return false;
		}
		if (InState != MCParseHeaderState::Parse_None)
		{
			return true;
		}
		if (!ParseShaderBindings(InDoc, InOutTotalDefines.Defines, InDebugJsonFilePath, OutShaderBase.Bindings))
		{
			MCLog.Info("ParseShaderBase '" + InDebugJsonFilePath + "' no shader bindings found");
		}
		else
		{
			MCLog.Info("ParseShaderBase '" + InDebugJsonFilePath + "' shader bindings parsed successfully");
		}
		return true;
	}

	static bool ParseVertexFactory(const std::string& InRootDir, const std::string& InDebugJsonPath, const rapidjson::Document& InDoc, MCVertexFactory& OutVF, MCFinalDefine& InOutTotalDefines, MCFinalReplace& InOutTotalReplaces, MCParseHeaderState InState)
	{
		const char* NameTxt = "Name";

		MCLogger& MCLog = MCLogger::Get();

		if (!ParseShaderBase(InRootDir, InDebugJsonPath, InDoc, OutVF.Shader, InOutTotalDefines, InOutTotalReplaces, InState))
		{
			return false;
		}
		if (InState != MCParseHeaderState::Parse_None)
		{
			return true;
		}

		if (!(InDoc.HasMember(NameTxt) && InDoc[NameTxt].IsString()))
		{
			MCLog.Error("ParseVertexFactory missing valid 'Name' in: " + InDebugJsonPath);
			return false;
		}
		OutVF.Name = InDoc[NameTxt].GetString();
		MCLog.Info("ParseVertexFactory '" + OutVF.Name + "'");

		if (!ParseVFInputStream(InDoc, InOutTotalDefines.Defines, -1, OutVF.InputStream))
		{
			MCLog.Info("ParseVertexFactory '" + OutVF.Name + "' failed to parse InputStream");
		}

		return true;
	}

	static bool ParseGeneralShader(const std::string& InRootDir, const std::string& InDebugJsonPath, const rapidjson::Document& InDoc, MCGeneralShader& OutGShader, MCFinalDefine& InOutTotalDefines, MCFinalReplace& InOutTotalReplaces, MCParseHeaderState InState)
	{
		const char* NameTxt = "Name";

		MCLogger& MCLog = MCLogger::Get();

		if (!ParseShaderBase(InRootDir, InDebugJsonPath, InDoc, OutGShader.Shader, InOutTotalDefines, InOutTotalReplaces, InState))
		{
			return false;
		}
		if (InState != MCParseHeaderState::Parse_None)
		{
			return true;
		}

		if (!(InDoc.HasMember(NameTxt) && InDoc[NameTxt].IsString()))
		{
			MCLog.Error("ParseGeneralShader missing valid 'Name' in: " + InDebugJsonPath);
			return false;
		}
		OutGShader.Name = InDoc[NameTxt].GetString();
		MCLog.Info("ParseGeneralShader '" + OutGShader.Name + "'");

		return true;
	}

	static bool ParseShaderBaseReplaceTypeStr(const std::string& InStr, MCShaderBaseReplaceType& OutType)
	{
		if (InStr == "VertexFactory")
		{
			OutType = MCShaderBaseReplaceType::ShaderReplace_VertexFactory;
			return true;
		}
		if (InStr == "General")
		{
			OutType = MCShaderBaseReplaceType::ShaderReplace_General;
			return true;
		}
		return false;
	}

#if 0	// Not used for now
	static bool ParseShaderBaseReplaceTypeEnum(MCShaderBaseReplaceType InType, std::string& OutStr)
	{
		switch (InType)
		{
			case MCShaderBaseReplaceType::ShaderReplace_VertexFactory:
			{
				OutStr = "VertexFactory";
				return true;
			}
			case MCShaderBaseReplaceType::ShaderReplace_General:
			{
				OutStr = "General";
				return true;
			}
		}
		return false;
	}
#endif	// 0 : Not used for now

	static bool ParseShaderBaseReplace(const rapidjson::Value& InJson, int InIndex, MCShaderBaseReplace& OutShaderRpl)
	{
		const char* NameTxt = "Name";
		const char* IndexTxt = "Index";
		const char* TypeTxt = "Type";
		const char* ReplaceTxt = "Replace";
		const char* JsonFileTxt = "JsonFile";

		MCLogger& MCLog = MCLogger::Get();

		if (!(InJson.HasMember(NameTxt) && InJson[NameTxt].IsString()))
		{
			MCLog.Error("ParseShaderBaseReplace missing valid 'Name'");
			return false;
		}
		OutShaderRpl.Name = InJson[NameTxt].GetString();
		if ((InIndex < 0) && (!(InJson.HasMember(IndexTxt))))
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' missing valid 'Index'");
			return false;
		}
		OutShaderRpl.Index = InJson.HasMember(IndexTxt) ? (InJson[IndexTxt].GetInt()) : InIndex;
		if ((OutShaderRpl.Index < 0) || (OutShaderRpl.Index > 12))	// ReplaceTemplateStringRecursive max depth is 16
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' Index = " + std::to_string(OutShaderRpl.Index) + " out of range [0, 12]");
			return false;
		}
		if (!(InJson.HasMember(TypeTxt) && InJson[TypeTxt].IsString()))
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' missing valid 'Type'");
			return false;
		}
		if (const std::string TypeStr = InJson[TypeTxt].GetString(); !ParseShaderBaseReplaceTypeStr(TypeStr, OutShaderRpl.Type))
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' unknown 'Type'=" + TypeStr);
			return false;
		}
		if (!(InJson.HasMember(ReplaceTxt) && InJson[ReplaceTxt].IsString()))
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' missing valid 'Replace'");
			return false;
		}
		OutShaderRpl.RplElem = InJson[ReplaceTxt].GetString();
		if (!(InJson.HasMember(JsonFileTxt) && InJson[JsonFileTxt].IsString()))
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' missing valid 'JsonFile'");
			return false;
		}
		OutShaderRpl.JsonFile = InJson[JsonFileTxt].GetString();
		return true;
	}

	template<typename _Ty>
	static bool CombineShaderBindings(const std::string& InDebugName, const std::vector<_Ty>& InBindings, const int InMinSlotIndex, const int InMaxSlotIndex, int& OutBindingOffset, std::vector<_Ty>& OutBindings)
	{
		if (!(InBindings.empty()))
		{
			MCLogger& MCLog = MCLogger::Get();

			const int Nums = static_cast<int>(InBindings.size());
			for (int Idx = 0; Idx < Nums; ++Idx)
			{
				_Ty TmpBinding = InBindings[Idx];
				TmpBinding.Slot = OutBindingOffset + Idx;
				if (TmpBinding.Slot < InMinSlotIndex || TmpBinding.Slot > InMaxSlotIndex)
				{
					MCLog.Error("CombineShaderBindings Name'" + InDebugName + "' Binding'" + TmpBinding.Name + "' FinalSlot'" + std::to_string(TmpBinding.Slot) + "' out of range [" + std::to_string(InMinSlotIndex) + ", " + std::to_string(InMaxSlotIndex) + "]");
					return false;
				}
				OutBindings.emplace_back(std::move(TmpBinding));
			}
			OutBindingOffset += Nums;
		}
		return true;
	}

	bool ParseShader(const MCLoadParam& InLoadParam, std::vector<MCDefine>& InOutDefs, const std::vector<MCReplace>& InRpls, std::string& OutHlsl)
	{
		MCLogger& MCLog = MCLogger::Get();

		const std::string FullJsonFilePath = InLoadParam.RootDir + InLoadParam.JsonFilePath;

		std::string RawJson;
		if (!ReadFile(FullJsonFilePath, RawJson))
		{
			MCLog.Error("ParseShader cannot read: " + FullJsonFilePath);
			return false;
		}
		rapidjson::Document Doc;
		if (!ParseDoc(RawJson, Doc))
		{
			MCLog.Error("ParseShader JSON parse error: " + FullJsonFilePath);
			return false;
		}

		MCLog.Info("ParseShader: " + FullJsonFilePath);

		MCFinalDefine		FinalDef{};
		MCFinalReplace		FinalRpl{};
		MCShader			Shader{};

		const char* NameTxt = "Name";
		const char* ShaderReplacesTxt = "ShaderReplaces";
		const char* EntryTxt = "Entry";

		if (!(Doc.HasMember(NameTxt) && Doc[NameTxt].IsString()))
		{
			MCLog.Error("ParseShader missing valid 'Name' in: " + FullJsonFilePath);
			return false;
		}
		Shader.Name = Doc[NameTxt].GetString();
		if (!(Doc.HasMember(EntryTxt) && Doc[EntryTxt].IsString()))
		{
			MCLog.Error("ParseShader missing valid 'Entry' in: " + FullJsonFilePath);
			return false;
		}
		Shader.Entry = Doc[EntryTxt].GetString();

		Shader.ShaderRpls.clear();
		if (Doc.HasMember(ShaderReplacesTxt) && Doc[ShaderReplacesTxt].IsArray())
		{
			int ShaderRplIndex = 0;
			for (const auto& ShaderRpl : Doc[ShaderReplacesTxt].GetArray())
			{
				MCShaderBaseReplace Rpl{};
				if (ParseShaderBaseReplace(ShaderRpl, ShaderRplIndex, Rpl))
				{
					Shader.ShaderRpls.emplace_back(std::move(Rpl));
					ShaderRplIndex++;
				}
			}
			if (!(Shader.ShaderRpls.empty()))
			{
				std::stable_sort(
					Shader.ShaderRpls.begin(),
					Shader.ShaderRpls.end(),
					[](const MCShaderBaseReplace& InL, const MCShaderBaseReplace& InR) -> bool
					{
						return (InL.Index < InR.Index);
					}
				);
			}
		}
		else
		{
			MCLog.Info("ParseShader no 'ShaderReplaces' section found in: " + FullJsonFilePath);
		}

		struct MCReflectIndex
		{
			int CBufferSlotOffset = 0;
			int ResourceSlotOffset = 0;
			int TexSamplerSlotOffset = 0;
			int RWResourceSlotOffset = 0;
		};

		struct MCFinalInputStream
		{
			std::string									Name;
			std::map<int, std::vector<MCVFInput>>		Inputs;
		};

		MCReflectIndex ReflectIndex{};
		MCFinalInputStream FinalInStream{};
		MCShaderBindings FinalBindings{};
		std::map<std::string, std::string> ShaderRpls;

		FinalInStream.Inputs.clear();
		FinalBindings.CBuffers.clear();
		FinalBindings.TexSamplers.clear();
		FinalBindings.Textures.clear();
		FinalBindings.Resources.clear();
		FinalBindings.RWTextures.clear();
		FinalBindings.RWResources.clear();
		ShaderRpls.clear();

		FinalInStream.Name = "FinalInputStream";

		auto CombineShaderBindingPart = [&ReflectIndex, &FinalBindings](const std::string& InDebugName, const MCShaderBindings& InBindings)->bool
		{
			if (!CombineShaderBindings<MCConstantBuffer>(InDebugName, InBindings.CBuffers, 0, 13, ReflectIndex.CBufferSlotOffset, FinalBindings.CBuffers))
			{
				return false;
			}
			if (!CombineShaderBindings<MCTextureSampler>(InDebugName, InBindings.TexSamplers, 0, 15, ReflectIndex.TexSamplerSlotOffset, FinalBindings.TexSamplers))
			{
				return false;
			}
			if (!CombineShaderBindings<MCTexture>(InDebugName, InBindings.Textures, 0, 127, ReflectIndex.ResourceSlotOffset, FinalBindings.Textures))
			{
				return false;
			}
			if (!CombineShaderBindings<MCShaderResource>(InDebugName, InBindings.Resources, 0, 127, ReflectIndex.ResourceSlotOffset, FinalBindings.Resources))
			{
				return false;
			}
			if (!CombineShaderBindings<MCTexture>(InDebugName, InBindings.RWTextures, 0, 15, ReflectIndex.RWResourceSlotOffset, FinalBindings.RWTextures))
			{
				return false;
			}
			if (!CombineShaderBindings<MCShaderResource>(InDebugName, InBindings.RWResources, 0, 15, ReflectIndex.RWResourceSlotOffset, FinalBindings.RWResources))
			{
				return false;
			}
			return true;
		};

		for (int Iter = 0; Iter < 2; ++Iter)
		{
			const bool bParseOnce = (Iter == 0);

			if (!(Shader.ShaderRpls.empty()))
			{
				for (const auto& ShaderRpl : Shader.ShaderRpls)
				{
					if (ShaderRpl.RplElem.empty() || ShaderRpl.JsonFile.empty())
					{
						MCLog.Error("ParseShader 'ShaderReplace' contains invalid content 'Name'=" + ShaderRpl.Name);
						return false;
					}

					const std::string RplJsonFilePath = InLoadParam.RootDir + ShaderRpl.JsonFile;
					std::string RplRawJson;
					if (!ReadFile(RplJsonFilePath, RplRawJson))
					{
						MCLog.Error("ParseShader cannot read: " + RplJsonFilePath);
						return false;
					}
					rapidjson::Document RplDoc;
					if (!ParseDoc(RplRawJson, RplDoc))
					{
						MCLog.Error("ParseShader JSON parse error: " + RplJsonFilePath);
						return false;
					}

					switch (ShaderRpl.Type)
					{
						case MCShaderBaseReplaceType::ShaderReplace_VertexFactory:
						{
							MCVertexFactory VF{};
							if (!ParseVertexFactory(InLoadParam.RootDir, RplJsonFilePath, RplDoc, VF, FinalDef, FinalRpl, bParseOnce ? MCParseHeaderState::Parse_All : MCParseHeaderState::Parse_None))
							{
								return false;
							}
							if (!bParseOnce)
							{
								if (!(VF.InputStream.Inputs.empty()))
								{
									FinalInStream.Name += "_" + VF.Name;
									for (size_t InputIdx = 0, NumInputs = VF.InputStream.Inputs.size(); InputIdx < NumInputs; ++InputIdx)
									{
										const MCVFInput& TmpInputRef = VF.InputStream.Inputs[InputIdx];
										auto IterInput = FinalInStream.Inputs.find(TmpInputRef.Slot);
										if (IterInput == FinalInStream.Inputs.end())
										{
											FinalInStream.Inputs.insert_or_assign(TmpInputRef.Slot, std::vector<MCVFInput>{});
											IterInput = FinalInStream.Inputs.find(TmpInputRef.Slot);
											if (IterInput != FinalInStream.Inputs.end())
											{
												IterInput->second.clear();
											}
										}
										if (IterInput == FinalInStream.Inputs.end())
										{
											MCLog.Error("ParseShader 'ShaderReplace' VF'" + VF.Name + "' contains invalid 'Slot'=" + std::to_string(TmpInputRef.Slot) + " for VFInput '" + TmpInputRef.Name + "' in '" + RplJsonFilePath + "' add to final input stream failed");
											return false;
										}
										for (size_t ExistIdx = 0, NumExists = IterInput->second.size(); ExistIdx < NumExists; ++ExistIdx)
										{
											const MCVFInput& ExistInputRef = IterInput->second[ExistIdx];
											if (ExistInputRef.Semantic == TmpInputRef.Semantic)
											{
												MCLog.Error("ParseShader 'ShaderReplace' VF'" + VF.Name + "' contains duplicate 'Semantic'=" + TmpInputRef.Semantic + " for VFInput '" + TmpInputRef.Name + "' in '" + RplJsonFilePath + "'");
												return false;
											}
										}
										IterInput->second.push_back(TmpInputRef);
									}
								}
								if (!CombineShaderBindingPart(VF.Name, VF.Shader.Bindings))
								{
									return false;
								}
								if (ShaderRpls.find(ShaderRpl.RplElem) != ShaderRpls.end())
								{
									MCLog.Error("ParseShader 'ShaderReplace' contains duplicate 'RplElem'=" + ShaderRpl.RplElem + " for VF'" + VF.Name + "' in '" + RplJsonFilePath + "'");
									return false;
								}
								ShaderRpls.insert_or_assign(ShaderRpl.RplElem, VF.Shader.ShaderComponent.Template);
							}
							break;
						}
						case MCShaderBaseReplaceType::ShaderReplace_General:
						{
							MCGeneralShader GeneralShader{};
							if (!ParseGeneralShader(InLoadParam.RootDir, RplJsonFilePath, RplDoc, GeneralShader, FinalDef, FinalRpl, bParseOnce ? MCParseHeaderState::Parse_All : MCParseHeaderState::Parse_None))
							{
								return false;
							}
							if (!bParseOnce)
							{
								if (!CombineShaderBindingPart(GeneralShader.Name, GeneralShader.Shader.Bindings))
								{
									return false;
								}
								if (ShaderRpls.find(ShaderRpl.RplElem) != ShaderRpls.end())
								{
									MCLog.Error("ParseShader 'ShaderReplace' contains duplicate 'RplElem'=" + ShaderRpl.RplElem + " for GeneralShader'" + GeneralShader.Name + "' in '" + RplJsonFilePath + "'");
									return false;
								}
								ShaderRpls.insert_or_assign(ShaderRpl.RplElem, GeneralShader.Shader.ShaderComponent.Template);
							}
							break;
						}
						default:
						{
							MCLog.Error("ParseShader 'ShaderReplace' contains invalid 'Type'=" + std::to_string(static_cast<int>(ShaderRpl.Type)) + " for 'Name'=" + ShaderRpl.Name);
							return false;
						}
					}
				}
			}
			if (!ParseShaderBase(InLoadParam.RootDir, FullJsonFilePath, Doc, Shader.Shader, FinalDef, FinalRpl, bParseOnce ? MCParseHeaderState::Parse_All : MCParseHeaderState::Parse_None))
			{
				return false;
			}
			if (bParseOnce)
			{
				for (const auto& InputDef : InOutDefs)
				{
					if (auto IterDef = FinalDef.DefineMappings.find(InputDef.Name); IterDef != FinalDef.DefineMappings.end())
					{
						IterDef->second = InputDef.Value;
					}
					else
					{
						FinalDef.DefineMappings.insert_or_assign(InputDef.Name, InputDef.Value);
					}
				}
				for (const auto& InputRpl : InRpls)
				{
					if (auto IterRpl = FinalRpl.ReplaceMappings.find(InputRpl.Name); IterRpl != FinalRpl.ReplaceMappings.end())
					{
						IterRpl->second = InputRpl.Element;
					}
					else
					{
						FinalRpl.ReplaceMappings.insert_or_assign(InputRpl.Name, InputRpl.Element);
					}
				}
				FinalDef.Defines.clear();
				for (const auto& Def : FinalDef.DefineMappings)
				{
					MCDefine TmpDef;
					TmpDef.Name = Def.first;
					TmpDef.Value = Def.second;
					FinalDef.Defines.emplace_back(std::move(TmpDef));
				}
				FinalRpl.Replaces.clear();
				for (const auto& Rpl : FinalRpl.ReplaceMappings)
				{
					MCReplace TmpRpl;
					TmpRpl.Name = Rpl.first;
					TmpRpl.Element = Rpl.second;
					FinalRpl.Replaces.emplace_back(std::move(TmpRpl));
				}
				MCLog.Info("ParseShader pass 0 complete: collected " + std::to_string(FinalDef.Defines.size()) + " defines, " + std::to_string(FinalRpl.Replaces.size()) + " replaces");
			}
			else
			{
				if (!CombineShaderBindingPart(Shader.Name, Shader.Shader.Bindings))
				{
					return false;
				}
			}
		}

		// Only one constant buffer may be fed through root 32 bit constants: the engine pushes exactly
		// one root constant block, so a second marker inside the assembled hlsl is a source error.
		{
			int NumRootConstantCBuffers = 0;
			std::string RootConstantCBufferNames = "";
			for (const auto& IterCBuffer : FinalBindings.CBuffers)
			{
				if (!IterCBuffer.bRootConstant)
				{
					continue;
				}
				NumRootConstantCBuffers++;
				RootConstantCBufferNames += ((RootConstantCBufferNames.empty() ? "" : ", ") + IterCBuffer.Name);
			}
			if (NumRootConstantCBuffers > 1)
			{
				MCLog.Error("ParseShader '" + Shader.Name + "' contains " + std::to_string(NumRootConstantCBuffers) + " constant buffers marked 'RootConstant' but only one is allowed: " + RootConstantCBufferNames);
				return false;
			}
			if (NumRootConstantCBuffers == 1)
			{
				MCLog.Info("ParseShader '" + Shader.Name + "' root constant buffer: " + RootConstantCBufferNames);
			}
		}

		{
			auto CheckReplaceDiff = [&MCLog](const std::string& InDebugString, const std::string& InSource, const std::map<std::string, std::string>& InRpls)->void
			{
				std::set<std::string> VerifyKeys;
				std::set<std::string> Keys = ExtractTemplatePlaceholders(InSource);
				for (const auto& Key : Keys)
				{
					if (InRpls.find(Key) != InRpls.end())
					{
						VerifyKeys.insert(Key);
					}
					else
					{
						MCLog.Info("CheckReplaceDiff " + InDebugString + " no matching replace rule for Key=" + Key);
					}
				}
				for (const auto& IterRpl : InRpls)
				{
					if (VerifyKeys.find(IterRpl.first) != VerifyKeys.end())
					{
						continue;
					}
					MCLog.Info("CheckReplaceDiff " + InDebugString + " contains extra replaces with Key=" + IterRpl.first);
				}
			};

			// Finalize all shader replaces into hlsl
			OutHlsl = Shader.Shader.ShaderComponent.Template;
			CheckReplaceDiff("MainShaderTemplate", OutHlsl, ShaderRpls);
			ReplaceTemplatePlaceholders(OutHlsl, ShaderRpls);
			{
				std::map<std::string, std::string> TmpRpls;
				for (size_t RplIdx = 0, NumRpls = FinalRpl.Replaces.size(); RplIdx < NumRpls; ++RplIdx)
				{
					const MCReplace& UsedRpl = FinalRpl.Replaces[RplIdx];
					TmpRpls.insert_or_assign(UsedRpl.Name, UsedRpl.Element);
				}
				CheckReplaceDiff("ShaderTemplate", OutHlsl, TmpRpls);
				ReplaceTemplatePlaceholders(OutHlsl, TmpRpls);
			}
		}

		{
			std::string Header = "\n";
			for (const auto& IterCBuffer : FinalBindings.CBuffers)
			{
				Header += "cbuffer " + IterCBuffer.Name + " : register(b" + std::to_string(IterCBuffer.Slot) + ")\n{\n";
				for (const auto& Field : IterCBuffer.Fields)
				{
					std::string FieldPrefixStr, FieldSuffixStr;
					if (!ParseVariableFormatPrefixEnum(Field.Format.Prefix, FieldPrefixStr))
					{
						MCLog.Error("ParseShader Resource '" + IterCBuffer.Name + "' ConstantBuffer '" + Field.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(Field.Format.Prefix)));
						return false;
					}
					if (!ParseVariableFormatSuffixEnum(Field.Format.Suffix, FieldSuffixStr))
					{
						MCLog.Error("ParseShader Resource '" + IterCBuffer.Name + "' ConstantBuffer '" + Field.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(Field.Format.Suffix)));
						return false;
					}
					Header += "\t" + FieldPrefixStr + FieldSuffixStr + " " + Field.Name;
					if (Field.bArray)
					{
						Header += "[" + std::to_string(Field.ArrayNumElems) + "]";
					}
					Header += ";\n";
				}
				Header += "};\n";
			}
			Header += "\n";
			auto ProcessResourceBindings = [&MCLog, &Header](const std::string& InTypePrefixStr, const std::string& InShaderTypePrefixStr, const std::vector<MCTexture>& InTexArr, const std::vector<MCShaderResource>& InResArr)->bool
			{
				for (const auto& Tex : InTexArr)
				{
					std::string PrefixStr;
					if (!ParseVariableFormatPrefixEnum(Tex.Format.Prefix, PrefixStr))
					{
						MCLog.Error("ParseShader " + InTypePrefixStr + "Texture '" + Tex.Name + "' unknown PrefixEnum=" + std::to_string(static_cast<int>(Tex.Format.Prefix)));
						return false;
					}
					std::string SuffixStr;
					if (!ParseVariableFormatSuffixEnum(Tex.Format.Suffix, SuffixStr))
					{
						MCLog.Error("ParseShader " + InTypePrefixStr + "Texture '" + Tex.Name + "' unknown SuffixEnum=" + std::to_string(static_cast<int>(Tex.Format.Suffix)));
						return false;
					}
					std::string TexTypeStr;
					if (!ParseTextureTypeEnum(Tex.Type, TexTypeStr))
					{
						MCLog.Error("ParseShader " + InTypePrefixStr + "Texture '" + Tex.Name + "' unknown TypeEnum=" + std::to_string(static_cast<int>(Tex.Type)));
						return false;
					}
					Header += InTypePrefixStr + "Texture" + TexTypeStr + "<" + PrefixStr + SuffixStr + "> " + Tex.Name + " : register(" + InShaderTypePrefixStr + std::to_string(Tex.Slot) + ");\n";
				}
				Header += "\n";
				for (const auto& Res : InResArr)
				{
					std::string ResTypeStr;
					if (!ParseShaderResourceTypeEnum(Res.Type, ResTypeStr))
					{
						MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' unknown Type Enum=" + std::to_string(static_cast<int>(Res.Type)));
						return false;
					}

					if (Res.Type == MCShaderResourceType::Buff_ByteAddressBuffer)
					{
						Header += InTypePrefixStr + ResTypeStr + " " + Res.Name + " : register(" + InShaderTypePrefixStr + std::to_string(Res.Slot) + ");\n";
					}
					else if (Res.bHasStruct)
					{
						if (Res.Type != MCShaderResourceType::Buff_StructuredBuffer)
						{
							MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' unknown Type '" + std::to_string(static_cast<int>(Res.Type)));
							return false;
						}
						std::string StructName = Res.Name + "_Struct";
						Header += "struct " + StructName + "\n{\n";
						for (const auto& Field : Res.StructFields)
						{
							std::string FieldPrefixStr, FieldSuffixStr;
							if (!ParseVariableFormatPrefixEnum(Field.Format.Prefix, FieldPrefixStr))
							{
								MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' StructField '" + Field.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(Field.Format.Prefix)));
								return false;
							}
							if (!ParseVariableFormatSuffixEnum(Field.Format.Suffix, FieldSuffixStr))
							{
								MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' StructField '" + Field.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(Field.Format.Suffix)));
								return false;
							}
							Header += "\t" + FieldPrefixStr + FieldSuffixStr + " " + Field.Name;
							if (Field.bArray)
							{
								Header += "[" + std::to_string(Field.ArrayNumElems) + "]";
							}
							Header += ";\n";
						}
						Header += "};\n";
						Header += InTypePrefixStr + ResTypeStr + "<" + StructName + "> " + Res.Name + " : register(" + InShaderTypePrefixStr + std::to_string(Res.Slot) + ");\n";
					}
					else
					{
						if (!(Res.Type == MCShaderResourceType::Buff_Buffer || Res.Type == MCShaderResourceType::Buff_StructuredBuffer))
						{
							MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' unknown Type '" + std::to_string(static_cast<int>(Res.Type)));
							return false;
						}
						std::string ElemPrefixStr;
						if (!ParseVariableFormatPrefixEnum(Res.ElemFmt.Prefix, ElemPrefixStr))
						{
							MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' unknown ElementPrefix Enum=" + std::to_string(static_cast<int>(Res.ElemFmt.Prefix)));
							return false;
						}
						std::string ElemSuffixStr;
						if (!ParseVariableFormatSuffixEnum(Res.ElemFmt.Suffix, ElemSuffixStr))
						{
							MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' unknown ElementSuffix Enum=" + std::to_string(static_cast<int>(Res.ElemFmt.Suffix)));
							return false;
						}
						Header += InTypePrefixStr + ResTypeStr + "<" + ElemPrefixStr + ElemSuffixStr + "> " + Res.Name + " : register(" + InShaderTypePrefixStr + std::to_string(Res.Slot) + ");\n";
					}
				}
				return true;
			};
			if (!ProcessResourceBindings("", "t", FinalBindings.Textures, FinalBindings.Resources))
			{
				return false;
			}
			Header += "\n";

			for (const auto& IterSampler : FinalBindings.TexSamplers)
			{
				Header += "SamplerState " + IterSampler.Name + " : register(s" + std::to_string(IterSampler.Slot) + ");\n";
			}
			Header += "\n";
			if (!ProcessResourceBindings("RW", "u", FinalBindings.RWTextures, FinalBindings.RWResources))
			{
				return false;
			}
			Header += "\n";
			if (!(FinalInStream.Inputs.empty()))
			{
				Header += "struct FinalInputStream\n{\n";
				for (const auto& IterInput : FinalInStream.Inputs)
				{
					for (const auto& Input : IterInput.second)
					{
						std::string FormatPrefixStr;
						if (!ParseVariableFormatPrefixEnum(Input.Format.Prefix, FormatPrefixStr))
						{
							MCLog.Error("ParseShader FinalInputStream '" + FinalInStream.Name + "' Input '" + Input.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(Input.Format.Prefix)));
							return false;
						}
						std::string FormatSuffixStr;
						if (!ParseVariableFormatSuffixEnum(Input.Format.Suffix, FormatSuffixStr))
						{
							MCLog.Error("ParseShader FinalInputStream '" + FinalInStream.Name + "' Input '" + Input.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(Input.Format.Suffix)));
							return false;
						}
						Header += "\t" + FormatPrefixStr + FormatSuffixStr + " " + Input.Name + " : " + Input.Semantic + std::to_string(Input.SemanticIdx) + ";\n";
					}
				}
				Header += "};\n\n";
			}

			OutHlsl = Header + OutHlsl;
		}

		{
			std::string ReplaceComments = "///////////////////////////////////////////////////////////////////////////////\n";
			ReplaceComments += "// Shader Replaces (" + std::to_string(FinalRpl.Replaces.size()) + "):\n";
			if (FinalRpl.Replaces.empty())
			{
				ReplaceComments += "//   (None)\n";
			}
			else
			{
				for (const auto& Def : FinalRpl.Replaces)
				{
					ReplaceComments += "//   " + EscapeCommentText(Def.Name) + " = " + EscapeCommentText(Def.Element) + "\n";
				}
			}
			ReplaceComments += "///////////////////////////////////////////////////////////////////////////////\n\n";
			OutHlsl = ReplaceComments + OutHlsl;
		}

		{
			std::string DefineComments = "///////////////////////////////////////////////////////////////////////////////\n";
			DefineComments += "// Shader Defines (" + std::to_string(FinalDef.Defines.size()) + "):\n";
			if (FinalDef.Defines.empty())
			{
				DefineComments += "//   (None)\n";
			}
			else
			{
				for (const auto& Def : FinalDef.Defines)
				{
					DefineComments += "//   " + EscapeCommentText(Def.Name) + " = " + std::to_string(Def.Value) + "\n";
				}
			}
			DefineComments += "///////////////////////////////////////////////////////////////////////////////\n\n";
			OutHlsl = DefineComments + OutHlsl;

			InOutDefs.clear();
			InOutDefs = std::move(FinalDef.Defines);
		}

		MCLog.Info("ParseShader '" + Shader.Name + "' completed, output HLSL size: " + std::to_string(OutHlsl.size()));

		{
			rapidjson::Document ReflectionDoc;
			ReflectionDoc.SetObject();
			auto& ReflectionAllocator = ReflectionDoc.GetAllocator();

			auto ParseReflectionVariableInput = [&MCLog, &ReflectionAllocator](const std::string& InDebugStr, bool InIsCheckExposure, const MCVariableInput& InVariable, rapidjson::Value& OutJson)->bool
			{
				OutJson.AddMember("Name", rapidjson::Value(InVariable.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
				std::string FieldPrefixStr;
				if (!ParseVariableFormatPrefixEnum(InVariable.Format.Prefix, FieldPrefixStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InDebugStr + " Field '" + InVariable.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(InVariable.Format.Prefix)));
					return false;
				}
				std::string FieldSuffixStr;
				if (!ParseVariableFormatSuffixEnum(InVariable.Format.Suffix, FieldSuffixStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InDebugStr + " Field '" + InVariable.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(InVariable.Format.Suffix)));
					return false;
				}
				const std::string FormatStr = FieldPrefixStr + FieldSuffixStr;
				OutJson.AddMember("Format", rapidjson::Value(FormatStr.c_str(), ReflectionAllocator), ReflectionAllocator);
				// Layout is filled by the shader compiler from its reflection after a successful compile:
				// zero means "unknown", CompilerCulled means "not present in the compiled shader".
				OutJson.AddMember("ByteOffset", 0, ReflectionAllocator);
				OutJson.AddMember("ContentBytes", 0, ReflectionAllocator);
				OutJson.AddMember("ArrayStride", 0, ReflectionAllocator);
				OutJson.AddMember("CompilerCulled", false, ReflectionAllocator);
				const bool bExposure = InIsCheckExposure && InVariable.bExposure;
				OutJson.AddMember("Exposure", bExposure, ReflectionAllocator);
				OutJson.AddMember("IsColor", InVariable.bColor, ReflectionAllocator);
				OutJson.AddMember("IsArray", InVariable.bArray, ReflectionAllocator);
				if (InVariable.bArray)
				{
					OutJson.AddMember("NumElements", InVariable.ArrayNumElems, ReflectionAllocator);
				}
				if (!(InVariable.Default.empty()))
				{
					rapidjson::Value DftArray(rapidjson::kArrayType);
					for (auto Dft : InVariable.Default)
					{
						DftArray.PushBack(Dft, ReflectionAllocator);
					}
					OutJson.AddMember("Default", DftArray, ReflectionAllocator);
				}
				return true;
			};

			ReflectionDoc.AddMember("ShaderName", rapidjson::Value(Shader.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
			ReflectionDoc.AddMember("EntryPoint", rapidjson::Value(Shader.Entry.c_str(), ReflectionAllocator), ReflectionAllocator);
			ReflectionDoc.AddMember("Stage", rapidjson::Value(InLoadParam.Stage.c_str(), ReflectionAllocator), ReflectionAllocator);

			// --- Constant Buffers ---
			if (!(FinalBindings.CBuffers.empty()))
			{
				rapidjson::Value CBArray(rapidjson::kArrayType);
				for (const auto& CB : FinalBindings.CBuffers)
				{
					rapidjson::Value CBObj(rapidjson::kObjectType);
					CBObj.AddMember("Name", rapidjson::Value(CB.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
					CBObj.AddMember("Slot", CB.Slot, ReflectionAllocator);
					CBObj.AddMember("RootConstant", CB.bRootConstant, ReflectionAllocator);
					// Filled by the shader compiler from its reflection after a successful compile.
					CBObj.AddMember("ByteSize", 0, ReflectionAllocator);
					CBObj.AddMember("CompilerCulled", false, ReflectionAllocator);

					rapidjson::Value FieldsArray(rapidjson::kArrayType);
					for (const auto& Field : CB.Fields)
					{
						rapidjson::Value FieldObj(rapidjson::kObjectType);
						if (!ParseReflectionVariableInput("ConstantBuffer '" + CB.Name + "'", true, Field, FieldObj))
						{
							return false;
						}
						FieldsArray.PushBack(FieldObj, ReflectionAllocator);
					}
					CBObj.AddMember("Fields", FieldsArray, ReflectionAllocator);
					CBArray.PushBack(CBObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("CBuffers", CBArray, ReflectionAllocator);
			}

			auto ParseReflectionTexture = [&MCLog, &ReflectionAllocator](const std::string& InPrefixStr, const MCTexture& InTex, rapidjson::Value& OutObj)->bool
			{
				OutObj.AddMember("Name", rapidjson::Value(InTex.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
				std::string FormatPrefixStr;
				if (!ParseVariableFormatPrefixEnum(InTex.Format.Prefix, FormatPrefixStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Texture Field '" + InTex.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(InTex.Format.Prefix)));
					return false;
				}
				std::string FormatSuffixStr;
				if (!ParseVariableFormatSuffixEnum(InTex.Format.Suffix, FormatSuffixStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Texture Field '" + InTex.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(InTex.Format.Suffix)));
					return false;
				}
				std::string FormatTypeStr;
				if (!ParseTextureTypeEnum(InTex.Type, FormatTypeStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Texture Field '" + InTex.Name + "' unknown Type Enum=" + std::to_string(static_cast<int>(InTex.Type)));
					return false;
				}
				const std::string FormatStr = FormatPrefixStr + FormatSuffixStr;
				OutObj.AddMember("Format", rapidjson::Value(FormatStr.c_str(), ReflectionAllocator), ReflectionAllocator);
				OutObj.AddMember("Type", rapidjson::Value(FormatTypeStr.c_str(), ReflectionAllocator), ReflectionAllocator);
				OutObj.AddMember("Slot", InTex.Slot, ReflectionAllocator);
				OutObj.AddMember("Sampler", rapidjson::Value(InTex.SamplerName.c_str(), ReflectionAllocator), ReflectionAllocator);
				OutObj.AddMember("Exposure", InTex.bExposure, ReflectionAllocator);
				OutObj.AddMember("CompilerCulled", false, ReflectionAllocator);
				return true;
			};

			// --- Textures (SRV) ---
			if (!(FinalBindings.Textures.empty()))
			{
				rapidjson::Value TexArray(rapidjson::kArrayType);
				for (const auto& Tex : FinalBindings.Textures)
				{
					rapidjson::Value TexObj(rapidjson::kObjectType);
					if (!ParseReflectionTexture("", Tex, TexObj))
					{
						return false;
					}
					TexArray.PushBack(TexObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("Textures", TexArray, ReflectionAllocator);
			}

			auto ParseReflectionResource = [&MCLog, &ReflectionAllocator, &ParseReflectionVariableInput](const std::string& InPrefixStr, const MCShaderResource& InRes, rapidjson::Value& OutObj)->bool
			{
				OutObj.AddMember("Name", rapidjson::Value(InRes.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
				std::string ResTypeStr;
				if (!ParseShaderResourceTypeEnum(InRes.Type, ResTypeStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Resource '" + InRes.Name + "' parse unknown TypeName Enum=" + std::to_string(static_cast<int>(InRes.Type)));
					return false;
				}
				OutObj.AddMember("Slot", InRes.Slot, ReflectionAllocator);
				OutObj.AddMember("IsStruct", InRes.bHasStruct, ReflectionAllocator);
				if (InRes.Type == MCShaderResourceType::Buff_ByteAddressBuffer)
				{
					// Do nothing
				}
				else
				{
					if (!((InRes.Type == MCShaderResourceType::Buff_Buffer) || (InRes.Type == MCShaderResourceType::Buff_StructuredBuffer)))
					{
						MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Resource '" + InRes.Name + "' unknown TypeName Enum=" + std::to_string(static_cast<int>(InRes.Type)));
						return false;
					}
					if (InRes.bHasStruct)
					{
						rapidjson::Value StructArray(rapidjson::kArrayType);
						for (const auto& StructField : InRes.StructFields)
						{
							rapidjson::Value StructFieldObj(rapidjson::kObjectType);
							if (!ParseReflectionVariableInput(InPrefixStr + "Resource '" + InRes.Name + "'", false, StructField, StructFieldObj))
							{
								return false;
							}
							StructArray.PushBack(StructFieldObj, ReflectionAllocator);
						}
						OutObj.AddMember("StructFields", StructArray, ReflectionAllocator);
					}
					else
					{
						std::string ResPrefixStr;
						if (!ParseVariableFormatPrefixEnum(InRes.ElemFmt.Prefix, ResPrefixStr))
						{
							MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Resource '" + InRes.Name + "' unknown ElementFormatPrefix Enum=" + std::to_string(static_cast<int>(InRes.ElemFmt.Prefix)));
							return false;
						}
						std::string ResSuffixStr;
						if (!ParseVariableFormatSuffixEnum(InRes.ElemFmt.Suffix, ResSuffixStr))
						{
							MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Resource '" + InRes.Name + "' unknown ElementFormatSuffix Enum=" + std::to_string(static_cast<int>(InRes.ElemFmt.Suffix)));
							return false;
						}
						const std::string FormatStr = ResPrefixStr + ResSuffixStr;
						OutObj.AddMember("Format", rapidjson::Value(FormatStr.c_str(), ReflectionAllocator), ReflectionAllocator);
					}
				}
				OutObj.AddMember("Exposure", InRes.bExposure, ReflectionAllocator);
				OutObj.AddMember("CompilerCulled", false, ReflectionAllocator);
				return true;
			};

			// --- Resources (SRV buffers) ---
			if (!(FinalBindings.Resources.empty()))
			{
				rapidjson::Value ResArray(rapidjson::kArrayType);
				for (const auto& Res : FinalBindings.Resources)
				{
					rapidjson::Value ResObj(rapidjson::kObjectType);
					if (!ParseReflectionResource("", Res, ResObj))
					{
						return false;
					}
					ResArray.PushBack(ResObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("Resources", ResArray, ReflectionAllocator);
			}

			// --- Samplers ---
			if (!(FinalBindings.TexSamplers.empty()))
			{
				rapidjson::Value SamplerArray(rapidjson::kArrayType);
				for (const auto& Sampler : FinalBindings.TexSamplers)
				{
					rapidjson::Value SamplerObj(rapidjson::kObjectType);
					SamplerObj.AddMember("Name", rapidjson::Value(Sampler.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
					SamplerObj.AddMember("Slot", Sampler.Slot, ReflectionAllocator);
					std::string FilterStr;
					if (!ParseTextureSamplerFilterEnum(Sampler.Filter, FilterStr))
					{
						MCLog.Error("ParseShader Output Reflection Sampler '" + Sampler.Name + "' unknown SamplerFilter Enum=" + std::to_string(static_cast<int>(Sampler.Filter)));
						return false;
					}
					SamplerObj.AddMember("Filter", rapidjson::Value(FilterStr.c_str(), ReflectionAllocator), ReflectionAllocator);
					std::string AddressUStr;
					if (!ParseTextureSamplerAddressEnum(Sampler.AddressU, AddressUStr))
					{
						MCLog.Error("ParseShader Output Reflection Sampler '" + Sampler.Name + "' unknown SamplerAddressU Enum=" + std::to_string(static_cast<int>(Sampler.AddressU)));
						return false;
					}
					SamplerObj.AddMember("AddressU", rapidjson::Value(AddressUStr.c_str(), ReflectionAllocator), ReflectionAllocator);
					std::string AddressVStr;
					if (!ParseTextureSamplerAddressEnum(Sampler.AddressV, AddressVStr))
					{
						MCLog.Error("ParseShader Output Reflection Sampler '" + Sampler.Name + "' unknown SamplerAddressV Enum=" + std::to_string(static_cast<int>(Sampler.AddressV)));
						return false;
					}
					SamplerObj.AddMember("AddressV", rapidjson::Value(AddressVStr.c_str(), ReflectionAllocator), ReflectionAllocator);
					std::string AddressWStr;
					if (!ParseTextureSamplerAddressEnum(Sampler.AddressW, AddressWStr))
					{
						MCLog.Error("ParseShader Output Reflection Sampler '" + Sampler.Name + "' unknown SamplerAddressW Enum=" + std::to_string(static_cast<int>(Sampler.AddressW)));
						return false;
					}
					SamplerObj.AddMember("AddressW", rapidjson::Value(AddressWStr.c_str(), ReflectionAllocator), ReflectionAllocator);
					std::string ComparisonFuncStr;
					if (!ParseTextureComparisonFuncEnum(Sampler.ComparisonFunc, ComparisonFuncStr))
					{
						MCLog.Error("ParseShader Output Reflection Sampler '" + Sampler.Name + "' unknown SamplerComparisonFunc Enum=" + std::to_string(static_cast<int>(Sampler.ComparisonFunc)));
						return false;
					}
					SamplerObj.AddMember("ComparisonFunc", rapidjson::Value(ComparisonFuncStr.c_str(), ReflectionAllocator), ReflectionAllocator);
					SamplerObj.AddMember("MinLOD", Sampler.MinLOD, ReflectionAllocator);
					SamplerObj.AddMember("MaxLOD", Sampler.MaxLOD, ReflectionAllocator);
					SamplerObj.AddMember("LODBias", Sampler.LODBias, ReflectionAllocator);
					SamplerObj.AddMember("Anisotropy", Sampler.Anisotropy, ReflectionAllocator);
					rapidjson::Value BorderColorArray(rapidjson::kArrayType);
					for (int Index = 0; Index < 4; ++Index)
					{
						BorderColorArray.PushBack(Sampler.BorderColor[Index], ReflectionAllocator);
					}
					SamplerObj.AddMember("BorderColor", BorderColorArray, ReflectionAllocator);
					SamplerObj.AddMember("CompilerCulled", false, ReflectionAllocator);
					SamplerArray.PushBack(SamplerObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("Samplers", SamplerArray, ReflectionAllocator);
			}

			// --- RW Textures / RW Resources ---
			if (!(FinalBindings.RWTextures.empty()))
			{
				rapidjson::Value RWTexArray(rapidjson::kArrayType);
				for (const auto& Tex : FinalBindings.RWTextures)
				{
					rapidjson::Value TexObj(rapidjson::kObjectType);
					if (!ParseReflectionTexture("RW", Tex, TexObj))
					{
						return false;
					}
					RWTexArray.PushBack(TexObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("RWTextures", RWTexArray, ReflectionAllocator);
			}
			if (!(FinalBindings.RWResources.empty()))
			{
				rapidjson::Value RWResArray(rapidjson::kArrayType);
				for (const auto& Res : FinalBindings.RWResources)
				{
					rapidjson::Value ResObj(rapidjson::kObjectType);
					if (!ParseReflectionResource("RW", Res, ResObj))
					{
						return false;
					}
					RWResArray.PushBack(ResObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("RWResources", RWResArray, ReflectionAllocator);
			}

			// --- Input Layout ---
			if (!(FinalInStream.Inputs.empty()))
			{
				rapidjson::Value InputLayoutArray(rapidjson::kArrayType);
				for (const auto& [Slot, Inputs] : FinalInStream.Inputs)
				{
					for (const auto& Input : Inputs)
					{
						rapidjson::Value InputObj(rapidjson::kObjectType);
						InputObj.AddMember("Name", rapidjson::Value(Input.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
						std::string FormatPrefixStr;
						if (!ParseVariableFormatPrefixEnum(Input.Format.Prefix, FormatPrefixStr))
						{
							MCLog.Error("ParseShader Output Reflection InputLayout '" + Input.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(Input.Format.Prefix)));
							return false;
						}
						std::string FormatSuffixStr;
						if (!ParseVariableFormatSuffixEnum(Input.Format.Suffix, FormatSuffixStr))
						{
							MCLog.Error("ParseShader Output Reflection InputLayout '" + Input.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(Input.Format.Suffix)));
							return false;
						}
						const std::string FormatStr = FormatPrefixStr + FormatSuffixStr;
						InputObj.AddMember("Format", rapidjson::Value(FormatStr.c_str(), ReflectionAllocator), ReflectionAllocator);
						InputObj.AddMember("Semantic", rapidjson::Value(Input.Semantic.c_str(), ReflectionAllocator), ReflectionAllocator);
						InputObj.AddMember("SemanticIndex", Input.SemanticIdx, ReflectionAllocator);
						InputObj.AddMember("Slot", Input.Slot, ReflectionAllocator);
						InputLayoutArray.PushBack(InputObj, ReflectionAllocator);
					}
				}
				ReflectionDoc.AddMember("InputLayout", InputLayoutArray, ReflectionAllocator);
			}

			// --- Write to file ---
			rapidjson::StringBuffer StrBuff;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> Writer(StrBuff);
			Writer.SetIndent(' ', 4);
			ReflectionDoc.Accept(Writer);

			const std::string OutJsonPath = InLoadParam.OutDir + InLoadParam.OutFileName;
			std::ofstream FileStream(OutJsonPath);
			if (FileStream.is_open())
			{
				FileStream << StrBuff.GetString();
			}
			else
			{
				MCLog.Error("ParseShader cannot write reflection: " + OutJsonPath);
				return false;
			}
		}

		return true;
	}

}	// namespace MC
