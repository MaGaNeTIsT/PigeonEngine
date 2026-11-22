#include <Editor/Asset/AssetFileOperations.h>

#if ENGINE_EDITOR_ONLY

#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>
#include <IO/FileHelper.h>

namespace PigeonEngine
{
	namespace EAssetFileOperations
	{
		namespace
		{
			EString NormalizePath(const EString& InPath)
			{
				EString Result = InPath.Replace("\\", "/");
				while ((Result.Length() > 1u) && (Result[Result.LastIndex()] == '/'))
				{
					Result = Result.Left(Result.Length() - 1u);
				}
				return Result;
			}

			EString CombinePath(const EString& InFolder, const EString& InName)
			{
				EString Folder = NormalizePath(InFolder);
				if ((Folder.Length() > 0u) && (Folder[Folder.LastIndex()] != '/'))
				{
					Folder += "/";
				}
				return Folder + InName;
			}

			EString GetParentFolder(const EString& InPath)
			{
				return NormalizePath(EPath::GetFileFolderPath(NormalizePath(InPath)));
			}
		}

		BOOL32 ValidateName(const EString& InName, EString& OutError)
		{
			OutError = EString();
			if (InName.Length() == 0u)
			{
				OutError = EString("Name cannot be empty.");
				return FALSE;
			}
			if (InName.Length() > 120u)
			{
				OutError = EString("Name is too long (limit 120 characters).");
				return FALSE;
			}
			if ((InName == ".") || (InName == ".."))
			{
				OutError = EString("Name is reserved.");
				return FALSE;
			}
			const CHAR InvalidChars[] = "\\/:*?\"<>|";
			for (UINT32 CharIndex = 0u; CharIndex < InName.Length(); CharIndex++)
			{
				const CHAR Char = InName[CharIndex];
				if (static_cast<UINT8>(Char) < 32u)
				{
					OutError = EString("Name contains a control character.");
					return FALSE;
				}
				for (UINT32 InvalidIndex = 0u; InvalidChars[InvalidIndex] != 0; InvalidIndex++)
				{
					if (Char == InvalidChars[InvalidIndex])
					{
						OutError = EString("Name contains an invalid character: ") + EString::FromChar(InvalidChars[InvalidIndex]);
						return FALSE;
					}
				}
			}
			const CHAR LastChar = InName[InName.LastIndex()];
			if ((LastChar == '.') || (LastChar == ' '))
			{
				OutError = EString("Name cannot end with a dot or a space.");
				return FALSE;
			}
			return TRUE;
		}

		EString StripAssetExtension(const EString& InName)
		{
			const EString Extension(EEngineSettings::ENGINE_ASSET_NAME_TYPE);
			const EString LowerName = InName.ToLower();
			const EString LowerExtension = Extension.ToLower();
			if ((LowerName.Length() > LowerExtension.Length())
				&& (LowerName.Right(LowerExtension.Length()) == LowerExtension))
			{
				return InName.Left(InName.Length() - LowerExtension.Length());
			}
			return InName;
		}

		BOOL32 RenameAssetFile(const EString& InAssetPath, const EString& InNewName
			, EString& OutNewPath, EString& OutError)
		{
			OutNewPath = EString();
			OutError = EString();
			const EString Stem = StripAssetExtension(InNewName);
			if (ValidateName(Stem, OutError) == FALSE)
			{
				return FALSE;
			}
			if (InAssetPath.Length() == 0u)
			{
				OutError = EString("No asset is selected.");
				return FALSE;
			}

			const EString NewPath = CombinePath(GetParentFolder(InAssetPath)
				, Stem + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE));
			if (NormalizePath(NewPath) == NormalizePath(InAssetPath))
			{
				OutNewPath = InAssetPath;
				return TRUE;
			}
			if (EFileHelper::IsFileExists(NewPath) != FALSE)
			{
				OutError = EString("An asset with that name already exists.");
				return FALSE;
			}
			if (!EFileHelper::MoveFile(InAssetPath, NewPath))
			{
				OutError = EString("Failed to rename asset file: ") + InAssetPath;
				return FALSE;
			}
			OutNewPath = NewPath;
			return TRUE;
		}

		BOOL32 MoveAssetFile(const EString& InAssetPath, const EString& InDestinationFolder
			, EString& OutNewPath, EString& OutError)
		{
			OutNewPath = EString();
			OutError = EString();
			if (InAssetPath.Length() == 0u)
			{
				OutError = EString("No asset is selected.");
				return FALSE;
			}
			if (InDestinationFolder.Length() == 0u)
			{
				OutError = EString("No destination folder.");
				return FALSE;
			}
			const EString SourceFolder = GetParentFolder(InAssetPath);
			const EString DestinationFolder = NormalizePath(InDestinationFolder);
			if (SourceFolder == DestinationFolder)
			{
				OutError = EString("The asset is already in that folder.");
				return FALSE;
			}

			const EString FileName = EPath::GetFileNameWithExtension(InAssetPath);
			const EString NewPath = CombinePath(DestinationFolder, FileName);
			if (EFileHelper::IsFileExists(NewPath) != FALSE)
			{
				OutError = EString("The destination already contains a file with that name.");
				return FALSE;
			}
			if (!EFileHelper::MoveFile(InAssetPath, NewPath))
			{
				OutError = EString("Failed to move asset file: ") + InAssetPath;
				return FALSE;
			}
			OutNewPath = NewPath;
			return TRUE;
		}

		BOOL32 DeleteAssetFile(const EString& InAssetPath, EString& OutError)
		{
			OutError = EString();
			if (InAssetPath.Length() == 0u)
			{
				OutError = EString("No asset is selected.");
				return FALSE;
			}
			if (!EFileHelper::DeleteFile(InAssetPath))
			{
				OutError = EString("Failed to delete asset file: ") + InAssetPath;
				return FALSE;
			}
			return TRUE;
		}

		BOOL32 CreateFolder(const EString& InParentFolder, const EString& InFolderName
			, EString& OutNewPath, EString& OutError)
		{
			OutNewPath = EString();
			OutError = EString();
			if (ValidateName(InFolderName, OutError) == FALSE)
			{
				return FALSE;
			}
			if (InParentFolder.Length() == 0u)
			{
				OutError = EString("No parent folder.");
				return FALSE;
			}
			const EString NewPath = CombinePath(InParentFolder, InFolderName);
			if (EFileHelper::IsDirectoryExists(NewPath) != FALSE)
			{
				OutError = EString("A folder with that name already exists.");
				return FALSE;
			}
			if (!EFileHelper::MakeDirectory(NewPath))
			{
				OutError = EString("Failed to create folder: ") + NewPath;
				return FALSE;
			}
			OutNewPath = NewPath;
			return TRUE;
		}

		BOOL32 RenameFolder(const EString& InFolderPath, const EString& InNewName
			, EString& OutNewPath, EString& OutError)
		{
			OutNewPath = EString();
			OutError = EString();
			if (ValidateName(InNewName, OutError) == FALSE)
			{
				return FALSE;
			}
			if (InFolderPath.Length() == 0u)
			{
				OutError = EString("No folder is selected.");
				return FALSE;
			}
			const EString NewPath = CombinePath(GetParentFolder(InFolderPath), InNewName);
			if (NormalizePath(NewPath) == NormalizePath(InFolderPath))
			{
				OutNewPath = InFolderPath;
				return TRUE;
			}
			if (EFileHelper::IsDirectoryExists(NewPath) != FALSE)
			{
				OutError = EString("A folder with that name already exists.");
				return FALSE;
			}
			if (!EFileHelper::MoveDirectory(InFolderPath, NewPath))
			{
				OutError = EString("Failed to rename folder: ") + InFolderPath;
				return FALSE;
			}
			OutNewPath = NewPath;
			return TRUE;
		}

		BOOL32 DeleteFolder(const EString& InFolderPath, EString& OutError)
		{
			OutError = EString();
			if (InFolderPath.Length() == 0u)
			{
				OutError = EString("No folder is selected.");
				return FALSE;
			}
			if (!EFileHelper::DeleteDirectory(InFolderPath))
			{
				OutError = EString("Failed to delete folder: ") + InFolderPath;
				return FALSE;
			}
			return TRUE;
		}
	}
}

#endif
