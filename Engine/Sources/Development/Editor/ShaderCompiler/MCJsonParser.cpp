#include "MCJsonParser.h"
#include "MCLogger.h"
#include <Config/EngineShaderConfig.h>
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
			return FALSE;
		}
		std::ostringstream StrStream;
		StrStream << FileStream.rdbuf();
		OutContent = StrStream.str();
		return TRUE;
	}

	static bool LoadHlsl(const std::string& InHlslFilePath, std::string& OutContent)
	{
		MCLogger& MCLog = MCLogger::Get();
		if ((InHlslFilePath.size() > 5) && (InHlslFilePath.substr(InHlslFilePath.size() - 5) == ".hlsl"))
		{
			return ReadFile(InHlslFilePath, OutContent);
		}
		MCLog.Error("Invalid HLSL file extension (expected .hlsl): " + InHlslFilePath);
		return FALSE;
	}

	static bool ParseDoc(const std::string& InRaw, rapidjson::Document& OutDoc)
	{
		MCLogger& MCLog = MCLogger::Get();
		OutDoc.Parse(InRaw.c_str());
		if (OutDoc.HasParseError())
		{
			MCLog.Error(std::string("JSON parse error: ") + rapidjson::GetParseError_En(OutDoc.GetParseError()));
			return FALSE;
		}
		return TRUE;
	}

	static std::set<std::string> ExtractTemplatePlaceholders(const std::string& InSource)
	{
		std::set<std::string> Keys;
		std::regex Pattern(R"(\{[ \t]*([A-Za-z_][A-Za-z0-9_]*)[ \t]*\})");
		std::sregex_iterator Begin = std::sregex_iterator(InSource.begin(), InSource.end(), Pattern);
		std::sregex_iterator End = std::sregex_iterator();
		for (std::sregex_iterator It = Begin; It != End; ++It)
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

		bool bAnyReplaced = FALSE;
		for (const std::pair<const std::string, std::string>& ReplaceRule : InReplaceRules)
		{
			size_t Pos = 0;
			while ((Pos = InOutStr.find(ReplaceRule.first, Pos)) != std::string::npos)
			{
				InOutStr.replace(Pos, ReplaceRule.first.size(), ReplaceRule.second);
				Pos += ReplaceRule.second.size();
				bAnyReplaced = TRUE;
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
		bool bAnyReplaced = FALSE;
		size_t LastPos = 0;

		std::sregex_iterator Begin = std::sregex_iterator(InOutStr.begin(), InOutStr.end(), Pattern);
		std::sregex_iterator End = std::sregex_iterator();

		for (std::sregex_iterator It = Begin; It != End; ++It)
		{
			std::smatch Match = *It;
			std::string Key = Match[1].str();

			std::map<std::string, std::string>::const_iterator RuleIt = InReplaceRules.find(Key);
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
			bAnyReplaced = TRUE;
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
			return TRUE;
		}
		if (InStr == "!=")
		{
			OutOp = MCConditionOpType::NotEqual;
			return TRUE;
		}
		if (InStr == ">")
		{
			OutOp = MCConditionOpType::Greater;
			return TRUE;
		}
		if (InStr == ">=")
		{
			OutOp = MCConditionOpType::GreaterEqual;
			return TRUE;
		}
		if (InStr == "<")
		{
			OutOp = MCConditionOpType::Less;
			return TRUE;
		}
		if (InStr == "<=")
		{
			OutOp = MCConditionOpType::LessEqual;
			return TRUE;
		}
		return FALSE;
	}

	static bool ParseConditionOpEnum(MCConditionOpType InOp, std::string& OutStr)
	{
		switch (InOp)
		{
			case MCConditionOpType::Equal:
			{
				OutStr = "==";
				return TRUE;
			}
			case MCConditionOpType::NotEqual:
			{
				OutStr = "!=";
				return TRUE;
			}
			case MCConditionOpType::Greater:
			{
				OutStr = ">";
				return TRUE;
			}
			case MCConditionOpType::GreaterEqual:
			{
				OutStr = ">=";
				return TRUE;
			}
			case MCConditionOpType::Less:
			{
				OutStr = "<";
				return TRUE;
			}
			case MCConditionOpType::LessEqual:
			{
				OutStr = "<=";
				return TRUE;
			}
		}
		return FALSE;
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
		return FALSE;
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
					return FALSE;
				}
			}
			OutField.Ref = (InJson.HasMember(DefineRefTxt) && InJson[DefineRefTxt].IsInt()) ? (InJson[DefineRefTxt].GetInt()) : 0;
		}

		MCLog.Verbose("ParseConditionField Define='" + OutField.DefineName + "' op=" + (OutField.DefineName.empty() ? "none" : std::to_string(static_cast<int>(OutField.Op))) + " ref=" + std::to_string(OutField.Ref));
		return TRUE;
	}

	static bool CheckConditionField(const MCConditionField& InField, const std::vector<MCDefine>& InDefs)
	{
		if (InField.DefineName.empty())
		{
			return TRUE;		// unconditional
		}

		MCLogger& MCLog = MCLogger::Get();

		bool		bFound = FALSE;
		int			DefineVal = 0;			// No defines mean the value is 0
		for (const MCDefine& Def : InDefs)
		{
			if (Def.Name == InField.DefineName)
			{
				DefineVal = Def.Value;
				bFound = TRUE;
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
			return FALSE;
		}
		return TRUE;
	}

	static void SplitVariableFormatStr(const std::string& InStr, std::string& OutPrefix, std::string& OutSuffix)
	{
		std::string::const_iterator It = std::find_if(InStr.begin(), InStr.end(), [](char InChr) { return ((InChr >= '0') && (InChr <= '9')); });
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
				return TRUE;
			}
			case MCVarFormatPrefixType::Var_Uint:
			{
				OutStr = "uint";
				return TRUE;
			}
			case MCVarFormatPrefixType::Var_Float:
			{
				OutStr = "float";
				return TRUE;
			}
		}
		return FALSE;
	}

	static bool ParseVariableFormatSuffixEnum(MCVarFormatSuffixType InFormat, std::string& OutStr)
	{
		switch (InFormat)
		{
			case MCVarFormatSuffixType::Var_1:
			{
				OutStr = "";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_2:
			{
				OutStr = "2";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_3:
			{
				OutStr = "3";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_4:
			{
				OutStr = "4";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_2x2:
			{
				OutStr = "2x2";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_3x2:
			{
				OutStr = "3x2";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_2x3:
			{
				OutStr = "2x3";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_3x3:
			{
				OutStr = "3x3";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_4x2:
			{
				OutStr = "4x2";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_2x4:
			{
				OutStr = "2x4";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_4x3:
			{
				OutStr = "4x3";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_3x4:
			{
				OutStr = "3x4";
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_4x4:
			{
				OutStr = "4x4";
				return TRUE;
			}
		}
		return FALSE;
	}

	static bool ParseVariableFormatSuffixElems(MCVarFormatSuffixType InFormat, int& OutNumElems)
	{
		switch (InFormat)
		{
			case MCVarFormatSuffixType::Var_1:
			{
				OutNumElems = 1;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_2:
			{
				OutNumElems = 2;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_3:
			{
				OutNumElems = 3;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_4:
			{
				OutNumElems = 4;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_2x2:
			{
				OutNumElems = 4;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_3x2:
			{
				OutNumElems = 6;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_2x3:
			{
				OutNumElems = 6;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_3x3:
			{
				OutNumElems = 9;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_4x2:
			{
				OutNumElems = 8;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_2x4:
			{
				OutNumElems = 8;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_4x3:
			{
				OutNumElems = 12;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_3x4:
			{
				OutNumElems = 12;
				return TRUE;
			}
			case MCVarFormatSuffixType::Var_4x4:
			{
				OutNumElems = 16;
				return TRUE;
			}
		}
		return FALSE;
	}

	static bool ParseVariableFormatPrefixStr(const std::string& InStr, MCVarFormatPrefixType& OutFormat)
	{
		if (InStr == "int")
		{
			OutFormat = MCVarFormatPrefixType::Var_Int;
			return TRUE;
		}
		if (InStr == "uint")
		{
			OutFormat = MCVarFormatPrefixType::Var_Uint;
			return TRUE;
		}
		if (InStr == "float")
		{
			OutFormat = MCVarFormatPrefixType::Var_Float;
			return TRUE;
		}
		return FALSE;
	}

	static bool ParseVariableFormatSuffixStr(const std::string& InStr, MCVarFormatSuffixType& OutFormat)
	{
		if (InStr == "")
		{
			OutFormat = MCVarFormatSuffixType::Var_1;
			return TRUE;
		}
		if (InStr == "2")
		{
			OutFormat = MCVarFormatSuffixType::Var_2;
			return TRUE;
		}
		if (InStr == "3")
		{
			OutFormat = MCVarFormatSuffixType::Var_3;
			return TRUE;
		}
		if (InStr == "4")
		{
			OutFormat = MCVarFormatSuffixType::Var_4;
			return TRUE;
		}
		if (InStr == "2x2")
		{
			OutFormat = MCVarFormatSuffixType::Var_2x2;
			return TRUE;
		}
		if (InStr == "3x2")
		{
			OutFormat = MCVarFormatSuffixType::Var_3x2;
			return TRUE;
		}
		if (InStr == "2x3")
		{
			OutFormat = MCVarFormatSuffixType::Var_2x3;
			return TRUE;
		}
		if (InStr == "3x3")
		{
			OutFormat = MCVarFormatSuffixType::Var_3x3;
			return TRUE;
		}
		if (InStr == "4x2")
		{
			OutFormat = MCVarFormatSuffixType::Var_4x2;
			return TRUE;
		}
		if (InStr == "2x4")
		{
			OutFormat = MCVarFormatSuffixType::Var_2x4;
			return TRUE;
		}
		if (InStr == "4x3")
		{
			OutFormat = MCVarFormatSuffixType::Var_4x3;
			return TRUE;
		}
		if (InStr == "3x4")
		{
			OutFormat = MCVarFormatSuffixType::Var_3x4;
			return TRUE;
		}
		if (InStr == "4x4")
		{
			OutFormat = MCVarFormatSuffixType::Var_4x4;
			return TRUE;
		}
		return FALSE;
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
				return TRUE;
			}
			case MCVarFormatPrefixType::Var_Uint:
			{
				PrefixVariableToUint32 DefaultVal;
				DefaultVal._UintVal = 0u;
				OutVal = DefaultVal.StorageVal;
				return TRUE;
			}
			case MCVarFormatPrefixType::Var_Float:
			{
				PrefixVariableToUint32 DefaultVal;
				DefaultVal._FloatVal = 0.0f;
				OutVal = DefaultVal.StorageVal;
				return TRUE;
			}
		}
		return FALSE;
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
			return FALSE;
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
			return FALSE;
		}
		OutTmpl.Input.Semantic = InJson[SemanticTxt].GetString();
		{
			int SemanticIdx = InJson[SemanticIdxTxt].GetInt();
			if (SemanticIdx < 0 || SemanticIdx > 15)
			{
				MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' input template contains invalid semantic index number: " + std::to_string(SemanticIdx));
				return FALSE;
			}
			OutTmpl.Input.SemanticIdx = SemanticIdx;
		}
		std::string StrPrefix, StrSuffix;
		SplitVariableFormatStr(InJson[FormatTxt].GetString(), StrPrefix, StrSuffix);
		if (StrPrefix == "color")
		{
			MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' can not use 'color' input stream format");
			return FALSE;
		}
		if (!ParseVariableFormatPrefixStr(StrPrefix, OutTmpl.Input.Format.Prefix))
		{
			MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' input template FormatPrefix = " + StrPrefix + " not valid");
			return FALSE;
		}
		if (!ParseVariableFormatSuffixStr(StrSuffix, OutTmpl.Input.Format.Suffix))
		{
			MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' input template FormatSuffix = " + StrSuffix + " not valid");
			return FALSE;
		}
		OutTmpl.Input.Slot = (InJson.HasMember(SlotTxt)) ? (InJson[SlotTxt].GetInt()) : InSlot;
		if ((OutTmpl.Input.Slot < 0) || (OutTmpl.Input.Slot > 15))
		{
			MCLog.Error("ParseVFInputTemplate '" + OutTmpl.Input.Name + "' input template contains invalid slot number: " + std::to_string(OutTmpl.Input.Slot));
			return FALSE;
		}

		// Define + op + ref
		if (!ParseConditionField(InJson, OutTmpl.Condition))
		{
			return FALSE;
		}

		return TRUE;
	}

	static bool ExpandVFInputTemplate(const MCVFInputTemplate& InTmpl, const std::vector<MCDefine>& InDefs, MCVFInput& OutInput)
	{
		if (!CheckConditionField(InTmpl.Condition, InDefs))
		{
			return FALSE;
		}
		OutInput = InTmpl.Input;
		return TRUE;
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
			return FALSE;
		}
		OutTmpl.Input.Name = InJson[NameTxt].GetString();
		if (!(InJson.HasMember(FormatTxt) && InJson[FormatTxt].IsString()))
		{
			MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' input template missing valid 'Format'");
			return FALSE;
		}
		std::string StrPrefix, StrSuffix;
		SplitVariableFormatStr(InJson[FormatTxt].GetString(), StrPrefix, StrSuffix);
		OutTmpl.Input.bColor = FALSE;
		if (StrPrefix == "color")
		{
			OutTmpl.Input.bColor = TRUE;
			if (!ParseVariableFormatPrefixEnum(MCVarFormatPrefixType::Var_Float, StrPrefix))
			{
				MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' something wrong in color check phase");
				return FALSE;
			}
		}
		if (!ParseVariableFormatPrefixStr(StrPrefix, OutTmpl.Input.Format.Prefix))
		{
			MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' input template FormatPrefix = " + StrPrefix + " not valid");
			return FALSE;
		}
		if (!ParseVariableFormatSuffixStr(StrSuffix, OutTmpl.Input.Format.Suffix))
		{
			MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' input template FormatSuffix = " + StrSuffix + " not valid");
			return FALSE;
		}
		OutTmpl.Input.bArray = FALSE;
		if (InJson.HasMember(ArrayElementTxt))
		{
			int ArrayElems = InJson[ArrayElementTxt].GetInt();
			if (ArrayElems > 0)
			{
				OutTmpl.Input.bArray = TRUE;
				OutTmpl.Input.ArrayNumElems = ArrayElems;
			}
		}
		uint32_t InitVal;
		if (!DefaultVariableFormatValue(OutTmpl.Input.Format.Prefix, InitVal))
		{
			MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' something wrong in get default value phase");
			return FALSE;
		}
		int FormatElems;
		if (!ParseVariableFormatSuffixElems(OutTmpl.Input.Format.Suffix, FormatElems))
		{
			MCLog.Error("ParseVariableInputTemplate '" + OutTmpl.Input.Name + "' something wrong in get suffix elements phase");
			return FALSE;
		}
		if (InJson.HasMember(DefaultTxt) && InJson[DefaultTxt].IsArray())
		{
			OutTmpl.Input.Default.clear();
			for (const rapidjson::Value& DefaultVal : InJson[DefaultTxt].GetArray())
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
			for (uint32_t& Elem : OutTmpl.Input.Default)
			{
				Elem = InitVal;
			}
		}
		OutTmpl.Input.bExposure = FALSE;
		if (InJson.HasMember(ExposureTxt) && InJson[ExposureTxt].IsBool() && InJson[ExposureTxt].GetBool())
		{
			OutTmpl.Input.bExposure = TRUE;
		}

		if (!ParseConditionField(InJson, OutTmpl.Condition))
		{
			return FALSE;
		}

		return TRUE;
	}

	static bool ExpandVariableInputTemplate(const MCVariableInputTemplate& InTmpl, const std::vector<MCDefine>& InDefs, MCVariableInput& OutInput)
	{
		if (!CheckConditionField(InTmpl.Condition, InDefs))
		{
			return FALSE;
		}
		OutInput = InTmpl.Input;
		return TRUE;
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
			return FALSE;
		}
		OutCB.Name = InJson[NameTxt].GetString();
		if (!(InJson.HasMember(FieldsTxt) && InJson[FieldsTxt].IsArray()))
		{
			MCLog.Error("ParseConstantBuffer '" + OutCB.Name + "' does not contain valid 'Fields'");
			return FALSE;
		}
		if ((InSlot < 0) && (!InJson.HasMember(SlotTxt)))
		{
			MCLog.Error("ParseConstantBuffer '" + OutCB.Name + "' does not contain valid 'Slot'");
			return FALSE;
		}
		OutCB.Slot = (InJson.HasMember(SlotTxt)) ? (InJson[SlotTxt].GetInt()) : InSlot;
		if ((OutCB.Slot < 0) || (OutCB.Slot > 13))
		{
			MCLog.Error("ParseConstantBuffer '" + OutCB.Name + "' slot=" + std::to_string(OutCB.Slot) + " out of range [0, 13]");
			return FALSE;
		}
		OutCB.bRootConstant = FALSE;
		if (InJson.HasMember(RootConstantTxt) && InJson[RootConstantTxt].IsBool() && InJson[RootConstantTxt].GetBool())
		{
			OutCB.bRootConstant = TRUE;
			MCLog.Info("ParseConstantBuffer '" + OutCB.Name + "' is marked 'RootConstant'");
		}
		for (const rapidjson::Value& Field : InJson[FieldsTxt].GetArray())
		{
			MCVariableInputTemplate InputTmpl{};
			if (!ParseVariableInputTemplate(Field, InputTmpl))
			{
				return FALSE;
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
			return FALSE;
		}
		OutStream.Name = InJson[NameTxt].GetString();
		if (!(InJson.HasMember(InputsTxt) && InJson[InputsTxt].IsArray()))
		{
			MCLog.Error("ParseVFInputStream '" + OutStream.Name + "' does not contain valid 'Inputs'");
			return FALSE;
		}
		for (const rapidjson::Value& Elem : InJson[InputsTxt].GetArray())
		{
			MCVFInputTemplate InputTmpl{};
			if (!ParseVFInputTemplate(Elem, InSlot, InputTmpl))
			{
				return FALSE;
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
			return TRUE;
		}
		if (InStr == "MinMagLinear")
		{
			OutFilter = MCTextureSamplerFilterType::Tex_Filter_MinMagLinear;
			return TRUE;
		}
		if (InStr == "MinMagMipLinear")
		{
			OutFilter = MCTextureSamplerFilterType::Tex_Filter_MinMagMipLinear;
			return TRUE;
		}
		if (InStr == "Anisotropic")
		{
			OutFilter = MCTextureSamplerFilterType::Tex_Filter_Anisotropic;
			return TRUE;
		}
		return FALSE;
	}

	static bool ParseTextureSamplerFilterEnum(MCTextureSamplerFilterType InFilter, std::string& OutStr)
	{
		switch (InFilter)
		{
			case MCTextureSamplerFilterType::Tex_Filter_Point:
			{
				OutStr = "Point";
				return TRUE;
			}
			case MCTextureSamplerFilterType::Tex_Filter_MinMagLinear:
			{
				OutStr = "MinMagLinear";
				return TRUE;
			}
			case MCTextureSamplerFilterType::Tex_Filter_MinMagMipLinear:
			{
				OutStr = "MinMagMipLinear";
				return TRUE;
			}
			case MCTextureSamplerFilterType::Tex_Filter_Anisotropic:
			{
				OutStr = "Anisotropic";
				return TRUE;
			}
		}
		return FALSE;
	}

	static bool ParseTextureSamplerAddressStr(const std::string& InStr, MCTexSamplerAddressModeType& OutMode)
	{
		if (InStr == "Wrap")
		{
			OutMode = MCTexSamplerAddressModeType::Tex_Sampler_Wrap;
			return TRUE;
		}
		if (InStr == "Mirror")
		{
			OutMode = MCTexSamplerAddressModeType::Tex_Sampler_Mirror;
			return TRUE;
		}
		if (InStr == "Clamp")
		{
			OutMode = MCTexSamplerAddressModeType::Tex_Sampler_Clamp;
			return TRUE;
		}
		if (InStr == "Border")
		{
			OutMode = MCTexSamplerAddressModeType::Tex_Sampler_Border;
			return TRUE;
		}
		if (InStr == "MirrorOnce")
		{
			OutMode = MCTexSamplerAddressModeType::Tex_Sampler_MirrorOnce;
			return TRUE;
		}
		return FALSE;
	}

	static bool ParseTextureSamplerAddressEnum(MCTexSamplerAddressModeType InMode, std::string& OutStr)
	{
		switch (InMode)
		{
			case MCTexSamplerAddressModeType::Tex_Sampler_Wrap:
			{
				OutStr = "Wrap";
				return TRUE;
			}
			case MCTexSamplerAddressModeType::Tex_Sampler_Mirror:
			{
				OutStr = "Mirror";
				return TRUE;
			}
			case MCTexSamplerAddressModeType::Tex_Sampler_Clamp:
			{
				OutStr = "Clamp";
				return TRUE;
			}
			case MCTexSamplerAddressModeType::Tex_Sampler_Border:
			{
				OutStr = "Border";
				return TRUE;
			}
			case MCTexSamplerAddressModeType::Tex_Sampler_MirrorOnce:
			{
				OutStr = "MirrorOnce";
				return TRUE;
			}
		}
		return FALSE;
	}

	static bool ParseTextureComparisonFuncStr(const std::string& InStr, MCTextureSamplerComparisonFuncType& OutFunc)
	{
		if (InStr == "Never")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Never;
			return TRUE;
		}
		if (InStr == "Less")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Less;
			return TRUE;
		}
		if (InStr == "Equal")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Equal;
			return TRUE;
		}
		if (InStr == "LessEqual")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_LessEqual;
			return TRUE;
		}
		if (InStr == "Greater")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Greater;
			return TRUE;
		}
		if (InStr == "NotEqual")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_NotEqual;
			return TRUE;
		}
		if (InStr == "GreaterEqual")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_GreaterEqual;
			return TRUE;
		}
		if (InStr == "Always")
		{
			OutFunc = MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Always;
			return TRUE;
		}
		return FALSE;
	}

	static bool ParseTextureComparisonFuncEnum(MCTextureSamplerComparisonFuncType InFunc, std::string& OutStr)
	{
		switch (InFunc)
		{
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Never:
			{
				OutStr = "Never";
				return TRUE;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Less:
			{
				OutStr = "Less";
				return TRUE;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Equal:
			{
				OutStr = "Equal";
				return TRUE;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_LessEqual:
			{
				OutStr = "LessEqual";
				return TRUE;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Greater:
			{
				OutStr = "Greater";
				return TRUE;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_NotEqual:
			{
				OutStr = "NotEqual";
				return TRUE;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_GreaterEqual:
			{
				OutStr = "GreaterEqual";
				return TRUE;
			}
			case MCTextureSamplerComparisonFuncType::Tex_ComparisonFunc_Always:
			{
				OutStr = "Always";
				return TRUE;
			}
		}
		return FALSE;
	}

	static bool ParseTextureTypeStr(const std::string& InStr, MCTextureType& OutType)
	{
		if (InStr == "1D")
		{
			OutType = MCTextureType::Tex_1D;
			return TRUE;
		}
		if (InStr == "2D")
		{
			OutType = MCTextureType::Tex_2D;
			return TRUE;
		}
		if (InStr == "3D")
		{
			OutType = MCTextureType::Tex_3D;
			return TRUE;
		}
		if (InStr == "1DArray")
		{
			OutType = MCTextureType::Tex_1DArray;
			return TRUE;
		}
		if (InStr == "2DArray")
		{
			OutType = MCTextureType::Tex_2DArray;
			return TRUE;
		}
		if (InStr == "3DArray")
		{
			OutType = MCTextureType::Tex_3DArray;
			return TRUE;
		}
		if (InStr == "Cube")
		{
			OutType = MCTextureType::Tex_Cube;
			return TRUE;
		}
		if (InStr == "CubeArray")
		{
			OutType = MCTextureType::Tex_CubeArray;
			return TRUE;
		}
		return FALSE;
	}

	static bool ParseTextureTypeEnum(MCTextureType InType, std::string& OutStr)
	{
		switch (InType)
		{
			case MCTextureType::Tex_1D:
			{
				OutStr = "1D";
				return TRUE;
			}
			case MCTextureType::Tex_2D:
			{
				OutStr = "2D";
				return TRUE;
			}
			case MCTextureType::Tex_3D:
			{
				OutStr = "3D";
				return TRUE;
			}
			case MCTextureType::Tex_1DArray:
			{
				OutStr = "1DArray";
				return TRUE;
			}
			case MCTextureType::Tex_2DArray:
			{
				OutStr = "2DArray";
				return TRUE;
			}
			case MCTextureType::Tex_3DArray:
			{
				OutStr = "3DArray";
				return TRUE;
			}
			case MCTextureType::Tex_Cube:
			{
				OutStr = "Cube";
				return TRUE;
			}
			case MCTextureType::Tex_CubeArray:
			{
				OutStr = "CubeArray";
				return TRUE;
			}
		}
		return FALSE;
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
			return FALSE;
		}
		OutTmpl.Sampler.Name = InJson[NameTxt].GetString();
		if ((InSlot < 0) && (!(InJson.HasMember(SlotTxt))))
		{
			MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' does not contain valid 'Slot'");
			return FALSE;
		}
		OutTmpl.Sampler.Slot = (InJson.HasMember(SlotTxt)) ? (InJson[SlotTxt].GetInt()) : InSlot;
		if ((OutTmpl.Sampler.Slot < 0) || (OutTmpl.Sampler.Slot > 15))
		{
			MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' sampler template Slot = " + std::to_string(OutTmpl.Sampler.Slot) + " out of range [0, 15]");
			return FALSE;
		}
		if (!(InJson.HasMember(FilterTxt) && InJson[FilterTxt].IsString()))
		{
			MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' sampler template missing valid 'Filter'");
			return FALSE;
		}
		if (!ParseTextureSamplerFilterStr(InJson[FilterTxt].GetString(), OutTmpl.Sampler.Filter))
		{
			MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' sampler template Filter = '" + std::string(InJson[FilterTxt].GetString()) + "' not valid");
			return FALSE;
		}
		if (InJson.HasMember(AddressUTxt) && InJson[AddressUTxt].IsString())
		{
			if (!ParseTextureSamplerAddressStr(InJson[AddressUTxt].GetString(), OutTmpl.Sampler.AddressU))
			{
				MCLog.Error("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' sampler template AddressU = '" + std::string(InJson[AddressUTxt].GetString()) + "' not valid");
				return FALSE;
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
				return FALSE;
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
				return FALSE;
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
				return FALSE;
			}
		}
		else
		{
			MCLog.Info("ParseTextureSamplerTemplate '" + OutTmpl.Sampler.Name + "' does not contain 'ComparisonFunc' so use default Never");
		}
		if (InJson.HasMember(BorderColorTxt) && InJson[BorderColorTxt].IsArray())
		{
			int Idx = 0;
			for (const rapidjson::Value& Elem : InJson[BorderColorTxt].GetArray())
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
		OutTmpl.Sampler.bExposure = FALSE;
		if (InJson.HasMember(ExposureTxt) && InJson[ExposureTxt].IsBool() && InJson[ExposureTxt].GetBool())
		{
			OutTmpl.Sampler.bExposure = TRUE;
		}
		// Bindless: the sampler asks for a descriptor heap index field of its own, declared in the
		// constant buffer it names. Both are recorded exactly as declared; the engine decides from
		// the target whether the field is emitted, and the reflection reports both either way.
		if (InJson.HasMember("PreferBindless") && InJson["PreferBindless"].IsBool() && InJson["PreferBindless"].GetBool())
		{
			OutTmpl.Sampler.bPreferBindless = TRUE;
		}
		if (InJson.HasMember("BindConstantBuffer") && InJson["BindConstantBuffer"].IsString())
		{
			OutTmpl.Sampler.BindConstantBuffer = InJson["BindConstantBuffer"].GetString();
		}

		if (!ParseConditionField(InJson, OutTmpl.Condition))
		{
			return FALSE;
		}

		return TRUE;
	}

	static bool ExpandTextureSamplerTemplate(const MCTextureSamplerTemplate& InTmpl, const std::vector<MCDefine>& InDefs, MCTextureSampler& OutSampler)
	{
		if (!CheckConditionField(InTmpl.Condition, InDefs))
		{
			return FALSE;
		}
		OutSampler = InTmpl.Sampler;
		return TRUE;
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
			return FALSE;
		}
		OutTmpl.Texture.Name = InJson[NameTxt].GetString();
		if ((InSlot < 0) && (!(InJson.HasMember(SlotTxt))))
		{
			MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture template missing valid 'Slot'");
			return FALSE;
		}
		OutTmpl.Texture.Slot = (InJson.HasMember(SlotTxt)) ? (InJson[SlotTxt].GetInt()) : InSlot;
		if ((OutTmpl.Texture.Slot < 0) || (OutTmpl.Texture.Slot > (InIsOutput ? 15 : 127)))
		{
			MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture template Slot = " + std::to_string(OutTmpl.Texture.Slot) + " out of range [0, " + std::to_string(InIsOutput ? 15 : 127) + "]");
			return FALSE;
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
				return FALSE;
			}
			if (!ParseVariableFormatSuffixStr(SuffixStr, OutTmpl.Texture.Format.Suffix))
			{
				MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture format '" + FormatStr + "' suffix '" + SuffixStr + "' invalid");
				return FALSE;
			}
		}
		if (!(InJson.HasMember(TypeTxt) && InJson[TypeTxt].IsString()))
		{
			MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture template missing valid 'Type'");
			return FALSE;
		}
		if (!ParseTextureTypeStr(InJson[TypeTxt].GetString(), OutTmpl.Texture.Type))
		{
			MCLog.Error("ParseTextureTemplate '" + OutTmpl.Texture.Name + "' texture template Type = '" + std::string(InJson[TypeTxt].GetString()) + "' not valid");
			return FALSE;
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
			return FALSE;
		}
		OutTmpl.Texture.bExposure = FALSE;
		if (InJson.HasMember(ExposureTxt) && InJson[ExposureTxt].IsBool() && InJson[ExposureTxt].GetBool())
		{
			OutTmpl.Texture.bExposure = TRUE;
		}
		// Bindless: the texture asks for a descriptor heap index field of its own, declared in the
		// constant buffer it names. Both are recorded exactly as declared; the engine decides from
		// the target whether the field is emitted, and the reflection reports both either way.
		if (InJson.HasMember("PreferBindless") && InJson["PreferBindless"].IsBool() && InJson["PreferBindless"].GetBool())
		{
			OutTmpl.Texture.bPreferBindless = TRUE;
		}
		if (InJson.HasMember("BindConstantBuffer") && InJson["BindConstantBuffer"].IsString())
		{
			OutTmpl.Texture.BindConstantBuffer = InJson["BindConstantBuffer"].GetString();
		}

		if (!ParseConditionField(InJson, OutTmpl.Condition))
		{
			return FALSE;
		}

		return TRUE;
	}

	static bool ExpandTextureTemplate(const MCTextureTemplate& InTmpl, const std::vector<MCDefine>& InDefs, MCTexture& OutTexture)
	{
		if (!CheckConditionField(InTmpl.Condition, InDefs))
		{
			return FALSE;
		}
		OutTexture = InTmpl.Texture;
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// Shader Resource enum <-> string helpers
	// ---------------------------------------------------------------------------

	static bool ParseShaderResourceTypeStr(const std::string& InStr, MCShaderResourceType& OutType)
	{
		if (InStr == "Buffer")
		{
			OutType = MCShaderResourceType::Buff_Buffer;
			return TRUE;
		}
		if (InStr == "ByteAddressBuffer")
		{
			OutType = MCShaderResourceType::Buff_ByteAddressBuffer;
			return TRUE;
		}
		if (InStr == "StructuredBuffer")
		{
			OutType = MCShaderResourceType::Buff_StructuredBuffer;
			return TRUE;
		}
		return FALSE;
	}

	static bool ParseShaderResourceTypeEnum(MCShaderResourceType InType, std::string& OutStr)
	{
		switch (InType)
		{
			case MCShaderResourceType::Buff_Buffer:
			{
				OutStr = "Buffer";
				return TRUE;
			}
			case MCShaderResourceType::Buff_ByteAddressBuffer:
			{
				OutStr = "ByteAddressBuffer";
				return TRUE;
			}
			case MCShaderResourceType::Buff_StructuredBuffer:
			{
				OutStr = "StructuredBuffer";
				return TRUE;
			}
		}
		return FALSE;
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
			return FALSE;
		}
		OutTmpl.Resource.Name = InJson[NameTxt].GetString();
		if ((InSlot < 0) && (!(InJson.HasMember(SlotTxt))))
		{
			MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' does not contain valid 'Slot'");
			return FALSE;
		}
		OutTmpl.Resource.Slot = (InJson.HasMember(SlotTxt)) ? (InJson[SlotTxt].GetInt()) : InSlot;
		if ((OutTmpl.Resource.Slot < 0) || (OutTmpl.Resource.Slot > (InIsOutput ? 15 : 127)))
		{
			MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' resource template Slot = " + std::to_string(OutTmpl.Resource.Slot) + " out of range [0, " + std::to_string(InIsOutput ? 15 : 127) + "]");
			return FALSE;
		}
		if (!(InJson.HasMember(TypeTxt) && InJson[TypeTxt].IsString()))
		{
			MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' resource template missing valid 'Type'");
			return FALSE;
		}
		if (!ParseShaderResourceTypeStr(InJson[TypeTxt].GetString(), OutTmpl.Resource.Type))
		{
			MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' resource template Type = '" + std::string(InJson[TypeTxt].GetString()) + "' not valid");
			return FALSE;
		}
		if (OutTmpl.Resource.Type == MCShaderResourceType::Buff_Buffer)
		{
			if (!(InJson.HasMember(ElementTypeTxt) && InJson[ElementTypeTxt].IsString()))
			{
				MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' Buffer type requires 'ElementType'");
				return FALSE;
			}
			std::string StrPrefix, StrSuffix;
			SplitVariableFormatStr(InJson[ElementTypeTxt].GetString(), StrPrefix, StrSuffix);
			if (!ParseVariableFormatPrefixStr(StrPrefix, OutTmpl.Resource.ElemFmt.Prefix))
			{
				MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' ElementType prefix = '" + StrPrefix + "' not valid");
				return FALSE;
			}
			if (!ParseVariableFormatSuffixStr(StrSuffix, OutTmpl.Resource.ElemFmt.Suffix))
			{
				MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' ElementType suffix = '" + StrSuffix + "' not valid");
				return FALSE;
			}
			OutTmpl.Resource.bHasStruct = FALSE;
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
				for (const rapidjson::Value& Field : InJson[StructTxt].GetArray())
				{
					MCVariableInputTemplate InputTmpl{};
					if (!ParseVariableInputTemplate(Field, InputTmpl))
					{
						return FALSE;
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
					return FALSE;
				}
				OutTmpl.Resource.bHasStruct = TRUE;
			}
			else if (bHasElementType)
			{
				std::string StrPrefix, StrSuffix;
				SplitVariableFormatStr(InJson[ElementTypeTxt].GetString(), StrPrefix, StrSuffix);
				if (!ParseVariableFormatPrefixStr(StrPrefix, OutTmpl.Resource.ElemFmt.Prefix))
				{
					MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' ElementType prefix = '" + StrPrefix + "' not valid");
					return FALSE;
				}
				if (!ParseVariableFormatSuffixStr(StrSuffix, OutTmpl.Resource.ElemFmt.Suffix))
				{
					MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' ElementType suffix = '" + StrSuffix + "' not valid");
					return FALSE;
				}
				OutTmpl.Resource.bHasStruct = FALSE;
			}
			else
			{
				MCLog.Error("ParseShaderResourceTemplate '" + OutTmpl.Resource.Name + "' StructuredBuffer requires either 'Struct' or 'ElementType'");
				return FALSE;
			}
		}
		// ByteAddressBuffer: no extra fields needed
		OutTmpl.Resource.bExposure = FALSE;
		if (InJson.HasMember(ExposureTxt) && InJson[ExposureTxt].IsBool() && InJson[ExposureTxt].GetBool())
		{
			OutTmpl.Resource.bExposure = TRUE;
		}
		// Bindless: the buffer asks for a descriptor heap index field of its own, declared in the
		// constant buffer it names. Both are recorded exactly as declared; the engine decides from
		// the target whether the field is emitted, and the reflection reports both either way.
		if (InJson.HasMember("PreferBindless") && InJson["PreferBindless"].IsBool() && InJson["PreferBindless"].GetBool())
		{
			OutTmpl.Resource.bPreferBindless = TRUE;
		}
		if (InJson.HasMember("BindConstantBuffer") && InJson["BindConstantBuffer"].IsString())
		{
			OutTmpl.Resource.BindConstantBuffer = InJson["BindConstantBuffer"].GetString();
		}

		if (!ParseConditionField(InJson, OutTmpl.Condition))
		{
			return FALSE;
		}

		return TRUE;
	}

	static bool ExpandShaderResourceTemplate(const MCShaderResourceTemplate& InTmpl, const std::vector<MCDefine>& InDefs, MCShaderResource& OutResource)
	{
		if (!CheckConditionField(InTmpl.Condition, InDefs))
		{
			return FALSE;
		}
		OutResource = InTmpl.Resource;
		return TRUE;
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
			for (const rapidjson::Value& Elem : InJson[ConstantBuffersTxt].GetArray())
			{
				MCConstantBuffer CB{};
				if (!ParseConstantBuffer(Elem, InDefs, -1, CB))
				{
					return FALSE;
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
			for (const rapidjson::Value& Elem : InJson[SamplersTxt].GetArray())
			{
				MCTextureSamplerTemplate Tmpl{};
				if (!ParseTextureSamplerTemplate(Elem, -1, Tmpl))
				{
					return FALSE;
				}
				MCTextureSampler Sampler{};
				if (!ExpandTextureSamplerTemplate(Tmpl, InDefs, Sampler))
				{
					return FALSE;
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
				for (const rapidjson::Value& Elem : InJson[InTxt].GetArray())
				{
					MCTextureTemplate Tmpl{};
					if (!ParseTextureTemplate(InIsOutput, Elem, -1, Tmpl))
					{
						return FALSE;
					}
					MCTexture Tex{};
					if (!ExpandTextureTemplate(Tmpl, InDefs, Tex))
					{
						return FALSE;
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
			return TRUE;
		};

		auto ProcessRes = [&InJson, &InDefs, &MCLog](const char* InTxt, bool InIsOutput, const std::string& InPrefixStr, std::vector<MCShaderResource>& Outer)->bool
		{
			if (InJson.HasMember(InTxt) && InJson[InTxt].IsArray())
			{
				for (const rapidjson::Value& Elem : InJson[InTxt].GetArray())
				{
					MCShaderResourceTemplate Tmpl{};
					if (!ParseShaderResourceTemplate(InIsOutput, Elem, -1, InDefs, Tmpl))
					{
						return FALSE;
					}
					MCShaderResource Res{};
					if (!ExpandShaderResourceTemplate(Tmpl, InDefs, Res))
					{
						return FALSE;
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
			return TRUE;
		};

		// Textures
		if (!ProcessTex(TexturesTxt, FALSE, "", OutBindings.Textures))
		{
			return FALSE;
		}

		// Resources (ShaderResources)
		if (!ProcessRes(ResourcesTxt, FALSE, "", OutBindings.Resources))
		{
			return FALSE;
		}

		// RWTextures
		if (!ProcessTex(RWTexturesTxt, TRUE, "RW", OutBindings.RWTextures))
		{
			return FALSE;
		}

		// RWResources
		if (!ProcessRes(RWResourcesTxt, TRUE, "RW", OutBindings.RWResources))
		{
			return FALSE;
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
			for (const rapidjson::Value& DefElem : InDoc[DefinesTxt].GetArray())
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
			for (const rapidjson::Value& RplElem : InDoc[ReplacesTxt].GetArray())
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
				for (const MCDefine& Def : TmpDefs)
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
				for (const MCReplace& Rpl : TmpRpls)
				{
					InOutTotalReplaces.ReplaceMappings[Rpl.Name] = Rpl.Element;
				}
			}
		}

		if (InState != MCParseHeaderState::Parse_None)
		{
			return TRUE;
		}

		const char* TemplateTxt = "Template";

		if (!(InDoc.HasMember(TemplateTxt) && InDoc[TemplateTxt].IsString()))
		{
			MCLog.Error("ParseShaderComponent no valid 'Template' section found in JSON: " + InDebugJsonFilePath);
			return FALSE;
		}
		OutShaderComponent.Template.clear();
		const std::string TemplateHlsl = InDoc[TemplateTxt].GetString();
		if (!LoadHlsl(InRootDir + TemplateHlsl, OutShaderComponent.Template))
		{
			MCLog.Error("ParseShaderComponent failed to load template HLSL file: " + InRootDir + TemplateHlsl + " in JSON: " + InDebugJsonFilePath);
			return FALSE;
		}
		OutShaderComponent.Defines.clear();
		ParseDefines(InDebugJsonFilePath, InDoc, OutShaderComponent.Defines);
		OutShaderComponent.Replaces.clear();
		ParseReplaces(InDebugJsonFilePath, InDoc, OutShaderComponent.Replaces);
		return TRUE;
	}

	static bool ParseShaderBase(const std::string& InRootDir, const std::string& InDebugJsonFilePath, const rapidjson::Document& InDoc, MCShaderBase& OutShaderBase, MCFinalDefine& InOutTotalDefines, MCFinalReplace& InOutTotalReplaces, MCParseHeaderState InState)
	{
		MCLogger& MCLog = MCLogger::Get();
		if (!ParseShaderComponent(InRootDir, InDebugJsonFilePath, InDoc, OutShaderBase.ShaderComponent, InOutTotalDefines, InOutTotalReplaces, InState))
		{
			return FALSE;
		}
		if (InState != MCParseHeaderState::Parse_None)
		{
			return TRUE;
		}
		if (!ParseShaderBindings(InDoc, InOutTotalDefines.Defines, InDebugJsonFilePath, OutShaderBase.Bindings))
		{
			MCLog.Info("ParseShaderBase '" + InDebugJsonFilePath + "' no shader bindings found");
		}
		else
		{
			MCLog.Info("ParseShaderBase '" + InDebugJsonFilePath + "' shader bindings parsed successfully");
		}
		return TRUE;
	}

	static bool ParseVertexFactory(const std::string& InRootDir, const std::string& InDebugJsonPath, const rapidjson::Document& InDoc, MCVertexFactory& OutVF, MCFinalDefine& InOutTotalDefines, MCFinalReplace& InOutTotalReplaces, MCParseHeaderState InState)
	{
		const char* NameTxt = "Name";

		MCLogger& MCLog = MCLogger::Get();

		if (!ParseShaderBase(InRootDir, InDebugJsonPath, InDoc, OutVF.Shader, InOutTotalDefines, InOutTotalReplaces, InState))
		{
			return FALSE;
		}
		if (InState != MCParseHeaderState::Parse_None)
		{
			return TRUE;
		}

		if (!(InDoc.HasMember(NameTxt) && InDoc[NameTxt].IsString()))
		{
			MCLog.Error("ParseVertexFactory missing valid 'Name' in: " + InDebugJsonPath);
			return FALSE;
		}
		OutVF.Name = InDoc[NameTxt].GetString();
		MCLog.Info("ParseVertexFactory '" + OutVF.Name + "'");

		if (!ParseVFInputStream(InDoc, InOutTotalDefines.Defines, -1, OutVF.InputStream))
		{
			MCLog.Info("ParseVertexFactory '" + OutVF.Name + "' failed to parse InputStream");
		}

		return TRUE;
	}

	static bool ParseGeneralShader(const std::string& InRootDir, const std::string& InDebugJsonPath, const rapidjson::Document& InDoc, MCGeneralShader& OutGShader, MCFinalDefine& InOutTotalDefines, MCFinalReplace& InOutTotalReplaces, MCParseHeaderState InState)
	{
		const char* NameTxt = "Name";

		MCLogger& MCLog = MCLogger::Get();

		if (!ParseShaderBase(InRootDir, InDebugJsonPath, InDoc, OutGShader.Shader, InOutTotalDefines, InOutTotalReplaces, InState))
		{
			return FALSE;
		}
		if (InState != MCParseHeaderState::Parse_None)
		{
			return TRUE;
		}

		if (!(InDoc.HasMember(NameTxt) && InDoc[NameTxt].IsString()))
		{
			MCLog.Error("ParseGeneralShader missing valid 'Name' in: " + InDebugJsonPath);
			return FALSE;
		}
		OutGShader.Name = InDoc[NameTxt].GetString();
		MCLog.Info("ParseGeneralShader '" + OutGShader.Name + "'");

		return TRUE;
	}

	static bool ParseShaderBaseReplaceTypeStr(const std::string& InStr, MCShaderBaseReplaceType& OutType)
	{
		if (InStr == "VertexFactory")
		{
			OutType = MCShaderBaseReplaceType::ShaderReplace_VertexFactory;
			return TRUE;
		}
		if (InStr == "General")
		{
			OutType = MCShaderBaseReplaceType::ShaderReplace_General;
			return TRUE;
		}
		return FALSE;
	}

#if 0	// Not used for now
	static bool ParseShaderBaseReplaceTypeEnum(MCShaderBaseReplaceType InType, std::string& OutStr)
	{
		switch (InType)
		{
			case MCShaderBaseReplaceType::ShaderReplace_VertexFactory:
			{
				OutStr = "VertexFactory";
				return TRUE;
			}
			case MCShaderBaseReplaceType::ShaderReplace_General:
			{
				OutStr = "General";
				return TRUE;
			}
		}
		return FALSE;
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
			return FALSE;
		}
		OutShaderRpl.Name = InJson[NameTxt].GetString();
		if ((InIndex < 0) && (!(InJson.HasMember(IndexTxt))))
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' missing valid 'Index'");
			return FALSE;
		}
		OutShaderRpl.Index = InJson.HasMember(IndexTxt) ? (InJson[IndexTxt].GetInt()) : InIndex;
		if ((OutShaderRpl.Index < 0) || (OutShaderRpl.Index > 12))	// ReplaceTemplateStringRecursive max depth is 16
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' Index = " + std::to_string(OutShaderRpl.Index) + " out of range [0, 12]");
			return FALSE;
		}
		if (!(InJson.HasMember(TypeTxt) && InJson[TypeTxt].IsString()))
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' missing valid 'Type'");
			return FALSE;
		}
		if (const std::string TypeStr = InJson[TypeTxt].GetString(); !ParseShaderBaseReplaceTypeStr(TypeStr, OutShaderRpl.Type))
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' unknown 'Type'=" + TypeStr);
			return FALSE;
		}
		if (!(InJson.HasMember(ReplaceTxt) && InJson[ReplaceTxt].IsString()))
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' missing valid 'Replace'");
			return FALSE;
		}
		OutShaderRpl.RplElem = InJson[ReplaceTxt].GetString();
		if (!(InJson.HasMember(JsonFileTxt) && InJson[JsonFileTxt].IsString()))
		{
			MCLog.Error("ParseShaderBaseReplace '" + OutShaderRpl.Name + "' missing valid 'JsonFile'");
			return FALSE;
		}
		OutShaderRpl.JsonFile = InJson[JsonFileTxt].GetString();
		return TRUE;
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
					return FALSE;
				}
				OutBindings.emplace_back(std::move(TmpBinding));
			}
			OutBindingOffset += Nums;
		}
		return TRUE;
	}

	bool ParseShader(const MCLoadParam& InLoadParam, std::vector<MCDefine>& InOutDefs, const std::vector<MCReplace>& InRpls, std::string& OutHlsl)
	{
		MCLogger& MCLog = MCLogger::Get();

		const std::string FullJsonFilePath = InLoadParam.RootDir + InLoadParam.JsonFilePath;

		std::string RawJson;
		if (!ReadFile(FullJsonFilePath, RawJson))
		{
			MCLog.Error("ParseShader cannot read: " + FullJsonFilePath);
			return FALSE;
		}
		rapidjson::Document Doc;
		if (!ParseDoc(RawJson, Doc))
		{
			MCLog.Error("ParseShader JSON parse error: " + FullJsonFilePath);
			return FALSE;
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
			return FALSE;
		}
		Shader.Name = Doc[NameTxt].GetString();
		if (!(Doc.HasMember(EntryTxt) && Doc[EntryTxt].IsString()))
		{
			MCLog.Error("ParseShader missing valid 'Entry' in: " + FullJsonFilePath);
			return FALSE;
		}
		Shader.Entry = Doc[EntryTxt].GetString();

		Shader.ShaderRpls.clear();
		if (Doc.HasMember(ShaderReplacesTxt) && Doc[ShaderReplacesTxt].IsArray())
		{
			int ShaderRplIndex = 0;
			for (const rapidjson::Value& ShaderRpl : Doc[ShaderReplacesTxt].GetArray())
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
				return FALSE;
			}
			if (!CombineShaderBindings<MCTextureSampler>(InDebugName, InBindings.TexSamplers, 0, 15, ReflectIndex.TexSamplerSlotOffset, FinalBindings.TexSamplers))
			{
				return FALSE;
			}
			if (!CombineShaderBindings<MCTexture>(InDebugName, InBindings.Textures, 0, 127, ReflectIndex.ResourceSlotOffset, FinalBindings.Textures))
			{
				return FALSE;
			}
			if (!CombineShaderBindings<MCShaderResource>(InDebugName, InBindings.Resources, 0, 127, ReflectIndex.ResourceSlotOffset, FinalBindings.Resources))
			{
				return FALSE;
			}
			if (!CombineShaderBindings<MCTexture>(InDebugName, InBindings.RWTextures, 0, 15, ReflectIndex.RWResourceSlotOffset, FinalBindings.RWTextures))
			{
				return FALSE;
			}
			if (!CombineShaderBindings<MCShaderResource>(InDebugName, InBindings.RWResources, 0, 15, ReflectIndex.RWResourceSlotOffset, FinalBindings.RWResources))
			{
				return FALSE;
			}
			return TRUE;
		};

		for (int Iter = 0; Iter < 2; ++Iter)
		{
			const bool bParseOnce = (Iter == 0);

			if (!(Shader.ShaderRpls.empty()))
			{
				for (const MCShaderBaseReplace& ShaderRpl : Shader.ShaderRpls)
				{
					if (ShaderRpl.RplElem.empty() || ShaderRpl.JsonFile.empty())
					{
						MCLog.Error("ParseShader 'ShaderReplace' contains invalid content 'Name'=" + ShaderRpl.Name);
						return FALSE;
					}

					const std::string RplJsonFilePath = InLoadParam.RootDir + ShaderRpl.JsonFile;
					std::string RplRawJson;
					if (!ReadFile(RplJsonFilePath, RplRawJson))
					{
						MCLog.Error("ParseShader cannot read: " + RplJsonFilePath);
						return FALSE;
					}
					rapidjson::Document RplDoc;
					if (!ParseDoc(RplRawJson, RplDoc))
					{
						MCLog.Error("ParseShader JSON parse error: " + RplJsonFilePath);
						return FALSE;
					}

					switch (ShaderRpl.Type)
					{
						case MCShaderBaseReplaceType::ShaderReplace_VertexFactory:
						{
							MCVertexFactory VF{};
							if (!ParseVertexFactory(InLoadParam.RootDir, RplJsonFilePath, RplDoc, VF, FinalDef, FinalRpl, bParseOnce ? MCParseHeaderState::Parse_All : MCParseHeaderState::Parse_None))
							{
								return FALSE;
							}
							if (!bParseOnce)
							{
								if (!(VF.InputStream.Inputs.empty()))
								{
									FinalInStream.Name += "_" + VF.Name;
									for (size_t InputIdx = 0, NumInputs = VF.InputStream.Inputs.size(); InputIdx < NumInputs; ++InputIdx)
									{
										const MCVFInput& TmpInputRef = VF.InputStream.Inputs[InputIdx];
										std::map<int, std::vector<MCVFInput>>::iterator IterInput = FinalInStream.Inputs.find(TmpInputRef.Slot);
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
											return FALSE;
										}
										for (size_t ExistIdx = 0, NumExists = IterInput->second.size(); ExistIdx < NumExists; ++ExistIdx)
										{
											const MCVFInput& ExistInputRef = IterInput->second[ExistIdx];
											if (ExistInputRef.Semantic == TmpInputRef.Semantic)
											{
												MCLog.Error("ParseShader 'ShaderReplace' VF'" + VF.Name + "' contains duplicate 'Semantic'=" + TmpInputRef.Semantic + " for VFInput '" + TmpInputRef.Name + "' in '" + RplJsonFilePath + "'");
												return FALSE;
											}
										}
										IterInput->second.push_back(TmpInputRef);
									}
								}
								if (!CombineShaderBindingPart(VF.Name, VF.Shader.Bindings))
								{
									return FALSE;
								}
								if (ShaderRpls.find(ShaderRpl.RplElem) != ShaderRpls.end())
								{
									MCLog.Error("ParseShader 'ShaderReplace' contains duplicate 'RplElem'=" + ShaderRpl.RplElem + " for VF'" + VF.Name + "' in '" + RplJsonFilePath + "'");
									return FALSE;
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
								return FALSE;
							}
							if (!bParseOnce)
							{
								if (!CombineShaderBindingPart(GeneralShader.Name, GeneralShader.Shader.Bindings))
								{
									return FALSE;
								}
								if (ShaderRpls.find(ShaderRpl.RplElem) != ShaderRpls.end())
								{
									MCLog.Error("ParseShader 'ShaderReplace' contains duplicate 'RplElem'=" + ShaderRpl.RplElem + " for GeneralShader'" + GeneralShader.Name + "' in '" + RplJsonFilePath + "'");
									return FALSE;
								}
								ShaderRpls.insert_or_assign(ShaderRpl.RplElem, GeneralShader.Shader.ShaderComponent.Template);
							}
							break;
						}
						default:
						{
							MCLog.Error("ParseShader 'ShaderReplace' contains invalid 'Type'=" + std::to_string(static_cast<int>(ShaderRpl.Type)) + " for 'Name'=" + ShaderRpl.Name);
							return FALSE;
						}
					}
				}
			}
			if (!ParseShaderBase(InLoadParam.RootDir, FullJsonFilePath, Doc, Shader.Shader, FinalDef, FinalRpl, bParseOnce ? MCParseHeaderState::Parse_All : MCParseHeaderState::Parse_None))
			{
				return FALSE;
			}
			if (bParseOnce)
			{
				for (const MCDefine& InputDef : InOutDefs)
				{
					if (std::map<std::string, int>::iterator IterDef = FinalDef.DefineMappings.find(InputDef.Name); IterDef != FinalDef.DefineMappings.end())
					{
						IterDef->second = InputDef.Value;
					}
					else
					{
						FinalDef.DefineMappings.insert_or_assign(InputDef.Name, InputDef.Value);
					}
				}
				for (const MCReplace& InputRpl : InRpls)
				{
					if (std::map<std::string, std::string>::iterator IterRpl = FinalRpl.ReplaceMappings.find(InputRpl.Name); IterRpl != FinalRpl.ReplaceMappings.end())
					{
						IterRpl->second = InputRpl.Element;
					}
					else
					{
						FinalRpl.ReplaceMappings.insert_or_assign(InputRpl.Name, InputRpl.Element);
					}
				}
				FinalDef.Defines.clear();
				for (const std::pair<const std::string, int>& Def : FinalDef.DefineMappings)
				{
					MCDefine TmpDef;
					TmpDef.Name = Def.first;
					TmpDef.Value = Def.second;
					FinalDef.Defines.emplace_back(std::move(TmpDef));
				}
				FinalRpl.Replaces.clear();
				for (const std::pair<const std::string, std::string>& Rpl : FinalRpl.ReplaceMappings)
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
					return FALSE;
				}
			}
		}

		// Only one constant buffer may be fed through root 32 bit constants: the engine pushes exactly
		// one root constant block, so a second marker inside the assembled hlsl is a source error.
		{
			int NumRootConstantCBuffers = 0;
			std::string RootConstantCBufferNames = "";
			for (const MCConstantBuffer& RootConstantCandidate : FinalBindings.CBuffers)
			{
				if (!RootConstantCandidate.bRootConstant)
				{
					continue;
				}
				NumRootConstantCBuffers++;
				RootConstantCBufferNames += ((RootConstantCBufferNames.empty() ? "" : ", ") + RootConstantCandidate.Name);
			}
			if (NumRootConstantCBuffers > 1)
			{
				MCLog.Error("ParseShader '" + Shader.Name + "' contains " + std::to_string(NumRootConstantCBuffers) + " constant buffers marked 'RootConstant' but only one is allowed: " + RootConstantCBufferNames);
				return FALSE;
			}
			if (NumRootConstantCBuffers == 1)
			{
				MCLog.Info("ParseShader '" + Shader.Name + "' root constant buffer: " + RootConstantCBufferNames);
			}
		}

		// Bindless index fields. A resource that declares 'PreferBindless' gets a descriptor heap
		// index of its own, and that index has to be a constant buffer member: the engine writes it
		// and the reflection reports it, which is what makes the field visible to both sides. The
		// declared buffer is resolved here because this is the first point where the whole component
		// chain is merged, and the declaration is validated on every target - a declaration that
		// cannot be honoured is a source error - while the field itself is only emitted for a build
		// whose target can express the path.
		{
			const bool bBindlessCompiledShader = InLoadParam.bSM66;

			auto EmitBindlessIndexField = [&MCLog, &Shader, &FinalBindings, bBindlessCompiledShader](const std::string& InKindName
				, const std::string& InResourceName, const bool InPreferBindless, const std::string& InBindConstantBufferName)->bool
			{
				if (!InPreferBindless)
				{
					return TRUE;
				}
				const std::string Where = (InKindName + " '" + InResourceName + "'");
				if (InBindConstantBufferName.empty())
				{
					MCLog.Error("ParseShader '" + Shader.Name + "' " + Where + " declares 'PreferBindless' without 'BindConstantBuffer'");
					return FALSE;
				}

				MCConstantBuffer* IndexConstantBuffer = nullptr;
				for (MCConstantBuffer& IndexCandidateConstantBuffer : FinalBindings.CBuffers)
				{
					if (IndexCandidateConstantBuffer.Name == InBindConstantBufferName)
					{
						IndexConstantBuffer = &IndexCandidateConstantBuffer;
						break;
					}
				}
				if (IndexConstantBuffer == nullptr)
				{
					MCLog.Error("ParseShader '" + Shader.Name + "' " + Where + " names 'BindConstantBuffer' = '" + InBindConstantBufferName + "', which this stage does not declare");
					return FALSE;
				}
				if (IndexConstantBuffer->bRootConstant)
				{
					// The engine pushes the root constant block itself, per draw, so an index the
					// material owns has no place inside it.
					MCLog.Error("ParseShader '" + Shader.Name + "' " + Where + " names the root constant buffer '" + IndexConstantBuffer->Name + "' as its index buffer");
					return FALSE;
				}
				if (!bBindlessCompiledShader)
				{
					// Declared, but this target cannot express the path: the reflection reports
					// Bindless = false for the resource and the shader keeps using its register slot,
					// so a build for an earlier shader model stays correct rather than half switched.
					MCLog.Info("ParseShader '" + Shader.Name + "' " + Where + " declares 'PreferBindless' but the target cannot express it, so its register slot is kept");
					return TRUE;
				}

				MCVariableInput IndexField;
				IndexField.Name = (InResourceName + "_" + MC_SHADER_BINDLESS_INDEX_SUFFIX);
				IndexField.Format.Prefix = MCVarFormatPrefixType::Var_Uint;
				IndexField.Format.Suffix = MCVarFormatSuffixType::Var_1;
				IndexField.Default.assign(1u, 0u);
				IndexField.bExposure = FALSE;
				for (const MCVariableInput& IterField : IndexConstantBuffer->Fields)
				{
					if (IterField.Name == IndexField.Name)
					{
						MCLog.Error("ParseShader '" + Shader.Name + "' " + Where + " needs the index field '" + IndexField.Name + "' in '" + IndexConstantBuffer->Name + "', which already declares a field with that name");
						return FALSE;
					}
				}

				const std::string IndexFieldName = IndexField.Name;
				IndexConstantBuffer->Fields.emplace_back(std::move(IndexField));
				MCLog.Info("ParseShader '" + Shader.Name + "' " + Where + " indexes the descriptor heap through '" + IndexConstantBuffer->Name + "." + IndexFieldName + "'");
				return TRUE;
			};

			for (const MCTexture& Texture : FinalBindings.Textures)
			{
				if (!EmitBindlessIndexField("texture", Texture.Name, Texture.bPreferBindless, Texture.BindConstantBuffer))
				{
					return FALSE;
				}
			}
			for (const MCShaderResource& Resource : FinalBindings.Resources)
			{
				if (!EmitBindlessIndexField("resource", Resource.Name, Resource.bPreferBindless, Resource.BindConstantBuffer))
				{
					return FALSE;
				}
			}
			for (const MCTexture& Texture : FinalBindings.RWTextures)
			{
				if (!EmitBindlessIndexField("rw texture", Texture.Name, Texture.bPreferBindless, Texture.BindConstantBuffer))
				{
					return FALSE;
				}
			}
			for (const MCShaderResource& Resource : FinalBindings.RWResources)
			{
				if (!EmitBindlessIndexField("rw resource", Resource.Name, Resource.bPreferBindless, Resource.BindConstantBuffer))
				{
					return FALSE;
				}
			}
			for (const MCTextureSampler& Sampler : FinalBindings.TexSamplers)
			{
				if (!EmitBindlessIndexField("sampler", Sampler.Name, Sampler.bPreferBindless, Sampler.BindConstantBuffer))
				{
					return FALSE;
				}
			}
		}

		{
			auto CheckReplaceDiff = [&MCLog](const std::string& InDebugString, const std::string& InSource, const std::map<std::string, std::string>& InRpls)->void
			{
				std::set<std::string> VerifyKeys;
				std::set<std::string> Keys = ExtractTemplatePlaceholders(InSource);
				for (const std::string& Key : Keys)
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
				for (const std::pair<const std::string, std::string>& IterRpl : InRpls)
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
			for (const MCConstantBuffer& IndexCandidateConstantBuffer : FinalBindings.CBuffers)
			{
				Header += "cbuffer " + IndexCandidateConstantBuffer.Name + " : register(b" + std::to_string(IndexCandidateConstantBuffer.Slot) + ")\n{\n";
				for (const MCVariableInput& Field : IndexCandidateConstantBuffer.Fields)
				{
					std::string FieldPrefixStr, FieldSuffixStr;
					if (!ParseVariableFormatPrefixEnum(Field.Format.Prefix, FieldPrefixStr))
					{
						MCLog.Error("ParseShader Resource '" + IndexCandidateConstantBuffer.Name + "' ConstantBuffer '" + Field.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(Field.Format.Prefix)));
						return FALSE;
					}
					if (!ParseVariableFormatSuffixEnum(Field.Format.Suffix, FieldSuffixStr))
					{
						MCLog.Error("ParseShader Resource '" + IndexCandidateConstantBuffer.Name + "' ConstantBuffer '" + Field.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(Field.Format.Suffix)));
						return FALSE;
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

			// Whether this compilation took the bindless path at all. The library needs to know,
			// because the access functions that carry a heap index cannot even be parsed by every
			// compiler the engine targets - a 5.0 build gets the register path, written out without
			// them - so the flag is emitted before every binding name and every declaration.
			Header += ("#define PE_BINDLESS_COMPILED " + std::string((InLoadParam.bSM66) ? "1" : "0") + "\n\n");

			// One access name per declared resource, so no shader ever spells the index suffix and one
			// source serves both paths: a resource this build made index driven resolves to its heap
			// index cast to the type the JSON declared, everything else resolves to itself. The cast
			// macro names and their argument order are the contract with ShaderBindless.hlsl.
			{
				auto MakeTextureCastMacroName = [](MCTextureType InType)->std::string
				{
					switch (InType)
					{
					case MCTextureType::Tex_1D:			return "PE_BINDLESS_TEX1D_CAST";
					case MCTextureType::Tex_2D:			return "PE_BINDLESS_TEX2D_CAST";
					case MCTextureType::Tex_3D:			return "PE_BINDLESS_TEX3D_CAST";
					case MCTextureType::Tex_Cube:		return "PE_BINDLESS_TEXCUBE_CAST";
					case MCTextureType::Tex_2DArray:	return "PE_BINDLESS_TEX2DARRAY_CAST";
					default:							return "";
					}
				};
				auto MakeResourceCastMacroName = [](MCShaderResourceType InType)->std::string
				{
					switch (InType)
					{
					case MCShaderResourceType::Buff_Buffer:				return "PE_BINDLESS_BUFFER_CAST";
					case MCShaderResourceType::Buff_StructuredBuffer:	return "PE_BINDLESS_STRUCTURED_BUFFER_CAST";
					case MCShaderResourceType::Buff_ByteAddressBuffer:	return "PE_BINDLESS_BYTE_ADDRESS_BUFFER_CAST";
					default:											return "";
					}
				};
				// The element type as HLSL spells it, from the declared format. It is what makes the
				// access match the declaration instead of a type guessed here.
				auto MakeFormatTypeToken = [](const MCVarFormat& InFormat, std::string& OutTypeToken)->bool
				{
					std::string PrefixStr, SuffixStr;
					if (!ParseVariableFormatPrefixEnum(InFormat.Prefix, PrefixStr))
					{
						return FALSE;
					}
					if (!ParseVariableFormatSuffixEnum(InFormat.Suffix, SuffixStr))
					{
						return FALSE;
					}
					OutTypeToken = (PrefixStr + SuffixStr);
					return TRUE;
				};
				auto AppendBindingMacro = [&Header, &InLoadParam](const std::string& InResourceName
					, const bool InPreferBindless, const std::string& InCastExpression)->void
				{
					Header += ("#define PE_BINDING_" + InResourceName + " ");
					if (InPreferBindless && InLoadParam.bSM66)
					{
						Header += InCastExpression;
					}
					else
					{
						Header += InResourceName;
					}
					Header += "\n";
				};
				// A bindless declaration is only usable when its type has a cast macro and the JSON
				// said what the element type is; either missing is a source error, not a shader that
				// silently reads the wrong thing.
				auto EmitTextureBinding = [&MCLog, &Shader, &InLoadParam, &MakeTextureCastMacroName
					, &MakeFormatTypeToken, &AppendBindingMacro](const std::string& InKindName, const MCTexture& InTexture)->bool
				{
					if (!(InTexture.bPreferBindless && InLoadParam.bSM66))
					{
						AppendBindingMacro(InTexture.Name, FALSE, "");
						return TRUE;
					}
					const std::string CastMacroName = MakeTextureCastMacroName(InTexture.Type);
					if (CastMacroName.empty())
					{
						MCLog.Error("ParseShader '" + Shader.Name + "' " + InKindName + " '" + InTexture.Name + "' cannot be index driven: ShaderBindless.hlsl declares no cast macro for its type");
						return FALSE;
					}
					std::string TypeToken;
					if (!MakeFormatTypeToken(InTexture.Format, TypeToken))
					{
						MCLog.Error("ParseShader '" + Shader.Name + "' " + InKindName + "'" + InTexture.Name + "' cannot be index driven: its declared format does not name an HLSL type");
						return FALSE;
					}
					AppendBindingMacro(InTexture.Name, TRUE
						, (CastMacroName + "(" + TypeToken + ", " + InTexture.Name + "_" + MC_SHADER_BINDLESS_INDEX_SUFFIX + ")"));
					return TRUE;
				};
				auto EmitResourceBinding = [&MCLog, &Shader, &InLoadParam, &MakeResourceCastMacroName
					, &MakeFormatTypeToken, &AppendBindingMacro](const std::string& InKindName, const MCShaderResource& InResource)->bool
				{
					if (!(InResource.bPreferBindless && InLoadParam.bSM66))
					{
						AppendBindingMacro(InResource.Name, FALSE, "");
						return TRUE;
					}
					const std::string CastMacroName = MakeResourceCastMacroName(InResource.Type);
					if (CastMacroName.empty())
					{
						MCLog.Error("ParseShader '" + Shader.Name + "' " + InKindName + " '" + InResource.Name + "' cannot be index driven: ShaderBindless.hlsl declares no cast macro for its type");
						return FALSE;
					}
					// A byte address buffer carries no element type, every other kind takes one: the
					// generated struct for a struct element, the declared element format otherwise.
					if (InResource.Type == MCShaderResourceType::Buff_ByteAddressBuffer)
					{
						AppendBindingMacro(InResource.Name, TRUE
							, (CastMacroName + "(" + InResource.Name + "_" + MC_SHADER_BINDLESS_INDEX_SUFFIX + ")"));
						return TRUE;
					}
					std::string TypeTokenForBuffer;
					if (InResource.bHasStruct)
					{
						TypeTokenForBuffer = (InResource.Name + "_Struct");
					}
					else if (!MakeFormatTypeToken(InResource.ElemFmt, TypeTokenForBuffer))
					{
						MCLog.Error("ParseShader '" + Shader.Name + "' " + InKindName + " '" + InResource.Name + "' cannot be index driven: its declared element format does not name an HLSL type");
						return FALSE;
					}
					AppendBindingMacro(InResource.Name, TRUE
						, (CastMacroName + "(" + TypeTokenForBuffer + ", " + InResource.Name + "_" + MC_SHADER_BINDLESS_INDEX_SUFFIX + ")"));
					return TRUE;
				};

				for (const MCTexture& Texture : FinalBindings.Textures)
				{
					if (!EmitTextureBinding("texture", Texture))
					{
						return FALSE;
					}
				}
				for (const MCTexture& Texture : FinalBindings.RWTextures)
				{
					if (!EmitTextureBinding("rw texture", Texture))
					{
						return FALSE;
					}
				}
				for (const MCShaderResource& Resource : FinalBindings.Resources)
				{
					if (!EmitResourceBinding("resource", Resource))
					{
						return FALSE;
					}
				}
				for (const MCShaderResource& Resource : FinalBindings.RWResources)
				{
					if (!EmitResourceBinding("rw resource", Resource))
					{
						return FALSE;
					}
				}
				for (const MCTextureSampler& Sampler : FinalBindings.TexSamplers)
				{
					AppendBindingMacro(Sampler.Name, Sampler.bPreferBindless
						, ("PE_BINDLESS_SAMPLER_CAST(" + Sampler.Name + "_" + MC_SHADER_BINDLESS_INDEX_SUFFIX + ")"));
				}
			}

			// Slot space needs no check here, and that is a property rather than an omission:
			// CombineShaderBindings assigns every texture, buffer and sampler of a stage a sequential
			// slot in declaration order, with textures and buffers of one direction sharing one
			// offset, so two declarations cannot land on the same slot. A 'Slot' the JSON declares is
			// nominal - that assignment overwrites it.
			auto ProcessResourceBindings = [&MCLog, &Header](const std::string& InTypePrefixStr, const std::string& InShaderTypePrefixStr, const std::vector<MCTexture>& InTexArr, const std::vector<MCShaderResource>& InResArr)->bool
			{
				for (const MCTexture& Tex : InTexArr)
				{
					std::string PrefixStr;
					if (!ParseVariableFormatPrefixEnum(Tex.Format.Prefix, PrefixStr))
					{
						MCLog.Error("ParseShader " + InTypePrefixStr + "Texture '" + Tex.Name + "' unknown PrefixEnum=" + std::to_string(static_cast<int>(Tex.Format.Prefix)));
						return FALSE;
					}
					std::string SuffixStr;
					if (!ParseVariableFormatSuffixEnum(Tex.Format.Suffix, SuffixStr))
					{
						MCLog.Error("ParseShader " + InTypePrefixStr + "Texture '" + Tex.Name + "' unknown SuffixEnum=" + std::to_string(static_cast<int>(Tex.Format.Suffix)));
						return FALSE;
					}
					std::string TexTypeStr;
					if (!ParseTextureTypeEnum(Tex.Type, TexTypeStr))
					{
						MCLog.Error("ParseShader " + InTypePrefixStr + "Texture '" + Tex.Name + "' unknown TypeEnum=" + std::to_string(static_cast<int>(Tex.Type)));
						return FALSE;
					}
					Header += InTypePrefixStr + "Texture" + TexTypeStr + "<" + PrefixStr + SuffixStr + "> " + Tex.Name + " : register(" + InShaderTypePrefixStr + std::to_string(Tex.Slot) + ");\n";
				}
				Header += "\n";
				for (const MCShaderResource& Res : InResArr)
				{
					std::string ResTypeStr;
					if (!ParseShaderResourceTypeEnum(Res.Type, ResTypeStr))
					{
						MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' unknown Type Enum=" + std::to_string(static_cast<int>(Res.Type)));
						return FALSE;
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
							return FALSE;
						}
						std::string StructName = Res.Name + "_Struct";
						Header += "struct " + StructName + "\n{\n";
						for (const MCVariableInput& Field : Res.StructFields)
						{
							std::string FieldPrefixStr, FieldSuffixStr;
							if (!ParseVariableFormatPrefixEnum(Field.Format.Prefix, FieldPrefixStr))
							{
								MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' StructField '" + Field.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(Field.Format.Prefix)));
								return FALSE;
							}
							if (!ParseVariableFormatSuffixEnum(Field.Format.Suffix, FieldSuffixStr))
							{
								MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' StructField '" + Field.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(Field.Format.Suffix)));
								return FALSE;
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
							return FALSE;
						}
						std::string ElemPrefixStr;
						if (!ParseVariableFormatPrefixEnum(Res.ElemFmt.Prefix, ElemPrefixStr))
						{
							MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' unknown ElementPrefix Enum=" + std::to_string(static_cast<int>(Res.ElemFmt.Prefix)));
							return FALSE;
						}
						std::string ElemSuffixStr;
						if (!ParseVariableFormatSuffixEnum(Res.ElemFmt.Suffix, ElemSuffixStr))
						{
							MCLog.Error("ParseShader " + InTypePrefixStr + "Resource '" + Res.Name + "' unknown ElementSuffix Enum=" + std::to_string(static_cast<int>(Res.ElemFmt.Suffix)));
							return FALSE;
						}
						Header += InTypePrefixStr + ResTypeStr + "<" + ElemPrefixStr + ElemSuffixStr + "> " + Res.Name + " : register(" + InShaderTypePrefixStr + std::to_string(Res.Slot) + ");\n";
					}
				}
				return TRUE;
			};
			if (!ProcessResourceBindings("", "t", FinalBindings.Textures, FinalBindings.Resources))
			{
				return FALSE;
			}
			Header += "\n";

			for (const MCTextureSampler& IterSampler : FinalBindings.TexSamplers)
			{
				Header += "SamplerState " + IterSampler.Name + " : register(s" + std::to_string(IterSampler.Slot) + ");\n";
			}
			Header += "\n";
			if (!ProcessResourceBindings("RW", "u", FinalBindings.RWTextures, FinalBindings.RWResources))
			{
				return FALSE;
			}
			Header += "\n";
			if (!(FinalInStream.Inputs.empty()))
			{
				Header += "struct FinalInputStream\n{\n";
				for (const std::pair<const int, std::vector<MCVFInput>>& IterInput : FinalInStream.Inputs)
				{
					for (const MCVFInput& Input : IterInput.second)
					{
						std::string FormatPrefixStr;
						if (!ParseVariableFormatPrefixEnum(Input.Format.Prefix, FormatPrefixStr))
						{
							MCLog.Error("ParseShader FinalInputStream '" + FinalInStream.Name + "' Input '" + Input.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(Input.Format.Prefix)));
							return FALSE;
						}
						std::string FormatSuffixStr;
						if (!ParseVariableFormatSuffixEnum(Input.Format.Suffix, FormatSuffixStr))
						{
							MCLog.Error("ParseShader FinalInputStream '" + FinalInStream.Name + "' Input '" + Input.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(Input.Format.Suffix)));
							return FALSE;
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
				for (const MCReplace& Def : FinalRpl.Replaces)
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
				for (const MCDefine& Def : FinalDef.Defines)
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
			rapidjson::Document::AllocatorType& ReflectionAllocator = ReflectionDoc.GetAllocator();

			auto ParseReflectionVariableInput = [&MCLog, &ReflectionAllocator](const std::string& InDebugStr, bool InIsCheckExposure, const MCVariableInput& InVariable, rapidjson::Value& OutJson)->bool
			{
				OutJson.AddMember("Name", rapidjson::Value(InVariable.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
				std::string FieldPrefixStr;
				if (!ParseVariableFormatPrefixEnum(InVariable.Format.Prefix, FieldPrefixStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InDebugStr + " Field '" + InVariable.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(InVariable.Format.Prefix)));
					return FALSE;
				}
				std::string FieldSuffixStr;
				if (!ParseVariableFormatSuffixEnum(InVariable.Format.Suffix, FieldSuffixStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InDebugStr + " Field '" + InVariable.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(InVariable.Format.Suffix)));
					return FALSE;
				}
				const std::string FormatStr = FieldPrefixStr + FieldSuffixStr;
				OutJson.AddMember("Format", rapidjson::Value(FormatStr.c_str(), ReflectionAllocator), ReflectionAllocator);
				// Layout is filled by the shader compiler from its reflection after a successful compile:
				// zero means "unknown", CompilerCulled means "not present in the compiled shader".
				OutJson.AddMember("ByteOffset", 0, ReflectionAllocator);
				OutJson.AddMember("ContentBytes", 0, ReflectionAllocator);
				OutJson.AddMember("ArrayStride", 0, ReflectionAllocator);
				OutJson.AddMember("CompilerCulled", FALSE, ReflectionAllocator);
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
					for (uint32_t Dft : InVariable.Default)
					{
						DftArray.PushBack(Dft, ReflectionAllocator);
					}
					OutJson.AddMember("Default", DftArray, ReflectionAllocator);
				}
				return TRUE;
			};

			ReflectionDoc.AddMember("ShaderName", rapidjson::Value(Shader.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
			ReflectionDoc.AddMember("EntryPoint", rapidjson::Value(Shader.Entry.c_str(), ReflectionAllocator), ReflectionAllocator);
			ReflectionDoc.AddMember("Stage", rapidjson::Value(InLoadParam.Stage.c_str(), ReflectionAllocator), ReflectionAllocator);

			// --- Constant Buffers ---
			if (!(FinalBindings.CBuffers.empty()))
			{
				rapidjson::Value CBArray(rapidjson::kArrayType);
				for (const MCConstantBuffer& CB : FinalBindings.CBuffers)
				{
					rapidjson::Value CBObj(rapidjson::kObjectType);
					CBObj.AddMember("Name", rapidjson::Value(CB.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
					CBObj.AddMember("Slot", CB.Slot, ReflectionAllocator);
					CBObj.AddMember("RootConstant", CB.bRootConstant, ReflectionAllocator);
					// Filled by the shader compiler from its reflection after a successful compile.
					CBObj.AddMember("ByteSize", 0, ReflectionAllocator);
					CBObj.AddMember("CompilerCulled", FALSE, ReflectionAllocator);

					rapidjson::Value FieldsArray(rapidjson::kArrayType);
					for (const MCVariableInput& Field : CB.Fields)
					{
						rapidjson::Value FieldObj(rapidjson::kObjectType);
						if (!ParseReflectionVariableInput("ConstantBuffer '" + CB.Name + "'", TRUE, Field, FieldObj))
						{
							return FALSE;
						}
						FieldsArray.PushBack(FieldObj, ReflectionAllocator);
					}
					CBObj.AddMember("Fields", FieldsArray, ReflectionAllocator);
					CBArray.PushBack(CBObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("CBuffers", CBArray, ReflectionAllocator);
			}

			// Whether this compilation took the bindless path at all. It is the deterministic half
			// of the pair the reflection reports: the declared flag is what the author asked for,
			// this is what the compiled stage actually carries, and it is false whenever the target
			// could not express the path - a 5.0 or 6.0 build of a resource that asked for it.
			const bool bBindlessCompiled = InLoadParam.bSM66;
			auto ParseReflectionTexture = [&MCLog, &ReflectionAllocator, bBindlessCompiled](const std::string& InPrefixStr, const MCTexture& InTex, rapidjson::Value& OutObj)->bool
			{
				OutObj.AddMember("Name", rapidjson::Value(InTex.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
				std::string FormatPrefixStr;
				if (!ParseVariableFormatPrefixEnum(InTex.Format.Prefix, FormatPrefixStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Texture Field '" + InTex.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(InTex.Format.Prefix)));
					return FALSE;
				}
				std::string FormatSuffixStr;
				if (!ParseVariableFormatSuffixEnum(InTex.Format.Suffix, FormatSuffixStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Texture Field '" + InTex.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(InTex.Format.Suffix)));
					return FALSE;
				}
				std::string FormatTypeStr;
				if (!ParseTextureTypeEnum(InTex.Type, FormatTypeStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Texture Field '" + InTex.Name + "' unknown Type Enum=" + std::to_string(static_cast<int>(InTex.Type)));
					return FALSE;
				}
				const std::string FormatStr = FormatPrefixStr + FormatSuffixStr;
				OutObj.AddMember("Format", rapidjson::Value(FormatStr.c_str(), ReflectionAllocator), ReflectionAllocator);
				OutObj.AddMember("Type", rapidjson::Value(FormatTypeStr.c_str(), ReflectionAllocator), ReflectionAllocator);
				OutObj.AddMember("Slot", InTex.Slot, ReflectionAllocator);
				OutObj.AddMember("Sampler", rapidjson::Value(InTex.SamplerName.c_str(), ReflectionAllocator), ReflectionAllocator);
				OutObj.AddMember("Exposure", InTex.bExposure, ReflectionAllocator);
				// Bindless, reported twice on purpose: PreferBindless is what the resource declared,
				// Bindless is what this compilation did with it. The runtime reads the second one -
				// it needs no inference and no culled flag, and a build whose target cannot express
				// the path simply reports false and stays runnable.
				OutObj.AddMember("PreferBindless", InTex.bPreferBindless, ReflectionAllocator);
				OutObj.AddMember("Bindless", (InTex.bPreferBindless && bBindlessCompiled), ReflectionAllocator);
				OutObj.AddMember("BindConstantBuffer", rapidjson::Value(InTex.BindConstantBuffer.c_str(), ReflectionAllocator), ReflectionAllocator);
				OutObj.AddMember("CompilerCulled", FALSE, ReflectionAllocator);
				return TRUE;
			};

			// --- Textures (SRV) ---
			if (!(FinalBindings.Textures.empty()))
			{
				rapidjson::Value TexArray(rapidjson::kArrayType);
				for (const MCTexture& Tex : FinalBindings.Textures)
				{
					rapidjson::Value TexObj(rapidjson::kObjectType);
					if (!ParseReflectionTexture("", Tex, TexObj))
					{
						return FALSE;
					}
					TexArray.PushBack(TexObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("Textures", TexArray, ReflectionAllocator);
			}

			auto ParseReflectionResource = [&MCLog, &ReflectionAllocator, &ParseReflectionVariableInput, bBindlessCompiled](const std::string& InPrefixStr, const MCShaderResource& InRes, rapidjson::Value& OutObj)->bool
			{
				OutObj.AddMember("Name", rapidjson::Value(InRes.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
				std::string ResTypeStr;
				if (!ParseShaderResourceTypeEnum(InRes.Type, ResTypeStr))
				{
					MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Resource '" + InRes.Name + "' parse unknown TypeName Enum=" + std::to_string(static_cast<int>(InRes.Type)));
					return FALSE;
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
						return FALSE;
					}
					if (InRes.bHasStruct)
					{
						rapidjson::Value StructArray(rapidjson::kArrayType);
						for (const MCVariableInput& StructField : InRes.StructFields)
						{
							rapidjson::Value StructFieldObj(rapidjson::kObjectType);
							if (!ParseReflectionVariableInput(InPrefixStr + "Resource '" + InRes.Name + "'", FALSE, StructField, StructFieldObj))
							{
								return FALSE;
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
							return FALSE;
						}
						std::string ResSuffixStr;
						if (!ParseVariableFormatSuffixEnum(InRes.ElemFmt.Suffix, ResSuffixStr))
						{
							MCLog.Error("ParseShader Output Reflection " + InPrefixStr + "Resource '" + InRes.Name + "' unknown ElementFormatSuffix Enum=" + std::to_string(static_cast<int>(InRes.ElemFmt.Suffix)));
							return FALSE;
						}
						const std::string FormatStr = ResPrefixStr + ResSuffixStr;
						OutObj.AddMember("Format", rapidjson::Value(FormatStr.c_str(), ReflectionAllocator), ReflectionAllocator);
					}
				}
				OutObj.AddMember("Exposure", InRes.bExposure, ReflectionAllocator);
				// Bindless: see ParseReflectionTexture. The declared flag and the effective one are
				// both reported, and the runtime reads the effective one.
				OutObj.AddMember("PreferBindless", InRes.bPreferBindless, ReflectionAllocator);
				OutObj.AddMember("Bindless", (InRes.bPreferBindless && bBindlessCompiled), ReflectionAllocator);
				OutObj.AddMember("BindConstantBuffer", rapidjson::Value(InRes.BindConstantBuffer.c_str(), ReflectionAllocator), ReflectionAllocator);
				OutObj.AddMember("CompilerCulled", FALSE, ReflectionAllocator);
				return TRUE;
			};

			// --- Resources (SRV buffers) ---
			if (!(FinalBindings.Resources.empty()))
			{
				rapidjson::Value ResArray(rapidjson::kArrayType);
				for (const MCShaderResource& Res : FinalBindings.Resources)
				{
					rapidjson::Value ResObj(rapidjson::kObjectType);
					if (!ParseReflectionResource("", Res, ResObj))
					{
						return FALSE;
					}
					ResArray.PushBack(ResObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("Resources", ResArray, ReflectionAllocator);
			}

			// --- Samplers ---
			if (!(FinalBindings.TexSamplers.empty()))
			{
				rapidjson::Value SamplerArray(rapidjson::kArrayType);
				for (const MCTextureSampler& Sampler : FinalBindings.TexSamplers)
				{
					rapidjson::Value SamplerObj(rapidjson::kObjectType);
					SamplerObj.AddMember("Name", rapidjson::Value(Sampler.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
					SamplerObj.AddMember("Slot", Sampler.Slot, ReflectionAllocator);
					std::string FilterStr;
					if (!ParseTextureSamplerFilterEnum(Sampler.Filter, FilterStr))
					{
						MCLog.Error("ParseShader Output Reflection Sampler '" + Sampler.Name + "' unknown SamplerFilter Enum=" + std::to_string(static_cast<int>(Sampler.Filter)));
						return FALSE;
					}
					SamplerObj.AddMember("Filter", rapidjson::Value(FilterStr.c_str(), ReflectionAllocator), ReflectionAllocator);
					std::string AddressUStr;
					if (!ParseTextureSamplerAddressEnum(Sampler.AddressU, AddressUStr))
					{
						MCLog.Error("ParseShader Output Reflection Sampler '" + Sampler.Name + "' unknown SamplerAddressU Enum=" + std::to_string(static_cast<int>(Sampler.AddressU)));
						return FALSE;
					}
					SamplerObj.AddMember("AddressU", rapidjson::Value(AddressUStr.c_str(), ReflectionAllocator), ReflectionAllocator);
					std::string AddressVStr;
					if (!ParseTextureSamplerAddressEnum(Sampler.AddressV, AddressVStr))
					{
						MCLog.Error("ParseShader Output Reflection Sampler '" + Sampler.Name + "' unknown SamplerAddressV Enum=" + std::to_string(static_cast<int>(Sampler.AddressV)));
						return FALSE;
					}
					SamplerObj.AddMember("AddressV", rapidjson::Value(AddressVStr.c_str(), ReflectionAllocator), ReflectionAllocator);
					std::string AddressWStr;
					if (!ParseTextureSamplerAddressEnum(Sampler.AddressW, AddressWStr))
					{
						MCLog.Error("ParseShader Output Reflection Sampler '" + Sampler.Name + "' unknown SamplerAddressW Enum=" + std::to_string(static_cast<int>(Sampler.AddressW)));
						return FALSE;
					}
					SamplerObj.AddMember("AddressW", rapidjson::Value(AddressWStr.c_str(), ReflectionAllocator), ReflectionAllocator);
					std::string ComparisonFuncStr;
					if (!ParseTextureComparisonFuncEnum(Sampler.ComparisonFunc, ComparisonFuncStr))
					{
						MCLog.Error("ParseShader Output Reflection Sampler '" + Sampler.Name + "' unknown SamplerComparisonFunc Enum=" + std::to_string(static_cast<int>(Sampler.ComparisonFunc)));
						return FALSE;
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
					SamplerObj.AddMember("CompilerCulled", FALSE, ReflectionAllocator);
					// Bindless: a sampler that declares it is index driven like a resource, and the
					// ones that do not are the static samplers the tables still carry. The runtime
					// reads the effective flag, exactly as it does for a texture or a buffer.
					SamplerObj.AddMember("PreferBindless", Sampler.bPreferBindless, ReflectionAllocator);
					SamplerObj.AddMember("Bindless", (Sampler.bPreferBindless && bBindlessCompiled), ReflectionAllocator);
					SamplerObj.AddMember("BindConstantBuffer", rapidjson::Value(Sampler.BindConstantBuffer.c_str(), ReflectionAllocator), ReflectionAllocator);
					SamplerArray.PushBack(SamplerObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("Samplers", SamplerArray, ReflectionAllocator);
			}

			// --- RW Textures / RW Resources ---
			if (!(FinalBindings.RWTextures.empty()))
			{
				rapidjson::Value RWTexArray(rapidjson::kArrayType);
				for (const MCTexture& Tex : FinalBindings.RWTextures)
				{
					rapidjson::Value TexObj(rapidjson::kObjectType);
					if (!ParseReflectionTexture("RW", Tex, TexObj))
					{
						return FALSE;
					}
					RWTexArray.PushBack(TexObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("RWTextures", RWTexArray, ReflectionAllocator);
			}
			if (!(FinalBindings.RWResources.empty()))
			{
				rapidjson::Value RWResArray(rapidjson::kArrayType);
				for (const MCShaderResource& Res : FinalBindings.RWResources)
				{
					rapidjson::Value ResObj(rapidjson::kObjectType);
					if (!ParseReflectionResource("RW", Res, ResObj))
					{
						return FALSE;
					}
					RWResArray.PushBack(ResObj, ReflectionAllocator);
				}
				ReflectionDoc.AddMember("RWResources", RWResArray, ReflectionAllocator);
			}

			// --- Input Layout ---
			if (!(FinalInStream.Inputs.empty()))
			{
				rapidjson::Value InputLayoutArray(rapidjson::kArrayType);
				for (const std::pair<const int, std::vector<MCVFInput>>& InputSlot : FinalInStream.Inputs)
				{
					for (const MCVFInput& Input : InputSlot.second)
					{
						rapidjson::Value InputObj(rapidjson::kObjectType);
						InputObj.AddMember("Name", rapidjson::Value(Input.Name.c_str(), ReflectionAllocator), ReflectionAllocator);
						std::string FormatPrefixStr;
						if (!ParseVariableFormatPrefixEnum(Input.Format.Prefix, FormatPrefixStr))
						{
							MCLog.Error("ParseShader Output Reflection InputLayout '" + Input.Name + "' unknown FormatPrefix Enum=" + std::to_string(static_cast<int>(Input.Format.Prefix)));
							return FALSE;
						}
						std::string FormatSuffixStr;
						if (!ParseVariableFormatSuffixEnum(Input.Format.Suffix, FormatSuffixStr))
						{
							MCLog.Error("ParseShader Output Reflection InputLayout '" + Input.Name + "' unknown FormatSuffix Enum=" + std::to_string(static_cast<int>(Input.Format.Suffix)));
							return FALSE;
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
				return FALSE;
			}
		}

		return TRUE;
	}

}	// namespace MC
