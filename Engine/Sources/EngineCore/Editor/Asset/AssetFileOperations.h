#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	/*
	* The filesystem half of content browser asset operations: name validation and the
	* actual file/folder create-rename-move-delete calls. No ImGui and no asset caches, so
	* the rules can be covered by a sanity check; the panel owns the policy questions
	* (is the asset open? is it referenced?).
	*/
	namespace EAssetFileOperations
	{
		PE_NODISCARD BOOL32 ValidateName(const EString& InName, EString& OutError);
		// "Foo" or "Foo.PAsset" both become "Foo"; the browser always appends the asset
		// extension itself, so typing it must not produce Foo.PAsset.PAsset.
		PE_NODISCARD EString StripAssetExtension(const EString& InName);

		PE_NODISCARD BOOL32 RenameAssetFile(const EString& InAssetPath, const EString& InNewName
			, EString& OutNewPath, EString& OutError);
		PE_NODISCARD BOOL32 MoveAssetFile(const EString& InAssetPath, const EString& InDestinationFolder
			, EString& OutNewPath, EString& OutError);
		PE_NODISCARD BOOL32 DeleteAssetFile(const EString& InAssetPath, EString& OutError);

		PE_NODISCARD BOOL32 CreateFolder(const EString& InParentFolder, const EString& InFolderName
			, EString& OutNewPath, EString& OutError);
		PE_NODISCARD BOOL32 RenameFolder(const EString& InFolderPath, const EString& InNewName
			, EString& OutNewPath, EString& OutError);
		PE_NODISCARD BOOL32 DeleteFolder(const EString& InFolderPath, EString& OutError);
	}
}

#endif
