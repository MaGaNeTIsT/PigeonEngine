#include <IO/JsonConfigFile.h>

#include <Development/EditorLogManager.h>
#include <IO/FileHelper.h>

namespace PigeonEngine
{
	namespace
	{
		BOOL32 ReadRoot(const EString& InPath, CJsonObject& OutRoot)
		{
			EString Text;
			if ((!EFileHelper::ReadFileAsString(InPath, Text)) || (Text.Length() == 0u))
			{
				return FALSE;
			}
			CJsonObject Parsed(*Text);
			if (!Parsed.IsValid())
			{
				PE_LOG_WARN(EString(PE_TEXT("Settings: '")) + InPath
					+ EString(PE_TEXT("' is not a valid JSON object; using the next source.")));
				return FALSE;
			}
			OutRoot = Parsed;
			return TRUE;
		}
	}

	EJsonConfigFile::EJsonConfigFile(const EString& InName)
		: Name(InName)
		, DefaultPath(EString(PE_TEXT("./Engine/Configs/")) + InName + EString(PE_TEXT(".default.json")))
		, UserPath(EString(PE_TEXT("./Engine/Configs/")) + InName + EString(PE_TEXT(".user.json")))
	{
	}

	void EJsonConfigFile::AddBool(const EString& InKey, const BOOL32& bInDefault)
	{
		if (Keys.Find(InKey) >= 0)
		{
			return;
		}
		Keys.Add(InKey);
		Schema.SetBoolFieldByPath(InKey, bInDefault != FALSE);
	}

	void EJsonConfigFile::AddInt(const EString& InKey, const INT32& InDefault)
	{
		if (Keys.Find(InKey) >= 0)
		{
			return;
		}
		Keys.Add(InKey);
		Schema.SetIntFieldByPath(InKey, InDefault);
	}

	void EJsonConfigFile::AddFloat(const EString& InKey, const FLOAT& InDefault)
	{
		if (Keys.Find(InKey) >= 0)
		{
			return;
		}
		Keys.Add(InKey);
		Schema.SetFloatFieldByPath(InKey, InDefault);
	}

	void EJsonConfigFile::AddString(const EString& InKey, const EString& InDefault)
	{
		if (Keys.Find(InKey) >= 0)
		{
			return;
		}
		Keys.Add(InKey);
		Schema.SetStringFieldByPath(InKey, InDefault);
	}

	void EJsonConfigFile::Load()
	{
		Defaults = Schema;
		ApplyFile(DefaultPath, Defaults);
		Document = Defaults;
		ApplyFile(UserPath, Document);
	}

	void EJsonConfigFile::ApplyFile(const EString& InPath, CJsonObject& OutDocument)
	{
		CJsonObject Root;
		if (!ReadRoot(InPath, Root))
		{
			return;
		}
		for (UINT32 Index = 0u, Num = Keys.Num<UINT32>(); Index < Num; Index++)
		{
			OutDocument.CopyFieldByPath(Keys[Index], Root);
		}
	}

	void EJsonConfigFile::Save() const
	{
		CJsonObject User;
		for (UINT32 Index = 0u, Num = Keys.Num<UINT32>(); Index < Num; Index++)
		{
			const EString& Key = Keys[Index];
			if (!Document.IsFieldEqualByPath(Key, Defaults))
			{
				User.CopyFieldByPath(Key, Document);
			}
		}

		if (User.IsEmpty())
		{
			// Every value is back at the tracked default; drop the local file so future
			// default changes apply without a stale override hiding them.
			EFileHelper::DeleteFile(UserPath);
			return;
		}
		if (!EFileHelper::SaveStringToFile(UserPath, User.ToString()))
		{
			PE_LOG_WARN(EString(PE_TEXT("Settings: could not write '")) + UserPath + EString(PE_TEXT("'.")));
		}
	}

	BOOL32 EJsonConfigFile::GetBool(const EString& InKey)
	{
		BOOL8 Raw = FALSE;
		if (!Document.GetBoolFieldByPath(InKey, Raw))
		{
			PE_LOG_WARN(EString(PE_TEXT("Settings: '")) + InKey + EString(PE_TEXT("' is not a bool in ")) + Name + EString(PE_TEXT(".")));
			return FALSE;
		}
		return (Raw != 0) ? TRUE : FALSE;
	}

	INT32 EJsonConfigFile::GetInt(const EString& InKey)
	{
		INT32 Raw = 0;
		if (!Document.GetIntFieldByPath(InKey, Raw))
		{
			PE_LOG_WARN(EString(PE_TEXT("Settings: '")) + InKey + EString(PE_TEXT("' is not an int in ")) + Name + EString(PE_TEXT(".")));
			return 0;
		}
		return Raw;
	}

	FLOAT EJsonConfigFile::GetFloat(const EString& InKey)
	{
		FLOAT Raw = 0.0f;
		if (!Document.GetFloatFieldByPath(InKey, Raw))
		{
			PE_LOG_WARN(EString(PE_TEXT("Settings: '")) + InKey + EString(PE_TEXT("' is not a float in ")) + Name + EString(PE_TEXT(".")));
			return 0.0f;
		}
		return Raw;
	}

	EString EJsonConfigFile::GetString(const EString& InKey)
	{
		EString Raw;
		if (!Document.GetStringFieldByPath(InKey, Raw))
		{
			PE_LOG_WARN(EString(PE_TEXT("Settings: '")) + InKey + EString(PE_TEXT("' is not a string in ")) + Name + EString(PE_TEXT(".")));
			return EString();
		}
		return Raw;
	}

	void EJsonConfigFile::SetBool(const EString& InKey, const BOOL32& bInValue)
	{
		BOOL8 Current = FALSE;
		if (Document.GetBoolFieldByPath(InKey, Current) && ((Current != 0) == (bInValue != FALSE)))
		{
			return;
		}
		Document.SetBoolFieldByPath(InKey, bInValue != FALSE);
		Save();
	}

	void EJsonConfigFile::SetInt(const EString& InKey, const INT32& InValue)
	{
		INT32 Current = 0;
		if (Document.GetIntFieldByPath(InKey, Current) && (Current == InValue))
		{
			return;
		}
		Document.SetIntFieldByPath(InKey, InValue);
		Save();
	}

	void EJsonConfigFile::SetFloat(const EString& InKey, const FLOAT& InValue)
	{
		FLOAT Current = 0.0f;
		if (Document.GetFloatFieldByPath(InKey, Current) && (Current == InValue))
		{
			return;
		}
		Document.SetFloatFieldByPath(InKey, InValue);
		Save();
	}

	void EJsonConfigFile::SetString(const EString& InKey, const EString& InValue)
	{
		EString Current;
		if (Document.GetStringFieldByPath(InKey, Current) && (Current == InValue))
		{
			return;
		}
		Document.SetStringFieldByPath(InKey, InValue);
		Save();
	}
}
