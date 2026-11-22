#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

#include <BaseAsset.h>

namespace PigeonEngine
{
	class EAssetManager;
	class FAssetFolderNode;
	class FAssetFileInfo;

	/*
	* The Content Browser window. Owns every piece of view state - current folder path,
	* selection, search, filters, dialogs, new asset draft - and talks to the filesystem
	* through AssetFileOperations and to the loaded assets through AssetUsage. The host
	* EAssetManager owns this object and the animation/mesh editors it opens.
	*/
	class FContentBrowserPanel final
	{
	public:
		void Initialize(EAssetManager* InHost);
		void ShutDown();
		// Draws the contents only; the window registry owns Begin/End and the size.
		void DrawContents();
		// Called by the host after a catalog rescan: drops selection entries that no
		// longer exist and falls back to a valid current folder.
		void PruneSelectionAfterCatalogRefresh();

	private:
		void DrawToolbar(const TSharedPtr<FAssetFolderNode>& InCurrent);
		void DrawBreadcrumbs(const TSharedPtr<FAssetFolderNode>& InCurrent);
		void DrawFolderTree();
		void DrawFolderNode(const TSharedPtr<FAssetFolderNode>& InNode, const EString& InRootPath
			, const BOOL32 bInIsRoot, const EString& InRootLabel);
		void DrawSelectedSummary();
		void DrawAssetGrid(const TSharedPtr<FAssetFolderNode>& InCurrent);
		void DrawFolderTile(const TSharedPtr<FAssetFolderNode>& InFolder, const FLOAT InTileWidth, const FLOAT InTileHeight);
		void DrawAssetTile(const TSharedPtr<FAssetFileInfo>& InFile
			, const TArray<TSharedPtr<FAssetFileInfo>>& InVisibleFiles, const INT32 InFileIndex
			, const FLOAT InTileWidth, const FLOAT InTileHeight);
		void DrawDialogs();

		void ProcessPendingOpen();
		void OpenAsset(const EString& InAssetPath, const EAssetType& InType);
		void SetCurrentFolder(const EString& InFolderPath);
		void EnsureCurrentFolder();
		void RebuildVisibleFiles(const TSharedPtr<FAssetFolderNode>& InCurrent
			, TArray<TSharedPtr<FAssetFileInfo>>& OutFiles) const;

		// Selection.
		void SelectOnly(const EString& InPath);
		void ToggleSelection(const EString& InPath);
		void SelectRange(const TArray<TSharedPtr<FAssetFileInfo>>& InVisibleFiles, const INT32 InFileIndex);
		void ClearSelection();
		void PruneSelection();
		PE_NODISCARD BOOL32 IsSelected(const EString& InPath) const;
		PE_NODISCARD TSharedPtr<FAssetFileInfo> FindPrimaryFile(const TSharedPtr<FAssetFolderNode>& InCurrent) const;
		PE_NODISCARD EAssetType FindTypeForPath(const EString& InPath) const;
		void RefreshUsersCache();

		// Policy shared by rename/delete/move/folder delete.
		PE_NODISCARD BOOL32 CanModifyAsset(const EString& InAssetPath, const EAssetType& InType
			, EString& OutReason, TArray<EString>* OutUsers = nullptr) const;
		PE_NODISCARD BOOL32 FolderHasBlockedAssets(const TSharedPtr<FAssetFolderNode>& InFolder
			, TArray<EString>& OutReasons) const;

		// Asset operations.
		void RequestRenameAsset(const EString& InPath, const EAssetType& InType);
		void RequestDeleteSelection();
		void PerformRenameAsset();
		void PerformDeleteAssets();
		void MoveDragSelectionToFolder(const EString& InDestinationFolder);
		void DrawRenameAssetDialog();
		void DrawDeleteAssetDialog();
		void DrawErrorPopup();
		void ShowError(const EString& InMessage);
		void SetStatus(const EString& InMessage, const BOOL32 bInIsError);

		// Folder operations.
		void RequestRenameFolder(const EString& InFolderPath);
		void RequestDeleteFolder(const EString& InFolderPath);
		void PerformRenameFolder();
		void PerformDeleteFolder();
		void DrawFolderDialogs();

		// New montage/state machine document.
		void OpenNewAssetDialog(const EAssetType& InType);
		void DrawNewAssetDialog();
		void RefreshNewAssetClipsAndSlots();
		void RefreshNewAssetSlots();
		PE_NODISCARD BOOL32 CreateNewAsset(EString& OutError);

	private:
		EAssetManager*	Host = nullptr;

		EString			CurrentFolderPath;
		TArray<EString>	SelectedPaths;
		EString			PrimaryPath;
		CHAR			SearchBuffer[256] = { 0 };
		// 0 is "All Types"; any other value is the EAssetType value.
		INT32			TypeFilterIndex = 0;
		FLOAT			TileSize = 96.0f;
		INT32			LastClickedFileIndex = -1;

		EString			UsersCachePath;
		TArray<EString>	UsersCache;

		EString			StatusMessage;
		BOOL32			bStatusIsError = FALSE;

		// Deferred open: an asset editor is opened before the next frame's draw, never
		// while the grid is iterating the catalog.
		BOOL32			bHasPendingOpen = FALSE;
		EString			PendingOpenPath;
		EAssetType		PendingOpenType = ASSET_TYPE_UNKNOWN;

		// Asset rename/delete dialogs.
		BOOL32			bRenameAssetOpen = FALSE;
		EString			RenameTargetPath;
		EAssetType		RenameTargetType = ASSET_TYPE_UNKNOWN;
		CHAR			RenameBuffer[128] = { 0 };
		BOOL32			bDeleteAssetOpen = FALSE;
		TArray<EString>	DeleteTargets;
		TArray<EString>	DeleteTargetReasons;
		BOOL32			bDeleteBlocked = FALSE;
		EString			DialogError;

		// Folder dialogs.
		BOOL32			bNewFolderOpen = FALSE;
		EString			FolderTargetPath;
		CHAR			FolderNameBuffer[128] = { 0 };
		BOOL32			bRenameFolderOpen = FALSE;
		BOOL32			bDeleteFolderOpen = FALSE;
		TArray<EString>	FolderDeleteReasons;
		BOOL32			bFolderDeleteBlocked = FALSE;

		BOOL32			bErrorPopupOpen = FALSE;
		EString			ErrorPopupMessage;

		// Drag source snapshot: a drop target can arrive after the selection changed.
		TArray<EString>	DragPaths;

		// New Asset window state.
		BOOL32			bNewAssetWindowOpen = FALSE;
		BOOL32			bResetNewAssetWindowPos = FALSE;
		BOOL32			bFocusNewAssetWindow = FALSE;
		EAssetType		NewAssetType = ASSET_TYPE_UNKNOWN;
		CHAR			NewAssetName[128] = { 0 };
		TArray<EString>	NewAssetAnimationNames;
		INT32			NewAssetAnimationIndex = -1;
		TArray<EString>	NewAssetClipNames;
		INT32			NewAssetClipIndex = -1;
		TArray<EString>	NewAssetSkeletonNames;
		INT32			NewAssetSkeletonIndex = -1;
		TArray<EString>	NewAssetSlotNames;
		INT32			NewAssetSlotIndex = -1;
		EString			NewAssetSkeletonName;
		EString			NewAssetError;
	};
}

#endif
