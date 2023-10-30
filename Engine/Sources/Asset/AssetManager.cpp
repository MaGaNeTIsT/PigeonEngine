#include "AssetManager.h"

#include "../EngineCore/IO/FileHelper.h"

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		RegisterClassType<EAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);


	EAssetFile::EAssetFile(const EString& InPath)
	{
		Path = InPath.Replace("\\", "/");
		void* Type = nullptr;
		if(EFileHelper::ReadFirstNumberOfBytesInFile(Path, Type, 4))
		{
			this->Type = *static_cast<EAssetType*>(Type);
		}
		
	}

	EString EAssetFile::GetDisplayName() const
	{
		auto index = Path.RightFind("/");
		EString target = Path;
		if(index == Path.LastIndex())
		{
			target = Path.Substring(0, Path.Length() - 1);
			index = target.RightFind("/");
		}
		EString Name =  target.Substring(index + 1, Path.Length() - index - 1);
		switch (this->Type)
		{
		case ASSET_TYPE_UNKNOWN	:
			{
				Name += " UnKnown";
				break;
			}
		case ASSET_TYPE_TEXTURE	:
			{
				Name += " Texture";
				break;
			}
		case ASSET_TYPE_MESH	:
			{
				Name += " Mesh";
				break;
			}
		case ASSET_TYPE_SKELETON	:
			{
				Name += " Skeleton";
				break;
			}
		case ASSET_TYPE_ANIMATION	:
			{
				Name += " Animation";
				break;
			}
		case ASSET_TYPE_SHADER	:
			{
				Name += " Shader";
				break;
			}
		case ASSET_TYPE_COUNT	:
			{
				Name += " Count";
				break;
			}

		}
		Name = Name.Replace(".PAsset", "");
		return Name;
	}

	EFolderTreeNode::EFolderTreeNode(EString InPath)
	{
		Path = InPath.Replace("\\", "/");
	}

	EFolderTreeNode::EFolderTreeNode(const TSharedPtr<EFolderTreeNode>& InParent, EString InPath)
		:
	Parent(InParent)
	{
		Path = InPath.Replace("\\", "/");
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
			if(elem.Contains(".PAsset"))
			{
				ChildrenFile.Add(TSharedPtr<EAssetFile>::MakeShared(elem));
			}
		}
	}
	
	const TArray<TSharedPtr<EFolderTreeNode>>& EFolderTreeNode::GetChildrenFolder() const
	{
		return this->ChildrenFolder;
	}

	const TArray<TSharedPtr<EAssetFile>>& EFolderTreeNode::GetChildrenFile() const
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
				if(Current.Get())
				{
					ImVec2 button_sz(100, 100);
					auto ChildrenFolder = Current->GetChildrenFolder();
					FLOAT window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
					UINT i = 0;
					for(; i < ChildrenFolder.Length(); ++i)
					{
						ImGui::PushID(i);
						if(ImGui::Button(*ChildrenFolder[i]->GetDisplayName(), button_sz))
						{
							Current = ChildrenFolder[i];
							// return;
						}
						const FLOAT last_button_x2 = ImGui::GetItemRectMax().x;
						const ImGuiStyle& style = ImGui::GetStyle();
						const FLOAT next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x; 
						if ( next_button_x2 < window_visible_x2)
							ImGui::SameLine();
						ImGui::PopID();
						
					}

					for(auto& elem : Current->GetChildrenFile())
					{
						ImGui::PushID(i);
						if(ImGui::Button(*elem->GetDisplayName(), button_sz))
						{
							// Current = ChildrenFolder[i];
							// return;
						}
						const FLOAT last_button_x2 = ImGui::GetItemRectMax().x;
						const ImGuiStyle& style = ImGui::GetStyle();
						const FLOAT next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x;
						if ( next_button_x2 < window_visible_x2)
							ImGui::SameLine();
						ImGui::PopID();
					}
					
				}
				
				ImGui::EndChild();
			}
		}
		
		ImGui::End();
	}

}
