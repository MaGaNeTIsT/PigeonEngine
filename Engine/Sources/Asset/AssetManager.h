#pragma once
#include "BaseAsset.h"
#include "Base/DataStructure/ObjectBase.h"
#include "Base/DataStructure/Container/Array.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"

namespace PigeonEngine
{

#if _EDITOR_ONLY
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
		ImTextureID GetThumbNail(const EAssetType& Type, const TSharedPtr<EAssetFile>& File = nullptr) ;
	private:
		TSharedPtr<EFolderTreeNode> EngineAssetRoot  = nullptr;
		TSharedPtr<EFolderTreeNode> ProjectAssetRoot = nullptr;
		TSharedPtr<EFolderTreeNode> Current          = nullptr;

		ImTextureID FolderTextureId;
		TMap<EAssetType, ImTextureID> TypeThumbNails;
		TMap<EString, ImTextureID>    TextureThumbNails;
#endif

	};

}

