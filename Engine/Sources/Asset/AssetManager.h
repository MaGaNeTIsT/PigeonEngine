#pragma once
#include "Base/DataStructure/ObjectBase.h"
#if _EDITOR_ONLY
#include <imgui.h>
#include "BaseAsset.h"
#include "Base/DataStructure/Container/Array.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"

namespace PigeonEngine
{
	class EMeshEditor;
	class EAnimationWorkspace;


	class EAssetFile
	{
	public:
		EAssetFile() = delete;
		EAssetFile(const EString& InPath);
		~EAssetFile() = default;
	public:
		PE_NODISCARD EString GetDisplayName() const;
		PE_NODISCARD EAssetType GetType() const;
		PE_NODISCARD EString GetPath() const;
	private:
		EString Path;
		EAssetType Type;
	};

	class EFolderTreeNode
	{
	public:
		EFolderTreeNode() = delete;
		EFolderTreeNode(EString InPath);
		EFolderTreeNode(const TSharedPtr<EFolderTreeNode>& InParent, EString InPath);

		static void TraverseFolder(const TSharedPtr<EFolderTreeNode>& Node, TSharedPtr<EFolderTreeNode>& CurrentSelectedFolder);
		~EFolderTreeNode();

	public:
		void SetChildrenFolder(const TArray<EString>& Paths);
		void SetChildrenFiles(const TArray<EString>& Paths);
		PE_NODISCARD const TArray<TSharedPtr<EFolderTreeNode>>& GetChildrenFolder() const;
		PE_NODISCARD const TArray<TSharedPtr<EAssetFile>>& GetChildrenFile() const;
		void ClearChildren();

		PE_NODISCARD const EString& GetPath() const;
		PE_NODISCARD EString GetDisplayName() const;

	private:
		EString Path;
		TSharedPtr<EFolderTreeNode> Parent = nullptr;
		TArray<TSharedPtr<EFolderTreeNode>> ChildrenFolder;
		TArray<TSharedPtr<EAssetFile>> ChildrenFile;
	};





	class EAssetManager : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EAssetManager)
	public:
		void Initialize() override;
		void ShutDown() override;

		void Init();
		void Uninit();
	public:
		void EditorInit();
	public:
		void EditorUpdate();
		// Opens the skeleton's SlotGroup document in the animation workspace. Montage
		// sections resolve their slot against this skeleton-owned order, so the montage
		// editor links here instead of maintaining a second group list.
		void OpenSkeletonSlotGroups(const EString& InAssetFullPath);

	protected:
		void GenerateContentBrowser();
		ImTextureID GetThumbNail(const EAssetType& Type, const TSharedPtr<EAssetFile>& File = nullptr);
		static EString AssetTypeAsString(const EAssetType& Type);
	private:
		void OpenNewAssetDialog(const EAssetType& InType);
		void DrawNewAssetDialog();
		void RefreshNewAssetClipsAndSlots();
		void RefreshNewAssetSlots();
		BOOL32 CreateNewAsset(EString& OutError);
		void RefreshContentTree();
		void RequestOpenAsset(const EString& InAssetPath, const EAssetType& InType);
		void ProcessPendingOpen();
		void CollectAssetUsers(const EString& InAssetPath, const EAssetType& InType
			, TArray<EString>& OutUsers)const;
		BOOL32 RenameSelectedAsset(const EString& InNewName, EString& OutError);
		BOOL32 DeleteSelectedAsset(EString& OutError);
		void DrawAssetManagementPopups();
	private:
		TSharedPtr<EFolderTreeNode> EngineAssetRoot  = nullptr;
		TSharedPtr<EFolderTreeNode> ProjectAssetRoot = nullptr;
		TSharedPtr<EFolderTreeNode> Current          = nullptr;
		TSharedPtr<EAssetFile>      SelectedFile     = nullptr;
		ImTextureID FolderTextureId;
		TMap<EAssetType, ImTextureID> TypeThumbNails;
		TMap<EString, ImTextureID>    TextureThumbNails;
		// Animation authoring is owned by the Workspace. The legacy per-asset editor
		// objects are retired and no longer constructed or drawn.
		EAnimationWorkspace*         AnimationWorkspace = nullptr;
		EMeshEditor*                 MeshEditor = nullptr;

		// New Asset window. Names currently come from ENGINE_ANIMATION_PATH; the
		// destination is the same fixed folder the JSON importers use.
		BOOL8		m_bNewAssetWindowOpen = FALSE;
		BOOL8		m_bResetNewAssetWindowPos = FALSE;
		BOOL8		m_bFocusNewAssetWindow = FALSE;
		EAssetType	m_NewAssetType = EAssetType::ASSET_TYPE_UNKNOWN;
		CHAR		m_NewAssetName[128] = { 0 };
		TArray<EString>	m_NewAssetAnimationNames;
		INT32		m_NewAssetAnimationIndex = -1;
		TArray<EString>	m_NewAssetClipNames;
		INT32		m_NewAssetClipIndex = -1;
		TArray<EString>	m_NewAssetSkeletonNames;
		INT32		m_NewAssetSkeletonIndex = -1;
		TArray<EString>	m_NewAssetSlotNames;
		INT32		m_NewAssetSlotIndex = -1;
		EString		m_NewAssetSkeletonName;
		EString		m_NewAssetError;

		CHAR		m_ContentSearch[128] = { 0 };
		BOOL8		m_bHasPendingOpen = FALSE;
		EString		m_PendingOpenPath;
		EAssetType	m_PendingOpenType = EAssetType::ASSET_TYPE_UNKNOWN;

		CHAR		m_RenameBuffer[128] = { 0 };
		EString		m_RenameTargetPath;
		EAssetType	m_RenameTargetType = EAssetType::ASSET_TYPE_UNKNOWN;
		EString		m_DeleteTargetPath;
		EAssetType	m_DeleteTargetType = EAssetType::ASSET_TYPE_UNKNOWN;
		EString		m_ManageAssetError;
		BOOL8		m_bRenameRequested = FALSE;
		BOOL8		m_bDeleteRequested = FALSE;


	};

}
#endif
