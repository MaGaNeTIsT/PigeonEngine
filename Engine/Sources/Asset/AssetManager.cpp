#include "AssetManager.h"

#include "../EngineCore/IO/FileHelper.h"
#include <TextureAsset/TextureAsset.h>

#include "../../EngineThirdParty/imGUI/Headers/imGUIManager.h"
#include "../EngineCore/Editor/EditorLogManager.h"

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
		
		Name = Name.Replace(".PAsset", "");
		return Name;
	}

	EAssetType EAssetFile::GetType() const
	{
		return this->Type;
	}

	EString EAssetFile::GetPath() const
	{
		return this->Path;
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

		// folder icon
		{
			EString ImportPath("./Engine/Assets/EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_Folder");
			EString ImportFileType("PNG");
		
			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(ESettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_Folder",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				FolderTextureId = EngineTextureToImgui(Asset);
			}	
		}

		// mesh
		{
			EString ImportPath("./Engine/Assets/EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_Mesh");
			EString ImportFileType("PNG");
		
			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(ESettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_Mesh",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				ImTextureID TextureId = EngineTextureToImgui(Asset);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_MESH, TextureId);
			}	
		}

		// shader
		{
			EString ImportPath("./Engine/Assets/EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_Shader");
			EString ImportFileType("PNG");
		
			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(ESettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_Shader",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				ImTextureID TextureId = EngineTextureToImgui(Asset);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_SHADER, TextureId);
			}	
		}

		// shader
		{
			EString ImportPath("./Engine/Assets/EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_Skeleton");
			EString ImportFileType("PNG");
		
			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(ESettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_Skeleton",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				ImTextureID TextureId = EngineTextureToImgui(Asset);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_SKELETON, TextureId);
			}	
		}

		// animation
		{
			EString ImportPath("./Engine/Assets/EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_Animation");
			EString ImportFileType("PNG");
		
			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(ESettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_Animation",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				ImTextureID TextureId = EngineTextureToImgui(Asset);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_ANIMATION, TextureId);
			}	
		}

		// animation
		{
			EString ImportPath("./Engine/Assets/EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_No");
			EString ImportFileType("PNG");
		
			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(ESettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_No",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				ImTextureID TextureId = EngineTextureToImgui(Asset);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_UNKNOWN, TextureId);
			}	
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
					ImGui::BeginChild("Project", ImVec2(180, 120), true, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
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
					UINT32 i = 0;
					for(; i < ChildrenFolder.Length(); ++i)
					{
						ImGui::PushID(i);
						ImGui::BeginChild("asdas", ImVec2(120, 150), false);
						if(ImGui::ImageButton(FolderTextureId, button_sz))
						{
							Current = ChildrenFolder[i];
						}
						ImGui::TextWrapped(*ChildrenFolder[i]->GetDisplayName());
						ImGui::EndChild();
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
						ImGui::BeginChild("asdas", ImVec2(120, 150), false);
						const BOOL8 bSelected = ImGui::Selectable(*(EString("##") + elem->GetDisplayName()), elem == SelectedFile, 0, ImVec2(120, 150));
						const ImTextureID id = GetThumbNail(elem->GetType(), elem);
						ImGui::SetItemAllowOverlap();
						ImGui::SetCursorPos(ImVec2(0,0));
						ImGui::Image(id, button_sz);
						ImGui::SetCursorPos(ImVec2(0, button_sz.y + 10));
						ImGui::TextWrapped(*elem->GetDisplayName());
						ImGui::EndChild();
						const FLOAT last_button_x2 = ImGui::GetItemRectMax().x;
						const ImGuiStyle& style = ImGui::GetStyle();
						const FLOAT next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x;
						if(bSelected)
						{
							SelectedFile = elem;
							PE_LOG_LOG(EString("ContentBrowser:Select ") + elem->GetDisplayName());
						}
						if ( next_button_x2 < window_visible_x2)
							ImGui::SameLine();
						++i;
						ImGui::PopID();
					}
					
				}
				
				ImGui::EndChild();
			}
		}
		
		ImGui::End();
	}

	ImTextureID EAssetManager::GetThumbNail(const EAssetType& Type, const TSharedPtr<EAssetFile>& File)
	{
		if(Type != EAssetType::ASSET_TYPE_TEXTURE)
		{
			return TypeThumbNails[Type];
		}
		
		if(!File)
		{
			return TypeThumbNails[EAssetType::ASSET_TYPE_UNKNOWN];
		}

		// const EString Path = File->GetPath();
		// if(TextureThumbNails.ContainsKey(Path))
		// {
		// 	return TextureThumbNails[Path];
		// }
		// EString ImportPath = Path.Replace(File->GetDisplayName(), "");
		// ImportPath = ImportPath.Replace(".PAsset", "");
		// EString ImportName = File->GetDisplayName();
		// EString ImportFileType("PNG");
		//
		// const ETexture2DAsset* Asset = nullptr;
		// TryLoadTexture2D(ESettings::ENGINE_TEXTURE_PATH, File->GetDisplayName(),Asset, &ImportPath, &ImportName, &ImportFileType);
		// if(Asset)
		// {
		// 	ImTextureID TextureId = EngineTextureToImgui(Asset);
		// 	TextureThumbNails.Add(Path, TextureId);
		// 	return TextureId;
		// }

		return TypeThumbNails[EAssetType::ASSET_TYPE_UNKNOWN];
	}

	EString EAssetManager::AssetTypeAsString(const EAssetType& Type)
	{
		EString Name;
		switch (Type)
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
		return Name;
	}
}
