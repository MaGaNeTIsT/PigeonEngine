#include "MontageEditorPanel.h"

#if ENGINE_EDITOR_ONLY

#include <AnimationAsset/AnimationAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <Editor/AssetManager.h>
#include <Animation/AnimationNotify.h>
#include <Config/EngineConfig.h>
#include <Base/DataStructure/Text/Path.h>
#include <imgui.h>
#include <cfloat>
#include <cstdio>

namespace PigeonEngine
{

	namespace
	{
		FLOAT MontageClipDurationSeconds(const ESkeletonAnimationClip& InClip)
		{
			if (InClip.Duration <= 0.0f)
			{
				return 0.0f;
			}
			const FLOAT TicksPerSecond = (InClip.TicksPerSecond > 1e-3f)
				? InClip.TicksPerSecond
				: EEngineSettings::ENGINE_ANIMATION_DEFAULT_TICKS_PER_SECOND;
			return ((TicksPerSecond > 0.0f) ? (InClip.Duration / TicksPerSecond) : 0.0f);
		}

		const CHAR* MontageLoopModeText(const EAnimationLoopMode InLoopMode)
		{
			switch (InLoopMode)
			{
			case ANIMATION_LOOP_ONCE:
				return "Once";
			case ANIMATION_LOOP_PINGPONG:
				return "PingPong";
			case ANIMATION_LOOP_CLAMP:
				return "Clamp";
			case ANIMATION_LOOP_REPEAT:
			default:
				return "Repeat";
			}
		}

		EString DefaultMontageNotifyName(const TArray<FAnimNotifyEvent>& InNotifies)
		{
			INT32 Suffix = InNotifies.Num();
			EString Name;
			BOOL32 bExists = TRUE;
			while (bExists != FALSE)
			{
				bExists = FALSE;
				Name = EString("Notify") + ToString(Suffix++);
				for (INT32 NotifyIndex = 0; NotifyIndex < InNotifies.Num(); NotifyIndex++)
				{
					if (InNotifies[NotifyIndex].NotifyName == Name)
					{
						bExists = TRUE;
						break;
					}
				}
			}
			return Name;
		}

		EString FindDefaultMontageNotifyClass(BOOL32& bOutIsState)
		{
			bOutIsState = FALSE;
			const TArray<EString> ClassNames = EAnimNotifyRegistry::GetRegisteredClassNames();
			for (INT32 ClassIndex = 0; ClassIndex < ClassNames.Num(); ClassIndex++)
			{
				BOOL32 bClassIsState = FALSE;
				if ((EAnimNotifyRegistry::IsStateClass(ClassNames[ClassIndex], bClassIsState) != FALSE)
					&& (bClassIsState == FALSE))
				{
					return ClassNames[ClassIndex];
				}
			}
			if (ClassNames.Num() > 0)
			{
				EAnimNotifyRegistry::IsStateClass(ClassNames[0], bOutIsState);
				return ClassNames[0];
			}
			return EString();
		}
	}

	FMontageEditorPanel::FMontageEditorPanel()
	{
		m_PreviewScene.Initialize("MontagePreview", Vector3::Zero());
		m_bPreviewInitialized = TRUE;
	}

	FMontageEditorPanel::~FMontageEditorPanel()
	{
		if (m_bPreviewInitialized != FALSE)
		{
			m_PreviewScene.Shutdown();
		}
	}

	void FMontageEditorPanel::Draw(FMontageDocument& InDocument)
	{
		const EAnimMontage* Montage = InDocument.GetDraftMontage();
		if (!Montage)
		{
			ImGui::TextDisabled("Montage draft has no sections.");
			return;
		}
		// The workspace shares one panel across Montage tabs. Reset the preview/selection when
		// the active document changes so state from the previous montage does not leak in.
		if (m_BoundDocument != &InDocument)
		{
			m_BoundDocument = &InDocument;
			m_SelectedSectionIndex = ((InDocument.Selection.ObjectIndex >= 0)
				? InDocument.Selection.ObjectIndex : 0);
			m_SelectedNotifyIndex = -1;
			m_Transport = FTransportState();
			m_bPreviewCatalogReady = FALSE;
			m_PreviewSkeletonName = EString();
			m_MeshIndex = -1;
			m_SkeletonIndex = -1;
			m_MeshAsset = nullptr;
			m_SkeletonAsset = nullptr;
			m_BoundMeshAsset = nullptr;
			m_BoundSkeletonAsset = nullptr;
			m_BoundAnimationAsset = nullptr;
			m_BoundClipIndex = -1;
			m_PreviewScene.SetPlayAnimation(FALSE);
			m_PreviewScene.SetAnimationAsset(nullptr, -1);
			ResetSectionBuffers();
			ResetNotifyBuffers();
			m_SourceDocument = nullptr;
		}
		EnsureSelection(InDocument);
		BindPreview(InDocument);
		m_PreviewScene.Tick(ImGui::GetIO().DeltaTime);
		SyncTransport(InDocument);
		DrawToolbar(InDocument);

		const FLOAT TotalHeight = EMath::Max(0.0f, ImGui::GetContentRegionAvail().y);
		const FLOAT SplitterCost = FEditorSplitter::GetCost();
		const FLOAT MinBottom = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
		if (m_bShowOutput != FALSE)
		{
			const FLOAT MaxBottom = EMath::Max(MinBottom, TotalHeight - SplitterCost);
			m_BottomHeight = EMath::Clamp(m_BottomHeight, MinBottom, MaxBottom);
			const FLOAT ContentHeight = EMath::Max(0.0f, TotalHeight - m_BottomHeight - SplitterCost);
			if (ContentHeight > 0.0f)
			{
				DrawLayout(InDocument, ContentHeight);
			}
			DrawBottomPanel(InDocument, TotalHeight);
		}
		else
		{
			DrawLayout(InDocument, TotalHeight);
		}
	}

	void FMontageEditorPanel::EnsureSelection(FMontageDocument& InDocument)
	{
		const EAnimMontage* Montage = InDocument.GetDraftMontage();
		if ((!Montage) || (Montage->Sections.Num() <= 0))
		{
			m_SelectedSectionIndex = -1;
			m_SelectedNotifyIndex = -1;
			InDocument.Selection.Clear();
			return;
		}
		m_SelectedSectionIndex = EMath::Clamp(m_SelectedSectionIndex, 0, Montage->Sections.Num() - 1);
		if (InDocument.Selection.Is(EEditorSelectionType::NOTIFY))
		{
			const EAnimMontageSection& Section = Montage->Sections[m_SelectedSectionIndex];
			if ((m_SelectedNotifyIndex < 0) || (m_SelectedNotifyIndex >= Section.Notifies.Num()))
			{
				m_SelectedNotifyIndex = -1;
				InDocument.Selection.Type = EEditorSelectionType::SECTION;
				InDocument.Selection.ObjectIndex = m_SelectedSectionIndex;
				InDocument.Selection.SubIndex = -1;
			}
			else
			{
				InDocument.Selection.ObjectIndex = m_SelectedSectionIndex;
				InDocument.Selection.SubIndex = m_SelectedNotifyIndex;
			}
			return;
		}
		m_SelectedNotifyIndex = -1;
		InDocument.Selection.Type = EEditorSelectionType::SECTION;
		InDocument.Selection.ObjectIndex = m_SelectedSectionIndex;
		InDocument.Selection.SubIndex = -1;
	}

	void FMontageEditorPanel::DrawToolbar(FMontageDocument& InDocument)
	{
		FImGuiIdScope ToolbarScope("MontageToolbar");
		ImGui::Text("%s", *InDocument.DisplayName);
		ImGui::SameLine();
		ImGui::TextDisabled("Montage");
		if (InDocument.IsDirty() != FALSE)
		{
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "unsaved");
		}
		ImGui::SameLine();
		if (ImGui::Button("Apply"))
		{
			EString Error;
			if (InDocument.Apply(Error))
			{
				m_StatusMessage = EString("Montage applied.");
				m_bStatusSucceeded = TRUE;
			}
			else
			{
				m_StatusMessage = Error;
				m_bStatusSucceeded = FALSE;
			}
		}
		ImGui::SameLine();
		auto DoUndo = [this, &InDocument]()
		{
			FEditorSnapshot Snapshot;
			if (InDocument.Commands.Undo(InDocument.CaptureSnapshot(), Snapshot))
			{
				RestoreDocumentSnapshot(InDocument, Snapshot);
				m_StatusMessage = EString("Undo.");
				m_bStatusSucceeded = TRUE;
			}
		};
		auto DoRedo = [this, &InDocument]()
		{
			FEditorSnapshot Snapshot;
			if (InDocument.Commands.Redo(InDocument.CaptureSnapshot(), Snapshot))
			{
				RestoreDocumentSnapshot(InDocument, Snapshot);
				m_StatusMessage = EString("Redo.");
				m_bStatusSucceeded = TRUE;
			}
		};
		if ((ImGui::Button("Undo") != false) && (InDocument.Commands.CanUndo() != FALSE))
		{
			DoUndo();
		}
		ImGui::SameLine();
		if ((ImGui::Button("Redo") != false) && (InDocument.Commands.CanRedo() != FALSE))
		{
			DoRedo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Save"))
		{
			EString Error;
			if (InDocument.Save(Error))
			{
				m_StatusMessage = EString("Montage saved.");
				m_bStatusSucceeded = TRUE;
			}
			else
			{
				m_StatusMessage = Error;
				m_bStatusSucceeded = FALSE;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reload"))
		{
			if (InDocument.IsDirty() != FALSE)
			{
				ImGui::OpenPopup("##montage_reload_confirm");
			}
			else
			{
				EString Error;
				if (InDocument.Reload(Error))
				{
					m_StatusMessage = EString("Montage reloaded from disk.");
					m_bStatusSucceeded = TRUE;
					m_SourceDocument = nullptr;
					ResetSectionBuffers();
					ResetNotifyBuffers();
				}
				else
				{
					m_StatusMessage = Error;
					m_bStatusSucceeded = FALSE;
				}
			}
		}
		ImGui::SameLine();
		ImGui::Checkbox("Output", &m_bShowOutput);
		const ImGuiIO& IO = ImGui::GetIO();
		if ((IO.WantTextInput == false) && (IO.KeyCtrl != false)
			&& (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) != false))
		{
			if ((ImGui::IsKeyPressed(ImGuiKey_Z, false) != false)
				&& (InDocument.Commands.CanUndo() != FALSE))
			{
				DoUndo();
			}
			else if ((ImGui::IsKeyPressed(ImGuiKey_Y, false) != false)
				&& (InDocument.Commands.CanRedo() != FALSE))
			{
				DoRedo();
			}
		}
		if (m_StatusMessage.Length() > 0u)
		{
			ImGui::TextColored((m_bStatusSucceeded != FALSE)
				? ImVec4(0.45f, 0.85f, 0.45f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f)
				, "%s", *m_StatusMessage);
		}

		if (ImGui::BeginPopupModal("##montage_reload_confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Discard unsaved montage changes and reload from disk?");
			if (ImGui::Button("Reload"))
			{
				EString Error;
				if (InDocument.Reload(Error))
				{
					m_StatusMessage = EString("Montage reloaded from disk.");
					m_bStatusSucceeded = TRUE;
					m_SourceDocument = nullptr;
					ResetSectionBuffers();
					ResetNotifyBuffers();
				}
				else
				{
					m_StatusMessage = Error;
					m_bStatusSucceeded = FALSE;
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void FMontageEditorPanel::DrawLayout(FMontageDocument& InDocument, FLOAT InHeight)
	{
		if (InHeight == 0.0f)
		{
			return;
		}
		const ImVec2 Available = ImGui::GetContentRegionAvail();
		const FLOAT FitWidth = EMath::Max(320.0f, Available.x);
		if (ImGui::BeginChild("##montage_layout_fit", ImVec2(FitWidth, InHeight), false))
		{
			const ImGuiTableFlags LayoutFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV
				| ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;
			if (ImGui::BeginTable("##montage_layout", 3, LayoutFlags))
			{
				ImGui::TableSetupColumn("##left", ImGuiTableColumnFlags_WidthFixed, 230.0f);
				ImGui::TableSetupColumn("##center", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("##right", ImGuiTableColumnFlags_WidthFixed, 350.0f);
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				if (ImGui::BeginChild("##montage_left", ImVec2(0.0f, 0.0f), false))
				{
					DrawLeftPanel(InDocument);
				}
				ImGui::EndChild();

				ImGui::TableSetColumnIndex(1);
				if (ImGui::BeginChild("##montage_center", ImVec2(0.0f, 0.0f), false))
				{
					DrawCenter(InDocument);
				}
				ImGui::EndChild();

				ImGui::TableSetColumnIndex(2);
				if (ImGui::BeginChild("##montage_right", ImVec2(0.0f, 0.0f), false))
				{
					DrawInspector(InDocument);
				}
				ImGui::EndChild();
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}

	void FMontageEditorPanel::DrawLeftPanel(FMontageDocument& InDocument)
	{
		const EAnimMontage* Montage = InDocument.GetDraftMontage();
		if (!Montage)
		{
			return;
		}
		FImGuiIdScope LeftScope("MontageSections");
		ImGui::Text("Sections: %d", Montage->Sections.Num());

		// Slot groups and their order are owned by the skeleton. The montage only names a
		// slot; resolving that name to a layer priority happens in EAnimMontageInstance.
		const ESkeleton* Skeleton = (m_SkeletonAsset ? m_SkeletonAsset->GetStoragedResource() : nullptr);
		ImGui::TextDisabled("Skeleton: %s"
			, (m_SkeletonAsset ? *m_SkeletonAsset->GetAssetName() : "None"));
		if (Skeleton)
		{
			const TArray<ESkeletonSlotGroup>& Groups = Skeleton->GetSlotGroups();
			ImGui::TextDisabled("Skeleton slot groups (priority order): %d", Groups.Num());
			for (INT32 GroupIndex = 0; GroupIndex < Groups.Num(); GroupIndex++)
			{
				const ESkeletonSlotGroup& Group = Groups[GroupIndex];
				ImGui::PushID(GroupIndex);
				ImGui::Text("%d  %s  (%d slots)", GroupIndex, *Group.Name, Group.SlotNames.Num());
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					if (Group.SlotNames.Num() <= 0)
					{
						ImGui::TextDisabled("(no slots)");
					}
					for (INT32 SlotIndex = 0; SlotIndex < Group.SlotNames.Num(); SlotIndex++)
					{
						ImGui::TextUnformatted(*Group.SlotNames[SlotIndex]);
					}
					ImGui::EndTooltip();
				}
				ImGui::PopID();
			}
		}
		else
		{
			ImGui::TextDisabled("Skeleton slot groups: preview skeleton unavailable");
		}
		ImGui::BeginDisabled(Skeleton == nullptr);
		if (ImGui::Button("Edit Skeleton Slot Groups", ImVec2(-FLT_MIN, 0.0f)) && Skeleton)
		{
			EAssetManager::GetManagerSingleton()->OpenSkeletonSlotGroups(
				m_SkeletonAsset->GetAssetFullPathName());
		}
		ImGui::EndDisabled();
		if (ImGui::Button("Add Section", ImVec2(-FLT_MIN, 0.0f)))
		{
			AddSection(InDocument);
			return;
		}
		ImGui::Separator();
		if (ImGui::BeginChild("##montage_section_list", ImVec2(0.0f, 0.0f), true))
		{
			for (INT32 SectionIndex = 0; SectionIndex < Montage->Sections.Num(); SectionIndex++)
			{
				const EAnimMontageSection& Section = Montage->Sections[SectionIndex];
				ImGui::PushID(SectionIndex);
				CHAR Label[256] = "";
				::snprintf(Label, sizeof(Label), "%s  [%s]", *Section.Name, *Section.ClipName);
				if (ImGui::Selectable(Label, (SectionIndex == m_SelectedSectionIndex)))
				{
					m_SelectedSectionIndex = SectionIndex;
					m_SelectedNotifyIndex = -1;
					InDocument.Selection.Type = EEditorSelectionType::SECTION;
					InDocument.Selection.ObjectIndex = SectionIndex;
					InDocument.Selection.SubIndex = -1;
					ResetSectionBuffers();
					ResetNotifyBuffers();
				}
				ImGui::PopID();
			}
		}
		ImGui::EndChild();
	}

	void FMontageEditorPanel::DrawCenter(FMontageDocument& InDocument)
	{
		const FLOAT TotalHeight = ImGui::GetContentRegionAvail().y;
		const FLOAT SplitterCost = FEditorSplitter::GetCost();
		const FLOAT AvailableForPanes = EMath::Max(0.0f, TotalHeight - SplitterCost);
		if (AvailableForPanes <= 0.0f)
		{
			return;
		}
		m_PreviewHeight = EMath::Clamp(m_PreviewHeight, 0.0f, AvailableForPanes);
		const FLOAT TimelineHeight = EMath::Max(0.0f, AvailableForPanes - m_PreviewHeight);
		if (m_PreviewHeight > 0.0f)
		{
			if (ImGui::BeginChild("##montage_preview_area", ImVec2(0.0f, m_PreviewHeight), true))
			{
				DrawPreviewArea(InDocument);
			}
			ImGui::EndChild();
		}
		FEditorSplitter::Horizontal("##montage_preview_split", m_PreviewHeight
			, 0.0f, 0.0f, TotalHeight);
		if (TimelineHeight > 0.0f)
		{
			if (ImGui::BeginChild("##montage_timeline_area", ImVec2(0.0f, TimelineHeight), true))
			{
				DrawTimelineArea(InDocument);
			}
			ImGui::EndChild();
		}
	}

	void FMontageEditorPanel::DrawPreviewArea(FMontageDocument& InDocument)
	{
		const ESkeletonAnimationAsset* AnimationAsset = InDocument.GetAnimationAsset();
		const ESkeletonAnimation* Animation = (AnimationAsset ? AnimationAsset->GetStoragedResource() : nullptr);
		if (!m_bPreviewCatalogReady)
		{
			RefreshPreviewCatalog(Animation ? Animation->GetSkeletonName() : EString());
		}
		if (!AnimationAsset)
		{
			ImGui::TextDisabled("Montage animation asset is not loaded.");
		}
		const TArray<EString>& MeshPaths = m_PreviewCatalog.GetMeshPaths();
		const TArray<EString>& SkeletonPaths = m_PreviewCatalog.GetSkeletonPaths();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##montage_preview_mesh", ((m_MeshIndex >= 0) && (m_MeshIndex < MeshPaths.Num()))
			? *EPath::GetFileNameWithoutExtension(MeshPaths[m_MeshIndex]) : "None"))
		{
			for (INT32 MeshIndex = 0; MeshIndex < MeshPaths.Num(); MeshIndex++)
			{
				const BOOL8 bSelected = (MeshIndex == m_MeshIndex);
				if (ImGui::Selectable(*EPath::GetFileNameWithoutExtension(MeshPaths[MeshIndex]), bSelected))
				{
					m_MeshIndex = MeshIndex;
					m_BoundMeshAsset = nullptr;
					m_BoundAnimationAsset = nullptr;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##montage_preview_skeleton"
			, ((m_SkeletonIndex >= 0) && (m_SkeletonIndex < SkeletonPaths.Num()))
			? *EPath::GetFileNameWithoutExtension(SkeletonPaths[m_SkeletonIndex]) : "None"))
		{
			for (INT32 SkeletonIndex = 0; SkeletonIndex < SkeletonPaths.Num(); SkeletonIndex++)
			{
				const BOOL8 bSelected = (SkeletonIndex == m_SkeletonIndex);
				if (ImGui::Selectable(*EPath::GetFileNameWithoutExtension(SkeletonPaths[SkeletonIndex]), bSelected))
				{
					m_SkeletonIndex = SkeletonIndex;
					m_BoundSkeletonAsset = nullptr;
					m_BoundAnimationAsset = nullptr;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		const ImVec2 Available = ImGui::GetContentRegionAvail();
		const ImVec2 ImageSize(EMath::Max(Available.x, 64.0f), EMath::Max(Available.y, 64.0f));
		m_PreviewScene.UpdateViewportSize(static_cast<UINT32>(ImageSize.x), static_cast<UINT32>(ImageSize.y));
		const ImTextureID TextureId = m_PreviewScene.GetTextureId();
		if (TextureId)
		{
			ImGui::Image(TextureId, ImageSize);
		}
		else
		{
			ImGui::Dummy(ImageSize);
		}
	}

	void FMontageEditorPanel::DrawTimelineArea(FMontageDocument& InDocument)
	{
		FTransportInteraction TransportInteraction;
		FTransportBar::Draw("##montage_transport", m_Transport, TransportInteraction);
		ApplyTransport(InDocument, TransportInteraction);

		if (ImGui::Button("Add Notify"))
		{
			AddNotify(InDocument);
			return;
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete Notify"))
		{
			DeleteSelectedNotify(InDocument);
			return;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Drag to retime; Shift+drag box select; Ctrl+click multi-select; Esc/empty click clears; Ctrl+Wheel zoom.");

		TArray<FTimelineTrack> Tracks;
		BuildTimelineTracks(InDocument, Tracks);
		FTimelineInteraction TimelineInteraction;
		m_Timeline.Draw("##montage_timeline", Tracks, GetSelectedClipDuration(InDocument)
			, m_Transport.Time, 0, m_SelectedNotifyIndex, TimelineInteraction);
		if (TimelineInteraction.bMarkerSelected != FALSE)
		{
			m_SelectedNotifyIndex = TimelineInteraction.SelectedMarker;
			if (m_SelectedNotifyIndex >= 0)
			{
				InDocument.Selection.Type = EEditorSelectionType::NOTIFY;
				InDocument.Selection.ObjectIndex = m_SelectedSectionIndex;
				InDocument.Selection.SubIndex = m_SelectedNotifyIndex;
			}
			else
			{
				InDocument.Selection.Type = EEditorSelectionType::SECTION;
				InDocument.Selection.ObjectIndex = m_SelectedSectionIndex;
				InDocument.Selection.SubIndex = -1;
			}
			ResetNotifyBuffers();
		}
		if (TimelineInteraction.bScrubbed != FALSE)
		{
			m_PreviewScene.SetPlaybackTimeSeconds(TimelineInteraction.ScrubbedTime);
			m_PreviewScene.SetPlayAnimation(FALSE);
			m_Transport.bPlaying = FALSE;
			m_Transport.Time = TimelineInteraction.ScrubbedTime;
		}

		const EAnimMontageSection* Section = GetSelectedSection(InDocument);
		if (!Section)
		{
			return;
		}
		if ((TimelineInteraction.bMarkerResizeFinished != FALSE) && (m_SelectedNotifyIndex >= 0))
		{
			TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
			if (m_SelectedNotifyIndex < Notifies.Num())
			{
				FAnimNotifyEvent& Notify = Notifies[m_SelectedNotifyIndex];
				if (Notify.bIsState != FALSE)
				{
					Notify.Time = EMath::Max(TimelineInteraction.MarkerResizeTime, 0.0f);
					Notify.StateDuration = EMath::Max(TimelineInteraction.MarkerResizeDuration, 0.01f);
					Notify.bStateDurationOverride = TRUE;
					Notify.ResetCompiledNotify();
					Notify.Compile();
					CommitSectionNotifies(InDocument, m_SelectedSectionIndex, Notifies
						, EString("Notify duration updated."));
				}
			}
		}
		else if ((TimelineInteraction.bMarkerDragFinished != FALSE) && (m_SelectedNotifyIndex >= 0))
		{
			TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
			if (m_SelectedNotifyIndex < Notifies.Num())
			{
				FAnimNotifyEvent& Notify = Notifies[m_SelectedNotifyIndex];
				Notify.Time = TimelineInteraction.MarkerDragTime;
				CommitSectionNotifies(InDocument, m_SelectedSectionIndex, Notifies
					, EString("Notify retimed."));
			}
		}
		if (TimelineInteraction.bMarkerMultiDragFinished != FALSE)
		{
			TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
			const FLOAT ClipDuration = GetSelectedClipDuration(InDocument);
			for (INT32 RefIndex = 0; RefIndex < TimelineInteraction.MultiDraggedMarkers.Num(); RefIndex++)
			{
				const FTimelineMarkerRef& Ref = TimelineInteraction.MultiDraggedMarkers[RefIndex];
				if ((Ref.TrackIndex != 0) || (Ref.MarkerIndex < 0)
					|| (Ref.MarkerIndex >= Notifies.Num()))
				{
					continue;
				}
				FAnimNotifyEvent& Notify = Notifies[Ref.MarkerIndex];
				const FLOAT RangeDuration = (Notify.bIsState != FALSE)
					? EMath::Max(Notify.StateDuration, 0.0f) : 0.0f;
				Notify.Time = EMath::Clamp(Notify.Time + TimelineInteraction.MarkerMultiDragDelta
					, 0.0f, EMath::Max(0.0f, ClipDuration - RangeDuration));
			}
			CommitSectionNotifies(InDocument, m_SelectedSectionIndex, Notifies
				, EString("Notifies retimed."));
			return;
		}

		const ImGuiIO& IO = ImGui::GetIO();
		if ((TimelineInteraction.SelectedMarkers.Num() > 1)
			&& (IO.WantTextInput == false)
			&& (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) != false)
			&& (ImGui::IsKeyPressed(ImGuiKey_Delete, false) != false))
		{
			TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
			TArray<BOOL8> RemoveMask;
			RemoveMask.SetNum(Notifies.Num());
			for (INT32 NotifyIndex = 0; NotifyIndex < RemoveMask.Num(); NotifyIndex++)
			{
				RemoveMask[NotifyIndex] = FALSE;
			}
			for (INT32 RefIndex = 0; RefIndex < TimelineInteraction.SelectedMarkers.Num(); RefIndex++)
			{
				const FTimelineMarkerRef& Ref = TimelineInteraction.SelectedMarkers[RefIndex];
				if ((Ref.TrackIndex == 0) && (Ref.MarkerIndex >= 0)
					&& (Ref.MarkerIndex < RemoveMask.Num()))
				{
					RemoveMask[Ref.MarkerIndex] = TRUE;
				}
			}
			TArray<FAnimNotifyEvent> RemainingNotifies;
			for (INT32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); NotifyIndex++)
			{
				if (RemoveMask[NotifyIndex] == FALSE)
				{
					RemainingNotifies.Add(Notifies[NotifyIndex]);
				}
			}
			CommitSectionNotifies(InDocument, m_SelectedSectionIndex, RemainingNotifies
				, EString("Notifies deleted."));
			m_SelectedNotifyIndex = -1;
			InDocument.Selection.Type = EEditorSelectionType::SECTION;
			InDocument.Selection.ObjectIndex = m_SelectedSectionIndex;
			InDocument.Selection.SubIndex = -1;
			ResetNotifyBuffers();
		}
	}

	void FMontageEditorPanel::DrawInspector(FMontageDocument& InDocument)
	{
		FImGuiIdScope InspectorScope("MontageInspector");
		ImGui::TextDisabled("Inspector");
		ImGui::Separator();
		const EAnimMontageSection* Section = GetSelectedSection(InDocument);
		if (!Section)
		{
			ImGui::TextDisabled("No section is selected.");
			return;
		}
		if ((InDocument.Selection.Is(EEditorSelectionType::NOTIFY)) && (m_SelectedNotifyIndex >= 0))
		{
			DrawNotifyInspector(InDocument, m_SelectedSectionIndex, m_SelectedNotifyIndex);
		}
		else
		{
			DrawSectionInspector(InDocument, m_SelectedSectionIndex);
		}
	}

	void FMontageEditorPanel::DrawSectionInspector(FMontageDocument& InDocument, INT32 InSectionIndex)
	{
		const EAnimMontage* Montage = InDocument.GetDraftMontage();
		if ((!Montage) || (InSectionIndex < 0) || (InSectionIndex >= Montage->Sections.Num()))
		{
			return;
		}
		// Copy: every commit rebuilds the draft and frees the old montage.
		EAnimMontageSection Section = Montage->Sections[InSectionIndex];
		const EString BufferSource = EString("montage-section:") + InDocument.AssetPath
			+ EString(":") + ToString(InSectionIndex);
		if (m_SectionBufferSource != BufferSource)
		{
			m_SectionNameBuffer.SetText(Section.Name);
			m_SlotNameBuffer.SetText(Section.SlotName);
			m_SectionBufferSource = BufferSource;
		}

		FImGuiIdScope SectionScope(InSectionIndex);
		ImGui::Text("Section: %s", *Section.Name);
		ImGui::TextDisabled("Name");
		ImGui::SetNextItemWidth(-FLT_MIN);
		m_SectionNameBuffer.InputText("##montage_section_name");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Section.Name = m_SectionNameBuffer.ToString();
			CommitSection(InDocument, InSectionIndex, Section, EString("Section renamed."));
			return;
		}

		const ESkeletonAnimation* Animation = ([&InDocument]() -> const ESkeletonAnimation*
			{
				const ESkeletonAnimationAsset* Asset = InDocument.GetAnimationAsset();
				return (Asset ? Asset->GetStoragedResource() : nullptr);
			})();
		ImGui::TextDisabled("Clip");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##montage_section_clip"
			, ((Section.ClipName.Length() > 0u) ? *Section.ClipName : "None")))
		{
			BOOL32 bCommitted = FALSE;
			if (Animation)
			{
				for (INT32 ClipIndex = 0; ClipIndex < Animation->GetAnimationClipNum(); ClipIndex++)
				{
					const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
					if (!Clip)
					{
						continue;
					}
					const BOOL8 bSelected = (Clip->ClipName == Section.ClipName);
					if (ImGui::Selectable(*Clip->ClipName, bSelected))
					{
						Section.ClipName = Clip->ClipName;
						bCommitted = TRUE;
						break;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
			}
			ImGui::EndCombo();
			if (bCommitted != FALSE)
			{
				CommitSection(InDocument, InSectionIndex, Section, EString("Section clip updated."));
				return;
			}
		}

		ImGui::TextDisabled("Slot");
		ImGui::SetNextItemWidth(-FLT_MIN);
		const ESkeleton* Skeleton = (m_SkeletonAsset ? m_SkeletonAsset->GetStoragedResource() : nullptr);
		if (Skeleton)
		{
			if (ImGui::BeginCombo("##montage_section_slot"
				, ((Section.SlotName.Length() > 0u) ? *Section.SlotName : "None")))
			{
				BOOL32 bCommitted = FALSE;
				const TArray<ESkeletonSlotGroup>& Groups = Skeleton->GetSlotGroups();
				for (INT32 GroupIndex = 0; GroupIndex < Groups.Num(); GroupIndex++)
				{
					const ESkeletonSlotGroup& Group = Groups[GroupIndex];
					for (INT32 SlotIndex = 0; SlotIndex < Group.SlotNames.Num(); SlotIndex++)
					{
						const EString& SlotName = Group.SlotNames[SlotIndex];
						const BOOL8 bSelected = (SlotName == Section.SlotName);
						if (ImGui::Selectable(*SlotName, bSelected))
						{
							Section.SlotName = SlotName;
							bCommitted = TRUE;
							break;
						}
						if (bSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
				}
				ImGui::EndCombo();
				if (bCommitted != FALSE)
				{
					CommitSection(InDocument, InSectionIndex, Section, EString("Section slot updated."));
					return;
				}
			}
		}
		else
		{
			m_SlotNameBuffer.InputText("##montage_section_slot_text");
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				Section.SlotName = m_SlotNameBuffer.ToString();
				CommitSection(InDocument, InSectionIndex, Section, EString("Section slot updated."));
				return;
			}
		}
		if (Skeleton && (Section.SlotName.Length() > 0u)
			&& (Skeleton->FindSlotGroup(Section.SlotName) < 0))
		{
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f)
				, "Slot '%s' is not defined by the preview skeleton.", *Section.SlotName);
		}

		ImGui::TextDisabled("Next Section");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##montage_section_next"
			, ((Section.NextSectionName.Length() > 0u) ? *Section.NextSectionName : "None")))
		{
			BOOL32 bCommitted = FALSE;
			if (ImGui::Selectable("None", (Section.NextSectionName.Length() == 0u)))
			{
				Section.NextSectionName = EString();
				bCommitted = TRUE;
			}
			for (INT32 NextIndex = 0; NextIndex < Montage->Sections.Num(); NextIndex++)
			{
				const EString& NextName = Montage->Sections[NextIndex].Name;
				const BOOL8 bSelected = (NextName == Section.NextSectionName);
				if (ImGui::Selectable(*NextName, bSelected))
				{
					Section.NextSectionName = NextName;
					bCommitted = TRUE;
					break;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
			if (bCommitted != FALSE)
			{
				CommitSection(InDocument, InSectionIndex, Section, EString("Section next updated."));
				return;
			}
		}

		ImGui::TextDisabled("Loop Mode");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##montage_section_loop", MontageLoopModeText(Section.LoopMode)))
		{
			BOOL32 bCommitted = FALSE;
			const EAnimationLoopMode LoopModes[] = { ANIMATION_LOOP_ONCE, ANIMATION_LOOP_REPEAT
				, ANIMATION_LOOP_PINGPONG, ANIMATION_LOOP_CLAMP };
			for (INT32 LoopIndex = 0; LoopIndex < 4; LoopIndex++)
			{
				const BOOL8 bSelected = (Section.LoopMode == LoopModes[LoopIndex]);
				if (ImGui::Selectable(MontageLoopModeText(LoopModes[LoopIndex]), bSelected))
				{
					Section.LoopMode = LoopModes[LoopIndex];
					bCommitted = TRUE;
					break;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
			if (bCommitted != FALSE)
			{
				CommitSection(InDocument, InSectionIndex, Section, EString("Section loop updated."));
				return;
			}
		}

		FLOAT PlayRate = Section.PlayRate;
		ImGui::TextDisabled("Play Rate");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputFloat("##montage_section_rate", &PlayRate, 0.0f, 0.0f, "%.3f");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Section.PlayRate = EMath::Max(PlayRate, 0.01f);
			CommitSection(InDocument, InSectionIndex, Section, EString("Section rate updated."));
			return;
		}

		FLOAT BlendIn = Section.BlendInSeconds;
		ImGui::TextDisabled("Blend In (s)");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputFloat("##montage_section_blend_in", &BlendIn, 0.0f, 0.0f, "%.3f");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Section.BlendInSeconds = EMath::Max(BlendIn, 0.0f);
			CommitSection(InDocument, InSectionIndex, Section, EString("Section blend-in updated."));
			return;
		}

		FLOAT BlendOut = Section.BlendOutSeconds;
		ImGui::TextDisabled("Blend Out (s)");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputFloat("##montage_section_blend_out", &BlendOut, 0.0f, 0.0f, "%.3f");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Section.BlendOutSeconds = EMath::Max(BlendOut, 0.0f);
			CommitSection(InDocument, InSectionIndex, Section, EString("Section blend-out updated."));
			return;
		}

		bool bAdditive = (Section.bAdditive != FALSE);
		if (ImGui::Checkbox("Additive", &bAdditive))
		{
			Section.bAdditive = (bAdditive ? TRUE : FALSE);
			CommitSection(InDocument, InSectionIndex, Section, EString("Section additive updated."));
			return;
		}

		if (ImGui::TreeNode("Bone Mask"))
		{
			ImGui::TextDisabled("Empty list = full body.");
			FLOAT MaskWeight = Section.BoneMask.Weight;
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::SliderFloat("##montage_mask_weight", &MaskWeight, 0.0f, 1.0f, "%.2f"
				, ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				Section.BoneMask.Weight = MaskWeight;
				CommitSection(InDocument, InSectionIndex, Section, EString("Bone mask weight updated."));
				ImGui::TreePop();
				return;
			}
		if (m_SkeletonAsset)
		{
			const ESkeleton* MaskSkeleton = m_SkeletonAsset->GetStoragedResource();
			if (MaskSkeleton)
			{
				ImGui::SetNextItemWidth(-FLT_MIN);
				m_MaskSearchBuffer.InputText("##montage_mask_search");
				const EString MaskSearch = m_MaskSearchBuffer.ToString();
				if (ImGui::Button("Clear Mask"))
				{
					if (!Section.BoneMask.BoneNames.IsEmpty())
					{
						Section.BoneMask.BoneNames.Empty();
						CommitSection(InDocument, InSectionIndex, Section, EString("Bone mask cleared."));
						ImGui::TreePop();
						return;
					}
				}
				ImGui::SameLine();
				ImGui::TextDisabled("%d selected", Section.BoneMask.BoneNames.Num());

				if (ImGui::BeginChild("##montage_mask_bones", ImVec2(0.0f, 150.0f), true))
				{
					INT32 VisibleBoneCount = 0;
					const TArray<EBoneData>& Bones = MaskSkeleton->GetBonePart();
					for (INT32 BoneIndex = 0; BoneIndex < Bones.Num(); BoneIndex++)
					{
						const EBoneData& Bone = Bones[BoneIndex];
						if ((MaskSearch.Length() > 0u) && (Bone.Name.Contains(MaskSearch) == FALSE))
						{
							continue;
						}
						VisibleBoneCount++;
						bool bChecked = (Section.BoneMask.BoneNames.Contains(Bone.Name) != FALSE);
						ImGui::PushID(BoneIndex);
						if (ImGui::Checkbox(*Bone.Name, &bChecked))
						{
							TArray<EString> NewBones;
							for (INT32 ExistingIndex = 0; ExistingIndex < Section.BoneMask.BoneNames.Num(); ExistingIndex++)
							{
								if (Section.BoneMask.BoneNames[ExistingIndex] != Bone.Name)
								{
									NewBones.Add(Section.BoneMask.BoneNames[ExistingIndex]);
								}
							}
							if (bChecked)
							{
								NewBones.Add(Bone.Name);
							}
							Section.BoneMask.BoneNames = NewBones;
							Section.BoneMask.Name = ((Section.BoneMask.Name.Length() > 0u)
								? Section.BoneMask.Name : EString("Mask"));
							CommitSection(InDocument, InSectionIndex, Section, EString("Bone mask updated."));
							ImGui::PopID();
							ImGui::EndChild();
							ImGui::TreePop();
							return;
						}
						ImGui::PopID();
					}
					if (VisibleBoneCount <= 0)
					{
						ImGui::TextDisabled("No bones match the filter.");
					}
				}
				ImGui::EndChild();
			}
		}
		else
		{
			ImGui::TextDisabled("Skeleton is not loaded.");
		}
			ImGui::TreePop();
		}

		if (ImGui::Button("Add Notify", ImVec2(-FLT_MIN, 0.0f)))
		{
			AddNotify(InDocument);
			return;
		}
		if (ImGui::Button("Delete Section", ImVec2(-FLT_MIN, 0.0f)))
		{
			DeleteSelectedSection(InDocument);
		}
	}

	void FMontageEditorPanel::DrawNotifyInspector(FMontageDocument& InDocument, INT32 InSectionIndex
		, INT32 InNotifyIndex)
	{
		const EAnimMontageSection* Section = GetSelectedSection(InDocument);
		if ((!Section) || (InNotifyIndex < 0) || (InNotifyIndex >= Section->Notifies.Num()))
		{
			return;
		}
		FAnimNotifyEvent Notify = Section->Notifies[InNotifyIndex];
		ImGui::Text("Notify: %s", *Notify.NotifyName);
		ImGui::TextDisabled("Section: %s", *Section->Name);
		if (ImGui::Button("Back to Section"))
		{
			m_SelectedNotifyIndex = -1;
			InDocument.Selection.Type = EEditorSelectionType::SECTION;
			InDocument.Selection.ObjectIndex = InSectionIndex;
			InDocument.Selection.SubIndex = -1;
			ResetNotifyBuffers();
			return;
		}

		const EString BufferSource = EString("montage-notify:") + InDocument.AssetPath
			+ EString(":") + ToString(InSectionIndex) + EString(":") + ToString(InNotifyIndex);
		if (m_NotifyBufferSource != BufferSource)
		{
			m_NotifyNameBuffer.SetText(Notify.NotifyName);
			m_NotifyClassBuffer.SetText(Notify.NotifyClassName);
			m_NotifyAssetBuffer.SetText(Notify.NotifyAssetName);
			m_NotifyPayloadBuffer.SetText(Notify.PayloadJson);
			m_NotifyBufferSource = BufferSource;
		}

		ImGui::TextDisabled("Name");
		ImGui::SetNextItemWidth(-FLT_MIN);
		m_NotifyNameBuffer.InputText("##montage_notify_name");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Notify.NotifyName = m_NotifyNameBuffer.ToString();
			TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
			Notifies[InNotifyIndex] = Notify;
			CommitSectionNotifies(InDocument, InSectionIndex, Notifies, EString("Notify name updated."));
			return;
		}

		const TArray<EString> ClassNames = EAnimNotifyRegistry::GetRegisteredClassNames();
		// A resolved notify asset owns class/state; editing the class here would be overwritten
		// on the next rebuild. Show it read-only and offer an explicit "clear reference".
		const BOOL32 bUsesNotifyAsset = (Notify.NotifyAssetName.Length() > 0u);
		ImGui::TextDisabled("Class");
		if (bUsesNotifyAsset != FALSE)
		{
			ImGui::Text("Class: %s (from asset)", ((Notify.NotifyClassName.Length() > 0u)
				? *Notify.NotifyClassName : "None"));
		}
		else
		{
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::BeginCombo("##montage_notify_class", ((Notify.NotifyClassName.Length() > 0u)
				? *Notify.NotifyClassName : "None")))
			{
				BOOL32 bCommitted = FALSE;
				for (INT32 ClassIndex = 0; ClassIndex < ClassNames.Num(); ClassIndex++)
				{
					BOOL32 bClassIsState = FALSE;
					const BOOL32 bKnownClass = EAnimNotifyRegistry::IsStateClass(ClassNames[ClassIndex], bClassIsState);
					EString ClassLabel = ClassNames[ClassIndex];
					if (bKnownClass != FALSE)
					{
						ClassLabel += ((bClassIsState != FALSE) ? EString(" (State)") : EString(" (Instant)"));
					}
					const BOOL8 bSelected = (Notify.NotifyClassName == ClassNames[ClassIndex]);
					if (ImGui::Selectable(*ClassLabel, bSelected))
					{
						Notify.NotifyClassName = ClassNames[ClassIndex];
						if (bKnownClass != FALSE)
						{
							Notify.bIsState = bClassIsState;
						}
						if (Notify.bIsState != FALSE)
						{
							Notify.StateDuration = EMath::Max(Notify.StateDuration, 0.1f);
							Notify.bStateDurationOverride = TRUE;
						}
						Notify.ResetCompiledNotify();
						Notify.Compile();
						bCommitted = TRUE;
						break;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
				if (bCommitted != FALSE)
				{
					TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
					Notifies[InNotifyIndex] = Notify;
					CommitSectionNotifies(InDocument, InSectionIndex, Notifies, EString("Notify class updated."));
					return;
				}
			}
		}

		ImGui::TextDisabled("Notify Asset");
		ImGui::SetNextItemWidth(-FLT_MIN);
		m_NotifyAssetBuffer.InputText("##montage_notify_asset");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Notify.NotifyAssetName = m_NotifyAssetBuffer.ToString();
			TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
			Notifies[InNotifyIndex] = Notify;
			CommitSectionNotifies(InDocument, InSectionIndex, Notifies, EString("Notify asset updated."));
			return;
		}
		if ((bUsesNotifyAsset != FALSE) && (ImGui::Button("Clear Asset Reference")))
		{
			Notify.NotifyAssetName = EString();
			Notify.ResetCompiledNotify();
			Notify.Compile();
			TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
			Notifies[InNotifyIndex] = Notify;
			CommitSectionNotifies(InDocument, InSectionIndex, Notifies, EString("Notify asset reference cleared."));
			return;
		}

		FLOAT Time = Notify.Time;
		ImGui::TextDisabled("Time (s)");
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputFloat("##montage_notify_time", &Time, 0.0f, 0.0f, "%.3f");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Notify.Time = EMath::Clamp(Time, 0.0f, GetSelectedClipDuration(InDocument));
			TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
			Notifies[InNotifyIndex] = Notify;
			CommitSectionNotifies(InDocument, InSectionIndex, Notifies, EString("Notify time updated."));
			return;
		}

		const CHAR* ClassNameText = ((Notify.NotifyClassName.Length() > 0u)
			? *Notify.NotifyClassName : "None");
		if (Notify.CachedNotify)
		{
			ImGui::Text("Kind: %s", (Notify.bIsState != FALSE) ? "State" : "Instant");
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f)
				, "Class '%s' is missing or does not match its State flag.", ClassNameText);
		}
		if (Notify.bIsState != FALSE)
		{
			FLOAT Duration = Notify.StateDuration;
			ImGui::TextDisabled("State Duration (s)");
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::InputFloat("##montage_notify_duration", &Duration, 0.0f, 0.0f, "%.3f");
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				Notify.StateDuration = EMath::Max(Duration, 0.0f);
				Notify.bStateDurationOverride = TRUE;
				TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
				Notifies[InNotifyIndex] = Notify;
				CommitSectionNotifies(InDocument, InSectionIndex, Notifies
					, EString("Notify duration updated."));
				return;
			}
		}

		ImGui::TextDisabled("Payload JSON");
		ImGui::SetNextItemWidth(-FLT_MIN);
		m_NotifyPayloadBuffer.InputTextMultiline("##montage_notify_payload", ImVec2(-FLT_MIN, 100.0f));
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Notify.PayloadJson = m_NotifyPayloadBuffer.ToString();
			Notify.bPayloadOverride = TRUE;
			TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
			Notifies[InNotifyIndex] = Notify;
			CommitSectionNotifies(InDocument, InSectionIndex, Notifies, EString("Notify payload updated."));
			return;
		}

		if (ImGui::Button("Delete Notify", ImVec2(-FLT_MIN, 0.0f)))
		{
			TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
			Notifies.RemoveAt(InNotifyIndex);
			CommitSectionNotifies(InDocument, InSectionIndex, Notifies, EString("Notify deleted."));
			m_SelectedNotifyIndex = -1;
			InDocument.Selection.Type = EEditorSelectionType::SECTION;
			InDocument.Selection.ObjectIndex = InSectionIndex;
			InDocument.Selection.SubIndex = -1;
			ResetNotifyBuffers();
		}
	}

	void FMontageEditorPanel::DrawBottomPanel(FMontageDocument& InDocument, FLOAT InTotalHeight)
	{
		const FLOAT MinBottom = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
		FEditorSplitter::HorizontalInverted("##montage_output_split", m_BottomHeight
			, MinBottom, 0.0f, InTotalHeight);
		if (ImGui::BeginChild("##montage_output", ImVec2(0.0f, m_BottomHeight), true))
		{
			if (ImGui::BeginTabBar("##montage_output_tabs"))
			{
				if (ImGui::BeginTabItem("Diagnostics"))
				{
					DrawDiagnostics(InDocument);
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Source"))
				{
					DrawSource(InDocument);
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::EndChild();
	}

	void FMontageEditorPanel::DrawDiagnostics(FMontageDocument& InDocument)
	{
		const TArray<FEditorDiagnostic>& Items = InDocument.Diagnostics.GetItems();
		if (Items.Num() <= 0)
		{
			ImGui::TextDisabled("No problems detected.");
			return;
		}
		for (INT32 DiagnosticIndex = 0; DiagnosticIndex < Items.Num(); DiagnosticIndex++)
		{
			const FEditorDiagnostic& Diagnostic = Items[DiagnosticIndex];
			const ImVec4 Color = (Diagnostic.Severity == EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR)
				? ImVec4(1.0f, 0.35f, 0.35f, 1.0f)
				: ((Diagnostic.Severity == EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING)
					? ImVec4(1.0f, 0.75f, 0.35f, 1.0f) : ImVec4(0.75f, 0.75f, 0.75f, 1.0f));
			ImGui::PushID(DiagnosticIndex);
			ImGui::PushStyleColor(ImGuiCol_Text, Color);
			if (ImGui::Selectable(*Diagnostic.Message))
			{
				InDocument.Selection.Type = Diagnostic.ObjectType;
				InDocument.Selection.ObjectId = Diagnostic.ObjectId;
				InDocument.Selection.ObjectIndex = Diagnostic.ObjectIndex;
				InDocument.Selection.SubIndex = Diagnostic.SubIndex;
				if ((Diagnostic.ObjectType == EEditorSelectionType::SECTION)
					|| (Diagnostic.ObjectType == EEditorSelectionType::NOTIFY))
				{
					m_SelectedSectionIndex = Diagnostic.ObjectIndex;
					m_SelectedNotifyIndex = (Diagnostic.ObjectType == EEditorSelectionType::NOTIFY)
						? Diagnostic.SubIndex : -1;
					ResetSectionBuffers();
					ResetNotifyBuffers();
				}
			}
			ImGui::PopStyleColor();
			ImGui::PopID();
		}
	}

	void FMontageEditorPanel::DrawSource(FMontageDocument& InDocument)
	{
		if (m_SourceDocument != &InDocument)
		{
			m_SourceBuffer.SetText(InDocument.DraftJson);
			m_SourceDocument = &InDocument;
		}
		if (ImGui::Button("Apply Text"))
		{
			const EString Text = m_SourceBuffer.ToString();
			RunMutation(InDocument, [&InDocument, Text](EString& OutError) -> BOOL32
				{
					return InDocument.CommitSourceText(Text, OutError);
				}, EString("Montage source applied."));
			if (InDocument.LastError.Length() <= 0u)
			{
				m_SourceBuffer.SetText(InDocument.DraftJson);
				ResetSectionBuffers();
				ResetNotifyBuffers();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Revert Text"))
		{
			m_SourceBuffer.SetText(InDocument.DraftJson);
		}
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));
		m_SourceBuffer.InputTextMultiline("##montage_source", ImVec2(-FLT_MIN, -FLT_MIN)
			, ImGuiInputTextFlags_AllowTabInput);
		ImGui::PopStyleColor();
	}

	void FMontageEditorPanel::BuildTimelineTracks(FMontageDocument& InDocument
		, TArray<FTimelineTrack>& OutTracks)const
	{
		OutTracks.Empty();
		const EAnimMontageSection* Section = GetSelectedSection(InDocument);
		if (!Section)
		{
			return;
		}
		FTimelineTrack Track;
		Track.Name = Section->Name;
		for (INT32 NotifyIndex = 0; NotifyIndex < Section->Notifies.Num(); NotifyIndex++)
		{
			const FAnimNotifyEvent& Notify = Section->Notifies[NotifyIndex];
			FTimelineMarker Marker;
			Marker.Label = ((Notify.NotifyName.Length() > 0u) ? Notify.NotifyName : Notify.NotifyClassName);
			Marker.Time = Notify.Time;
			Marker.Duration = (Notify.bIsState != FALSE) ? Notify.StateDuration : 0.0f;
			Marker.bRange = Notify.bIsState;
			Marker.bDraggable = TRUE;
			Marker.bResizable = Notify.bIsState;
			Marker.Color = (Notify.bIsState != FALSE)
				? IM_COL32(80, 150, 240, 220) : IM_COL32(235, 150, 60, 255);
			Marker.UserIndex = NotifyIndex;
			Track.Markers.Add(Marker);
		}
		OutTracks.Add(Track);
	}

	void FMontageEditorPanel::BindPreview(FMontageDocument& InDocument)
	{
		const EAnimMontage* Montage = InDocument.GetDraftMontage();
		const EAnimMontageSection* Section = GetSelectedSection(InDocument);
		const ESkeletonAnimationAsset* AnimationAsset = InDocument.GetAnimationAsset();
		if ((!AnimationAsset) || (!Montage) || (!Section))
		{
			if ((m_BoundAnimationAsset) || (m_BoundClipIndex != -1) || (m_PreviewScene.IsMontageMode() != FALSE))
			{
				m_PreviewScene.SetPlayAnimation(FALSE);
				m_PreviewScene.ClearMontage();
				m_PreviewScene.SetAnimationAsset(nullptr, -1);
				m_BoundAnimationAsset = nullptr;
				m_BoundClipIndex = -1;
			}
			return;
		}

		const ESkeletonAnimation* Animation = AnimationAsset->GetStoragedResource();
		if ((!m_bPreviewCatalogReady)
			|| (Animation && (m_PreviewSkeletonName != Animation->GetSkeletonName())))
		{
			RefreshPreviewCatalog(Animation ? Animation->GetSkeletonName() : EString());
		}
		const ESkinnedMeshAsset* Mesh = m_PreviewCatalog.LoadMesh(m_MeshIndex);
		const ESkeletonAsset* Skeleton = m_PreviewCatalog.LoadSkeleton(m_SkeletonIndex);
		if ((Mesh != m_MeshAsset) || (Skeleton != m_SkeletonAsset))
		{
			m_MeshAsset = Mesh;
			m_SkeletonAsset = Skeleton;
			m_BoundMeshAsset = nullptr;
			m_BoundSkeletonAsset = nullptr;
			m_BoundAnimationAsset = nullptr;
		}
		if ((m_BoundMeshAsset != m_MeshAsset) || (m_BoundSkeletonAsset != m_SkeletonAsset))
		{
			m_PreviewScene.SetSkinnedMesh(m_MeshAsset, m_SkeletonAsset);
			m_BoundMeshAsset = m_MeshAsset;
			m_BoundSkeletonAsset = m_SkeletonAsset;
			m_BoundAnimationAsset = nullptr;
			m_BoundClipIndex = -1;
		}
		// Play the draft montage section, not just its source clip, so BoneMask and the layer
		// are visible. The scene keeps the playhead when the same section is rebuilt by an edit.
		m_PreviewScene.SetMontage(Montage, AnimationAsset, Section->Name);
		m_BoundAnimationAsset = AnimationAsset;
		m_BoundClipIndex = InDocument.GetSectionClipIndex(m_SelectedSectionIndex);
		m_PreviewScene.SetPlayAnimation(m_Transport.bPlaying);
		m_PreviewScene.SetPlaybackRate(m_Transport.PlayRate);
		m_PreviewScene.SetPlaybackLoopMode(m_Transport.LoopMode);
	}

	void FMontageEditorPanel::RefreshPreviewCatalog(const EString& InSkeletonName)
	{
		m_PreviewCatalog.Refresh(InSkeletonName);
		m_MeshIndex = ((m_PreviewCatalog.GetMeshPaths().Num() > 0) ? 0 : -1);
		m_SkeletonIndex = FPreviewAssetCatalog::FindSkeletonIndex(m_PreviewCatalog.GetSkeletonPaths(), InSkeletonName);
		if ((m_SkeletonIndex < 0) && (m_PreviewCatalog.GetSkeletonPaths().Num() > 0))
		{
			m_SkeletonIndex = 0;
		}
		m_PreviewSkeletonName = InSkeletonName;
		m_bPreviewCatalogReady = TRUE;
		m_MeshAsset = nullptr;
		m_SkeletonAsset = nullptr;
		m_BoundMeshAsset = nullptr;
		m_BoundSkeletonAsset = nullptr;
		m_BoundAnimationAsset = nullptr;
		m_BoundClipIndex = -1;
	}

	BOOL32 FMontageEditorPanel::ApplyTransport(FMontageDocument& InDocument
		, const FTransportInteraction& InInteraction)
	{
		m_PreviewScene.SetPlayAnimation(m_Transport.bPlaying);
		m_PreviewScene.SetPlaybackRate(m_Transport.PlayRate);
		m_PreviewScene.SetPlaybackLoopMode(m_Transport.LoopMode);
		if (InInteraction.bSeekRequested != FALSE)
		{
			m_Transport.Time = EMath::Clamp(InInteraction.SeekTime, 0.0f
				, EMath::Max(GetSelectedClipDuration(InDocument), 0.0f));
			m_Transport.bPlaying = FALSE;
			m_PreviewScene.SetPlayAnimation(FALSE);
			m_PreviewScene.SetPlaybackTimeSeconds(m_Transport.Time);
			return TRUE;
		}
		return FALSE;
	}

	void FMontageEditorPanel::SyncTransport(FMontageDocument& InDocument)
	{
		m_Transport.Duration = GetSelectedClipDuration(InDocument);
		if (!ImGui::IsAnyItemActive())
		{
			m_Transport.Time = m_PreviewScene.GetPlaybackTimeSeconds();
		}
		m_Transport.Time = EMath::Clamp(m_Transport.Time, 0.0f, m_Transport.Duration);
	}

	void FMontageEditorPanel::RunMutation(FMontageDocument& InDocument
		, const TFunction<BOOL32(EString&)>& InMutation, const EString& InSuccessMessage)
	{
		InDocument.Commands.BeginTransaction(InDocument.CaptureSnapshot());
		EString Error;
		if (InMutation(Error))
		{
			InDocument.Commands.CommitTransaction(InDocument.CaptureSnapshot());
			InDocument.LastError = EString();
			m_StatusMessage = InSuccessMessage;
			m_bStatusSucceeded = TRUE;
			m_SourceDocument = nullptr;
			ResetSectionBuffers();
			ResetNotifyBuffers();
		}
		else
		{
			InDocument.Commands.CancelTransaction();
			InDocument.LastError = Error;
			m_StatusMessage = Error;
			m_bStatusSucceeded = FALSE;
		}
	}

	void FMontageEditorPanel::CommitSection(FMontageDocument& InDocument, INT32 InSectionIndex
		, const EAnimMontageSection& InSection, const EString& InSuccessMessage)
	{
		RunMutation(InDocument, [&InDocument, InSectionIndex, &InSection](EString& OutError) -> BOOL32
			{
				return InDocument.UpdateSection(InSectionIndex, InSection, OutError);
			}, InSuccessMessage);
	}

	void FMontageEditorPanel::CommitSectionNotifies(FMontageDocument& InDocument, INT32 InSectionIndex
		, const TArray<FAnimNotifyEvent>& InNotifies, const EString& InSuccessMessage)
	{
		RunMutation(InDocument, [&InDocument, InSectionIndex, &InNotifies](EString& OutError) -> BOOL32
			{
				return InDocument.SetSectionNotifies(InSectionIndex, InNotifies, OutError);
			}, InSuccessMessage);
	}

	void FMontageEditorPanel::AddSection(FMontageDocument& InDocument)
	{
		const EString DefaultClip = GetDefaultSectionClip(InDocument);
		const EString DefaultSlot = GetDefaultSectionSlot();
		RunMutation(InDocument, [&InDocument, DefaultClip, DefaultSlot](EString& OutError) -> BOOL32
			{
				return InDocument.AddSection(DefaultClip, DefaultSlot, OutError);
			}, EString("Section added."));
		const EAnimMontage* Montage = InDocument.GetDraftMontage();
		if (Montage && (Montage->Sections.Num() > 0))
		{
			m_SelectedSectionIndex = Montage->Sections.Num() - 1;
			m_SelectedNotifyIndex = -1;
			InDocument.Selection.Type = EEditorSelectionType::SECTION;
			InDocument.Selection.ObjectIndex = m_SelectedSectionIndex;
			InDocument.Selection.SubIndex = -1;
			ResetSectionBuffers();
		}
	}

	void FMontageEditorPanel::DeleteSelectedSection(FMontageDocument& InDocument)
	{
		if (m_SelectedSectionIndex < 0)
		{
			return;
		}
		const INT32 SectionIndex = m_SelectedSectionIndex;
		RunMutation(InDocument, [&InDocument, SectionIndex](EString& OutError) -> BOOL32
			{
				return InDocument.DeleteSection(SectionIndex, OutError);
			}, EString("Section deleted."));
		const EAnimMontage* Montage = InDocument.GetDraftMontage();
		if (Montage && (Montage->Sections.Num() > 0))
		{
			m_SelectedSectionIndex = EMath::Clamp(SectionIndex, 0, Montage->Sections.Num() - 1);
			m_SelectedNotifyIndex = -1;
			InDocument.Selection.Type = EEditorSelectionType::SECTION;
			InDocument.Selection.ObjectIndex = m_SelectedSectionIndex;
			InDocument.Selection.SubIndex = -1;
			ResetSectionBuffers();
		}
	}

	void FMontageEditorPanel::AddNotify(FMontageDocument& InDocument)
	{
		const EAnimMontageSection* Section = GetSelectedSection(InDocument);
		if (!Section)
		{
			return;
		}
		TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
		FAnimNotifyEvent Notify;
		Notify.NotifyName = DefaultMontageNotifyName(Notifies);
		BOOL32 bDefaultIsState = FALSE;
		Notify.NotifyClassName = FindDefaultMontageNotifyClass(bDefaultIsState);
		Notify.bIsState = bDefaultIsState;
		if (Notify.bIsState != FALSE)
		{
			Notify.StateDuration = 0.1f;
			Notify.bStateDurationOverride = TRUE;
		}
		Notify.Time = EMath::Clamp(m_Transport.Time, 0.0f
			, EMath::Max(GetSelectedClipDuration(InDocument), 0.0f));
		Notify.PayloadJson = EString("{}");
		Notify.Compile();
		Notifies.Add(Notify);
		const INT32 NewNotifyIndex = Notifies.Num() - 1;
		CommitSectionNotifies(InDocument, m_SelectedSectionIndex, Notifies, EString("Notify added."));
		m_SelectedNotifyIndex = NewNotifyIndex;
		InDocument.Selection.Type = EEditorSelectionType::NOTIFY;
		InDocument.Selection.ObjectIndex = m_SelectedSectionIndex;
		InDocument.Selection.SubIndex = m_SelectedNotifyIndex;
		ResetNotifyBuffers();
	}

	void FMontageEditorPanel::DeleteSelectedNotify(FMontageDocument& InDocument)
	{
		const EAnimMontageSection* Section = GetSelectedSection(InDocument);
		if ((!Section) || (m_SelectedNotifyIndex < 0) || (m_SelectedNotifyIndex >= Section->Notifies.Num()))
		{
			return;
		}
		TArray<FAnimNotifyEvent> Notifies = Section->Notifies;
		Notifies.RemoveAt(m_SelectedNotifyIndex);
		CommitSectionNotifies(InDocument, m_SelectedSectionIndex, Notifies, EString("Notify deleted."));
		m_SelectedNotifyIndex = -1;
		InDocument.Selection.Type = EEditorSelectionType::SECTION;
		InDocument.Selection.ObjectIndex = m_SelectedSectionIndex;
		InDocument.Selection.SubIndex = -1;
		ResetNotifyBuffers();
	}

	void FMontageEditorPanel::ResetSectionBuffers()
	{
		m_SectionBufferSource = EString();
	}

	void FMontageEditorPanel::ResetNotifyBuffers()
	{
		m_NotifyBufferSource = EString();
	}

	void FMontageEditorPanel::RestoreDocumentSnapshot(FMontageDocument& InDocument
		, const FEditorSnapshot& InSnapshot)
	{
		InDocument.RestoreSnapshot(InSnapshot);
		const EAnimMontage* Montage = InDocument.GetDraftMontage();
		if (Montage && (Montage->Sections.Num() > 0))
		{
			m_SelectedSectionIndex = EMath::Clamp(InDocument.Selection.ObjectIndex
				, 0, Montage->Sections.Num() - 1);
			if (InDocument.Selection.Is(EEditorSelectionType::NOTIFY))
			{
				const EAnimMontageSection& Section = Montage->Sections[m_SelectedSectionIndex];
				m_SelectedNotifyIndex = InDocument.Selection.SubIndex;
				if ((m_SelectedNotifyIndex < 0) || (m_SelectedNotifyIndex >= Section.Notifies.Num()))
				{
					m_SelectedNotifyIndex = -1;
					InDocument.Selection.Type = EEditorSelectionType::SECTION;
					InDocument.Selection.ObjectIndex = m_SelectedSectionIndex;
					InDocument.Selection.SubIndex = -1;
				}
			}
			else
			{
				m_SelectedNotifyIndex = -1;
			}
		}
		else
		{
			m_SelectedSectionIndex = -1;
			m_SelectedNotifyIndex = -1;
		}
		ResetSectionBuffers();
		ResetNotifyBuffers();
		m_SourceDocument = nullptr;
	}

	INT32 FMontageEditorPanel::GetSelectedSectionIndex(const FMontageDocument& InDocument)const
	{
		const EAnimMontage* Montage = InDocument.GetDraftMontage();
		if ((!Montage) || (m_SelectedSectionIndex < 0) || (m_SelectedSectionIndex >= Montage->Sections.Num()))
		{
			return -1;
		}
		return m_SelectedSectionIndex;
	}

	const EAnimMontageSection* FMontageEditorPanel::GetSelectedSection(const FMontageDocument& InDocument)const
	{
		const INT32 SectionIndex = GetSelectedSectionIndex(InDocument);
		return ((SectionIndex >= 0) ? &InDocument.GetDraftMontage()->Sections[SectionIndex] : nullptr);
	}

	const ESkeletonAnimationClip* FMontageEditorPanel::GetSelectedClip(const FMontageDocument& InDocument)const
	{
		return InDocument.GetSectionClip(m_SelectedSectionIndex);
	}

	FLOAT FMontageEditorPanel::GetSelectedClipDuration(const FMontageDocument& InDocument)const
	{
		const ESkeletonAnimationClip* Clip = GetSelectedClip(InDocument);
		return (Clip ? MontageClipDurationSeconds(*Clip) : 0.0f);
	}

	EString FMontageEditorPanel::GetDefaultSectionClip(const FMontageDocument& InDocument)const
	{
		const ESkeletonAnimationClip* Clip = GetSelectedClip(InDocument);
		return (Clip ? Clip->ClipName : EString());
	}

	EString FMontageEditorPanel::GetDefaultSectionSlot()const
	{
		const ESkeleton* Skeleton = (m_SkeletonAsset ? m_SkeletonAsset->GetStoragedResource() : nullptr);
		if (Skeleton)
		{
			const TArray<ESkeletonSlotGroup>& Groups = Skeleton->GetSlotGroups();
			for (INT32 GroupIndex = 0; GroupIndex < Groups.Num(); GroupIndex++)
			{
				if (Groups[GroupIndex].SlotNames.Num() > 0)
				{
					return Groups[GroupIndex].SlotNames[0];
				}
			}
		}
		return EString("DefaultSlot");
	}

}

#endif
