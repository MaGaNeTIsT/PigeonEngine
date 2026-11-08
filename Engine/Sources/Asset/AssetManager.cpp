#include "AssetManager.h"
#if ENGINE_EDITOR_ONLY
#include "../EngineCore/IO/FileHelper.h"
#include "Editor/AnimationEditor/AnimationEditor.h"
#include "Editor/AnimationMontageEditor/AnimationMontageEditor.h"
#include "Editor/AnimationNotifyEditor/AnimationNotifyEditor.h"
#include "Editor/AnimationStateMachineEditor/AnimationStateMachineEditor.h"
#include "Editor/SkeletonSlotGroupEditor/SkeletonSlotGroupEditor.h"
#include "Editor/MeshEditor/MeshEditor.h"
#include <TextureAsset/TextureAsset.h>

#include "../../EngineThirdParty/imGUI/Headers/imGUIManager.h"
#include "../EngineCore/Editor/EditorLogManager.h"
#include "../Asset/Editor/AssetImporter/ImportManager.h"

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
		INT32 FolderSeparatorIndex = Path.RightFind("/");
		EString target = Path;
		if(FolderSeparatorIndex == (INT32)Path.LastIndex())
		{
			target = Path.Substring(0, Path.Length() - 1);
			FolderSeparatorIndex = target.RightFind("/");
		}
		EString Name =  target.Substring(FolderSeparatorIndex + 1, Path.Length() - FolderSeparatorIndex - 1);

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
			for (const TSharedPtr<EFolderTreeNode>& elem : Node->GetChildrenFolder())
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
		for (const EString& elem : Paths)
		{
			ChildrenFolder.Add(EMemory::MakeShared<EFolderTreeNode>(elem));
		}
	}

	void EFolderTreeNode::SetChildrenFiles(const TArray<EString>& Paths)
	{
		ChildrenFile.Empty();
		for (const EString& elem : Paths)
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
		INT32 FolderSeparatorIndex = Path.RightFind("/");
		EString target = Path;
		if(FolderSeparatorIndex == (INT32)Path.LastIndex())
		{
			target = Path.Substring(0, Path.Length() - 1);
			FolderSeparatorIndex = target.RightFind("/");
		}
		EString Name =  target.Substring(FolderSeparatorIndex + 1, Path.Length() - FolderSeparatorIndex - 1);
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
		if (AnimationEditor)
		{
			delete AnimationEditor;
			AnimationEditor = nullptr;
		}
		if (StateMachineEditor)
		{
			delete StateMachineEditor;
			StateMachineEditor = nullptr;
		}
		if (MontageEditor)
		{
			delete MontageEditor;
			MontageEditor = nullptr;
		}
		if (NotifyEditor)
		{
			delete NotifyEditor;
			NotifyEditor = nullptr;
		}
		if (SkeletonSlotEditor)
		{
			delete SkeletonSlotEditor;
			SkeletonSlotEditor = nullptr;
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
		for (const TSharedPtr<EFolderTreeNode>& elem : Node->GetChildrenFolder())
		{
			ScanFolder(elem);
		}
		return TRUE;
	}

	// Finds a folder in a rebuilt tree by its stored path. RefreshContentBrowser uses it to
	// keep the current folder across a rescan: the old node is detached, not deleted, and only
	// its path is guaranteed to survive.
	static TSharedPtr<EFolderTreeNode> FindFolderByPath(const TSharedPtr<EFolderTreeNode>& InNode, const EString& InPath)
	{
		if (!InNode)
		{
			return nullptr;
		}
		if (InNode->GetPath() == InPath)
		{
			return InNode;
		}
		const TArray<TSharedPtr<EFolderTreeNode>>& Children = InNode->GetChildrenFolder();
		for (UINT32 ChildIndex = 0u, ChildNum = Children.Num<UINT32>(); ChildIndex < ChildNum; ChildIndex++)
		{
			TSharedPtr<EFolderTreeNode> Found = FindFolderByPath(Children[ChildIndex], InPath);
			if (Found)
			{
				return Found;
			}
		}
		return nullptr;
	}

	// The file counterpart of FindFolderByPath. A file that no longer exists answers null, and
	// the caller clears the selection rather than leaving a pointer into the old tree.
	static TSharedPtr<EAssetFile> FindFileByPath(const TSharedPtr<EFolderTreeNode>& InNode, const EString& InPath)
	{
		if (!InNode)
		{
			return nullptr;
		}
		const TArray<TSharedPtr<EAssetFile>>& Files = InNode->GetChildrenFile();
		for (UINT32 FileIndex = 0u, FileNum = Files.Num<UINT32>(); FileIndex < FileNum; FileIndex++)
		{
			if (Files[FileIndex] && (Files[FileIndex]->GetPath() == InPath))
			{
				return Files[FileIndex];
			}
		}
		const TArray<TSharedPtr<EFolderTreeNode>>& Children = InNode->GetChildrenFolder();
		for (UINT32 ChildIndex = 0u, ChildNum = Children.Num<UINT32>(); ChildIndex < ChildNum; ChildIndex++)
		{
			TSharedPtr<EAssetFile> Found = FindFileByPath(Children[ChildIndex], InPath);
			if (Found)
			{
				return Found;
			}
		}
		return nullptr;
	}

	void EAssetManager::EditorInit()
	{
		if (!AnimationEditor)
		{
			AnimationEditor = new EAnimationEditor();
			AnimationEditor->EditorInit();
		}
		if (!StateMachineEditor)
		{
			StateMachineEditor = new EAnimStateMachineEditor();
			StateMachineEditor->EditorInit();
		}
		if (!MontageEditor)
		{
			MontageEditor = new EAnimMontageEditor();
			MontageEditor->EditorInit();
		}
		if (!NotifyEditor)
		{
			NotifyEditor = new EAnimNotifyEditor();
			NotifyEditor->EditorInit();
		}
		if (!SkeletonSlotEditor)
		{
			SkeletonSlotEditor = new ESkeletonSlotGroupEditor();
			SkeletonSlotEditor->EditorInit();
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

		// Loads (or imports) the icon textures. Their GPU proxies are built by the render
		// thread and are still queued here, so the ImGui handles are resolved later by
		// EnsureEditorIconTextureIds once the first update has drained those commands.
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

		// The initial scan ran before the editor icons were imported, and the world may have
		// imported assets before this call. Rescan on the first editor frame so the browser
		// starts from the final contents.
		bContentRefreshRequested = TRUE;
	}

	void EAssetManager::RequestContentRefresh()
	{
		bContentRefreshRequested = TRUE;
	}

	void EAssetManager::RefreshContentBrowser()
	{
		const EString CurrentPath = (Current ? Current->GetPath() : EString());
		const EString SelectedPath = (SelectedFile ? SelectedFile->GetPath() : EString());

		// Rescan in place: the root objects and the ImGui tree node identities stay, so the
		// expansion state survives. Only the child arrays are rebuilt.
		if (EngineAssetRoot)
		{
			ScanFolder(EngineAssetRoot);
		}
		if (ProjectAssetRoot)
		{
			ScanFolder(ProjectAssetRoot);
		}

		Current = (CurrentPath.Length() > 0u) ? FindFolderByPath(EngineAssetRoot, CurrentPath) : TSharedPtr<EFolderTreeNode>();
		if ((!Current) && (CurrentPath.Length() > 0u) && (ProjectAssetRoot))
		{
			Current = FindFolderByPath(ProjectAssetRoot, CurrentPath);
		}
		if ((!Current) && (CurrentPath.Length() > 0u))
		{
			// The folder the user was in is gone; the root is the useful place to land.
			Current = EngineAssetRoot;
		}

		SelectedFile = (SelectedPath.Length() > 0u) ? FindFileByPath(EngineAssetRoot, SelectedPath) : TSharedPtr<EAssetFile>();
		if ((!SelectedFile) && (SelectedPath.Length() > 0u) && (ProjectAssetRoot))
		{
			SelectedFile = FindFileByPath(ProjectAssetRoot, SelectedPath);
		}
	}

	// Resolves one editor icon to its ImGui handle. The texture exists as an asset once
	// EditorInit has loaded it, but its GPU proxy is built by the render thread: the handle
	// stays invalid until that command has run, which is what makes the caller retry.
	static ImTextureID ResolveEditorIconTexture(const CHAR* InTextureName)
	{
		const ETexture2DAsset* Asset = nullptr;
		// Cached lookup only: EditorInit owns the load / import, and a missing icon must not
		// turn every frame into a disk read.
		if (ETextureAssetManager::GetManagerSingleton()->LoadTexture2DAsset(
			EBaseSettings::ENGINE_TEXTURE_PATH, EString(InTextureName), Asset) == FALSE)
		{
			return ImTextureID_Invalid;
		}
		return ((Asset != nullptr) ? EngineTextureToImgui(Asset) : ImTextureID_Invalid);
	}

	void EAssetManager::EnsureEditorIconTextureIds()
	{
		if (bEditorIconTextureIdsReady != FALSE)
		{
			return;
		}

		// EditorInit loads the icon textures, but their GPU proxies are built on the render
		// thread. This runs from EditorUpdate, after the frame boundary has retired those
		// creation commands, and retries until every icon has a live handle.
		BOOL32 bAllReady = TRUE;

		const ImTextureID ResolvedFolderTextureId = ResolveEditorIconTexture(PE_TEXT("Icon_ContentBrowser_Folder"));
		if (ResolvedFolderTextureId != ImTextureID_Invalid)
		{
			FolderTextureId = ResolvedFolderTextureId;
		}
		else
		{
			bAllReady = FALSE;
		}

		const struct FEditorThumbnailRequest
		{
			EAssetType	Type;
			const CHAR*	TextureName;
		} Requests[] =
		{
			{ EAssetType::ASSET_TYPE_MESH,					PE_TEXT("Icon_ContentBrowser_Mesh") },
			{ EAssetType::ASSET_TYPE_SHADER,				PE_TEXT("Icon_ContentBrowser_Shader") },
			{ EAssetType::ASSET_TYPE_SKELETON,				PE_TEXT("Icon_ContentBrowser_Skeleton") },
			{ EAssetType::ASSET_TYPE_ANIMATION,				PE_TEXT("Icon_ContentBrowser_Animation") },
			// The montage / notify / state machine families have no dedicated icon yet; they
			// share the animation icon rather than falling back to the font atlas.
			{ EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE,	PE_TEXT("Icon_ContentBrowser_Animation") },
			{ EAssetType::ASSET_TYPE_ANIM_MONTAGE,			PE_TEXT("Icon_ContentBrowser_Animation") },
			{ EAssetType::ASSET_TYPE_ANIM_NOTIFY,			PE_TEXT("Icon_ContentBrowser_Animation") },
			{ EAssetType::ASSET_TYPE_UNKNOWN,				PE_TEXT("Icon_ContentBrowser_No") },
		};
		constexpr UINT32 RequestCount = PE_ARRAYSIZE(Requests);
		for (UINT32 RequestIndex = 0u; RequestIndex < RequestCount; RequestIndex++)
		{
			const ImTextureID ResolvedTextureId = ResolveEditorIconTexture(Requests[RequestIndex].TextureName);
			if (ResolvedTextureId != ImTextureID_Invalid)
			{
				TypeThumbNails.Add(Requests[RequestIndex].Type, ResolvedTextureId);
			}
			else
			{
				bAllReady = FALSE;
			}
		}

		bEditorIconTextureIdsReady = bAllReady;
	}

	void EAssetManager::EditorUpdate()
	{
		// A refresh request may come from an importer (through EImportManager) or from the
		// Refresh button. Both are consumed here, before anything draws, so the tree is never
		// rebuilt while GenerateContentBrowser iterates it.
		const BOOL32 bImportRefreshRequested = EImportManager::GetManagerSingleton()->ConsumeContentBrowserRefreshRequest();
		if ((bContentRefreshRequested != FALSE) || (bImportRefreshRequested != FALSE))
		{
			bContentRefreshRequested = FALSE;
			RefreshContentBrowser();
		}

		// The icon handles need the render thread's texture creation, so they are resolved here
		// rather than in EditorInit.
		EnsureEditorIconTextureIds();
		GenerateContentBrowser();
		if (AnimationEditor)
		{
			AnimationEditor->EditorUpdate();
		}
		if (StateMachineEditor)
		{
			StateMachineEditor->EditorUpdate();
		}
		if (MontageEditor)
		{
			MontageEditor->EditorUpdate();
		}
		if (NotifyEditor)
		{
			NotifyEditor->EditorUpdate();
		}
		if (SkeletonSlotEditor)
		{
			SkeletonSlotEditor->EditorUpdate();
		}
		if (MeshEditor)
		{
			MeshEditor->EditorUpdate();
		}
	}

	void EAssetManager::GenerateContentBrowser()
	{
		ImGui::Begin("PigeonContent", FALSE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
		{
			ImGui::Text("PigeonContent");
			{
				// The tree is a resizable child: its right border is the drag line, and ImGui
				// persists the width in the ini. The constraints keep it from collapsing and
				// leave the content pane at least 200px, whatever the dock node's width is.
				const FLOAT MaximumTreeWidth = EMath::Max(160.0f, ImGui::GetContentRegionAvail().x - 200.0f);
				ImGui::SetNextWindowSizeConstraints(ImVec2(160.0f, 0.0f), ImVec2(MaximumTreeWidth, FLT_MAX));
				ImGui::BeginChild("Tree", ImVec2(220.0f, 0.0f)
					, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX
					, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
				ImGui::Text("FolderTree");
				ImGui::SameLine();
				EImportManager::GetManagerSingleton()->ShowImporterButton();
				ImGui::SameLine();
				if (ImGui::Button("Refresh"))
				{
					RequestContentRefresh();
				}
				{
					// The Project/Engine split is vertical and draggable, and both panes follow
					// the tree width instead of a fixed 180.
					ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 80.0f), ImVec2(FLT_MAX, 400.0f));
					ImGui::BeginChild("Project", ImVec2(0.0f, 140.0f)
						, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY
						, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
					ImGui::Text("Project");
					EFolderTreeNode::TraverseFolder(ProjectAssetRoot, Current);
					ImGui::EndChild();
				}
				{
					ImGui::BeginChild("Engine", ImVec2(0.0f, 0.0f)
						, ImGuiChildFlags_Borders
						, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
					ImGui::Text("Engine");
					EFolderTreeNode::TraverseFolder(EngineAssetRoot, Current);
					ImGui::EndChild();
				}

				ImGui::EndChild();
			}

			ImGui::SameLine();
			{
				ImGui::BeginChild("Content", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
				const ImGuiStyle& Style = ImGui::GetStyle();
				const FLOAT SidebarWidth = 260.0f;
				const FLOAT ContentWidth = ImGui::GetContentRegionAvail().x;
				const FLOAT GridChildWidth = EMath::Max(ContentWidth - SidebarWidth - Style.ItemSpacing.x, 1.0f);
				// The scrollbar is reserved before it appears, so a row that only fits the full
				// width does not overflow by its width once the grid starts scrolling.
				const FLOAT AvailableWidth = EMath::Max(GridChildWidth - Style.ScrollbarSize, 1.0f);
				const FLOAT CellPadding = 6.0f;
				const FLOAT CellWidth = ContentIconSize + Style.FramePadding.x * 2.0f + CellPadding * 2.0f;
				const FLOAT CellHeight = ContentIconSize + Style.FramePadding.x * 2.0f + CellPadding * 2.0f
					+ ImGui::GetTextLineHeight() * 2.0f + Style.ItemSpacing.y;
				const FLOAT ItemStride = CellWidth + Style.ItemSpacing.x;
				const UINT32 ItemsPerRow = EMath::Max(1u, static_cast<UINT32>(AvailableWidth / ItemStride));

				// The grid is the scrolling part; the sidebar beside it keeps the size control
				// and the selected asset's actions in view no matter where the grid is scrolled.
				ImGui::BeginChild("Grid", ImVec2(GridChildWidth, 0.0f), ImGuiChildFlags_None);
				if (EMemory::GetPtr(Current))
				{
					// The folder whose contents are being drawn. A click on a folder updates
					// Current for the next frame; keeping this snapshot here stops the file loop
					// below from suddenly drawing the clicked folder's files in this same frame.
					const TSharedPtr<EFolderTreeNode> ContentFolder = Current;
					const TArray<TSharedPtr<EFolderTreeNode>>& ChildrenFolder = ContentFolder->GetChildrenFolder();
					UINT32 ItemIndex = 0u;
					for (UINT32 FolderIndex = 0u, FolderNum = ChildrenFolder.Num<UINT32>(); FolderIndex < FolderNum; FolderIndex++)
					{
						ImGui::PushID(ItemIndex);
						ImGui::BeginChild("##item", ImVec2(CellWidth, CellHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
						const FLOAT FolderButtonWidth = ContentIconSize + Style.FramePadding.x * 2.0f;
						ImGui::SetCursorPos(ImVec2(CellPadding + (CellWidth - CellPadding * 2.0f - FolderButtonWidth) * 0.5f, CellPadding));
						if (ImGui::ImageButton("##folder", FolderTextureId, ImVec2(ContentIconSize, ContentIconSize)))
						{
							Current = ChildrenFolder[FolderIndex];
						}
						const FLOAT FolderButtonHeight = ImGui::GetItemRectSize().y;
						ImGui::SetCursorPos(ImVec2(CellPadding, CellPadding + FolderButtonHeight + Style.ItemSpacing.y));
						ImGui::PushTextWrapPos(CellWidth - CellPadding);
						ImGui::TextWrapped("%s", *ChildrenFolder[FolderIndex]->GetDisplayName());
						ImGui::PopTextWrapPos();
						ImGui::EndChild();
						ImGui::PopID();
						++ItemIndex;
						if ((ItemIndex % ItemsPerRow) != 0u)
						{
							ImGui::SameLine();
						}
					}

					const TArray<TSharedPtr<EAssetFile>>& ChildrenFile = ContentFolder->GetChildrenFile();
					for (UINT32 FileIndex = 0u, FileNum = ChildrenFile.Num<UINT32>(); FileIndex < FileNum; FileIndex++)
					{
						const TSharedPtr<EAssetFile>& File = ChildrenFile[FileIndex];
						ImGui::PushID(ItemIndex);
						ImGui::BeginChild("##item", ImVec2(CellWidth, CellHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
						// imgui 1.91+ removed SetItemAllowOverlap(); replaced with
						// SetNextItemAllowOverlap() declared BEFORE the underlying
						// Selectable so a subsequent overlay Image+Text doesn't
						// hijack the click.
						ImGui::SetNextItemAllowOverlap();
						const BOOL8 bSelected = ImGui::Selectable("##file", File == SelectedFile, 0, ImVec2(CellWidth, CellHeight));
						const ImTextureID IconTextureId = GetThumbNail(File->GetType(), File);
						ImGui::SetCursorPos(ImVec2(CellPadding + (CellWidth - CellPadding * 2.0f - ContentIconSize) * 0.5f, CellPadding));
						ImGui::Image(IconTextureId, ImVec2(ContentIconSize, ContentIconSize));
						ImGui::SetCursorPos(ImVec2(CellPadding, CellPadding + ContentIconSize + Style.ItemSpacing.y));
						ImGui::PushTextWrapPos(CellWidth - CellPadding);
						ImGui::TextWrapped("%s", *File->GetDisplayName());
						ImGui::PopTextWrapPos();
						if (bSelected)
						{
							SelectedFile = File;
							if ((File->GetType() == EAssetType::ASSET_TYPE_ANIMATION) && AnimationEditor && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
							{
								AnimationEditor->OpenAnimationAsset(File->GetPath());
							}
							else if ((File->GetType() == EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE) && StateMachineEditor && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
							{
								StateMachineEditor->OpenStateMachineAsset(File->GetPath());
							}
							else if ((File->GetType() == EAssetType::ASSET_TYPE_ANIM_MONTAGE) && MontageEditor && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
							{
								MontageEditor->OpenMontageAsset(File->GetPath());
							}
							else if ((File->GetType() == EAssetType::ASSET_TYPE_ANIM_NOTIFY) && NotifyEditor && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
							{
								NotifyEditor->OpenNotifyAsset(File->GetPath());
							}
							else if ((File->GetType() == EAssetType::ASSET_TYPE_SKELETON) && SkeletonSlotEditor && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
							{
								SkeletonSlotEditor->OpenSkeletonAsset(File->GetPath());
							}
							else if ((File->GetType() == EAssetType::ASSET_TYPE_MESH) && MeshEditor && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
							{
								MeshEditor->OpenMeshAsset(File->GetPath());
							}
							PE_LOG_LOG(EString(PE_TEXT("ContentBrowser:Select ")) + File->GetDisplayName());
						}
						ImGui::EndChild();
						ImGui::PopID();
						++ItemIndex;
						if ((ItemIndex % ItemsPerRow) != 0u)
						{
							ImGui::SameLine();
						}
					}

				} // if Current

				ImGui::EndChild(); // Grid

				ImGui::SameLine();
				ImGui::BeginChild("Sidebar", ImVec2(SidebarWidth, 0.0f), ImGuiChildFlags_Borders);
				ImGui::TextDisabled("Icon Size");
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				ImGui::SliderFloat("##IconSize", &ContentIconSize, 30.0f, 160.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::TextDisabled("(%u per row)", ItemsPerRow);
				ImGui::Separator();
				if (EMemory::GetPtr(SelectedFile))
				{
					ImGui::TextDisabled("Selected Asset");
					ImGui::TextWrapped("%s", *SelectedFile->GetDisplayName());
					ImGui::TextDisabled("(%s)", *AssetTypeAsString(SelectedFile->GetType()));
					ImGui::Separator();
					const EAssetType SelectedType = SelectedFile->GetType();
					const BOOL32 bCanOpenEditor = ((SelectedType == EAssetType::ASSET_TYPE_ANIMATION)
						|| (SelectedType == EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE)
						|| (SelectedType == EAssetType::ASSET_TYPE_ANIM_MONTAGE)
						|| (SelectedType == EAssetType::ASSET_TYPE_ANIM_NOTIFY)
						|| (SelectedType == EAssetType::ASSET_TYPE_SKELETON)
						|| (SelectedType == EAssetType::ASSET_TYPE_MESH)) ? TRUE : FALSE;
					if (bCanOpenEditor != FALSE)
					{
						const FLOAT SidebarButtonWidth = ImGui::GetContentRegionAvail().x;
						if (ImGui::Button("Open Editor", ImVec2(SidebarButtonWidth, 0.0f)))
						{
							if ((SelectedType == EAssetType::ASSET_TYPE_ANIMATION) && AnimationEditor)
							{
								AnimationEditor->OpenAnimationAsset(SelectedFile->GetPath());
							}
							else if ((SelectedType == EAssetType::ASSET_TYPE_ANIM_STATE_MACHINE) && StateMachineEditor)
							{
								StateMachineEditor->OpenStateMachineAsset(SelectedFile->GetPath());
							}
							else if ((SelectedType == EAssetType::ASSET_TYPE_ANIM_MONTAGE) && MontageEditor)
							{
								MontageEditor->OpenMontageAsset(SelectedFile->GetPath());
							}
							else if ((SelectedType == EAssetType::ASSET_TYPE_ANIM_NOTIFY) && NotifyEditor)
							{
								NotifyEditor->OpenNotifyAsset(SelectedFile->GetPath());
							}
							else if ((SelectedType == EAssetType::ASSET_TYPE_SKELETON) && SkeletonSlotEditor)
							{
								SkeletonSlotEditor->OpenSkeletonAsset(SelectedFile->GetPath());
							}
							else if ((SelectedType == EAssetType::ASSET_TYPE_MESH) && MeshEditor)
							{
								MeshEditor->OpenMeshAsset(SelectedFile->GetPath());
							}
						}
					}
				}
				else
				{
					ImGui::TextDisabled("No asset selected.");
				}
				ImGui::EndChild(); // Sidebar

				ImGui::EndChild(); // Content
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
		case ASSET_TYPE_COUNT	:
			{
				return "Count";
			}

		}
		return "Invalid";
	}

}
#endif
