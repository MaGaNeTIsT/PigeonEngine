#pragma once
#include "Base/DataStructure/ObjectBase.h"
#include "Base/DataStructure/Container/Array.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"

namespace PigeonEngine
{

#if _EDITOR_ONLY
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
		PE_NODISCARD const TArray<TSharedPtr<EFolderTreeNode>>& GetChildrenFile() const;
		void ClearChildren();

		PE_NODISCARD const EString& GetPath() const;
		PE_NODISCARD EString GetDisplayName() const;
	private:
		EString Path;
		TSharedPtr<EFolderTreeNode> Parent = nullptr;
		TArray<TSharedPtr<EFolderTreeNode>> ChildrenFolder;
		TArray<TSharedPtr<EFolderTreeNode>> ChildrenFile;
	};

	
#endif
	
	
	class EAssetManager : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EAssetManager)
	public:
		void Initialize() override;
		void ShutDown() override;

		void Init();
		void Uninit();
#if _EDITOR_ONLY
	public:
		void EditorInit();
	public:
		void EditorUpdate();

	protected:
		void GenerateContentBrowser();

	private:
		TSharedPtr<EFolderTreeNode> EngineAssetRoot  = nullptr;
		TSharedPtr<EFolderTreeNode> ProjectAssetRoot = nullptr;
		TSharedPtr<EFolderTreeNode> Current          = nullptr;
#endif

	};

}

