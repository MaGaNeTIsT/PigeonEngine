#include <Editor/Asset/ContentBrowserPanel.h>

#if ENGINE_EDITOR_ONLY

#include <Editor/Asset/AssetCatalog.h>
#include <Editor/Asset/AssetFileOperations.h>
#include <Editor/Asset/AssetIconRegistry.h>
#include <Editor/Asset/AssetManager.h>
#include <Editor/Asset/AssetTypeUtil.h>
#include <Editor/Asset/AssetUsage.h>
#include <Editor/AnimationEditorCore/AnimationWorkspace.h>
#include <Editor/MeshEditor/MeshEditor.h>
#include <Editor/AssetImporter/ImportManager.h>

#include <AnimationAsset/AnimationAsset.h>
#include <AnimationMontageAsset/AnimationMontageAsset.h>
#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>
#include <IO/FileHelper.h>
#include <RapidJson/Headers/JsonObject.h>
#include <SkeletonAsset/SkeletonAsset.h>

#include <Windows.h>
#include <shellapi.h>
#include <imgui.h>

namespace PigeonEngine
{
	namespace
	{
		const CHAR* const ContentAssetDragPayload = "PAssetDrag";

		EString NormalizePath(const EString& InPath)
		{
			EString Result = InPath.Replace("\\", "/");
			while ((Result.Length() > 1u) && (Result[Result.LastIndex()] == '/'))
			{
				Result = Result.Left(Result.Length() - 1u);
			}
			return Result;
		}

		EString GetParentFolder(const EString& InPath)
		{
			return NormalizePath(EPath::GetFileFolderPath(NormalizePath(InPath)));
		}

		BOOL32 IsManagedAssetType(const EAssetType& InType)
		{
			switch (InType)
			{
			case ASSET_TYPE_ANIMATION:
			case ASSET_TYPE_ANIM_STATE_MACHINE:
			case ASSET_TYPE_ANIM_MONTAGE:
			case ASSET_TYPE_ANIM_NOTIFY:
			case ASSET_TYPE_SKELETON:
			case ASSET_TYPE_NAV_MESH:
				return TRUE;
			default:
				return FALSE;
			}
		}

		BOOL32 IsOpenableAssetType(const EAssetType& InType)
		{
			switch (InType)
			{
			case ASSET_TYPE_ANIMATION:
			case ASSET_TYPE_ANIM_STATE_MACHINE:
			case ASSET_TYPE_ANIM_MONTAGE:
			case ASSET_TYPE_ANIM_NOTIFY:
			case ASSET_TYPE_SKELETON:
			case ASSET_TYPE_MESH:
				return TRUE;
			default:
				return FALSE;
			}
		}

		void RevealInExplorer(const EString& InPath)
		{
			const EString Normalized = InPath.Replace("\\", "/");
			const EString WindowsPath = Normalized.Replace("/", "\\");
			const EString Arguments = EString("/select,\"") + WindowsPath + EString("\"");
			::ShellExecuteA(nullptr, "open", "explorer.exe", *Arguments, nullptr, SW_SHOWNORMAL);
		}

		EString JoinStrings(const TArray<EString>& InStrings)
		{
			EString Result;
			for (INT32 Index = 0, Num = InStrings.Num<INT32>(); Index < Num; Index++)
			{
				if (Index > 0)
				{
					Result += ", ";
				}
				Result += InStrings[Index];
			}
			return Result;
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

	void FContentBrowserPanel::Initialize(EAssetManager* InHost)
	{
		Host = InHost;
		CurrentFolderPath = EString();
		SelectedPaths.Empty();
		PrimaryPath = EString();
		SearchBuffer[0] = 0;
		TypeFilterIndex = 0;
		ClearSelection();
	}

	void FContentBrowserPanel::ShutDown()
	{
		Host = nullptr;		SelectedPaths.Empty();
		DeleteTargets.Empty();
		DeleteTargetReasons.Empty();
		DragPaths.Empty();
		NewAssetAnimationNames.Empty();
		NewAssetClipNames.Empty();
		NewAssetSkeletonNames.Empty();
		NewAssetSlotNames.Empty();
		UsersCache.Empty();
	}

	void FContentBrowserPanel::PruneSelectionAfterCatalogRefresh()
	{
		if (!Host)
		{
			return;
		}
		EnsureCurrentFolder();
		PruneSelection();
	}

	void FContentBrowserPanel::DrawContents()
	{
		if (!Host)
		{
			ImGui::TextDisabled("The asset catalog is not available.");
			return;
		}

		ProcessPendingOpen();
		EnsureCurrentFolder();

		const TSharedPtr<FAssetFolderNode> CurrentFolder = Host->GetCatalog().FindFolderByPath(CurrentFolderPath);
		// F2/Delete act on the current selection only while the browser owns the keyboard;
		// the modal dialogs take focus, so they cannot fire behind a text field.
		if ((ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) != false)
			&& (ImGui::GetIO().WantTextInput == false))
		{
			if ((ImGui::IsKeyPressed(ImGuiKey_F2) != false) && (PrimaryPath.Length() > 0u))
			{
				const EAssetType PrimaryType = FindTypeForPath(PrimaryPath);
				if (IsManagedAssetType(PrimaryType) != FALSE)
				{
					RequestRenameAsset(PrimaryPath, PrimaryType);
				}
			}
			if ((ImGui::IsKeyPressed(ImGuiKey_Delete) != false) && (!SelectedPaths.IsEmpty()))
			{
				RequestDeleteSelection();
			}
		}
		DrawToolbar(CurrentFolder);
		ImGui::Separator();

		ImGui::BeginChild("##cb_tree", ImVec2(240.0f, 0.0f), TRUE);
		DrawFolderTree();
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("##cb_content", ImVec2(0.0f, 0.0f), FALSE);
		if (CurrentFolder)
		{
			DrawBreadcrumbs(CurrentFolder);
			DrawSelectedSummary();
			DrawAssetGrid(CurrentFolder);
		}
		else
		{
			ImGui::TextDisabled("No content roots are available. Check the engine content paths.");
		}
		ImGui::EndChild();

		if (StatusMessage.Length() > 0u)
		{
			ImGui::Separator();
			const ImVec4 Color = (bStatusIsError != FALSE)
				? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.7f, 0.8f, 0.7f, 1.0f);
			ImGui::TextColored(Color, "%s", *StatusMessage);
		}

		DrawDialogs();
		DrawNewAssetDialog();
	}

	void FContentBrowserPanel::DrawToolbar(const TSharedPtr<FAssetFolderNode>& InCurrent)
	{
		if (ImGui::Button("Import"))
		{
			EImportManager::GetManagerSingleton()->RequestImportDialog();
		}
		ImGui::SameLine();
		if (ImGui::Button("Refresh"))
		{
			Host->RequestContentRefresh();
		}
		ImGui::SameLine();
		if (ImGui::Button("New"))
		{
			ImGui::OpenPopup("##cb_new_asset");
		}
		if (ImGui::BeginPopup("##cb_new_asset"))
		{
			if (ImGui::MenuItem("Montage"))
			{
				OpenNewAssetDialog(ASSET_TYPE_ANIM_MONTAGE);
			}
			if (ImGui::MenuItem("State Machine"))
			{
				OpenNewAssetDialog(ASSET_TYPE_ANIM_STATE_MACHINE);
			}
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		ImGui::BeginDisabled(InCurrent ? FALSE : TRUE);
		if (ImGui::Button("New Folder"))
		{
			FolderTargetPath = InCurrent ? InCurrent->GetPath() : EString();
			::snprintf(FolderNameBuffer, sizeof(FolderNameBuffer), "%s", "NewFolder");
			DialogError = EString();
			bNewFolderOpen = TRUE;
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::SetNextItemWidth(220.0f);
		ImGui::InputTextWithHint("##cb_search", "Search name or type...", SearchBuffer, sizeof(SearchBuffer));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(170.0f);
		const CHAR* TypePreview = (TypeFilterIndex > 0) && (TypeFilterIndex < static_cast<INT32>(ASSET_TYPE_COUNT))
			? EAssetTypeUtil::ToString(static_cast<EAssetType>(TypeFilterIndex)) : "All Types";
		if (ImGui::BeginCombo("##cb_type_filter", TypePreview))
		{
			if (ImGui::Selectable("All Types", TypeFilterIndex == 0))
			{
				TypeFilterIndex = 0;
			}
			for (UINT32 TypeIndex = 1u; TypeIndex < static_cast<UINT32>(ASSET_TYPE_COUNT); TypeIndex++)
			{
				const EAssetType Type = static_cast<EAssetType>(TypeIndex);
				if (ImGui::Selectable(EAssetTypeUtil::ToString(Type), TypeFilterIndex == static_cast<INT32>(TypeIndex)))
				{
					TypeFilterIndex = static_cast<INT32>(TypeIndex);
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(90.0f);
		ImGui::SliderFloat("##cb_tile_size", &TileSize, 64.0f, 160.0f, "%.0f px");
	}

	void FContentBrowserPanel::DrawBreadcrumbs(const TSharedPtr<FAssetFolderNode>& InCurrent)
	{
		const EString CurrentPath = NormalizePath(InCurrent->GetPath());
		EString RootLabel("Content");
		EString RootPath;
		const TArray<FAssetRoot>& Roots = Host->GetCatalog().GetRoots();
		for (INT32 RootIndex = 0, RootNum = Roots.Num<INT32>(); RootIndex < RootNum; RootIndex++)
		{
			if (!Roots[RootIndex].Node)
			{
				continue;
			}
			const EString Candidate = NormalizePath(Roots[RootIndex].Node->GetPath());
			if ((CurrentPath == Candidate) || (CurrentPath.StartWith(Candidate + "/") != FALSE))
			{
				RootLabel = Roots[RootIndex].Label;
				RootPath = Candidate;
				break;
			}
		}

		if (RootPath.Length() == 0u)
		{
			ImGui::TextDisabled("%s", *CurrentPath);
			return;
		}

		if (ImGui::SmallButton(*RootLabel))
		{
			SetCurrentFolder(RootPath);
		}
		const EString Relative = CurrentPath.Substring(RootPath.Length(), CurrentPath.Length() - RootPath.Length());
		EString Segment;
		EString Accumulated = RootPath;
		for (UINT32 CharIndex = 0u; CharIndex <= Relative.Length(); CharIndex++)
		{
			const BOOL32 bAtEnd = (CharIndex == Relative.Length()) ? TRUE : FALSE;
			const CHAR Char = (bAtEnd != FALSE) ? static_cast<CHAR>('/') : Relative[CharIndex];
			if (Char == '/')
			{
				if (Segment.Length() > 0u)
				{
					Accumulated += "/";
					Accumulated += Segment;
					ImGui::SameLine();
					ImGui::TextDisabled("/");
					ImGui::SameLine();
					ImGui::PushID(*Accumulated);
					if (ImGui::SmallButton(*Segment))
					{
						SetCurrentFolder(Accumulated);
					}
					ImGui::PopID();
					Segment = EString();
				}
			}
			else
			{
				Segment += EString::FromChar(Char);
			}
		}
	}

	void FContentBrowserPanel::DrawFolderTree()
	{
		// A drop target can move files and rescan the catalog in the middle of this
		// recursion. Iterate a local snapshot of the shared nodes so the frame that
		// performed the move finishes against the tree it started with; the next frame
		// picks up the rebuilt catalog.
		const TArray<FAssetRoot>& CatalogRoots = Host->GetCatalog().GetRoots();
		TArray<FAssetRoot> Roots;
		Roots.Reserve(CatalogRoots.Num<UINT32>());
		for (INT32 RootIndex = 0, RootNum = CatalogRoots.Num<INT32>(); RootIndex < RootNum; RootIndex++)
		{
			Roots.Add(CatalogRoots[RootIndex]);
		}
		for (INT32 RootIndex = 0, RootNum = Roots.Num<INT32>(); RootIndex < RootNum; RootIndex++)
		{
			if (!Roots[RootIndex].Node)
			{
				continue;
			}
			DrawFolderNode(Roots[RootIndex].Node, Roots[RootIndex].Node->GetPath(), TRUE, Roots[RootIndex].Label);
		}
	}

	void FContentBrowserPanel::DrawFolderNode(const TSharedPtr<FAssetFolderNode>& InNode, const EString& InRootPath
		, const BOOL32 bInIsRoot, const EString& InRootLabel)
	{
		(void)InRootPath;
		if (!InNode)
		{
			return;
		}
		ImGui::PushID(*InNode->GetPath());

		const BOOL32 bIsCurrent = (NormalizePath(InNode->GetPath()) == NormalizePath(CurrentFolderPath)) ? TRUE : FALSE;
		ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (InNode->GetChildrenFolder().IsEmpty())
		{
			Flags |= ImGuiTreeNodeFlags_Leaf;
		}
		if (bIsCurrent != FALSE)
		{
			Flags |= ImGuiTreeNodeFlags_Selected;
		}
		if (bInIsRoot != FALSE)
		{
			Flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		const EString Label = (bInIsRoot != FALSE) ? InRootLabel : InNode->GetDisplayName();
		const BOOL8 bOpen = ImGui::TreeNodeEx("##folder_node", Flags, "%s", *Label);
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			SetCurrentFolder(InNode->GetPath());
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (ImGui::AcceptDragDropPayload(ContentAssetDragPayload) != nullptr)
			{
				MoveDragSelectionToFolder(InNode->GetPath());
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginPopupContextItem("##folder_context"))
		{
			if (ImGui::MenuItem("New Folder"))
			{
				FolderTargetPath = InNode->GetPath();
				::snprintf(FolderNameBuffer, sizeof(FolderNameBuffer), "%s", "NewFolder");
				DialogError = EString();
				bNewFolderOpen = TRUE;
			}
			if (bInIsRoot == FALSE)
			{
				if (ImGui::MenuItem("Rename Folder"))
				{
					RequestRenameFolder(InNode->GetPath());
				}
				if (ImGui::MenuItem("Delete Folder"))
				{
					RequestDeleteFolder(InNode->GetPath());
				}
			}
			if (ImGui::MenuItem("Show in Explorer"))
			{
				RevealInExplorer(InNode->GetPath());
			}
			ImGui::EndPopup();
		}

		if (bOpen != FALSE)
		{
			const TArray<TSharedPtr<FAssetFolderNode>>& Children = InNode->GetChildrenFolder();
			for (INT32 Index = 0, ChildNum = Children.Num<INT32>(); Index < ChildNum; Index++)
			{
				DrawFolderNode(Children[Index], InNode->GetPath(), FALSE, EString());
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void FContentBrowserPanel::DrawSelectedSummary()
	{
		const TSharedPtr<FAssetFolderNode> CurrentFolder = Host->GetCatalog().FindFolderByPath(CurrentFolderPath);
		const TSharedPtr<FAssetFileInfo> Primary = FindPrimaryFile(CurrentFolder);
		if (!Primary)
		{
			if (SelectedPaths.Num<INT32>() > 0)
			{
				ImGui::TextDisabled("%d assets selected.", SelectedPaths.Num<INT32>());
			}
			else
			{
				ImGui::TextDisabled("Select an asset to see its actions.");
			}
			return;
		}

		RefreshUsersCache();
		ImGui::Text("%s", *Primary->GetDisplayName());
		ImGui::SameLine();
		ImGui::TextDisabled("(%s)", EAssetTypeUtil::ToString(Primary->GetType()));
		if (SelectedPaths.Num<INT32>() > 1)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("and %d more", SelectedPaths.Num<INT32>() - 1);
		}

		ImGui::SameLine();
		ImGui::BeginDisabled(IsOpenableAssetType(Primary->GetType()) ? FALSE : TRUE);
		if (ImGui::Button("Open"))
		{
			OpenAsset(Primary->GetPath(), Primary->GetType());
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(IsManagedAssetType(Primary->GetType()) ? FALSE : TRUE);
		if (ImGui::Button("Rename"))
		{
			RequestRenameAsset(Primary->GetPath(), Primary->GetType());
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Delete"))
		{
			RequestDeleteSelection();
		}

		if (!UsersCache.IsEmpty())
		{
			const EString UserText = JoinStrings(UsersCache);
			ImGui::TextWrapped("Referenced by loaded assets: %s", *UserText);
		}
		else if (IsManagedAssetType(Primary->GetType()) != FALSE)
		{
			ImGui::TextDisabled("No loaded asset references this asset.");
		}
		else
		{
			ImGui::TextDisabled("Reference tracking is not available for this asset type.");
		}
		ImGui::TextDisabled("%s", *Primary->GetPath());
	}

	void FContentBrowserPanel::DrawAssetGrid(const TSharedPtr<FAssetFolderNode>& InCurrent)
	{
		TArray<TSharedPtr<FAssetFileInfo>> VisibleFiles;
		RebuildVisibleFiles(InCurrent, VisibleFiles);

		const FLOAT TileWidth = TileSize + 24.0f;
		const FLOAT TileHeight = TileSize + 48.0f;
		const FLOAT WindowRight = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		const ImGuiStyle& Style = ImGui::GetStyle();

		const TArray<TSharedPtr<FAssetFolderNode>>& Folders = InCurrent->GetChildrenFolder();
		if (Folders.IsEmpty() && VisibleFiles.IsEmpty())
		{
			ImGui::TextDisabled("This folder is empty.");
		}
		for (INT32 Index = 0, FolderNum = Folders.Num<INT32>(); Index < FolderNum; Index++)
		{
			DrawFolderTile(Folders[Index], TileWidth, TileHeight);
			const FLOAT LastX = ImGui::GetItemRectMax().x;
			if (LastX + Style.ItemSpacing.x + TileWidth < WindowRight)
			{
				ImGui::SameLine();
			}
		}
		for (INT32 Index = 0, FileNum = VisibleFiles.Num<INT32>(); Index < FileNum; Index++)
		{
			DrawAssetTile(VisibleFiles[Index], VisibleFiles, Index, TileWidth, TileHeight);
			const FLOAT LastX = ImGui::GetItemRectMax().x;
			if (LastX + Style.ItemSpacing.x + TileWidth < WindowRight)
			{
				ImGui::SameLine();
			}
		}

		if (ImGui::BeginPopupContextWindow("##cb_grid_context"
			, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("New Folder"))
			{
				FolderTargetPath = InCurrent->GetPath();
				::snprintf(FolderNameBuffer, sizeof(FolderNameBuffer), "%s", "NewFolder");
				DialogError = EString();
				bNewFolderOpen = TRUE;
			}
			if (ImGui::MenuItem("Import"))
			{
				EImportManager::GetManagerSingleton()->RequestImportDialog();
			}
			if (ImGui::MenuItem("Refresh"))
			{
				Host->RequestContentRefresh();
			}
			ImGui::EndPopup();
		}
	}

	void FContentBrowserPanel::DrawFolderTile(const TSharedPtr<FAssetFolderNode>& InFolder
		, const FLOAT InTileWidth, const FLOAT InTileHeight)
	{
		if (!InFolder)
		{
			return;
		}
		ImGui::PushID(*InFolder->GetPath());
		ImGui::BeginChild("##folder_tile", ImVec2(InTileWidth, InTileHeight), FALSE
			, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::SetNextItemAllowOverlap();
		const BOOL8 bSelected = (NormalizePath(InFolder->GetPath()) == NormalizePath(CurrentFolderPath)) ? TRUE : FALSE;
		if (ImGui::Selectable("##folder_tile_select", bSelected != FALSE, ImGuiSelectableFlags_None
			, ImVec2(InTileWidth, InTileHeight)))
		{
			SetCurrentFolder(InFolder->GetPath());
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (ImGui::AcceptDragDropPayload(ContentAssetDragPayload) != nullptr)
			{
				MoveDragSelectionToFolder(InFolder->GetPath());
			}
			ImGui::EndDragDropTarget();
		}
		if (ImGui::BeginPopupContextItem("##folder_tile_context"))
		{
			if (ImGui::MenuItem("New Folder"))
			{
				FolderTargetPath = InFolder->GetPath();
				::snprintf(FolderNameBuffer, sizeof(FolderNameBuffer), "%s", "NewFolder");
				DialogError = EString();
				bNewFolderOpen = TRUE;
			}
			if (ImGui::MenuItem("Rename Folder"))
			{
				RequestRenameFolder(InFolder->GetPath());
			}
			if (ImGui::MenuItem("Delete Folder"))
			{
				RequestDeleteFolder(InFolder->GetPath());
			}
			ImGui::EndPopup();
		}

		const ImTextureID Icon = Host->GetIcons().GetFolderIcon();
		const FLOAT IconSize = TileSize;
		ImGui::SetCursorPos(ImVec2((InTileWidth - IconSize) * 0.5f, 4.0f));
		if (Icon != static_cast<ImTextureID>(0))
		{
			ImGui::Image(Icon, ImVec2(IconSize, IconSize));
		}
		else
		{
			ImGui::Dummy(ImVec2(IconSize, IconSize));
		}
		ImGui::SetCursorPos(ImVec2(6.0f, IconSize + 10.0f));
		ImGui::TextWrapped("%s", *InFolder->GetDisplayName());
		ImGui::EndChild();
		ImGui::PopID();
	}

	void FContentBrowserPanel::DrawAssetTile(const TSharedPtr<FAssetFileInfo>& InFile
		, const TArray<TSharedPtr<FAssetFileInfo>>& InVisibleFiles, const INT32 InFileIndex
		, const FLOAT InTileWidth, const FLOAT InTileHeight)
	{
		if (!InFile)
		{
			return;
		}
		const EString& Path = InFile->GetPath();
		ImGui::PushID(*Path);
		ImGui::BeginChild("##asset_tile", ImVec2(InTileWidth, InTileHeight), FALSE
			, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::SetNextItemAllowOverlap();
		const BOOL8 bSelected = IsSelected(Path);
		const BOOL8 bClicked = ImGui::Selectable("##asset_tile_select", bSelected != FALSE
			, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(InTileWidth, InTileHeight));
		// The click lands on release, but ImGui records the double click on the second
		// press; read it before the overlay image replaces the last item.
		const BOOL8 bDoubleClicked = ((ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByItem)
			&& ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) ? TRUE : FALSE);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && (bSelected == FALSE))
		{
			SelectOnly(Path);
		}
		if (ImGui::BeginPopupContextItem("##asset_tile_context"))
		{
			ImGui::BeginDisabled(IsOpenableAssetType(InFile->GetType()) ? FALSE : TRUE);
			if (ImGui::MenuItem("Open"))
			{
				OpenAsset(Path, InFile->GetType());
			}
			ImGui::EndDisabled();
			ImGui::BeginDisabled(IsManagedAssetType(InFile->GetType()) ? FALSE : TRUE);
			if (ImGui::MenuItem("Rename"))
			{
				RequestRenameAsset(Path, InFile->GetType());
			}
			ImGui::EndDisabled();
			if (ImGui::MenuItem("Delete"))
			{
				if (IsSelected(Path) == FALSE)
				{
					SelectOnly(Path);
				}
				RequestDeleteSelection();
			}
			if (ImGui::MenuItem("Copy Path"))
			{
				ImGui::SetClipboardText(*Path);
			}
			if (ImGui::MenuItem("Show in Explorer"))
			{
				RevealInExplorer(Path);
			}
			ImGui::EndPopup();
		}
		if (ImGui::BeginDragDropSource())
		{
			if (IsSelected(Path) == FALSE)
			{
				SelectOnly(Path);
			}
			DragPaths = SelectedPaths;
			const UINT8 PayloadMarker = 1u;
			ImGui::SetDragDropPayload(ContentAssetDragPayload, &PayloadMarker, sizeof(PayloadMarker));
			ImGui::TextUnformatted(*InFile->GetDisplayName());
			ImGui::EndDragDropSource();
		}

		const ImTextureID Icon = Host->GetIcons().GetTypeIcon(InFile->GetType());
		const FLOAT IconSize = TileSize;
		ImGui::SetCursorPos(ImVec2((InTileWidth - IconSize) * 0.5f, 4.0f));
		if (Icon != static_cast<ImTextureID>(0))
		{
			ImGui::Image(Icon, ImVec2(IconSize, IconSize));
		}
		else
		{
			ImGui::Dummy(ImVec2(IconSize, IconSize));
		}
		ImGui::SetCursorPos(ImVec2(6.0f, IconSize + 10.0f));
		ImGui::TextWrapped("%s", *InFile->GetDisplayName());
		ImGui::EndChild();
		ImGui::PopID();

		if (bDoubleClicked != FALSE)
		{
			SelectOnly(Path);
			LastClickedFileIndex = InFileIndex;
			OpenAsset(Path, InFile->GetType());
		}
		else if (bClicked != FALSE)
		{
			if (ImGui::GetIO().KeyCtrl != false)
			{
				ToggleSelection(Path);
			}
			else if (ImGui::GetIO().KeyShift != false)
			{
				SelectRange(InVisibleFiles, InFileIndex);
			}
			else
			{
				SelectOnly(Path);
			}
			LastClickedFileIndex = InFileIndex;
		}
	}

	void FContentBrowserPanel::RebuildVisibleFiles(const TSharedPtr<FAssetFolderNode>& InCurrent
		, TArray<TSharedPtr<FAssetFileInfo>>& OutFiles) const
	{
		OutFiles.Empty();
		if (!InCurrent)
		{
			return;
		}
		const EString SearchLower = EString(SearchBuffer).ToLower();
		const TArray<TSharedPtr<FAssetFileInfo>>& Files = InCurrent->GetChildrenFile();
		for (INT32 Index = 0, FileNum = Files.Num<INT32>(); Index < FileNum; Index++)
		{
			const TSharedPtr<FAssetFileInfo>& File = Files[Index];
			if ((TypeFilterIndex > 0) && (static_cast<INT32>(File->GetType()) != TypeFilterIndex))
			{
				continue;
			}
			if (SearchLower.Length() > 0u)
			{
				const EString TypeLabel = EString(EAssetTypeUtil::ToString(File->GetType())).ToLower();
				if ((File->GetSearchName().Contains(SearchLower) == FALSE)
					&& (TypeLabel.Contains(SearchLower) == FALSE))
				{
					continue;
				}
			}
			OutFiles.Add(File);
		}
	}

	void FContentBrowserPanel::ProcessPendingOpen()
	{
		if (bHasPendingOpen == FALSE)
		{
			return;
		}
		const EString AssetPath = PendingOpenPath;
		const EAssetType AssetType = PendingOpenType;
		PendingOpenPath = EString();
		PendingOpenType = ASSET_TYPE_UNKNOWN;
		bHasPendingOpen = FALSE;

		EAnimationWorkspace* Workspace = Host ? Host->GetAnimationWorkspace() : nullptr;
		if (!Workspace || (AssetPath.Length() == 0u))
		{
			return;
		}
		switch (AssetType)
		{
		case ASSET_TYPE_ANIMATION:
			Workspace->OpenAnimationAsset(AssetPath);
			break;
		case ASSET_TYPE_ANIM_STATE_MACHINE:
			Workspace->OpenStateMachineAsset(AssetPath);
			break;
		case ASSET_TYPE_ANIM_MONTAGE:
			Workspace->OpenMontageAsset(AssetPath);
			break;
		case ASSET_TYPE_ANIM_NOTIFY:
			Workspace->OpenNotifyAsset(AssetPath);
			break;
		case ASSET_TYPE_SKELETON:
			Workspace->OpenSlotGroupAsset(AssetPath);
			break;
		default:
			break;
		}
	}

	void FContentBrowserPanel::OpenAsset(const EString& InAssetPath, const EAssetType& InType)
	{
		if (!Host)
		{
			return;
		}
		if (InType == ASSET_TYPE_MESH)
		{
			if (Host->GetMeshEditor())
			{
				Host->GetMeshEditor()->OpenMeshAsset(InAssetPath);
			}
			return;
		}
		if ((InType == ASSET_TYPE_ANIMATION) || (InType == ASSET_TYPE_ANIM_STATE_MACHINE)
			|| (InType == ASSET_TYPE_ANIM_MONTAGE) || (InType == ASSET_TYPE_ANIM_NOTIFY)
			|| (InType == ASSET_TYPE_SKELETON))
		{
			if (!Host->GetAnimationWorkspace())
			{
				ShowError("The animation workspace is not available.");
				return;
			}
			PendingOpenPath = InAssetPath;
			PendingOpenType = InType;
			bHasPendingOpen = TRUE;
			return;
		}
		ShowError("This asset type has no editor.");
	}

	void FContentBrowserPanel::SetCurrentFolder(const EString& InFolderPath)
	{
		const EString Normalized = NormalizePath(InFolderPath);
		if (NormalizePath(CurrentFolderPath) == Normalized)
		{
			return;
		}
		CurrentFolderPath = InFolderPath;
		ClearSelection();
	}

	void FContentBrowserPanel::EnsureCurrentFolder()
	{
		if ((CurrentFolderPath.Length() > 0u) && Host->GetCatalog().FindFolderByPath(CurrentFolderPath))
		{
			return;
		}
		const TSharedPtr<FAssetFolderNode> DefaultFolder = Host->GetCatalog().GetDefaultFolder();
		CurrentFolderPath = DefaultFolder ? DefaultFolder->GetPath() : EString();
		ClearSelection();
	}

	void FContentBrowserPanel::SelectOnly(const EString& InPath)
	{
		SelectedPaths.Empty();
		if (InPath.Length() > 0u)
		{
			SelectedPaths.Add(InPath);
		}
		PrimaryPath = InPath;
		UsersCachePath = EString();
		UsersCache.Empty();
	}

	void FContentBrowserPanel::ToggleSelection(const EString& InPath)
	{
		const UINT32 FoundIndex = SelectedPaths.Find(InPath);
		if (FoundIndex != static_cast<UINT32>(-1))
		{
			SelectedPaths.RemoveAt(FoundIndex);
			if (PrimaryPath == InPath)
			{
				PrimaryPath = SelectedPaths.IsEmpty() ? EString() : SelectedPaths.LastRef();
			}
		}
		else
		{
			SelectedPaths.Add(InPath);
			PrimaryPath = InPath;
		}
		UsersCachePath = EString();
		UsersCache.Empty();
	}

	void FContentBrowserPanel::SelectRange(const TArray<TSharedPtr<FAssetFileInfo>>& InVisibleFiles
		, const INT32 InFileIndex)
	{
		if ((InFileIndex < 0) || (InFileIndex >= InVisibleFiles.Num<INT32>()))
		{
			return;
		}
		INT32 StartIndex = LastClickedFileIndex;
		if ((StartIndex < 0) || (StartIndex >= InVisibleFiles.Num<INT32>()))
		{
			StartIndex = InFileIndex;
		}
		const INT32 MinIndex = (StartIndex < InFileIndex) ? StartIndex : InFileIndex;
		const INT32 MaxIndex = (StartIndex < InFileIndex) ? InFileIndex : StartIndex;
		for (INT32 Index = MinIndex; Index <= MaxIndex; Index++)
		{
			const EString& Path = InVisibleFiles[Index]->GetPath();
			if (IsSelected(Path) == FALSE)
			{
				SelectedPaths.Add(Path);
			}
		}
		PrimaryPath = InVisibleFiles[InFileIndex]->GetPath();
		UsersCachePath = EString();
		UsersCache.Empty();
	}

	void FContentBrowserPanel::ClearSelection()
	{
		SelectedPaths.Empty();
		PrimaryPath = EString();
		LastClickedFileIndex = -1;
		UsersCachePath = EString();
		UsersCache.Empty();
	}

	void FContentBrowserPanel::PruneSelection()
	{
		const TSharedPtr<FAssetFolderNode> CurrentFolder = Host->GetCatalog().FindFolderByPath(CurrentFolderPath);
		if (!CurrentFolder)
		{
			ClearSelection();
			return;
		}
		TArray<EString> ValidPaths;
		const TArray<TSharedPtr<FAssetFileInfo>>& Files = CurrentFolder->GetChildrenFile();
		for (INT32 Index = 0, FileNum = Files.Num<INT32>(); Index < FileNum; Index++)
		{
			if (IsSelected(Files[Index]->GetPath()) != FALSE)
			{
				ValidPaths.Add(Files[Index]->GetPath());
			}
		}
		SelectedPaths.Empty();
		SelectedPaths.Append(ValidPaths);
		if ((PrimaryPath.Length() > 0u) && (IsSelected(PrimaryPath) == FALSE))
		{
			PrimaryPath = SelectedPaths.IsEmpty() ? EString() : SelectedPaths.LastRef();
		}
		LastClickedFileIndex = -1;
		UsersCachePath = EString();
		UsersCache.Empty();
	}

	BOOL32 FContentBrowserPanel::IsSelected(const EString& InPath) const
	{
		return (SelectedPaths.Find(InPath) == static_cast<UINT32>(-1)) ? FALSE : TRUE;
	}

	TSharedPtr<FAssetFileInfo> FContentBrowserPanel::FindPrimaryFile(const TSharedPtr<FAssetFolderNode>& InCurrent) const
	{
		if (!InCurrent || (PrimaryPath.Length() == 0u))
		{
			return nullptr;
		}
		const TArray<TSharedPtr<FAssetFileInfo>>& Files = InCurrent->GetChildrenFile();
		for (INT32 Index = 0, FileNum = Files.Num<INT32>(); Index < FileNum; Index++)
		{
			if (Files[Index]->GetPath() == PrimaryPath)
			{
				return Files[Index];
			}
		}
		return nullptr;
	}

	EAssetType FContentBrowserPanel::FindTypeForPath(const EString& InPath) const
	{
		const TSharedPtr<FAssetFolderNode> CurrentFolder = Host ? Host->GetCatalog().FindFolderByPath(CurrentFolderPath) : nullptr;
		if (CurrentFolder)
		{
			const TArray<TSharedPtr<FAssetFileInfo>>& Files = CurrentFolder->GetChildrenFile();
			for (INT32 Index = 0, FileNum = Files.Num<INT32>(); Index < FileNum; Index++)
			{
				if (Files[Index]->GetPath() == InPath)
				{
					return Files[Index]->GetType();
				}
			}
		}
		return EAssetTypeUtil::ReadTypeFromFile(InPath);
	}

	void FContentBrowserPanel::RefreshUsersCache()
	{
		if (PrimaryPath == UsersCachePath)
		{
			return;
		}
		UsersCachePath = PrimaryPath;
		UsersCache.Empty();
		if (PrimaryPath.Length() == 0u)
		{
			return;
		}
		const EAssetType Type = FindTypeForPath(PrimaryPath);
		if (IsManagedAssetType(Type) != FALSE)
		{
			EAssetUsage::CollectUsers(PrimaryPath, Type, UsersCache);
		}
	}

	BOOL32 FContentBrowserPanel::CanModifyAsset(const EString& InAssetPath, const EAssetType& InType
		, EString& OutReason, TArray<EString>* OutUsers) const
	{
		OutReason = EString();
		if (OutUsers)
		{
			OutUsers->Empty();
		}
		if (IsManagedAssetType(InType) == FALSE)
		{
			OutReason = EString("Reference tracking is not available for this asset type.");
			return FALSE;
		}
		if (Host && Host->GetAnimationWorkspace() && (Host->GetAnimationWorkspace()->IsAssetOpen(InAssetPath) != FALSE))
		{
			OutReason = EString("Close the asset in the Workspace before modifying it.");
			return FALSE;
		}
		TArray<EString> Users;
		EAssetUsage::CollectUsers(InAssetPath, InType, Users);
		if (OutUsers)
		{
			*OutUsers = Users;
		}
		if (!Users.IsEmpty())
		{
			OutReason = EString("Referenced by loaded assets.");
			return FALSE;
		}
		return TRUE;
	}

	BOOL32 FContentBrowserPanel::FolderHasBlockedAssets(const TSharedPtr<FAssetFolderNode>& InFolder
		, TArray<EString>& OutReasons) const
	{
		if (!InFolder)
		{
			return FALSE;
		}
		BOOL32 bBlocked = FALSE;
		const TArray<TSharedPtr<FAssetFileInfo>>& Files = InFolder->GetChildrenFile();
		for (INT32 Index = 0, FileNum = Files.Num<INT32>(); Index < FileNum; Index++)
		{
			const TSharedPtr<FAssetFileInfo>& File = Files[Index];
			EString Reason;
			TArray<EString> Users;
			if (IsManagedAssetType(File->GetType()) != FALSE)
			{
				if (CanModifyAsset(File->GetPath(), File->GetType(), Reason, &Users) == FALSE)
				{
					bBlocked = TRUE;
					EString Text = File->GetDisplayName() + EString(": ") + Reason;
					if (!Users.IsEmpty())
					{
						Text += EString(" Used by: ") + JoinStrings(Users);
					}
					OutReasons.Add(Text);
				}
			}
			else if (Host && Host->GetAnimationWorkspace()
				&& (Host->GetAnimationWorkspace()->IsAssetOpen(File->GetPath()) != FALSE))
			{
				bBlocked = TRUE;
				OutReasons.Add(File->GetDisplayName() + EString(": open in the Workspace."));
			}
		}
		const TArray<TSharedPtr<FAssetFolderNode>>& Children = InFolder->GetChildrenFolder();
		for (INT32 Index = 0, ChildNum = Children.Num<INT32>(); Index < ChildNum; Index++)
		{
			if (FolderHasBlockedAssets(Children[Index], OutReasons) != FALSE)
			{
				bBlocked = TRUE;
			}
		}
		return bBlocked;
	}

	void FContentBrowserPanel::RequestRenameAsset(const EString& InPath, const EAssetType& InType)
	{
		if (IsManagedAssetType(InType) == FALSE)
		{
			ShowError("Reference tracking is not available for this asset type, so rename is disabled.");
			return;
		}
		RenameTargetPath = InPath;
		RenameTargetType = InType;
		const EString Stem = EPath::GetFileNameWithoutExtension(InPath);
		::snprintf(RenameBuffer, sizeof(RenameBuffer), "%s", *Stem);
		DialogError = EString();
		bRenameAssetOpen = TRUE;
	}

	void FContentBrowserPanel::PerformRenameAsset()
	{
		EString Reason;
		if (CanModifyAsset(RenameTargetPath, RenameTargetType, Reason) == FALSE)
		{
			DialogError = Reason;
			return;
		}
		EString NewPath;
		EString Error;
		if (EAssetFileOperations::RenameAssetFile(RenameTargetPath, EString(RenameBuffer), NewPath, Error) == FALSE)
		{
			DialogError = Error;
			return;
		}
		Host->GetCatalog().Refresh();
		const EString NewFolder = GetParentFolder(NewPath);
		CurrentFolderPath = NewFolder;
		SelectOnly(NewPath);
		SetStatus(EString("Renamed to ") + EPath::GetFileNameWithoutExtension(NewPath), FALSE);
		DialogError = EString();
		ImGui::CloseCurrentPopup();
	}

	void FContentBrowserPanel::RequestDeleteSelection()
	{
		if (SelectedPaths.IsEmpty())
		{
			ShowError("Select an asset first.");
			return;
		}
		DeleteTargets.Empty();
		DeleteTargetReasons.Empty();
		bDeleteBlocked = FALSE;
		for (INT32 Index = 0, PathNum = SelectedPaths.Num<INT32>(); Index < PathNum; Index++)
		{
			const EString& Path = SelectedPaths[Index];
			const EAssetType Type = FindTypeForPath(Path);
			EString Reason;
			TArray<EString> Users;
			if (CanModifyAsset(Path, Type, Reason, &Users) == FALSE)
			{
				bDeleteBlocked = TRUE;
				EString Text = Path + EString(": ") + Reason;
				if (!Users.IsEmpty())
				{
					Text += EString(" Used by: ") + JoinStrings(Users);
				}
				DeleteTargetReasons.Add(Text);
			}
			else
			{
				DeleteTargets.Add(Path);
			}
		}
		DialogError = EString();
		bDeleteAssetOpen = TRUE;
	}

	void FContentBrowserPanel::PerformDeleteAssets()
	{
		if (bDeleteBlocked != FALSE)
		{
			return;
		}
		EString FirstError;
		INT32 DeletedNum = 0;
		for (INT32 Index = 0, TargetNum = DeleteTargets.Num<INT32>(); Index < TargetNum; Index++)
		{
			EString Error;
			if (EAssetFileOperations::DeleteAssetFile(DeleteTargets[Index], Error))
			{
				DeletedNum++;
			}
			else if (FirstError.Length() == 0u)
			{
				FirstError = Error;
			}
		}
		Host->GetCatalog().Refresh();
		ClearSelection();
		if (FirstError.Length() > 0u)
		{
			DialogError = FirstError;
			SetStatus(EString("Deleted ") + EString::FromInt(static_cast<UINT32>(DeletedNum))
				+ EString(" asset(s); some failed."), TRUE);
			return;
		}
		SetStatus(EString("Deleted ") + EString::FromInt(static_cast<UINT32>(DeletedNum)) + EString(" asset(s)."), FALSE);
		DeleteTargets.Empty();
		DeleteTargetReasons.Empty();
		ImGui::CloseCurrentPopup();
	}

	void FContentBrowserPanel::MoveDragSelectionToFolder(const EString& InDestinationFolder)
	{
		if (DragPaths.IsEmpty())
		{
			return;
		}
		const EString Destination = NormalizePath(InDestinationFolder);
		TArray<EString> MovablePaths;
		TArray<EString> Problems;
		for (INT32 Index = 0, PathNum = DragPaths.Num<INT32>(); Index < PathNum; Index++)
		{
			const EString& Path = DragPaths[Index];
			if (GetParentFolder(Path) == Destination)
			{
				Problems.Add(Path + EString(": already in that folder."));
				continue;
			}
			const EAssetType Type = FindTypeForPath(Path);
			EString Reason;
			if (CanModifyAsset(Path, Type, Reason) == FALSE)
			{
				Problems.Add(Path + EString(": ") + Reason);
				continue;
			}
			MovablePaths.Add(Path);
		}
		if (!Problems.IsEmpty())
		{
			ShowError(EString("Cannot move all selected assets:\n") + JoinStrings(Problems));
			DragPaths.Empty();
			return;
		}

		EString FirstError;
		EString LastNewPath;
		INT32 MovedNum = 0;
		for (INT32 Index = 0, PathNum = MovablePaths.Num<INT32>(); Index < PathNum; Index++)
		{
			EString NewPath;
			EString Error;
			if (EAssetFileOperations::MoveAssetFile(MovablePaths[Index], Destination, NewPath, Error))
			{
				MovedNum++;
				LastNewPath = NewPath;
			}
			else if (FirstError.Length() == 0u)
			{
				FirstError = Error;
			}
		}
		Host->GetCatalog().Refresh();
		ClearSelection();
		if (FirstError.Length() > 0u)
		{
			SetStatus(FirstError, TRUE);
		}
		else
		{
			SetStatus(EString("Moved ") + EString::FromInt(static_cast<UINT32>(MovedNum)) + EString(" asset(s)."), FALSE);
			if ((NormalizePath(CurrentFolderPath) == Destination) && (LastNewPath.Length() > 0u))
			{
				SelectOnly(LastNewPath);
			}
		}
		DragPaths.Empty();
	}

	void FContentBrowserPanel::RequestRenameFolder(const EString& InFolderPath)
	{
		FolderTargetPath = InFolderPath;
		const EString Name(EPath::GetFileNameWithExtension(InFolderPath));
		::snprintf(FolderNameBuffer, sizeof(FolderNameBuffer), "%s", *Name);
		DialogError = EString();
		bRenameFolderOpen = TRUE;
	}

	void FContentBrowserPanel::PerformRenameFolder()
	{
		TArray<EString> Reasons;
		const TSharedPtr<FAssetFolderNode> Folder = Host->GetCatalog().FindFolderByPath(FolderTargetPath);
		if (FolderHasBlockedAssets(Folder, Reasons) != FALSE)
		{
			DialogError = EString("Folder contains assets that are open or referenced: ") + JoinStrings(Reasons);
			return;
		}
		EString NewPath;
		EString Error;
		if (EAssetFileOperations::RenameFolder(FolderTargetPath, EString(FolderNameBuffer), NewPath, Error) == FALSE)
		{
			DialogError = Error;
			return;
		}
		const EString OldPath = NormalizePath(FolderTargetPath);
		const EString RenamedPath = NormalizePath(NewPath);
		Host->GetCatalog().Refresh();
		if (NormalizePath(CurrentFolderPath).StartWith(OldPath) != FALSE)
		{
			CurrentFolderPath = RenamedPath + NormalizePath(CurrentFolderPath).Substring(OldPath.Length()
				, NormalizePath(CurrentFolderPath).Length() - OldPath.Length());
		}
		ClearSelection();
		SetStatus(EString("Renamed folder to ") + EString(FolderNameBuffer) + EString("."), FALSE);
		ImGui::CloseCurrentPopup();
	}

	void FContentBrowserPanel::RequestDeleteFolder(const EString& InFolderPath)
	{
		FolderTargetPath = InFolderPath;
		FolderDeleteReasons.Empty();
		const TSharedPtr<FAssetFolderNode> Folder = Host->GetCatalog().FindFolderByPath(InFolderPath);
		bFolderDeleteBlocked = FolderHasBlockedAssets(Folder, FolderDeleteReasons);
		DialogError = EString();
		bDeleteFolderOpen = TRUE;
	}

	void FContentBrowserPanel::PerformDeleteFolder()
	{
		if (bFolderDeleteBlocked != FALSE)
		{
			return;
		}
		EString Error;
		if (EAssetFileOperations::DeleteFolder(FolderTargetPath, Error) == FALSE)
		{
			DialogError = Error;
			return;
		}
		const EString Parent = GetParentFolder(FolderTargetPath);
		Host->GetCatalog().Refresh();
		CurrentFolderPath = Parent;
		ClearSelection();
		SetStatus(EString("Deleted folder."), FALSE);
		ImGui::CloseCurrentPopup();
	}

	void FContentBrowserPanel::DrawDialogs()
	{
		DrawRenameAssetDialog();
		DrawDeleteAssetDialog();
		DrawFolderDialogs();
		DrawErrorPopup();
	}

	void FContentBrowserPanel::DrawRenameAssetDialog()
	{
		if (bRenameAssetOpen != FALSE)
		{
			bRenameAssetOpen = FALSE;
			ImGui::OpenPopup("##cb_rename_asset");
		}
		if (ImGui::IsPopupOpen("##cb_rename_asset") != false)
		{
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		}
		if (ImGui::BeginPopupModal("##cb_rename_asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Rename Asset");
			ImGui::SetNextItemWidth(360.0f);
			ImGui::InputText("##cb_rename_name", RenameBuffer, sizeof(RenameBuffer));
			ImGui::TextDisabled(".PAsset is appended automatically.");
			if (DialogError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *DialogError);
			}
			if (ImGui::Button("Rename", ImVec2(120.0f, 0.0f)))
			{
				PerformRenameAsset();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			{
				DialogError = EString();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void FContentBrowserPanel::DrawDeleteAssetDialog()
	{
		if (bDeleteAssetOpen != FALSE)
		{
			bDeleteAssetOpen = FALSE;
			ImGui::OpenPopup("##cb_delete_asset");
		}
		if (ImGui::IsPopupOpen("##cb_delete_asset") != false)
		{
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		}
		if (ImGui::BeginPopupModal("##cb_delete_asset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Delete %d asset(s)?", DeleteTargets.Num<INT32>() + DeleteTargetReasons.Num<INT32>());
			if (!DeleteTargetReasons.IsEmpty())
			{
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.35f, 1.0f)
					, "These assets cannot be deleted:");
				const EString Reasons = JoinStrings(DeleteTargetReasons);
				ImGui::TextWrapped("%s", *Reasons);
			}
			if (DialogError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *DialogError);
			}
			ImGui::BeginDisabled(bDeleteBlocked != FALSE);
			if (ImGui::Button("Delete", ImVec2(120.0f, 0.0f)))
			{
				PerformDeleteAssets();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			{
				DialogError = EString();
				DeleteTargets.Empty();
				DeleteTargetReasons.Empty();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void FContentBrowserPanel::DrawFolderDialogs()
	{
		if (bNewFolderOpen != FALSE)
		{
			bNewFolderOpen = FALSE;
			ImGui::OpenPopup("##cb_new_folder");
		}
		if (ImGui::IsPopupOpen("##cb_new_folder") != false)
		{
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		}
		if (ImGui::BeginPopupModal("##cb_new_folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("New Folder");
			ImGui::SetNextItemWidth(360.0f);
			ImGui::InputText("##cb_new_folder_name", FolderNameBuffer, sizeof(FolderNameBuffer));
			if (DialogError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *DialogError);
			}
			if (ImGui::Button("Create", ImVec2(120.0f, 0.0f)))
			{
				EString NewPath;
				EString Error;
				if (EAssetFileOperations::CreateFolder(FolderTargetPath, EString(FolderNameBuffer), NewPath, Error))
				{
					Host->GetCatalog().Refresh();
					SetStatus(EString("Created folder ") + EString(FolderNameBuffer) + EString("."), FALSE);
					DialogError = EString();
					ImGui::CloseCurrentPopup();
				}
				else
				{
					DialogError = Error;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			{
				DialogError = EString();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (bRenameFolderOpen != FALSE)
		{
			bRenameFolderOpen = FALSE;
			ImGui::OpenPopup("##cb_rename_folder");
		}
		if (ImGui::IsPopupOpen("##cb_rename_folder") != false)
		{
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		}
		if (ImGui::BeginPopupModal("##cb_rename_folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Rename Folder");
			ImGui::SetNextItemWidth(360.0f);
			ImGui::InputText("##cb_rename_folder_name", FolderNameBuffer, sizeof(FolderNameBuffer));
			if (DialogError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *DialogError);
			}
			if (ImGui::Button("Rename", ImVec2(120.0f, 0.0f)))
			{
				PerformRenameFolder();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			{
				DialogError = EString();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (bDeleteFolderOpen != FALSE)
		{
			bDeleteFolderOpen = FALSE;
			ImGui::OpenPopup("##cb_delete_folder");
		}
		if (ImGui::IsPopupOpen("##cb_delete_folder") != false)
		{
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		}
		if (ImGui::BeginPopupModal("##cb_delete_folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Delete this folder and everything inside it?");
			if (!FolderDeleteReasons.IsEmpty())
			{
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.35f, 1.0f)
					, "The folder contains assets that cannot be removed:");
				const EString Reasons = JoinStrings(FolderDeleteReasons);
				ImGui::TextWrapped("%s", *Reasons);
			}
			if (DialogError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *DialogError);
			}
			ImGui::BeginDisabled(bFolderDeleteBlocked != FALSE);
			if (ImGui::Button("Delete", ImVec2(120.0f, 0.0f)))
			{
				PerformDeleteFolder();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
			{
				DialogError = EString();
				FolderDeleteReasons.Empty();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void FContentBrowserPanel::DrawErrorPopup()
	{
		if (bErrorPopupOpen != FALSE)
		{
			bErrorPopupOpen = FALSE;
			ImGui::OpenPopup("##cb_error");
		}
		if (ImGui::IsPopupOpen("##cb_error") != false)
		{
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		}
		if (ImGui::BeginPopupModal("##cb_error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextWrapped("%s", *ErrorPopupMessage);
			if (ImGui::Button("OK", ImVec2(120.0f, 0.0f)))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void FContentBrowserPanel::ShowError(const EString& InMessage)
	{
		ErrorPopupMessage = InMessage;
		bErrorPopupOpen = TRUE;
		SetStatus(InMessage, TRUE);
	}

	void FContentBrowserPanel::SetStatus(const EString& InMessage, const BOOL32 bInIsError)
	{
		StatusMessage = InMessage;
		bStatusIsError = bInIsError;
	}

	void FContentBrowserPanel::OpenNewAssetDialog(const EAssetType& InType)
	{
		NewAssetType = InType;
		bResetNewAssetWindowPos = TRUE;
		bFocusNewAssetWindow = TRUE;
		bNewAssetWindowOpen = TRUE;
		NewAssetError = EString();
		NewAssetAnimationNames.Empty();
		NewAssetAnimationIndex = -1;
		NewAssetClipNames.Empty();
		NewAssetClipIndex = -1;
		NewAssetSkeletonNames.Empty();
		NewAssetSkeletonIndex = -1;
		NewAssetSlotNames.Empty();
		NewAssetSlotIndex = -1;
		NewAssetSkeletonName = EString();

		TArray<EString> ScanFolders;
		TArray<EString> ScanFiles;
		if (EFileHelper::ScanDirectory(EBaseSettings::ENGINE_ANIMATION_PATH, ScanFolders, ScanFiles, TRUE))
		{
			for (INT32 FileIndex = 0, FileNum = ScanFiles.Num<INT32>(); FileIndex < FileNum; FileIndex++)
			{
				if (ScanFiles[FileIndex].ToLower().Contains(EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE).ToLower()))
				{
					NewAssetAnimationNames.Add(EPath::GetFileNameWithoutExtension(ScanFiles[FileIndex]));
				}
			}
		}
		if (!NewAssetAnimationNames.IsEmpty())
		{
			NewAssetAnimationIndex = 0;
			RefreshNewAssetClipsAndSlots();
		}

		const BOOL32 bIsMontage = (InType == ASSET_TYPE_ANIM_MONTAGE) ? TRUE : FALSE;
		const EString SavePath((bIsMontage != FALSE)
			? EBaseSettings::ENGINE_MONTAGE_PATH : EBaseSettings::ENGINE_STATE_MACHINE_PATH);
		const EString BaseName((bIsMontage != FALSE) ? EString("NewMontage") : EString("NewStateMachine"));
		EString Candidate = BaseName;
		INT32 Suffix = 1;
		while (EFileHelper::IsFileExists(SavePath + Candidate
			+ EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE)) != FALSE)
		{
			Candidate = BaseName + EString("_") + ToString(Suffix++);
		}
		::snprintf(NewAssetName, sizeof(NewAssetName), "%s", *Candidate);
	}

	void FContentBrowserPanel::RefreshNewAssetClipsAndSlots()
	{
		NewAssetClipNames.Empty();
		NewAssetClipIndex = -1;
		NewAssetSkeletonNames.Empty();
		NewAssetSkeletonIndex = -1;
		NewAssetSlotNames.Empty();
		NewAssetSlotIndex = -1;
		NewAssetSkeletonName = EString();
		if ((NewAssetAnimationIndex < 0) || (NewAssetAnimationIndex >= NewAssetAnimationNames.Num<INT32>()))
		{
			return;
		}

		const ESkeletonAnimationAsset* AnimationAsset = nullptr;
		if (!(EAnimationManager::GetManagerSingleton()->LoadSkeletonAnimationAsset(
			EBaseSettings::ENGINE_ANIMATION_PATH, NewAssetAnimationNames[NewAssetAnimationIndex]
			, AnimationAsset)) || (!AnimationAsset))
		{
			return;
		}
		const ESkeletonAnimation* Animation = AnimationAsset->GetStoragedResource();
		if (!Animation)
		{
			return;
		}
		NewAssetSkeletonName = Animation->GetSkeletonName();
		for (INT32 ClipIndex = 0, ClipNum = Animation->GetAnimationClipNum(); ClipIndex < ClipNum; ClipIndex++)
		{
			const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
			if (Clip)
			{
				NewAssetClipNames.Add(Clip->ClipName);
			}
		}
		if (!NewAssetClipNames.IsEmpty())
		{
			NewAssetClipIndex = 0;
		}

		TArray<EString> SkeletonFolders;
		TArray<EString> SkeletonFiles;
		if (EFileHelper::ScanDirectory(EBaseSettings::ENGINE_SKELETON_PATH, SkeletonFolders, SkeletonFiles, TRUE))
		{
			for (INT32 FileIndex = 0, FileNum = SkeletonFiles.Num<INT32>(); FileIndex < FileNum; FileIndex++)
			{
				if (SkeletonFiles[FileIndex].ToLower().Contains(EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE).ToLower()))
				{
					NewAssetSkeletonNames.Add(EPath::GetFileNameWithoutExtension(SkeletonFiles[FileIndex]));
				}
			}
		}
		if (!NewAssetSkeletonNames.IsEmpty())
		{
			INT32 PreferredSkeletonIndex = -1;
			for (INT32 SkeletonIndex = 0, SkeletonNum = NewAssetSkeletonNames.Num<INT32>(); SkeletonIndex < SkeletonNum; SkeletonIndex++)
			{
				if (NewAssetSkeletonNames[SkeletonIndex] == NewAssetSkeletonName)
				{
					PreferredSkeletonIndex = SkeletonIndex;
					break;
				}
			}
			if (PreferredSkeletonIndex < 0)
			{
				PreferredSkeletonIndex = 0;
			}
			NewAssetSkeletonIndex = PreferredSkeletonIndex;
			NewAssetSkeletonName = NewAssetSkeletonNames[PreferredSkeletonIndex];
		}
		RefreshNewAssetSlots();
	}

	void FContentBrowserPanel::RefreshNewAssetSlots()
	{
		NewAssetSlotNames.Empty();
		NewAssetSlotIndex = -1;
		if (NewAssetType != ASSET_TYPE_ANIM_MONTAGE)
		{
			return;
		}
		const ESkeletonAsset* SkeletonAsset = nullptr;
		if ((NewAssetSkeletonIndex >= 0)
			&& (NewAssetSkeletonIndex < NewAssetSkeletonNames.Num<INT32>())
			&& ESkeletonAssetManager::GetManagerSingleton()->LoadSkeletonAsset(
				EBaseSettings::ENGINE_SKELETON_PATH
				, NewAssetSkeletonNames[NewAssetSkeletonIndex], SkeletonAsset) && SkeletonAsset)
		{
			const ESkeleton* Skeleton = SkeletonAsset->GetStoragedResource();
			if (Skeleton)
			{
				INT32 PreferredSlotIndex = -1;
				const TArray<ESkeletonSlotGroup>& Groups = Skeleton->GetSlotGroups();
				for (INT32 GroupIndex = 0, GroupNum = Groups.Num<INT32>(); GroupIndex < GroupNum; GroupIndex++)
				{
					for (INT32 SlotIndex = 0, SlotNum = Groups[GroupIndex].SlotNames.Num<INT32>(); SlotIndex < SlotNum; SlotIndex++)
					{
						const EString& SlotName = Groups[GroupIndex].SlotNames[SlotIndex];
						if (SlotName == EString("Default"))
						{
							PreferredSlotIndex = NewAssetSlotNames.Num<INT32>();
						}
						NewAssetSlotNames.Add(SlotName);
					}
				}
				if (PreferredSlotIndex >= 0)
				{
					NewAssetSlotIndex = PreferredSlotIndex;
				}
				else if (!NewAssetSlotNames.IsEmpty())
				{
					NewAssetSlotIndex = 0;
				}
			}
		}
		if (NewAssetSlotNames.IsEmpty())
		{
			// A montage can still be authored with the conventional default slot; the editor
			// warns when it cannot resolve the slot against a preview skeleton.
			NewAssetSlotNames.Add(EString("Default"));
			NewAssetSlotIndex = 0;
		}
	}

	void FContentBrowserPanel::DrawNewAssetDialog()
	{
		if (bNewAssetWindowOpen == FALSE)
		{
			return;
		}
		const ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (bResetNewAssetWindowPos != FALSE)
		{
			ImGui::SetNextWindowPos(ImVec2(Viewport->WorkPos.x + 60.0f, Viewport->WorkPos.y + 60.0f)
				, ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(520.0f, 0.0f), ImGuiCond_Always);
			bResetNewAssetWindowPos = FALSE;
		}
		if (bFocusNewAssetWindow != FALSE)
		{
			ImGui::SetNextWindowFocus();
			bFocusNewAssetWindow = FALSE;
		}
		if (!ImGui::Begin("New Asset", &bNewAssetWindowOpen
			, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking))
		{
			ImGui::End();
			return;
		}

		const BOOL32 bIsMontage = (NewAssetType == ASSET_TYPE_ANIM_MONTAGE) ? TRUE : FALSE;
		ImGui::Text("%s", (bIsMontage != FALSE) ? "New Montage" : "New State Machine");
		ImGui::Separator();

		ImGui::TextDisabled("Name");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##cb_new_asset_name", NewAssetName, sizeof(NewAssetName));

		ImGui::TextDisabled("Animation");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (NewAssetAnimationNames.IsEmpty())
		{
			ImGui::TextDisabled("No animation assets found. Import an animation first.");
		}
		else if (ImGui::BeginCombo("##cb_new_asset_animation"
			, ((NewAssetAnimationIndex >= 0) && (NewAssetAnimationIndex < NewAssetAnimationNames.Num<INT32>()))
			? *NewAssetAnimationNames[NewAssetAnimationIndex] : "None"))
		{
			for (INT32 Index = 0, Num = NewAssetAnimationNames.Num<INT32>(); Index < Num; Index++)
			{
				const BOOL8 bSelected = (Index == NewAssetAnimationIndex) ? TRUE : FALSE;
				if (ImGui::Selectable(*NewAssetAnimationNames[Index], bSelected != FALSE))
				{
					NewAssetAnimationIndex = Index;
					NewAssetError = EString();
					RefreshNewAssetClipsAndSlots();
				}
				if (bSelected != FALSE)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::TextDisabled("Clip");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (NewAssetClipNames.IsEmpty())
		{
			ImGui::TextDisabled("This animation has no clips.");
		}
		else if (ImGui::BeginCombo("##cb_new_asset_clip"
			, ((NewAssetClipIndex >= 0) && (NewAssetClipIndex < NewAssetClipNames.Num<INT32>()))
			? *NewAssetClipNames[NewAssetClipIndex] : "None"))
		{
			for (INT32 Index = 0, Num = NewAssetClipNames.Num<INT32>(); Index < Num; Index++)
			{
				const BOOL8 bSelected = (Index == NewAssetClipIndex) ? TRUE : FALSE;
				if (ImGui::Selectable(*NewAssetClipNames[Index], bSelected != FALSE))
				{
					NewAssetClipIndex = Index;
					NewAssetError = EString();
				}
				if (bSelected != FALSE)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::TextDisabled("Skeleton");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (NewAssetSkeletonNames.IsEmpty())
		{
			ImGui::TextDisabled("No skeleton assets found.");
		}
		else if (ImGui::BeginCombo("##cb_new_asset_skeleton"
			, ((NewAssetSkeletonIndex >= 0) && (NewAssetSkeletonIndex < NewAssetSkeletonNames.Num<INT32>()))
			? *NewAssetSkeletonNames[NewAssetSkeletonIndex] : "None"))
		{
			for (INT32 Index = 0, Num = NewAssetSkeletonNames.Num<INT32>(); Index < Num; Index++)
			{
				const BOOL8 bSelected = (Index == NewAssetSkeletonIndex) ? TRUE : FALSE;
				if (ImGui::Selectable(*NewAssetSkeletonNames[Index], bSelected != FALSE))
				{
					NewAssetSkeletonIndex = Index;
					NewAssetSkeletonName = NewAssetSkeletonNames[Index];
					NewAssetError = EString();
					RefreshNewAssetSlots();
				}
				if (bSelected != FALSE)
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
			if (NewAssetSlotNames.IsEmpty())
			{
				ImGui::TextDisabled("No slot is available.");
			}
			else if (ImGui::BeginCombo("##cb_new_asset_slot"
				, ((NewAssetSlotIndex >= 0) && (NewAssetSlotIndex < NewAssetSlotNames.Num<INT32>()))
				? *NewAssetSlotNames[NewAssetSlotIndex] : "None"))
			{
				for (INT32 Index = 0, Num = NewAssetSlotNames.Num<INT32>(); Index < Num; Index++)
				{
					const BOOL8 bSelected = (Index == NewAssetSlotIndex) ? TRUE : FALSE;
					if (ImGui::Selectable(*NewAssetSlotNames[Index], bSelected != FALSE))
					{
						NewAssetSlotIndex = Index;
						NewAssetError = EString();
					}
					if (bSelected != FALSE)
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

		if (NewAssetError.Length() > 0u)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *NewAssetError);
		}

		const BOOL32 bHasName = (NewAssetName[0] != 0) ? TRUE : FALSE;
		const BOOL32 bHasAnimation = ((NewAssetAnimationIndex >= 0)
			&& (NewAssetAnimationIndex < NewAssetAnimationNames.Num<INT32>())) ? TRUE : FALSE;
		const BOOL32 bHasClip = ((NewAssetClipIndex >= 0)
			&& (NewAssetClipIndex < NewAssetClipNames.Num<INT32>())) ? TRUE : FALSE;
		const BOOL32 bHasSlot = ((bIsMontage == FALSE) || ((NewAssetSlotIndex >= 0)
			&& (NewAssetSlotIndex < NewAssetSlotNames.Num<INT32>()))) ? TRUE : FALSE;
		const BOOL32 bCanCreate = ((bHasName != FALSE) && (bHasAnimation != FALSE)
			&& (bHasClip != FALSE) && (bHasSlot != FALSE)) ? TRUE : FALSE;

		ImGui::BeginDisabled(bCanCreate == FALSE);
		if (ImGui::Button("Create", ImVec2(120.0f, 0.0f)))
		{
			EString Error;
			if (CreateNewAsset(Error))
			{
				bNewAssetWindowOpen = FALSE;
			}
			else
			{
				NewAssetError = Error;
			}
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
		{
			bNewAssetWindowOpen = FALSE;
		}
		ImGui::End();
	}

	BOOL32 FContentBrowserPanel::CreateNewAsset(EString& OutError)
	{
		OutError = EString();
		const EString AssetName(NewAssetName);
		if (EAssetFileOperations::ValidateName(AssetName, OutError) == FALSE)
		{
			return FALSE;
		}
		if ((NewAssetAnimationIndex < 0) || (NewAssetAnimationIndex >= NewAssetAnimationNames.Num<INT32>())
			|| (NewAssetClipIndex < 0) || (NewAssetClipIndex >= NewAssetClipNames.Num<INT32>()))
		{
			OutError = EString("Select an animation and a clip.");
			return FALSE;
		}
		const BOOL32 bIsMontage = (NewAssetType == ASSET_TYPE_ANIM_MONTAGE) ? TRUE : FALSE;
		if ((bIsMontage != FALSE)
			&& ((NewAssetSlotIndex < 0) || (NewAssetSlotIndex >= NewAssetSlotNames.Num<INT32>())))
		{
			OutError = EString("Select a slot for the montage section.");
			return FALSE;
		}

		const EString& AnimationName = NewAssetAnimationNames[NewAssetAnimationIndex];
		const EString& ClipName = NewAssetClipNames[NewAssetClipIndex];
		const EString SavePath((bIsMontage != FALSE)
			? EBaseSettings::ENGINE_MONTAGE_PATH : EBaseSettings::ENGINE_STATE_MACHINE_PATH);
		const EString JsonText = ((bIsMontage != FALSE)
			? BuildNewMontageJson(AssetName, NewAssetSkeletonName, AnimationName, ClipName
				, NewAssetSlotNames[NewAssetSlotIndex])
			: BuildNewStateMachineJson(AssetName, NewAssetSkeletonName, AnimationName, ClipName));

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

		Host->GetCatalog().Refresh();
		const TSharedPtr<FAssetFolderNode> Destination = Host->GetCatalog().FindFolderByPath(SavePath);
		if (Destination)
		{
			CurrentFolderPath = Destination->GetPath();
		}
		const EString FullPath = SavePath + AssetName + EString(EEngineSettings::ENGINE_ASSET_NAME_TYPE);
		SelectOnly(FullPath);
		SetStatus(EString("Created ") + AssetName + EString("."), FALSE);

		EAnimationWorkspace* Workspace = Host->GetAnimationWorkspace();
		if (Workspace)
		{
			if (bIsMontage != FALSE)
			{
				Workspace->OpenMontageAsset(FullPath);
			}
			else
			{
				Workspace->OpenStateMachineAsset(FullPath);
			}
		}
		return TRUE;
	}
}

#endif
