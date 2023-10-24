#include "AssetManager.h"

#include "../EngineCore/IO/FileHelper.h"

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		RegisterClassType<EAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);


	EFolderTreeNode::EFolderTreeNode(EString InPath)
		:
	Path(std::move(InPath))
	{
	}

	EFolderTreeNode::EFolderTreeNode(const TSharedPtr<EFolderTreeNode>& InParent, EString InPath)
		:
	Path(std::move(InPath)),
	Parent(InParent)
	{
	}

	void EFolderTreeNode::TraverseFolder(const TSharedPtr<EFolderTreeNode>& Node, TSharedPtr<EFolderTreeNode>& CurrentSelectedFolder)
	{
		if(!Node)
		{
			return;
		}
		BOOL8 bSelectedFolder= CurrentSelectedFolder == Node;
		
		const ImGuiTreeNodeFlags TreeNodeFlag = (Node->GetChildrenFolder().Length() > 0 ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf);
		const BOOL8 bMenuOpened = ImGui::TreeNodeEx(*(EString("##") + Node->GetDisplayName() + EString("_FolderTreeNode")), TreeNodeFlag);
		ImGui::SameLine();
		const BOOL8 bSelected	= ImGui::Selectable(*Node->GetDisplayName(), &bSelectedFolder);
		if (bMenuOpened)
		{
			for(const auto& elem : Node->GetChildrenFolder())
			{
				TraverseFolder(elem, CurrentSelectedFolder);
			}
			ImGui::TreePop();
		}
		
		if(bSelected)
		{
			CurrentSelectedFolder = Node;
		}
	}

	EFolderTreeNode::~EFolderTreeNode()
	{
		ClearChildren();
	}

	void EFolderTreeNode::SetChildrenFolder(const TArray<EString>& Paths)
	{
		ChildrenFolder.Clear();
		for(const auto& elem : Paths)
		{
			ChildrenFolder.Add(TSharedPtr<EFolderTreeNode>::MakeShared(this, elem));
		}
	}
	
	void EFolderTreeNode::SetChildrenFiles(const TArray<EString>& Paths)
	{
		ChildrenFile.Clear();
		for(const auto& elem : Paths)
		{
			ChildrenFile.Add(TSharedPtr<EFolderTreeNode>::MakeShared(this, elem));
		}
	}
	
	const TArray<TSharedPtr<EFolderTreeNode>>& EFolderTreeNode::GetChildrenFolder() const
	{
		return this->ChildrenFolder;
	}

	const TArray<TSharedPtr<EFolderTreeNode>>& EFolderTreeNode::GetChildrenFile() const
	{
		return this->ChildrenFile;
	}

	void EFolderTreeNode::ClearChildren()
	{
		ChildrenFolder.Clear();
		ChildrenFile.Clear();
		
	}

	const EString& EFolderTreeNode::GetPath() const
	{
		return this->Path;
	}

	EString EFolderTreeNode::GetDisplayName() const
	{
		auto index = Path.RightFind("/");
		EString target = Path;
		if(index == Path.LastIndex())
		{
			target = Path.Substring(0, Path.Length() - 1);
			index = target.RightFind("/");
		}
		EString Name =  target.Substring(index + 1, Path.Length() - index - 1);
		return Name;
	}


	EAssetManager::EAssetManager()
	{

	}

	EAssetManager::~EAssetManager()
	{

	}
	void EAssetManager::Initialize()
	{

	}

	void EAssetManager::ShutDown()
	{

	}

	void EAssetManager::Init()
	{
		
	}

	void EAssetManager::Uninit()
	{

	}

	BOOL8 ScanFolder(const TSharedPtr<EFolderTreeNode>& Node)
	{
		TArray<EString> ChildFolders;
		TArray<EString> ChildFiles;
		if(!EFileHelper::ScanDirectory(Node->GetPath(), ChildFolders, ChildFiles, TRUE))
		{
			return FALSE;
		}
		Node->SetChildrenFolder(ChildFolders);
		Node->SetChildrenFiles(ChildFiles);
		for(const auto&  elem : Node->GetChildrenFolder())
		{
			ScanFolder(elem);
		}
		return TRUE;
	}
	
	void EAssetManager::EditorInit()
	{
		EngineAssetRoot  = TSharedPtr<EFolderTreeNode>::MakeShared(nullptr, ESettings::ENGINE_CONTENT_PATH);
		ProjectAssetRoot = TSharedPtr<EFolderTreeNode>::MakeShared(nullptr, ESettings::PROJECT_CONTENT_PATH);
		if(!ScanFolder(EngineAssetRoot))
		{
			EngineAssetRoot = nullptr;
		}
		if(!ScanFolder(ProjectAssetRoot))
		{
			ProjectAssetRoot = nullptr;
		}
	}

	void EAssetManager::EditorUpdate()
	{
		GenerateContentBrowser();
	}

	void EAssetManager::GenerateContentBrowser()
	{
		ImGui::Begin("PigeonContent", FALSE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
		{
			ImGui::Text("PigeonContent");
			{
				ImGui::BeginChild("Tree", ImVec2(200, 0), true, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
				ImGui::Text("FolderTree");
				{
					ImGui::BeginChild("Project", ImVec2(180, 150), true, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
					ImGui::Text("Project");
					EFolderTreeNode::TraverseFolder(ProjectAssetRoot, Current);
					ImGui::EndChild();
				}
				{
					ImGui::BeginChild("Engine", ImVec2(180, 0), true, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
					ImGui::Text("Engine");
					EFolderTreeNode::TraverseFolder(EngineAssetRoot, Current);
					ImGui::EndChild();
				}
		
				ImGui::EndChild();
			}
			
			ImGui::SameLine();
			{
				ImGui::BeginChild("Content", ImVec2(700, 0), true);
				
				// if(ImGui::CollapsingHeader("Detail", ImGuiTreeNodeFlags_DefaultOpen))
				// {
				// 	
				// }
				ImGui::EndChild();
			}
		}
		
		ImGui::End();
	}

}
