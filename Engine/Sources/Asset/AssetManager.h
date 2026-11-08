#pragma once
#include "Base/DataStructure/ObjectBase.h"
#if ENGINE_EDITOR_ONLY
#include <imgui.h>
#include "BaseAsset.h"
#include "Base/DataStructure/Container/Array.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"

namespace PigeonEngine
{
	class EAnimationEditor;
	class EAnimStateMachineEditor;
	class EAnimMontageEditor;
	class EAnimNotifyEditor;
	class ESkeletonSlotGroupEditor;
	class EMeshEditor;


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
		// Requests a rebuild of the folder tree and file entries on the next editor frame.
		// Importers call this after writing assets; multiple requests in one frame coalesce
		// into one rescan.
		void RequestContentRefresh();

	protected:
		void GenerateContentBrowser();
		ImTextureID GetThumbNail(const EAssetType& Type, const TSharedPtr<EAssetFile>& File = nullptr);
		static EString AssetTypeAsString(const EAssetType& Type);
		// Resolves the editor icons' ImGui handles. EditorInit loads the assets, but their GPU
		// proxies are built by the render thread, so the handles only exist after a frame has
		// drained the creation commands. Called from EditorUpdate until every icon is live.
		void EnsureEditorIconTextureIds();
		// Rebuilds the tree in place, preserving the current folder and selected file by path.
		void RefreshContentBrowser();
	private:
		TSharedPtr<EFolderTreeNode> EngineAssetRoot  = nullptr;
		TSharedPtr<EFolderTreeNode> ProjectAssetRoot = nullptr;
		TSharedPtr<EFolderTreeNode> Current          = nullptr;
		TSharedPtr<EAssetFile>      SelectedFile     = nullptr;
		ImTextureID FolderTextureId = ImTextureID_Invalid;
		BOOL32 bEditorIconTextureIdsReady = FALSE;
		FLOAT ContentIconSize = 50.0f;
		BOOL32 bContentRefreshRequested = FALSE;
		TMap<EAssetType, ImTextureID> TypeThumbNails;
		TMap<EString, ImTextureID>    TextureThumbNails;
		EAnimationEditor*            AnimationEditor = nullptr;
		EAnimStateMachineEditor*     StateMachineEditor = nullptr;
		EAnimMontageEditor*          MontageEditor = nullptr;
		EAnimNotifyEditor*           NotifyEditor = nullptr;
		ESkeletonSlotGroupEditor*    SkeletonSlotEditor = nullptr;
		EMeshEditor*                 MeshEditor = nullptr;


	};

}
#endif
