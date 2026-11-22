#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

#include <BaseAsset.h>

namespace PigeonEngine
{
	/*
	* One .PAsset on disk. Identity is the path; the type is read from the four-byte
	* header every PAsset starts with. The lowercase display name is what search and the
	* sort order use.
	*/
	class FAssetFileInfo final
	{
	public:
		FAssetFileInfo() = delete;
		explicit FAssetFileInfo(const EString& InPath);

		PE_NODISCARD const EString& GetPath() const;
		PE_NODISCARD const EString& GetDisplayName() const;
		PE_NODISCARD const EString& GetSearchName() const;
		PE_NODISCARD EAssetType GetType() const;

	private:
		EString		Path;
		EString		DisplayName;
		EString		SearchName;
		EAssetType	Type = ASSET_TYPE_UNKNOWN;
	};

	/*
	* A folder and its direct children. The node is data only: the content browser panel
	* draws the tree, so the model carries no ImGui dependency.
	*/
	class FAssetFolderNode final
	{
	public:
		FAssetFolderNode() = delete;
		explicit FAssetFolderNode(const EString& InPath);

		void SetChildren(const TArray<EString>& InFolderPaths, const TArray<EString>& InFilePaths);

		PE_NODISCARD const TArray<TSharedPtr<FAssetFolderNode>>& GetChildrenFolder() const;
		PE_NODISCARD const TArray<TSharedPtr<FAssetFileInfo>>& GetChildrenFile() const;
		PE_NODISCARD const EString& GetPath() const;
		PE_NODISCARD const EString& GetSearchName() const;
		PE_NODISCARD EString GetDisplayName() const;

	private:
		EString		Path;
		EString		SearchName;
		TArray<TSharedPtr<FAssetFolderNode>>	ChildrenFolder;
		TArray<TSharedPtr<FAssetFileInfo>>		ChildrenFile;
	};

	struct FAssetRoot
	{
		EString							Label;
		TSharedPtr<FAssetFolderNode>	Node;
	};

	/*
	* The content browser's filesystem model: the configured roots, their scanned trees,
	* and the coalesced refresh request. No ImGui, no selection, no window state - those
	* belong to FContentBrowserPanel.
	*/
	class FAssetCatalog final
	{
	public:
		void Initialize();
		void ShutDown();

		// Coalesces several writes in one frame into a single rescan at the top of the
		// next editor frame.
		void RequestRefresh();
		PE_NODISCARD BOOL32 ConsumeRefreshRequest();
		void Refresh();

		PE_NODISCARD const TArray<FAssetRoot>& GetRoots() const;
		PE_NODISCARD TSharedPtr<FAssetFolderNode> FindFolderByPath(const EString& InPath) const;
		PE_NODISCARD TSharedPtr<FAssetFolderNode> GetDefaultFolder() const;

	private:
		void ScanFolder(const TSharedPtr<FAssetFolderNode>& InNode);

	private:
		TArray<FAssetRoot>	Roots;
		BOOL32				bRefreshRequested = FALSE;
	};
}

#endif
