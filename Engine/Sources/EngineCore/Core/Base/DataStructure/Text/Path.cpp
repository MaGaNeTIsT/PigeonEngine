#include "Path.h"
#include <Config/ErrorCaption.h>
#include <Base/Math/Math.h>
#include <Config/EngineConfig.h>
#include "../../../../../Development/Alert/DevelopmentDefines.h"
#include <filesystem>

namespace PigeonEngine
{
	UINT32 FindFirstSlashIndex(const EString& Path)
	{
		const CHAR slash[] = { '/' ,'\0' };
		UINT32 FirstSlash = Path.Find(slash);
		if (Path.Length() >= (FirstSlash + 1u))
		{
			const CHAR backslash[] = { '\\', '\0' };
			FirstSlash = Path.Find(backslash);
			PE_CHECK((ENGINE_STRING_ERROR), ("Check string rest length error."), (Path.Length() >= (FirstSlash + 1u)));
		}
		return FirstSlash;
	};

	UINT32 FindLastSlashIndex(const EString& Path)
	{
		const CHAR slash[] = { '/' ,'\0' };
		UINT32 LastSlash = Path.RightFind(slash);
		if (Path.Length() >= (LastSlash + 1u))
		{
			const CHAR backslash[] = { '\\', '\0' };
			LastSlash = Path.RightFind(backslash);
			PE_CHECK((ENGINE_STRING_ERROR), ("Check string rest length error."), (Path.Length() >= (LastSlash + 1u)));
		}
		return LastSlash;
	};

	EString EPath::ChangeExtension(const EString& Path, const EString& Extension)
	{
		const CHAR Dot[] = { '.' ,'\0' };
		const UINT32 LastDot = Path.RightFind(Dot);
		PE_CHECK((ENGINE_STRING_ERROR), ("Check string rest length error."), (Path.Length() >= (LastDot + 1u)));
		return Path.Substring(0u, LastDot + 1u) + Extension;
	}

	EString EPath::Combine(const EString& Path1, const EString& Path2)
	{
		return Path1 + "/" + Path2;
	}

	EString EPath::Combine(const EString& Path1, const EString& Path2, const EString& Path3)
	{
		return Path1 + "/" + Path2 + "/" + Path3;
	}

	EString EPath::Combine(const EString* Paths, INT32 Count)
	{
		EString result;
		for (int i = 0; i < Count - 1; i++)
		{
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

	EString EPath::GetFullPath(const EString& Path)
	{
		return EString(EBaseSettings::ENGINE_ASSET_DIRECTORY) + Path;
	}

	EString EPath::GetFullPath(const EString& Path, const EString& BasePath)
	{
		return BasePath + "/" + Path;
	}

	EString EPath::GetRootPath(const EString& Path)
	{
		UINT32 SlashIndex = FindFirstSlashIndex(Path);
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
