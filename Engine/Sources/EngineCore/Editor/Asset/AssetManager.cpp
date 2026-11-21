#include <Editor/Asset/AssetManager.h>
#if ENGINE_EDITOR_ONLY
#include <IO/FileHelper.h>
#include <Editor/AnimationEditorCore/AnimationWorkspace.h>
#include <AnimationAsset/AnimationAsset.h>
#include <AnimationMontageAsset/AnimationMontageAsset.h>
#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>
#include <RapidJson/Headers/JsonObject.h>
#include <Editor/MeshEditor/MeshEditor.h>
#include <TextureAsset/TextureAsset.h>

#include <imGUI/Headers/imGUIManager.h>
#include <Development/EditorLogManager.h>
#include <Editor/AssetImporter/ImportManager.h>

namespace PigeonEngine
{

	namespace
	{
		EString NormalizeFolderPath(const EString& InPath)
		{
			EString Result = InPath.Replace("\\", "/");
			while ((Result.Length() > 1u) && (Result[Result.LastIndex()] == '/'))
			{
				Result = Result.Left(Result.Length() - 1u);
			}
			return Result;
		}

		TSharedPtr<EFolderTreeNode> FindFolderByPath(const TSharedPtr<EFolderTreeNode>& InNode
			, const EString& InPath)
		{
			if (!InNode)
			{
				return nullptr;
			}
			if (NormalizeFolderPath(InNode->GetPath()) == NormalizeFolderPath(InPath))
			{
				return InNode;
			}
			for (const auto& Child : InNode->GetChildrenFolder())
			{
				if (TSharedPtr<EFolderTreeNode> Found = FindFolderByPath(Child, InPath))
				{
					return Found;
				}
			}
			return nullptr;
		}

		BOOL8 IsValidNewAssetName(const EString& InName)
		{
			if (InName.Length() <= 0u)
			{
				return FALSE;
			}
			const CHAR InvalidChars[] = "\\/:*?\"<>|";
			for (UINT32 CharIndex = 0u; CharIndex < InName.Length(); CharIndex++)
			{
				for (UINT32 InvalidIndex = 0u; InvalidChars[InvalidIndex] != 0; InvalidIndex++)
				{
					if (InName[CharIndex] == InvalidChars[InvalidIndex])
					{
						return FALSE;
					}
				}
			}
			return TRUE;
		}

		BOOL8 IsManagedAssetType(const EAssetType& InType)
		{
			switch (InType)
			{
			case EAssetType::ASSET_TYPE_ANIMATION:
			case EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE:
			case EAssetType::ASSET_TYPE_ANIM_MONTAGE:
			case EAssetType::ASSET_TYPE_ANIM_NOTIFY:
			case EAssetType::ASSET_TYPE_SKELETON:
			case EAssetType::ASSET_TYPE_NAV_MESH:
				return TRUE;
			default:
				return FALSE;
			}
		}

		EString BuildNewMontageJson(const EString& InName, const EString& InSkeleton
			, const EString& InAnimation, const EString& InClip, const EString& InSlot)
		{
			CJsonObject Root;
			Root.SetStringField(PE_TEXT("Name"), InName);
			Root.SetStringField(PE_TEXT("Skeleton"), InSkeleton);
			Root.SetStringField(PE_TEXT("Animation"), InAnimation);

			CJsonObject* Section = new CJsonObject();
			Section->SetStringField(PE_TEXT("Name"), EString("Default"));
			Section->SetStringField(PE_TEXT("Clip"), InClip);
			Section->SetStringField(PE_TEXT("Slot"), InSlot);
			Section->SetStringField(PE_TEXT("Loop"), EString("Once"));
			Section->SetFloatField(PE_TEXT("Rate"), 1.0f);
			Section->SetFloatField(PE_TEXT("BlendIn"), 0.1f);
			Section->SetFloatField(PE_TEXT("BlendOut"), 0.2f);
			TArray<CJsonObject*> Sections;
			Sections.Add(Section);
			Root.SetObjectArrayField(PE_TEXT("Sections"), Sections);
			delete Section;
			return Root.ToString();
		}

		EString BuildNewStateMachineJson(const EString& InName, const EString& InSkeleton
			, const EString& InAnimation, const EString& InClip)
		{
			CJsonObject Root;
			Root.SetStringField(PE_TEXT("Name"), InName);
			Root.SetStringField(PE_TEXT("Skeleton"), InSkeleton);
			Root.SetStringField(PE_TEXT("Animation"), InAnimation);
			Root.SetStringField(PE_TEXT("Entry"), EString("Idle"));

			CJsonObject* State = new CJsonObject();
			State->SetStringField(PE_TEXT("Name"), EString("Idle"));
			State->SetStringField(PE_TEXT("Clip"), InClip);
			State->SetStringField(PE_TEXT("Loop"), EString("Repeat"));
			State->SetFloatField(PE_TEXT("Rate"), 1.0f);
			State->SetFloatField(PE_TEXT("Blend"), 0.2f);
			TArray<CJsonObject*> States;
			States.Add(State);
			Root.SetObjectArrayField(PE_TEXT("States"), States);
			delete State;

			// Keep the arrays present so the new document's Add Parameter/Transition actions
			// can edit them without a separate "create array" step.
			TArray<CJsonObject*> EmptyParameters;
			TArray<CJsonObject*> EmptyTransitions;
			Root.SetObjectArrayField(PE_TEXT("Parameters"), EmptyParameters);
			Root.SetObjectArrayField(PE_TEXT("Transitions"), EmptyTransitions);
			return Root.ToString();
		}
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<EAssetManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);


	EAssetFile::EAssetFile(const EString& InPath)
	{
		Path = InPath.Replace("\\", "/");
		void* TempType = nullptr;
		if(EFileHelper::ReadFirstNumberOfBytesInFile(Path, TempType, 4))
		{
			this->Type = *static_cast<EAssetType*>(TempType);
			delete[] static_cast<CHAR*>(TempType);
			TempType = nullptr;
		}

	}

	EString EAssetFile::GetDisplayName() const
	{
		auto index = Path.RightFind("/");
		EString target = Path;
		if(index == (INT32)Path.LastIndex())
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

		const ImGuiTreeNodeFlags TreeNodeFlag = (Node->GetChildrenFolder().Num() > 0 ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf);
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
		ChildrenFolder.Empty();
		for(const auto& elem : Paths)
		{
			ChildrenFolder.Add(EMemory::MakeShared<EFolderTreeNode>(elem));
		}
	}

	void EFolderTreeNode::SetChildrenFiles(const TArray<EString>& Paths)
	{
		ChildrenFile.Empty();
		for(const auto& elem : Paths)
		{
			if(elem.Contains(".PAsset"))
			{
				ChildrenFile.Add(EMemory::MakeShared<EAssetFile>(elem));
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
		ChildrenFolder.Empty();
		ChildrenFile.Empty();

	}

	const EString& EFolderTreeNode::GetPath() const
	{
		return this->Path;
	}

	EString EFolderTreeNode::GetDisplayName() const
	{
		auto index = Path.RightFind("/");
		EString target = Path;
		if(index == (INT32)Path.LastIndex())
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
		if (AnimationWorkspace)
		{
			AnimationWorkspace->ShutDown();
			delete AnimationWorkspace;
			AnimationWorkspace = nullptr;
		}
		if (MeshEditor)
		{
			delete MeshEditor;
			MeshEditor = nullptr;
		}

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
		if (!AnimationWorkspace)
		{
			AnimationWorkspace = new EAnimationWorkspace();
			AnimationWorkspace->EditorInit();
		}
		if (!MeshEditor)
		{
			MeshEditor = new EMeshEditor();
			MeshEditor->EditorInit();
		}

		EngineAssetRoot  = EMemory::MakeShared<EFolderTreeNode>(nullptr, EBaseSettings::ENGINE_CONTENT_PATH);
		ProjectAssetRoot = EMemory::MakeShared<EFolderTreeNode>(nullptr, EEngineSettings::PROJECT_CONTENT_PATH);
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
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_Folder");
			EString ImportFileType("PNG");

			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_Folder",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				FolderTextureId = EngineTextureToImgui(Asset);
			}	
		}

		// mesh
		{
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_Mesh");
			EString ImportFileType("PNG");

			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_Mesh",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				ImTextureID TextureId = EngineTextureToImgui(Asset);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_MESH, TextureId);
			}	
		}

		// navmesh: reuses the mesh icon. A navigation mesh is geometry, and no dedicated icon
		// exists; adding one is a content change, not a code change.
		if (ImTextureID* MeshThumbnail = TypeThumbNails.FindValueAsPtr(EAssetType::ASSET_TYPE_MESH))
		{
			TypeThumbNails.Add(EAssetType::ASSET_TYPE_NAV_MESH, *MeshThumbnail);
		}

		// shader
		{
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_Shader");
			EString ImportFileType("PNG");

			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_Shader",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				ImTextureID TextureId = EngineTextureToImgui(Asset);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_SHADER, TextureId);
			}	
		}

		// shader
		{
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_Skeleton");
			EString ImportFileType("PNG");

			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_Skeleton",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				ImTextureID TextureId = EngineTextureToImgui(Asset);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_SKELETON, TextureId);
			}	
		}

		// animation
		{
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_Animation");
			EString ImportFileType("PNG");

			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_Animation",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				ImTextureID TextureId = EngineTextureToImgui(Asset);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_ANIMATION, TextureId);
				// Montage/StateMachine/Notify share the animation icon until dedicated
				// editor icons exist; without this the new asset grids render a blank image.
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_ANIM_MONTAGE, TextureId);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE, TextureId);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_ANIM_NOTIFY, TextureId);
			}	
		}

		// animation
		{
			EString AssetBasePath(EBaseSettings::ENGINE_ASSET_DIRECTORY);
			EString ImportPath(AssetBasePath + "EngineTextures/Editor/");
			EString ImportName("Icon_ContentBrowser_No");
			EString ImportFileType("PNG");

			const ETexture2DAsset* Asset = nullptr;
			TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, "Icon_ContentBrowser_No",Asset, &ImportPath, &ImportName, &ImportFileType);
			if(Asset)
			{
				ImTextureID TextureId = EngineTextureToImgui(Asset);
				TypeThumbNails.Add(EAssetType::ASSET_TYPE_UNKNOWN, TextureId);
			}	
		}
	}

	void EAssetManager::EditorUpdate()
	{
		// A refresh request may come from an importer (through EImportManager) or from a family
		// manager that wrote an asset. Both are consumed here, before anything draws, so the tree is
		// never rebuilt while GenerateContentBrowser iterates it.
		const BOOL32 bImportRefreshRequested = EImportManager::GetManagerSingleton()->ConsumeContentBrowserRefreshRequest();
		if ((m_bContentRefreshRequested != FALSE) || (bImportRefreshRequested != FALSE))
		{
			m_bContentRefreshRequested = FALSE;
			RefreshContentTree();
		}

		ProcessPendingOpen();
		GenerateContentBrowser();
		if (AnimationWorkspace)
		{
			AnimationWorkspace->EditorUpdate();
		}
		if (MeshEditor)
		{
			MeshEditor->EditorUpdate();
		}
	}

	void EAssetManager::OpenSkeletonSlotGroups(const EString& InAssetFullPath)
	{
		if ((AnimationWorkspace) && (InAssetFullPath.Length() > 0u))
		{
			AnimationWorkspace->OpenSlotGroupAsset(InAssetFullPath);
		}
	}

	void EAssetManager::GenerateContentBrowser()
	{
		ImGui::Begin("PigeonContent", FALSE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
		{
			ImGui::Text("PigeonContent");
			{
				ImGui::BeginChild("Tree", ImVec2(200, 0), TRUE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
				ImGui::Text("FolderTree");
				{
					EImportManager::GetManagerSingleton()->ShowImporterButton();
					ImGui::SameLine();
					if (ImGui::Button("New"))
					{
						ImGui::OpenPopup("##new_asset_type_menu");
					}
					if (ImGui::BeginPopup("##new_asset_type_menu"))
					{
						if (ImGui::MenuItem("Montage"))
						{
							OpenNewAssetDialog(EAssetType::ASSET_TYPE_ANIM_MONTAGE);
							m_bNewAssetWindowOpen = TRUE;
							ImGui::CloseCurrentPopup();
						}
						if (ImGui::MenuItem("State Machine"))
						{
							OpenNewAssetDialog(EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE);
							m_bNewAssetWindowOpen = TRUE;
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndPopup();
					}
					ImGui::BeginChild("Project", ImVec2(180, 120), TRUE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
					ImGui::Text("Project");
					EFolderTreeNode::TraverseFolder(ProjectAssetRoot, Current);
					ImGui::EndChild();
				}
				{
					ImGui::BeginChild("Engine", ImVec2(180, 0), TRUE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
					ImGui::Text("Engine");
					EFolderTreeNode::TraverseFolder(EngineAssetRoot, Current);
					ImGui::EndChild();
				}

				ImGui::EndChild();
			}

			ImGui::SameLine();
			{
				ImGui::BeginChild("Content", ImVec2(700, 0), TRUE);
				ImGui::SetNextItemWidth(320.0f);
				if (ImGui::InputTextWithHint("##content_search", "Search name or type...", m_ContentSearch
					, sizeof(m_ContentSearch)))
				{
					SelectedFile = nullptr;
				}
				ImGui::SameLine();
				ImGui::TextDisabled("Filter");
				ImGui::Separator();
				if (EMemory::GetPtr(Current))
				{
					if (EMemory::GetPtr(SelectedFile))
					{
						ImGui::Text("Selected Asset: %s", *SelectedFile->GetDisplayName());
						ImGui::SameLine();
						ImGui::TextDisabled("(%s)", *AssetTypeAsString(SelectedFile->GetType()));
						if ((SelectedFile->GetType() == EAssetType::ASSET_TYPE_ANIMATION)
							&& AnimationWorkspace)
						{
							ImGui::SameLine();
							if (ImGui::Button("Open Animation Editor"))
							{
								RequestOpenAsset(SelectedFile->GetPath(), SelectedFile->GetType());
							}
						}
						else if ((SelectedFile->GetType() == EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE)
							&& AnimationWorkspace)
						{
							ImGui::SameLine();
							if (ImGui::Button("Open State Machine Editor"))
							{
								RequestOpenAsset(SelectedFile->GetPath(), SelectedFile->GetType());
							}
						}
						else if ((SelectedFile->GetType() == EAssetType::ASSET_TYPE_ANIM_MONTAGE)
							&& AnimationWorkspace)
						{
							ImGui::SameLine();
							if (ImGui::Button("Open Montage Editor"))
							{
								RequestOpenAsset(SelectedFile->GetPath(), SelectedFile->GetType());
							}
						}
						else if ((SelectedFile->GetType() == EAssetType::ASSET_TYPE_ANIM_NOTIFY)
							&& AnimationWorkspace)
						{
							ImGui::SameLine();
							if (ImGui::Button("Open Notify Editor"))
							{
								RequestOpenAsset(SelectedFile->GetPath(), SelectedFile->GetType());
							}
						}
						else if ((SelectedFile->GetType() == EAssetType::ASSET_TYPE_SKELETON)
							&& AnimationWorkspace)
						{
							ImGui::SameLine();
							if (ImGui::Button("Edit Slot Groups"))
							{
								RequestOpenAsset(SelectedFile->GetPath(), SelectedFile->GetType());
							}
						}
						else if ((SelectedFile->GetType() == EAssetType::ASSET_TYPE_MESH) && MeshEditor)
						{
							ImGui::SameLine();
							if (ImGui::Button("Open Mesh Editor"))
							{
								MeshEditor->OpenMeshAsset(SelectedFile->GetPath());
							}
						}
						if (IsManagedAssetType(SelectedFile->GetType()) != FALSE)
						{
							ImGui::SameLine();
							if (ImGui::Button("Rename"))
							{
								m_RenameTargetPath = SelectedFile->GetPath();
								m_RenameTargetType = SelectedFile->GetType();
								m_ManageAssetError = EString();
								const EString SourceName = EPath::GetFileNameWithoutExtension(SelectedFile->GetPath());
								::snprintf(m_RenameBuffer, sizeof(m_RenameBuffer), "%s", *SourceName);
								m_bRenameRequested = TRUE;
							}
							ImGui::SameLine();
							if (ImGui::Button("Delete"))
							{
								m_DeleteTargetPath = SelectedFile->GetPath();
								m_DeleteTargetType = SelectedFile->GetType();
								m_ManageAssetError = EString();
								m_bDeleteRequested = TRUE;
							}
							TArray<EString> Users;
							CollectAssetUsers(SelectedFile->GetPath(), SelectedFile->GetType(), Users);
							if (!Users.IsEmpty())
							{
								EString UserText;
								for (INT32 UserIndex = 0; UserIndex < Users.Num(); UserIndex++)
								{
									if (UserIndex > 0)
									{
										UserText += EString(", ");
									}
									UserText += Users[UserIndex];
								}
								ImGui::TextWrapped("Used by loaded assets: %s", *UserText);
							}
							else
							{
								ImGui::TextDisabled("No loaded asset references this asset.");
							}
						}
						ImGui::Separator();
					}

					ImVec2 button_sz(100, 100);
					auto ChildrenFolder = Current->GetChildrenFolder();
					FLOAT window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
					UINT32 i = 0;
					for (UINT32 n = ChildrenFolder.Num<UINT32>(); i < n; ++i)
					{
						ImGui::PushID(i);
						ImGui::BeginChild("asdas", ImVec2(120, 150), FALSE);
						// imgui 1.91+ ImageButton requires a str_id first argument
						// (was inferred from texture id before). Use the loop index
						// stringified - already inside a PushID(i) so collisions are
						// fine within the parent ID scope.
						if(ImGui::ImageButton("##folder", FolderTextureId, button_sz))
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
						if (m_ContentSearch[0] != 0)
						{
							const EString SearchText(m_ContentSearch);
							const EString TypeLabel = AssetTypeAsString(elem->GetType());
							if ((elem->GetDisplayName().Contains(SearchText) == FALSE)
								&& (TypeLabel.Contains(SearchText) == FALSE))
							{
								continue;
							}
						}
						ImGui::PushID(i);
						ImGui::BeginChild("asdas", ImVec2(120, 150), FALSE);
						// imgui 1.91+ removed SetItemAllowOverlap(); replaced with
						// SetNextItemAllowOverlap() declared BEFORE the underlying
						// Selectable so a subsequent overlay Image+Text doesn't
						// hijack the click.
						ImGui::SetNextItemAllowOverlap();
						const BOOL8 bSelected = ImGui::Selectable(*(EString("##") + elem->GetDisplayName())
							, elem == SelectedFile, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(120, 150));
						// Selectable reports clicks on release, while ImGui records a double click
						// on the second press. Capture the double-click state before the overlay
						// Image/Text changes the current item; testing it after EndChild() misses
						// the press frame on most double clicks.
						const BOOL8 bDoubleClicked = ((ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByItem)
							&& ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) ? TRUE : FALSE);
						const ImTextureID id = GetThumbNail(elem->GetType(), elem);
						ImGui::SetCursorPos(ImVec2(0,0));
						ImGui::Image(id, button_sz);
						ImGui::SetCursorPos(ImVec2(0, button_sz.y + 10));
						ImGui::TextWrapped(*elem->GetDisplayName());
						ImGui::EndChild();
						const FLOAT last_button_x2 = ImGui::GetItemRectMax().x;
						const ImGuiStyle& style = ImGui::GetStyle();
						const FLOAT next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x;
						if ((bSelected != FALSE) || (bDoubleClicked != FALSE))
						{
							SelectedFile = elem;
							PE_LOG_LOG(EString("ContentBrowser:Select ") + elem->GetDisplayName());
						}
							if (bDoubleClicked != FALSE)
							{
								if ((elem->GetType() == EAssetType::ASSET_TYPE_ANIMATION)
									&& AnimationWorkspace)
								{
									RequestOpenAsset(elem->GetPath(), elem->GetType());
								}
								else if ((elem->GetType() == EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE)
									&& AnimationWorkspace)
								{
									RequestOpenAsset(elem->GetPath(), elem->GetType());
								}
								else if ((elem->GetType() == EAssetType::ASSET_TYPE_ANIM_MONTAGE)
									&& AnimationWorkspace)
								{
									RequestOpenAsset(elem->GetPath(), elem->GetType());
								}
								else if ((elem->GetType() == EAssetType::ASSET_TYPE_ANIM_NOTIFY)
									&& AnimationWorkspace)
								{
									RequestOpenAsset(elem->GetPath(), elem->GetType());
								}
								else if ((elem->GetType() == EAssetType::ASSET_TYPE_SKELETON)
									&& AnimationWorkspace)
								{
									RequestOpenAsset(elem->GetPath(), elem->GetType());
								}
								else if ((elem->GetType() == EAssetType::ASSET_TYPE_MESH) && MeshEditor)
								{
									MeshEditor->OpenMeshAsset(elem->GetPath());
								}
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

		DrawAssetManagementPopups();
		DrawNewAssetDialog();
		ImGui::End();
	}

	void EAssetManager::RequestOpenAsset(const EString& InAssetPath, const EAssetType& InType)
	{
		m_PendingOpenPath = InAssetPath;
		m_PendingOpenType = InType;
		m_bHasPendingOpen = TRUE;
	}

	void EAssetManager::ProcessPendingOpen()
	{
		if (m_bHasPendingOpen == FALSE)
		{
			return;
		}
		const EString AssetPath = m_PendingOpenPath;
		const EAssetType AssetType = m_PendingOpenType;
		m_PendingOpenPath = EString();
		m_PendingOpenType = EAssetType::ASSET_TYPE_UNKNOWN;
		m_bHasPendingOpen = FALSE;
		if ((!AnimationWorkspace) || (AssetPath.Length() <= 0u))
		{
			return;
		}
		switch (AssetType)
		{
		case EAssetType::ASSET_TYPE_ANIMATION:
			AnimationWorkspace->OpenAnimationAsset(AssetPath);
			break;
		case EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE:
			AnimationWorkspace->OpenStateMachineAsset(AssetPath);
			break;
		case EAssetType::ASSET_TYPE_ANIM_MONTAGE:
			AnimationWorkspace->OpenMontageAsset(AssetPath);
			break;
		case EAssetType::ASSET_TYPE_ANIM_NOTIFY:
			AnimationWorkspace->OpenNotifyAsset(AssetPath);
			break;
		case EAssetType::ASSET_TYPE_SKELETON:
			AnimationWorkspace->OpenSlotGroupAsset(AssetPath);
			break;
		default:
			break;
		}
	}

	void EAssetManager::CollectAssetUsers(const EString& InAssetPath, const EAssetType& InType
		, TArray<EString>& OutUsers)const
	{
		OutUsers.Empty();
		const EString AssetName = EPath::GetFileNameWithoutExtension(InAssetPath);
		if (AssetName.Length() <= 0u)
		{
			return;
		}
		auto AddUser = [&OutUsers](const EString& InUserName)
		{
			if ((InUserName.Length() > 0u) && (OutUsers.Find(InUserName) == -1))
			{
				OutUsers.Add(InUserName);
			}
		};

		if (InType == EAssetType::ASSET_TYPE_ANIMATION)
		{
			TArray<const EAnimMontageAsset*> MontageAssets;
			EAnimMontageManager::GetManagerSingleton()->GetLoadedAssets(MontageAssets);
			for (INT32 AssetIndex = 0; AssetIndex < MontageAssets.Num(); AssetIndex++)
			{
				const EAnimMontage* Montage = (MontageAssets[AssetIndex]
					? MontageAssets[AssetIndex]->GetStoragedResource() : nullptr);
				if (Montage && (Montage->AnimationAssetName == AssetName))
				{
					AddUser(MontageAssets[AssetIndex]->GetAssetName());
				}
			}
			TArray<const EAnimStateMachineAsset*> MachineAssets;
			EAnimStateMachineManager::GetManagerSingleton()->GetLoadedAssets(MachineAssets);
			for (INT32 AssetIndex = 0; AssetIndex < MachineAssets.Num(); AssetIndex++)
			{
				const EAnimStateMachine* Machine = (MachineAssets[AssetIndex]
					? MachineAssets[AssetIndex]->GetStoragedResource() : nullptr);
				if (Machine && (Machine->AnimationAssetName == AssetName))
				{
					AddUser(MachineAssets[AssetIndex]->GetAssetName());
				}
			}
		}
		else if (InType == EAssetType::ASSET_TYPE_SKELETON)
		{
			TArray<const EAnimMontageAsset*> MontageAssets;
			EAnimMontageManager::GetManagerSingleton()->GetLoadedAssets(MontageAssets);
			for (INT32 AssetIndex = 0; AssetIndex < MontageAssets.Num(); AssetIndex++)
			{
				const EAnimMontage* Montage = (MontageAssets[AssetIndex]
					? MontageAssets[AssetIndex]->GetStoragedResource() : nullptr);
				if (Montage && (Montage->SkeletonName == AssetName))
				{
					AddUser(MontageAssets[AssetIndex]->GetAssetName());
				}
			}
			TArray<const EAnimStateMachineAsset*> MachineAssets;
			EAnimStateMachineManager::GetManagerSingleton()->GetLoadedAssets(MachineAssets);
			for (INT32 AssetIndex = 0; AssetIndex < MachineAssets.Num(); AssetIndex++)
			{
				const EAnimStateMachine* Machine = (MachineAssets[AssetIndex]
					? MachineAssets[AssetIndex]->GetStoragedResource() : nullptr);
				if (Machine && (Machine->SkeletonName == AssetName))
				{
					AddUser(MachineAssets[AssetIndex]->GetAssetName());
				}
			}
			TArray<const ESkeletonAnimationAsset*> AnimationAssets;
			EAnimationManager::GetManagerSingleton()->GetLoadedAssets(AnimationAssets);
			for (INT32 AssetIndex = 0; AssetIndex < AnimationAssets.Num(); AssetIndex++)
			{
				const ESkeletonAnimation* Animation = (AnimationAssets[AssetIndex]
					? AnimationAssets[AssetIndex]->GetStoragedResource() : nullptr);
				if (Animation && (Animation->GetSkeletonName() == AssetName))
				{
					AddUser(AnimationAssets[AssetIndex]->GetAssetName());
				}
			}
		}
		else if (InType == EAssetType::ASSET_TYPE_ANIM_NOTIFY)
		{
			TArray<const ESkeletonAnimationAsset*> AnimationAssets;
			EAnimationManager::GetManagerSingleton()->GetLoadedAssets(AnimationAssets);
			for (INT32 AssetIndex = 0; AssetIndex < AnimationAssets.Num(); AssetIndex++)
			{
				const ESkeletonAnimation* Animation = (AnimationAssets[AssetIndex]
					? AnimationAssets[AssetIndex]->GetStoragedResource() : nullptr);
				if (!Animation)
				{
					continue;
				}
				BOOL32 bUsed = FALSE;
				for (INT32 ClipIndex = 0; (ClipIndex < Animation->GetAnimationClipNum()) && (bUsed == FALSE); ClipIndex++)
				{
					const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
					for (INT32 NotifyIndex = 0; Clip && (NotifyIndex < Clip->Notifies.Num()); NotifyIndex++)
					{
						if (Clip->Notifies[NotifyIndex].NotifyAssetName == AssetName)
						{
							bUsed = TRUE;
							break;
						}
					}
				}
				if (bUsed != FALSE)
				{
					AddUser(AnimationAssets[AssetIndex]->GetAssetName());
				}
			}
			TArray<const EAnimMontageAsset*> MontageAssets;
			EAnimMontageManager::GetManagerSingleton()->GetLoadedAssets(MontageAssets);
			for (INT32 AssetIndex = 0; AssetIndex < MontageAssets.Num(); AssetIndex++)
			{
				const EAnimMontage* Montage = (MontageAssets[AssetIndex]
					? MontageAssets[AssetIndex]->GetStoragedResource() : nullptr);
				if (!Montage)
				{
					continue;
				}
				BOOL32 bUsed = FALSE;
				for (INT32 SectionIndex = 0; (SectionIndex < Montage->Sections.Num()) && (bUsed == FALSE); SectionIndex++)
				{
					const TArray<FAnimNotifyEvent>& Notifies = Montage->Sections[SectionIndex].Notifies;
					for (INT32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); NotifyIndex++)
					{
						if (Notifies[NotifyIndex].NotifyAssetName == AssetName)
						{
							bUsed = TRUE;
							break;
						}
					}
				}
				if (bUsed != FALSE)
				{
					AddUser(MontageAssets[AssetIndex]->GetAssetName());
				}
			}
		}
	}

	BOOL32 EAssetManager::RenameSelectedAsset(const EString& InNewName, EString& OutError)
	{
		OutError = EString();
		if (IsValidNewAssetName(InNewName) == FALSE)
		{
			OutError = EString("Asset name is empty or contains an invalid character.");
			return FALSE;
		}
		if (m_RenameTargetPath.Length() <= 0u)
		{
			OutError = EString("No asset is selected.");
			return FALSE;
		}
		if ((AnimationWorkspace) && (AnimationWorkspace->IsAssetOpen(m_RenameTargetPath) != FALSE))
		{
			OutError = EString("Close the asset in the Workspace before renaming it.");
			return FALSE;
		}
		TArray<EString> Users;
		CollectAssetUsers(m_RenameTargetPath, m_RenameTargetType, Users);
		if (!Users.IsEmpty())
		{
			OutError = EString("Asset is referenced by loaded assets and cannot be renamed.");
			return FALSE;
		}

		EString Folder = EPath::GetFileFolderPath(m_RenameTargetPath);
		if ((Folder.Length() > 0u) && (Folder[Folder.LastIndex()] != '/'))
		{
			Folder += "/";
		}
		const EString NewPath = Folder + InNewName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE);
		if (NewPath == m_RenameTargetPath)
		{
			return TRUE;
		}
		if (EFileHelper::IsFileExists(NewPath) != FALSE)
		{
			OutError = EString("An asset with that name already exists: ") + NewPath;
			return FALSE;
		}
		if (!EFileHelper::MoveFile(m_RenameTargetPath, NewPath))
		{
			OutError = EString("Failed to rename asset file: ") + m_RenameTargetPath;
			return FALSE;
		}
		RefreshContentTree();
		SelectedFile = nullptr;
		return TRUE;
	}

	BOOL32 EAssetManager::DeleteSelectedAsset(EString& OutError)
	{
		OutError = EString();
		if (m_DeleteTargetPath.Length() <= 0u)
		{
			OutError = EString("No asset is selected.");
			return FALSE;
		}
		if ((AnimationWorkspace) && (AnimationWorkspace->IsAssetOpen(m_DeleteTargetPath) != FALSE))
		{
			OutError = EString("Close the asset in the Workspace before deleting it.");
			return FALSE;
		}
		TArray<EString> Users;
		CollectAssetUsers(m_DeleteTargetPath, m_DeleteTargetType, Users);
		if (!Users.IsEmpty())
		{
			OutError = EString("Asset is referenced by loaded assets and cannot be deleted.");
			return FALSE;
		}
		if (!EFileHelper::DeleteFile(m_DeleteTargetPath))
		{
			OutError = EString("Failed to delete asset file: ") + m_DeleteTargetPath;
			return FALSE;
		}
		RefreshContentTree();
		SelectedFile = nullptr;
		return TRUE;
	}

	void EAssetManager::DrawAssetManagementPopups()
	{
		// Open at the content-browser window level. Opening inside the Content child made the
		// popup ID belong to that child while BeginPopupModal used the parent, so the modal
		// never appeared.
		if (m_bRenameRequested != FALSE)
		{
			m_bRenameRequested = FALSE;
			ImGui::OpenPopup("##rename_asset");
		}
		if (m_bDeleteRequested != FALSE)
		{
			m_bDeleteRequested = FALSE;
			ImGui::OpenPopup("##delete_asset");
		}

		if (ImGui::IsPopupOpen("##rename_asset") != false)
		{
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		}
		if (ImGui::BeginPopupModal("##rename_asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Rename Asset");
			ImGui::SetNextItemWidth(360.0f);
			ImGui::InputText("##rename_asset_name", m_RenameBuffer, sizeof(m_RenameBuffer));
			if (m_ManageAssetError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_ManageAssetError);
			}
			if (ImGui::Button("Rename", ImVec2(120.0f, 0.0f)))
			{
				EString Error;
				if (RenameSelectedAsset(EString(m_RenameBuffer), Error))
				{
					m_ManageAssetError = EString();
					ImGui::CloseCurrentPopup();
				}
				else
				{
					m_ManageAssetError = Error;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			{
				m_ManageAssetError = EString();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::IsPopupOpen("##delete_asset") != false)
		{
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		}
		if (ImGui::BeginPopupModal("##delete_asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Delete %s?", *EPath::GetFileNameWithoutExtension(m_DeleteTargetPath));
			TArray<EString> Users;
			CollectAssetUsers(m_DeleteTargetPath, m_DeleteTargetType, Users);
			if (!Users.IsEmpty())
			{
				EString UserText;
				for (INT32 UserIndex = 0; UserIndex < Users.Num(); UserIndex++)
				{
					if (UserIndex > 0)
					{
						UserText += EString(", ");
					}
					UserText += Users[UserIndex];
				}
				ImGui::TextWrapped("Used by loaded assets: %s", *UserText);
			}
			if (m_ManageAssetError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_ManageAssetError);
			}
			ImGui::BeginDisabled(!Users.IsEmpty());
			if (ImGui::Button("Delete", ImVec2(120.0f, 0.0f)))
			{
				EString Error;
				if (DeleteSelectedAsset(Error))
				{
					m_ManageAssetError = EString();
					ImGui::CloseCurrentPopup();
				}
				else
				{
					m_ManageAssetError = Error;
				}
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			{
				m_ManageAssetError = EString();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void EAssetManager::OpenNewAssetDialog(const EAssetType& InType)
	{
		m_NewAssetType = InType;
		m_bResetNewAssetWindowPos = TRUE;
		m_bFocusNewAssetWindow = TRUE;
		m_NewAssetError = EString();
		m_NewAssetAnimationNames.Empty();
		m_NewAssetAnimationIndex = -1;
		m_NewAssetClipNames.Empty();
		m_NewAssetClipIndex = -1;
		m_NewAssetSkeletonNames.Empty();
		m_NewAssetSkeletonIndex = -1;
		m_NewAssetSlotNames.Empty();
		m_NewAssetSlotIndex = -1;
		m_NewAssetSkeletonName = EString();

		TArray<EString> ScanFolders;
		TArray<EString> ScanFiles;
		if (EFileHelper::ScanDirectory(EBaseSettings::ENGINE_ANIMATION_PATH, ScanFolders, ScanFiles, TRUE))
		{
			for (INT32 FileIndex = 0; FileIndex < ScanFiles.Num(); FileIndex++)
			{
				if (ScanFiles[FileIndex].Contains(EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE)))
				{
					m_NewAssetAnimationNames.Add(EPath::GetFileNameWithoutExtension(ScanFiles[FileIndex]));
				}
			}
		}
		if (!m_NewAssetAnimationNames.IsEmpty())
		{
			m_NewAssetAnimationIndex = 0;
			RefreshNewAssetClipsAndSlots();
		}

		const BOOL32 bIsMontage = ((InType == EAssetType::ASSET_TYPE_ANIM_MONTAGE) ? TRUE : FALSE);
		const EString SavePath((bIsMontage != FALSE)
			? EBaseSettings::ENGINE_MONTAGE_PATH : EBaseSettings::ENGINE_STATE_MACHINE_PATH);
		const EString BaseName((bIsMontage != FALSE)
			? EString("NewMontage") : EString("NewStateMachine"));
		EString Candidate = BaseName;
		INT32 Suffix = 1;
		while (EFileHelper::IsFileExists(SavePath + Candidate
			+ EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE)) != FALSE)
		{
			Candidate = BaseName + EString("_") + ToString(Suffix++);
		}
		::snprintf(m_NewAssetName, sizeof(m_NewAssetName), "%s", *Candidate);
	}

	void EAssetManager::RefreshNewAssetClipsAndSlots()
	{
		m_NewAssetClipNames.Empty();
		m_NewAssetClipIndex = -1;
		m_NewAssetSkeletonNames.Empty();
		m_NewAssetSkeletonIndex = -1;
		m_NewAssetSlotNames.Empty();
		m_NewAssetSlotIndex = -1;
		m_NewAssetSkeletonName = EString();
		if ((m_NewAssetAnimationIndex < 0)
			|| (m_NewAssetAnimationIndex >= m_NewAssetAnimationNames.Num()))
		{
			return;
		}

		const ESkeletonAnimationAsset* AnimationAsset = nullptr;
		if (!(EAnimationManager::GetManagerSingleton()->LoadSkeletonAnimationAsset(
			EBaseSettings::ENGINE_ANIMATION_PATH, m_NewAssetAnimationNames[m_NewAssetAnimationIndex]
			, AnimationAsset)) || (!AnimationAsset))
		{
			return;
		}
		const ESkeletonAnimation* Animation = AnimationAsset->GetStoragedResource();
		if (!Animation)
		{
			return;
		}
		m_NewAssetSkeletonName = Animation->GetSkeletonName();
		for (INT32 ClipIndex = 0; ClipIndex < Animation->GetAnimationClipNum(); ClipIndex++)
		{
			const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
			if (Clip)
			{
				m_NewAssetClipNames.Add(Clip->ClipName);
			}
		}
		if (!m_NewAssetClipNames.IsEmpty())
		{
			m_NewAssetClipIndex = 0;
		}

		TArray<EString> SkeletonFolders;
		TArray<EString> SkeletonFiles;
		if (EFileHelper::ScanDirectory(EBaseSettings::ENGINE_SKELETON_PATH
			, SkeletonFolders, SkeletonFiles, TRUE))
		{
			for (INT32 FileIndex = 0; FileIndex < SkeletonFiles.Num(); FileIndex++)
			{
				if (SkeletonFiles[FileIndex].Contains(EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE)))
				{
					m_NewAssetSkeletonNames.Add(EPath::GetFileNameWithoutExtension(SkeletonFiles[FileIndex]));
				}
			}
		}
		if (!m_NewAssetSkeletonNames.IsEmpty())
		{
			INT32 PreferredSkeletonIndex = -1;
			for (INT32 SkeletonIndex = 0; SkeletonIndex < m_NewAssetSkeletonNames.Num(); SkeletonIndex++)
			{
				if (m_NewAssetSkeletonNames[SkeletonIndex] == m_NewAssetSkeletonName)
				{
					PreferredSkeletonIndex = SkeletonIndex;
					break;
				}
			}
			if (PreferredSkeletonIndex < 0)
			{
				PreferredSkeletonIndex = 0;
			}
			m_NewAssetSkeletonIndex = PreferredSkeletonIndex;
			m_NewAssetSkeletonName = m_NewAssetSkeletonNames[PreferredSkeletonIndex];
		}
		else
		{
			m_NewAssetSkeletonIndex = -1;
		}
		RefreshNewAssetSlots();
	}

	void EAssetManager::RefreshNewAssetSlots()
	{
		m_NewAssetSlotNames.Empty();
		m_NewAssetSlotIndex = -1;
		if (m_NewAssetType != EAssetType::ASSET_TYPE_ANIM_MONTAGE)
		{
			return;
		}
		const ESkeletonAsset* SkeletonAsset = nullptr;
		if ((m_NewAssetSkeletonIndex >= 0)
			&& (m_NewAssetSkeletonIndex < m_NewAssetSkeletonNames.Num())
			&& ESkeletonAssetManager::GetManagerSingleton()->LoadSkeletonAsset(
				EBaseSettings::ENGINE_SKELETON_PATH
				, m_NewAssetSkeletonNames[m_NewAssetSkeletonIndex], SkeletonAsset) && SkeletonAsset)
		{
			const ESkeleton* Skeleton = SkeletonAsset->GetStoragedResource();
			if (Skeleton)
			{
				INT32 PreferredSlotIndex = -1;
				const TArray<ESkeletonSlotGroup>& Groups = Skeleton->GetSlotGroups();
				for (INT32 GroupIndex = 0; GroupIndex < Groups.Num(); GroupIndex++)
				{
					for (INT32 SlotIndex = 0; SlotIndex < Groups[GroupIndex].SlotNames.Num(); SlotIndex++)
					{
						const EString& SlotName = Groups[GroupIndex].SlotNames[SlotIndex];
						if (SlotName == EString("Default"))
						{
							PreferredSlotIndex = m_NewAssetSlotNames.Num();
						}
						m_NewAssetSlotNames.Add(SlotName);
					}
				}
				if (PreferredSlotIndex >= 0)
				{
					m_NewAssetSlotIndex = PreferredSlotIndex;
				}
				else if (!m_NewAssetSlotNames.IsEmpty())
				{
					m_NewAssetSlotIndex = 0;
				}
			}
		}
		if (m_NewAssetSlotNames.IsEmpty())
		{
			// A montage can still be authored with the conventional default slot; the editor
			// warns when it cannot resolve the slot against a preview skeleton.
			m_NewAssetSlotNames.Add(EString("Default"));
			m_NewAssetSlotIndex = 0;
		}
	}

	void EAssetManager::DrawNewAssetDialog()
	{
		if (m_bNewAssetWindowOpen == FALSE)
		{
			return;
		}
		const ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (m_bResetNewAssetWindowPos != FALSE)
		{
			// One-shot placement after opening. ImGuiCond_Always is used here because the
			// window may have a saved position from a previous session; the user can move or
			// resize it freely after this frame.
			ImGui::SetNextWindowPos(ImVec2(Viewport->WorkPos.x + 60.0f, Viewport->WorkPos.y + 60.0f)
				, ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(520.0f, 0.0f), ImGuiCond_Always);
			m_bResetNewAssetWindowPos = FALSE;
		}
		if (m_bFocusNewAssetWindow != FALSE)
		{
			ImGui::SetNextWindowFocus();
			m_bFocusNewAssetWindow = FALSE;
		}
		if (!ImGui::Begin("New Asset", &m_bNewAssetWindowOpen
			, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking))
		{
			ImGui::End();
			return;
		}
		const BOOL32 bIsMontage = ((m_NewAssetType == EAssetType::ASSET_TYPE_ANIM_MONTAGE) ? TRUE : FALSE);
		ImGui::Text("%s", (bIsMontage != FALSE) ? "New Montage" : "New State Machine");
		ImGui::Separator();

		ImGui::TextDisabled("Name");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##new_asset_name", m_NewAssetName, sizeof(m_NewAssetName));

		ImGui::TextDisabled("Animation");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (m_NewAssetAnimationNames.IsEmpty())
		{
			ImGui::TextDisabled("No animation assets found. Import an animation first.");
		}
		else if (ImGui::BeginCombo("##new_asset_animation"
			, ((m_NewAssetAnimationIndex >= 0) && (m_NewAssetAnimationIndex < m_NewAssetAnimationNames.Num()))
			? *m_NewAssetAnimationNames[m_NewAssetAnimationIndex] : "None"))
		{
			for (INT32 AnimationIndex = 0; AnimationIndex < m_NewAssetAnimationNames.Num(); AnimationIndex++)
			{
				const BOOL8 bSelected = (AnimationIndex == m_NewAssetAnimationIndex);
				if (ImGui::Selectable(*m_NewAssetAnimationNames[AnimationIndex], bSelected))
				{
					m_NewAssetAnimationIndex = AnimationIndex;
					m_NewAssetError = EString();
					RefreshNewAssetClipsAndSlots();
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::TextDisabled("Clip");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (m_NewAssetClipNames.IsEmpty())
		{
			ImGui::TextDisabled("This animation has no clips.");
		}
		else if (ImGui::BeginCombo("##new_asset_clip", ((m_NewAssetClipIndex >= 0)
			&& (m_NewAssetClipIndex < m_NewAssetClipNames.Num()))
			? *m_NewAssetClipNames[m_NewAssetClipIndex] : "None"))
		{
			for (INT32 ClipIndex = 0; ClipIndex < m_NewAssetClipNames.Num(); ClipIndex++)
			{
				const BOOL8 bSelected = (ClipIndex == m_NewAssetClipIndex);
				if (ImGui::Selectable(*m_NewAssetClipNames[ClipIndex], bSelected))
				{
					m_NewAssetClipIndex = ClipIndex;
					m_NewAssetError = EString();
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::TextDisabled("Skeleton");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (m_NewAssetSkeletonNames.IsEmpty())
		{
			ImGui::TextDisabled("No skeleton assets found.");
		}
		else if (ImGui::BeginCombo("##new_asset_skeleton"
			, ((m_NewAssetSkeletonIndex >= 0) && (m_NewAssetSkeletonIndex < m_NewAssetSkeletonNames.Num()))
			? *m_NewAssetSkeletonNames[m_NewAssetSkeletonIndex] : "None"))
		{
			for (INT32 SkeletonIndex = 0; SkeletonIndex < m_NewAssetSkeletonNames.Num(); SkeletonIndex++)
			{
				const BOOL8 bSelected = (SkeletonIndex == m_NewAssetSkeletonIndex);
				if (ImGui::Selectable(*m_NewAssetSkeletonNames[SkeletonIndex], bSelected))
				{
					m_NewAssetSkeletonIndex = SkeletonIndex;
					m_NewAssetSkeletonName = m_NewAssetSkeletonNames[SkeletonIndex];
					m_NewAssetError = EString();
					RefreshNewAssetSlots();
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (bIsMontage != FALSE)
		{
			ImGui::TextDisabled("Slot");
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (m_NewAssetSlotNames.IsEmpty())
			{
				ImGui::TextDisabled("No slot is available.");
			}
			else if (ImGui::BeginCombo("##new_asset_slot", ((m_NewAssetSlotIndex >= 0)
				&& (m_NewAssetSlotIndex < m_NewAssetSlotNames.Num()))
				? *m_NewAssetSlotNames[m_NewAssetSlotIndex] : "None"))
			{
				for (INT32 SlotIndex = 0; SlotIndex < m_NewAssetSlotNames.Num(); SlotIndex++)
				{
					const BOOL8 bSelected = (SlotIndex == m_NewAssetSlotIndex);
					if (ImGui::Selectable(*m_NewAssetSlotNames[SlotIndex], bSelected))
					{
						m_NewAssetSlotIndex = SlotIndex;
						m_NewAssetError = EString();
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}

		const EString SavePath((bIsMontage != FALSE)
			? EBaseSettings::ENGINE_MONTAGE_PATH : EBaseSettings::ENGINE_STATE_MACHINE_PATH);
		ImGui::TextDisabled("Destination:");
		ImGui::TextWrapped("%s", *SavePath);

		if (m_NewAssetError.Length() > 0u)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_NewAssetError);
		}

		const BOOL32 bHasName = ((m_NewAssetName[0] != 0) ? TRUE : FALSE);
		const BOOL32 bHasAnimation = ((m_NewAssetAnimationIndex >= 0)
			&& (m_NewAssetAnimationIndex < m_NewAssetAnimationNames.Num())) ? TRUE : FALSE;
		const BOOL32 bHasClip = ((m_NewAssetClipIndex >= 0)
			&& (m_NewAssetClipIndex < m_NewAssetClipNames.Num())) ? TRUE : FALSE;
		const BOOL32 bHasSlot = ((bIsMontage == FALSE) || ((m_NewAssetSlotIndex >= 0)
			&& (m_NewAssetSlotIndex < m_NewAssetSlotNames.Num()))) ? TRUE : FALSE;
		const BOOL32 bCanCreate = ((bHasName != FALSE) && (bHasAnimation != FALSE)
			&& (bHasClip != FALSE) && (bHasSlot != FALSE)) ? TRUE : FALSE;

		ImGui::Separator();
		ImGui::BeginDisabled(bCanCreate == FALSE);
		if (ImGui::Button("Create", ImVec2(120.0f, 0.0f)))
		{
			EString Error;
			if (CreateNewAsset(Error))
			{
				m_bNewAssetWindowOpen = FALSE;
			}
			else
			{
				m_NewAssetError = Error;
			}
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
		{
			m_bNewAssetWindowOpen = FALSE;
		}
		ImGui::End();
	}

	BOOL32 EAssetManager::CreateNewAsset(EString& OutError)
	{
		OutError = EString();
		const EString AssetName(m_NewAssetName);
		if (IsValidNewAssetName(AssetName) == FALSE)
		{
			OutError = EString("Asset name is empty or contains an invalid character.");
			return FALSE;
		}
		if ((m_NewAssetAnimationIndex < 0)
			|| (m_NewAssetAnimationIndex >= m_NewAssetAnimationNames.Num())
			|| (m_NewAssetClipIndex < 0)
			|| (m_NewAssetClipIndex >= m_NewAssetClipNames.Num()))
		{
			OutError = EString("Select an animation and a clip.");
			return FALSE;
		}

		const BOOL32 bIsMontage = ((m_NewAssetType == EAssetType::ASSET_TYPE_ANIM_MONTAGE) ? TRUE : FALSE);
		if ((bIsMontage != FALSE)
			&& ((m_NewAssetSlotIndex < 0) || (m_NewAssetSlotIndex >= m_NewAssetSlotNames.Num())))
		{
			OutError = EString("Select a slot for the montage section.");
			return FALSE;
		}

		const EString& AnimationName = m_NewAssetAnimationNames[m_NewAssetAnimationIndex];
		const EString& ClipName = m_NewAssetClipNames[m_NewAssetClipIndex];
		const EString SavePath((bIsMontage != FALSE)
			? EBaseSettings::ENGINE_MONTAGE_PATH : EBaseSettings::ENGINE_STATE_MACHINE_PATH);
		const EString JsonText = ((bIsMontage != FALSE)
			? BuildNewMontageJson(AssetName, m_NewAssetSkeletonName, AnimationName, ClipName
				, m_NewAssetSlotNames[m_NewAssetSlotIndex])
			: BuildNewStateMachineJson(AssetName, m_NewAssetSkeletonName, AnimationName, ClipName));

		BOOL32 bCreated = FALSE;
		if (bIsMontage != FALSE)
		{
			bCreated = EAnimMontageManager::GetManagerSingleton()->CreateMontageAsset(
				AssetName, JsonText, SavePath, OutError);
		}
		else
		{
			bCreated = EAnimStateMachineManager::GetManagerSingleton()->CreateStateMachineAsset(
				AssetName, JsonText, SavePath, OutError);
		}
		if (bCreated == FALSE)
		{
			return FALSE;
		}

		RefreshContentTree();
		TSharedPtr<EFolderTreeNode> Destination = FindFolderByPath(EngineAssetRoot, SavePath);
		if (!Destination)
		{
			Destination = FindFolderByPath(ProjectAssetRoot, SavePath);
		}
		if (Destination)
		{
			Current = Destination;
		}

		const EString FullPath = SavePath + AssetName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE);
		if (AnimationWorkspace)
		{
			if (bIsMontage != FALSE)
			{
				AnimationWorkspace->OpenMontageAsset(FullPath);
			}
			else
			{
				AnimationWorkspace->OpenStateMachineAsset(FullPath);
			}
		}
		return TRUE;
	}

	void EAssetManager::RefreshAssetTree()
	{
		RefreshContentTree();
	}

	void EAssetManager::RequestContentRefresh()
	{
		m_bContentRefreshRequested = TRUE;
	}

	void EAssetManager::RefreshContentTree()
	{
		const EString CurrentPath = (Current ? Current->GetPath() : EString());
		if (EngineAssetRoot)
		{
			ScanFolder(EngineAssetRoot);
		}
		if (ProjectAssetRoot)
		{
			ScanFolder(ProjectAssetRoot);
		}
		TSharedPtr<EFolderTreeNode> Found = FindFolderByPath(EngineAssetRoot, CurrentPath);
		if (!Found)
		{
			Found = FindFolderByPath(ProjectAssetRoot, CurrentPath);
		}
		Current = (Found ? Found : EngineAssetRoot);
		SelectedFile = nullptr;
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
		// TryLoadTexture2D(EEngineSettings::ENGINE_TEXTURE_PATH, File->GetDisplayName(),Asset, &ImportPath, &ImportName, &ImportFileType);
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
		switch (Type)
		{
		case ASSET_TYPE_UNKNOWN	:
			{
				return "UnKnown";
			}
		case ASSET_TYPE_TEXTURE	:
			{
				return "Texture";
			}
		case ASSET_TYPE_MESH	:
			{
				return "Mesh";
			}
		case ASSET_TYPE_SKELETON	:
			{
				return "Skeleton";
			}
		case ASSET_TYPE_ANIMATION	:
			{
				return "Animation";
			}
		case ASSET_TYPE_SHADER	:
			{
				return "Shader";
			}
		case ASSET_TYPE_ANIM_STATE_MACHINE	:
			{
				return "AnimStateMachine";
			}
		case ASSET_TYPE_ANIM_MONTAGE	:
			{
				return "AnimMontage";
			}
		case ASSET_TYPE_ANIM_NOTIFY	:
			{
				return "AnimNotify";
			}
		case ASSET_TYPE_NAV_MESH	:
			{
				return "NavMesh";
			}
		case ASSET_TYPE_COUNT	:
			{
				return "Count";
			}

		}
		return "Invalid";
	}

}
#endif
