#include "Path.h"
#include <Config/ErrorCaption.h>
#include <Base/Math/Math.h>
#include <Config/EngineConfig.h>
#include "../../../../../Development/Alert/DevelopmentDefines.h"
#include <filesystem>
#include <Base/DataStructure/Container/Array.h>

namespace PigeonEngine
{
	UINT32 FindFirstSlashIndex(const EString& Path)
	{
		const CHAR slash[] = { '/' ,'\0' };
		INT32 FirstSlash = Path.Find(slash);
		if (Path.Length() >= (FirstSlash + 1u))
		{
			const CHAR backslash[] = { '\\', '\0' };
			INT32 FirstSlash1 = Path.Find(backslash);
			PE_CHECK((ENGINE_STRING_ERROR), ("Check string rest length error."), (Path.Length() >= (FirstSlash1 + 1u)));
			if (FirstSlash1 < FirstSlash)
				FirstSlash = FirstSlash1;
		}
		return (UINT32)FirstSlash;
	}

	UINT32 FindLastSlashIndex(const EString& Path)
	{
		const CHAR slash[] = { '/' ,'\0' };
		INT32 LastSlash = Path.RightFind(slash);
		if (Path.Length() >= (LastSlash + 1u))
		{
			const CHAR backslash[] = { '\\', '\0' };
			INT32 LastSlash1 = Path.RightFind(backslash);
			PE_CHECK((ENGINE_STRING_ERROR), ("Check string rest length error."), (Path.Length() >= (LastSlash1 + 1u)));
			if (LastSlash1 > LastSlash)
				LastSlash = LastSlash1;
		}
		return (UINT32)LastSlash;
	}

	EString ToLower(const EString& String)
	{
		TArray<CHAR> out;
		for(UINT32 i = 0;i < String.Length();i++)
			out.Add((CHAR)::tolower(String[i]));
		out.Add('\0');//TODO : did this necessary?
		return EString(out.GetData());
	}

	EString ToUpper(const EString& String)
	{
		TArray<CHAR> out;
		for (UINT32 i = 0; i < String.Length(); i++)
			out.Add((CHAR)::toupper(String[i]));
		out.Add('\0');//TODO : did this necessary?
		return EString(out.GetData());
	}

	EString EPath::ChangeExtension(const EString& Path, const EString& Extension)
	{
		const CHAR Dot[] = { '.' ,'\0' };
		const UINT32 LastDot = Path.RightFind(Dot);
		PE_CHECK((ENGINE_STRING_ERROR), ("Check string rest length error."), (Path.Length() >= (LastDot + 1u)));
		return Path.Substring(0u, LastDot + 1u) + Extension;
	}

	EString EPath::Combine(const EString& Path1, const EString& Path2)
	{
		if (Path1.LastIndex() == FindLastSlashIndex(Path1))
			return Path1 + Path2;
		return Path1 + "/" + Path2;
	}

	EString EPath::Combine(const EString& Path1, const EString& Path2, const EString& Path3)
	{
		EString Path;
		if (Path1.LastIndex() == FindLastSlashIndex(Path1))
			Path = Path1 + Path2;
		else
			Path = Path1 + "/" + Path2;
		if (Path2.LastIndex() == FindLastSlashIndex(Path2))
			return Path + Path3;
		else
			return Path + "/" + Path3;
	}

	EString EPath::Combine(const EString* Paths, INT32 Count)
	{
		EString result;
		for (INT32 i = 0; i < Count - 1; i++)
		{
			if (Paths[i].LastIndex() == FindLastSlashIndex(Paths[i]))
				result += Paths[i];
			else
				result += Paths[i] + "/";
		}
		result += Paths[Count - 1];
		return result;
	}
	BOOL32 EPath::Exists(const EString& Path)
	{
		return std::filesystem::exists(Path.GetDataAsCopy());
	}
	EString EPath::GetDirectoryName(const EString& Path)
	{
		UINT32 LastSlash = FindLastSlashIndex(Path);
		EString TempPath = Path.Substring(0u, LastSlash - 1u);
		UINT32 LeastSlash = FindLastSlashIndex(TempPath);
		return Path.Substring(LeastSlash + 1u, (LastSlash - LeastSlash) - 1u);
	}
	EString EPath::GetExtension(const EString& Path)
	{
		const CHAR Dot[] = { '.' ,'\0' };
		const UINT32 LastDot = Path.RightFind(Dot);
		PE_CHECK((ENGINE_STRING_ERROR), ("Check string rest length error."), (Path.Length() >= (LastDot + 1u)));
		return Path.Substring(LastDot + 1u, Path.Length() - (LastDot + 1u));
	}

	EString EPath::GetFileNameWithExtension(const EString& Path)
	{
		UINT32 LastSlash = FindLastSlashIndex(Path);
		return Path.Substring(LastSlash + 1u, Path.Length() - LastSlash + 1u);
	}

	EString EPath::GetFileNameWithoutExtension(const EString& Path)
	{
		UINT32 LastSlash = FindLastSlashIndex(Path);
		const CHAR Dot[] = { '.' ,'\0' };
		EString SubString = Path.Substring(LastSlash + 1u, Path.Length() - LastSlash + 1u);
		const UINT32 LastDot = SubString.RightFind(Dot);
		return SubString.Substring(0u, LastDot);
	}

	EString EPath::GetFullAssetPath(const EString& Path)
	{
		return Combine(EString(EBaseSettings::ENGINE_ASSET_DIRECTORY), Path);
	}

	EString EPath::GetFullPath(const EString& Path, const EString& BasePath)
	{
		return Combine(BasePath, Path);
	}

	EString EPath::GetRootPath(const EString& Path)
	{
		UINT32 SlashIndex = FindFirstSlashIndex(Path);
		return Path.Substring(0, SlashIndex);
	}

	EString EPath::GetFileFolderPath(const EString& Path)
	{
		UINT32 SlashIndex = FindLastSlashIndex(Path);
		return Path.Substring(0, SlashIndex);
	}

	EString EPath::GetRelativePath(const EString& Path, const EString& BasePath)
	{
		if (Path.Contains(BasePath))
		{
			EString subString = Path.Replace(BasePath, "");
			return subString.Substring(1, subString.LastIndex());
		}
		return EString();
	}

	BOOL32 EPath::HasExtension(const EString& Path)
	{
		return EPath::GetExtension(Path).Length() > 0;
	}
};
