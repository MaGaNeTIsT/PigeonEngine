#include "ClipEditorPanel.h"

#if _EDITOR_ONLY

#include <AnimationAsset/AnimationAsset.h>
#include <Animation/AnimationNotify.h>
#include <AnimationNotifyAsset/AnimationNotifyAsset.h>
#include "../AnimationCurvePlot/AnimationCurvePlot.h"
#include <Config/EngineConfig.h>
#include <Base/DataStructure/Text/Path.h>
#include <imgui.h>
#include <cfloat>
#include <cstdio>

namespace PigeonEngine
{

	namespace
	{
		FLOAT ClipDurationSeconds(const ESkeletonAnimationClip& InClip)
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

		INT32 CurveKeyCount(const EBoneAnimationCurve& InCurve)
		{
			return InCurve.PositionKeys.Num() + InCurve.RotationKeys.Num() + InCurve.ScalingKeys.Num();
		}

		EString DefaultNotifyName(const TArray<TArray<FAnimNotifyEvent>>& InNotifies, INT32 InClipIndex)
		{
			INT32 Suffix = 0;
			EString Name;
			BOOL32 bExists = TRUE;
			while (bExists != FALSE)
			{
				bExists = FALSE;
				Name = EString("Notify") + ToString(Suffix++);
				for (INT32 NotifyIndex = 0; NotifyIndex < InNotifies[InClipIndex].Num(); NotifyIndex++)
				{
					if (InNotifies[InClipIndex][NotifyIndex].NotifyName == Name)
					{
						bExists = TRUE;
						break;
					}
				}
			}
			return Name;
		}
	}

	FClipEditorPanel::FClipEditorPanel()
	{
		m_PreviewScene.Initialize("ClipPreview", Vector3::Zero());
		m_bPreviewInitialized = TRUE;
	}

	FClipEditorPanel::~FClipEditorPanel()
	{
		if (m_bPreviewInitialized != FALSE)
		{
			m_PreviewScene.Shutdown();
		}
	}

	void FClipEditorPanel::Draw(FClipDocument& InDocument)
	{
		const ESkeletonAnimation* Animation = (InDocument.GetAsset()
			? InDocument.GetAsset()->GetStoragedResource() : nullptr);
		if (!Animation)
		{
			ImGui::TextDisabled("Animation asset has no clips.");
			return;
		}
		EnsureSelection(InDocument);
		BindPreview(InDocument);
		m_PreviewScene.Tick(ImGui::GetIO().DeltaTime);
		SyncTransport(InDocument);
		DrawToolbar(InDocument);
		const FLOAT AvailableHeight = ImGui::GetContentRegionAvail().y;
		// Layout against the real content area. There is no artificial height cap: the bottom
		// pane can grow until the content above collapses, and the workspace window itself is
		// clamped inside the viewport (see EAnimationWorkspace::DrawWorkspace).
		const FLOAT TotalHeight = EMath::Max(0.0f, AvailableHeight);
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

	void FClipEditorPanel::EnsureSelection(FClipDocument& InDocument)
	{
		const ESkeletonAnimation* Animation = InDocument.GetAsset()->GetStoragedResource();
		if (!Animation)
		{
			return;
		}
		m_SelectedClipIndex = EMath::Clamp(m_SelectedClipIndex, 0, Animation->GetAnimationClipNum() - 1);
		const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(m_SelectedClipIndex);
		if (!Clip)
		{
			return;
		}
		m_SelectedCurveIndex = EMath::Clamp(m_SelectedCurveIndex, 0
			, EMath::Max(Clip->AnimationCurves.Num() - 1, 0));
		if ((InDocument.Selection.Is(EEditorSelectionType::NOTIFY))
			&& (InDocument.Selection.ObjectIndex == m_SelectedClipIndex)
			&& (InDocument.Selection.SubIndex >= 0)
			&& (InDocument.Selection.SubIndex < InDocument.GetDraftNotifies()[m_SelectedClipIndex].Num()))
		{
			m_SelectedNotifyIndex = InDocument.Selection.SubIndex;
		}
		else if ((m_SelectedNotifyIndex < 0)
			|| (m_SelectedNotifyIndex >= InDocument.GetDraftNotifies()[m_SelectedClipIndex].Num()))
		{
			m_SelectedNotifyIndex = -1;
		}
		if (InDocument.Selection.Type == EEditorSelectionType::NONE)
		{
			InDocument.Selection.Type = EEditorSelectionType::CLIP;
			InDocument.Selection.ObjectIndex = m_SelectedClipIndex;
		}
	}

	ESkeletonAnimationClip* FClipEditorPanel::GetSelectedClip(FClipDocument& InDocument)const
	{
		const ESkeletonAnimationAsset* Asset = InDocument.GetAsset();
		if (!Asset)
		{
			return nullptr;
		}
		const ESkeletonAnimation* Animation = Asset->GetStoragedResource();
		if (!Animation)
		{
			return nullptr;
		}
		return const_cast<ESkeletonAnimationClip*>(Animation->GetAnimationClip(m_SelectedClipIndex));
	}

	FLOAT FClipEditorPanel::GetSelectedClipDuration(FClipDocument& InDocument)const
	{
		const ESkeletonAnimationClip* Clip = GetSelectedClip(InDocument);
		return (Clip ? ClipDurationSeconds(*Clip) : 0.0f);
	}

	void FClipEditorPanel::DrawToolbar(FClipDocument& InDocument)
	{
		if (ImGui::Button("Apply"))
		{
			EString Error;
			if (InDocument.Apply(Error))
			{
				m_StatusMessage = EString("Applied notify draft to the live clip.");
				m_bStatusSucceeded = TRUE;
				m_SourceDocument = nullptr;
			}
			else
			{
				m_StatusMessage = Error;
				m_bStatusSucceeded = FALSE;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Save"))
		{
			EString Error;
			if (InDocument.Save(Error))
			{
				m_StatusMessage = EString("Saved to disk.");
				m_bStatusSucceeded = TRUE;
				m_SourceDocument = nullptr;
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
			if (InDocument.IsDirty())
			{
				ImGui::OpenPopup("##clip_reload_confirm");
			}
			else
			{
				EString Error;
				if (InDocument.Reload(Error))
				{
					m_StatusMessage = EString("Reloaded from disk.");
					m_bStatusSucceeded = TRUE;
					ResetNotifyBuffers();
					m_SourceDocument = nullptr;
				}
				else
				{
					m_StatusMessage = Error;
					m_bStatusSucceeded = FALSE;
				}
			}
		}
		if (ImGui::BeginPopupModal("##clip_reload_confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Reload will discard unsaved changes.");
			EString Error;
			if (ImGui::Button("Save and Reload"))
			{
				if (InDocument.Save(Error) && InDocument.Reload(Error))
				{
					m_StatusMessage = EString("Saved and reloaded.");
					m_bStatusSucceeded = TRUE;
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
			if (ImGui::Button("Discard and Reload"))
			{
				if (InDocument.Discard(Error))
				{
					m_StatusMessage = EString("Discarded and reloaded.");
					m_bStatusSucceeded = TRUE;
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

		ImGui::SameLine();
		ImGui::BeginDisabled(InDocument.Commands.CanUndo() == FALSE);
		if (ImGui::Button("Undo"))
		{
			FEditorSnapshot Snapshot;
			if (InDocument.Commands.Undo(InDocument.CaptureSnapshot(), Snapshot))
			{
				InDocument.RestoreSnapshot(Snapshot);
				ResetNotifyBuffers();
				m_SourceDocument = nullptr;
				m_StatusMessage = EString("Undo.");
				m_bStatusSucceeded = TRUE;
			}
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(InDocument.Commands.CanRedo() == FALSE);
		if (ImGui::Button("Redo"))
		{
			FEditorSnapshot Snapshot;
			if (InDocument.Commands.Redo(InDocument.CaptureSnapshot(), Snapshot))
			{
				InDocument.RestoreSnapshot(Snapshot);
				ResetNotifyBuffers();
				m_SourceDocument = nullptr;
				m_StatusMessage = EString("Redo.");
				m_bStatusSucceeded = TRUE;
			}
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Output"))
		{
			m_bShowOutput = ((m_bShowOutput != FALSE) ? FALSE : TRUE);
		}

		ImGui::SameLine();
		if (InDocument.IsDirty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f), "UNSAVED");
		}
		else
		{
			ImGui::TextDisabled("SAVED");
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%s", *InDocument.AssetPath);
		if (InDocument.LastError.Length() > 0u)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *InDocument.LastError);
		}
		if (m_StatusMessage.Length() > 0u)
		{
			const ImVec4 Color = (m_bStatusSucceeded != FALSE)
				? ImVec4(0.45f, 0.85f, 0.45f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
			ImGui::TextColored(Color, "%s", *m_StatusMessage);
		}
		ImGui::Separator();
	}

	void FClipEditorPanel::DrawLayout(FClipDocument& InDocument, FLOAT InHeight)
	{
		if (InHeight == 0.0f)
		{
			return;
		}
		const ImVec2 Available = ImGui::GetContentRegionAvail();
		const FLOAT FitWidth = EMath::Max(320.0f, Available.x);
		const FLOAT FitHeight = InHeight;
		if (ImGui::BeginChild("##clip_layout_fit", ImVec2(FitWidth, FitHeight), false))
		{
			const ImGuiTableFlags LayoutFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV
				| ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;
			if (ImGui::BeginTable("##clip_layout", 3, LayoutFlags))
			{
				ImGui::TableSetupColumn("##left", ImGuiTableColumnFlags_WidthFixed, 230.0f);
				ImGui::TableSetupColumn("##center", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("##right", ImGuiTableColumnFlags_WidthFixed, 350.0f);
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				if (ImGui::BeginChild("##clip_left", ImVec2(0.0f, 0.0f), false))
				{
					DrawLeftPanel(InDocument);
				}
				ImGui::EndChild();

				ImGui::TableSetColumnIndex(1);
				if (ImGui::BeginChild("##clip_center", ImVec2(0.0f, 0.0f), false))
				{
					DrawCenter(InDocument);
				}
				ImGui::EndChild();

				ImGui::TableSetColumnIndex(2);
				if (ImGui::BeginChild("##clip_right", ImVec2(0.0f, 0.0f), false))
				{
					DrawInspector(InDocument);
				}
				ImGui::EndChild();
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}

	void FClipEditorPanel::DrawCenter(FClipDocument& InDocument)
	{
		const FLOAT TotalHeight = ImGui::GetContentRegionAvail().y;
		const FLOAT SplitterCost = FEditorSplitter::GetCost();
		const FLOAT AvailableForPanes = EMath::Max(0.0f, TotalHeight - 2.0f * SplitterCost);
		if (AvailableForPanes <= 0.0f)
		{
			return;
		}

		m_PreviewHeight = EMath::Clamp(m_PreviewHeight, 0.0f, AvailableForPanes);
		m_TimelineHeight = EMath::Clamp(m_TimelineHeight, 0.0f
			, EMath::Max(0.0f, AvailableForPanes - m_PreviewHeight));
		const FLOAT CurveHeight = EMath::Max(0.0f
			, AvailableForPanes - m_PreviewHeight - m_TimelineHeight);

		if (m_PreviewHeight > 0.0f)
		{
			if (ImGui::BeginChild("##clip_preview_area", ImVec2(0.0f, m_PreviewHeight), true))
			{
				DrawPreviewArea(InDocument);
			}
			ImGui::EndChild();
		}
		FEditorSplitter::Horizontal("##clip_preview_split", m_PreviewHeight
			, 0.0f, 0.0f, AvailableForPanes + SplitterCost);

		const FLOAT RemainingForTimeline = EMath::Max(0.0f, AvailableForPanes - m_PreviewHeight);
		if (m_TimelineHeight > 0.0f)
		{
			if (ImGui::BeginChild("##clip_timeline_area", ImVec2(0.0f, m_TimelineHeight), true))
			{
				DrawTimelineArea(InDocument);
			}
			ImGui::EndChild();
		}
		FEditorSplitter::Horizontal("##clip_timeline_split", m_TimelineHeight
			, 0.0f, 0.0f, RemainingForTimeline + SplitterCost);

		if (CurveHeight > 0.0f)
		{
			if (ImGui::BeginChild("##clip_curve_area", ImVec2(0.0f, CurveHeight), true))
			{
				DrawCurveArea(InDocument);
			}
			ImGui::EndChild();
		}
	}

	void FClipEditorPanel::DrawLeftPanel(FClipDocument& InDocument)
	{
		const ESkeletonAnimation* Animation = InDocument.GetAsset()->GetStoragedResource();
		ImGui::Text("Clips: %d", Animation->GetAnimationClipNum());
		ImGui::Separator();
		if (ImGui::BeginChild("##clip_list", ImVec2(0.0f, 180.0f), true))
		{
			for (INT32 ClipIndex = 0; ClipIndex < Animation->GetAnimationClipNum(); ClipIndex++)
			{
				const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
				if (!Clip)
				{
					continue;
				}
				ImGui::PushID(ClipIndex);
				CHAR Label[160] = "";
				::snprintf(Label, sizeof(Label), "%s  %.2fs", *Clip->ClipName, ClipDurationSeconds(*Clip));
				if (ImGui::Selectable(Label, (ClipIndex == m_SelectedClipIndex)))
				{
					m_SelectedClipIndex = ClipIndex;
					m_SelectedCurveIndex = 0;
					m_SelectedNotifyIndex = -1;
					InDocument.Selection.Type = EEditorSelectionType::CLIP;
					InDocument.Selection.ObjectIndex = ClipIndex;
					ResetNotifyBuffers();
					m_PreviewScene.ResetPlayback();
				}
				ImGui::PopID();
			}
		}
		ImGui::EndChild();

		ImGui::Separator();
		ImGui::TextDisabled("Curves");
		const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(m_SelectedClipIndex);
		if ((!Clip) || (Clip->AnimationCurves.Num() <= 0))
		{
			ImGui::TextDisabled("No curves.");
			return;
		}
		if (ImGui::BeginChild("##curve_list", ImVec2(0.0f, 0.0f), true))
		{
			for (INT32 CurveIndex = 0; CurveIndex < Clip->AnimationCurves.Num(); CurveIndex++)
			{
				const EBoneAnimationCurve& Curve = Clip->AnimationCurves[CurveIndex];
				ImGui::PushID(CurveIndex);
				CHAR Label[192] = "";
				::snprintf(Label, sizeof(Label), "%s  (%d keys)", *Curve.BoneName, CurveKeyCount(Curve));
				if (ImGui::Selectable(Label, (CurveIndex == m_SelectedCurveIndex)))
				{
					m_SelectedCurveIndex = CurveIndex;
					InDocument.Selection.Type = EEditorSelectionType::CURVE;
					InDocument.Selection.ObjectId = Curve.BoneName;
					InDocument.Selection.ObjectIndex = CurveIndex;
					m_SelectedNotifyIndex = -1;
				}
				ImGui::PopID();
			}
		}
		ImGui::EndChild();
	}

	void FClipEditorPanel::BuildTimelineTracks(FClipDocument& InDocument, TArray<FTimelineTrack>& OutTracks)const
	{
		OutTracks.Empty();
		const ESkeletonAnimationClip* Clip = GetSelectedClip(InDocument);
		if (!Clip)
		{
			return;
		}
		FTimelineTrack Track;
		Track.Name = Clip->ClipName;
		const TArray<FAnimNotifyEvent>& Notifies = InDocument.GetDraftNotifies()[m_SelectedClipIndex];
		for (INT32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); NotifyIndex++)
		{
			const FAnimNotifyEvent& Notify = Notifies[NotifyIndex];
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

	void FClipEditorPanel::DrawTimelineArea(FClipDocument& InDocument)
	{
		FTransportInteraction TransportInteraction;
		FTransportBar::Draw("##clip_transport", m_Transport, TransportInteraction);
		ApplyTransport(InDocument, TransportInteraction);

		if (ImGui::Button("Add Notify"))
		{
			AddNotify(InDocument);
			return;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Drag a marker to retime it; Ctrl+Wheel zoom, Shift+Wheel/middle-drag pan, middle double-click fit.");

		TArray<FTimelineTrack> Tracks;
		BuildTimelineTracks(InDocument, Tracks);
		FTimelineInteraction TimelineInteraction;
		m_Timeline.Draw("##clip_timeline", Tracks, GetSelectedClipDuration(InDocument)
			, m_Transport.Time, 0, m_SelectedNotifyIndex, TimelineInteraction);
		if (TimelineInteraction.bMarkerSelected != FALSE)
		{
			m_SelectedNotifyIndex = TimelineInteraction.SelectedMarker;
			InDocument.Selection.Type = EEditorSelectionType::NOTIFY;
			InDocument.Selection.ObjectIndex = m_SelectedClipIndex;
			InDocument.Selection.SubIndex = m_SelectedNotifyIndex;
			ResetNotifyBuffers();
		}
		if (TimelineInteraction.bScrubbed != FALSE)
		{
			m_PreviewScene.SetPlaybackTimeSeconds(TimelineInteraction.ScrubbedTime);
			m_PreviewScene.SetPlayAnimation(FALSE);
			m_Transport.bPlaying = FALSE;
			m_Transport.Time = TimelineInteraction.ScrubbedTime;
		}
		if ((TimelineInteraction.bMarkerResizeFinished != FALSE)
			&& (m_SelectedNotifyIndex >= 0))
		{
			TArray<TArray<FAnimNotifyEvent>> NewNotifies = InDocument.GetDraftNotifies();
			if ((m_SelectedClipIndex >= 0) && (m_SelectedClipIndex < NewNotifies.Num())
				&& (m_SelectedNotifyIndex >= 0)
				&& (m_SelectedNotifyIndex < NewNotifies[m_SelectedClipIndex].Num()))
			{
				FAnimNotifyEvent& Notify = NewNotifies[m_SelectedClipIndex][m_SelectedNotifyIndex];
				if (Notify.bIsState != FALSE)
				{
					Notify.Time = EMath::Max(TimelineInteraction.MarkerResizeTime, 0.0f);
					Notify.StateDuration = EMath::Max(TimelineInteraction.MarkerResizeDuration, 0.01f);
					Notify.bStateDurationOverride = TRUE;
					RunMutation(InDocument, [&InDocument, &NewNotifies](EString& OutError) -> BOOL32
						{
							return InDocument.CommitNotifies(NewNotifies, OutError);
						}, EString("Notify duration updated."));
					ResetNotifyBuffers();
				}
			}
		}
		if ((TimelineInteraction.bMarkerDragFinished != FALSE)
			&& (m_SelectedNotifyIndex >= 0))
		{
			TArray<TArray<FAnimNotifyEvent>> NewNotifies = InDocument.GetDraftNotifies();
			if ((m_SelectedClipIndex >= 0) && (m_SelectedClipIndex < NewNotifies.Num())
				&& (m_SelectedNotifyIndex >= 0)
				&& (m_SelectedNotifyIndex < NewNotifies[m_SelectedClipIndex].Num()))
			{
				NewNotifies[m_SelectedClipIndex][m_SelectedNotifyIndex].Time =
					TimelineInteraction.MarkerDragTime;
				RunMutation(InDocument, [&InDocument, &NewNotifies](EString& OutError) -> BOOL32
					{
						return InDocument.CommitNotifies(NewNotifies, OutError);
					}, EString("Notify retimed."));
				ResetNotifyBuffers();
			}
		}
	}

	void FClipEditorPanel::DrawPreviewArea(FClipDocument& InDocument)
	{
		if (!m_bPreviewCatalogReady)
		{
			const ESkeletonAnimation* Animation = InDocument.GetAsset()->GetStoragedResource();
			RefreshPreviewCatalog(Animation ? Animation->GetSkeletonName() : EString());
		}
		const TArray<EString>& MeshPaths = m_PreviewCatalog.GetMeshPaths();
		const TArray<EString>& SkeletonPaths = m_PreviewCatalog.GetSkeletonPaths();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##clip_preview_mesh", ((m_MeshIndex >= 0) && (m_MeshIndex < MeshPaths.Num()))
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
		if (ImGui::BeginCombo("##clip_preview_skeleton"
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

	void FClipEditorPanel::DrawCurveArea(FClipDocument& InDocument)
	{
		const ESkeletonAnimationClip* Clip = GetSelectedClip(InDocument);
		if ((!Clip) || (Clip->AnimationCurves.Num() <= 0))
		{
			ImGui::TextDisabled("No curves.");
			return;
		}
		m_SelectedCurveIndex = EMath::Clamp(m_SelectedCurveIndex, 0, Clip->AnimationCurves.Num() - 1);
		const EBoneAnimationCurve& Curve = Clip->AnimationCurves[m_SelectedCurveIndex];
		ImGui::Text("Bone: %s  (%d keys)", *Curve.BoneName, CurveKeyCount(Curve));
		const FLOAT Duration = ClipDurationSeconds(*Clip);
		const FLOAT Width = EMath::Max(ImGui::GetContentRegionAvail().x - 8.0f, 80.0f);
		if (Curve.PositionKeys.Num() > 0)
		{
			EAnimationCurvePlot::DrawVectorKeys("Position", Curve.PositionKeys, Duration, Width, 60.0f);
		}
		if (Curve.RotationKeys.Num() > 0)
		{
			EAnimationCurvePlot::DrawRotationKeys(Curve.RotationKeys, Duration, Width, 60.0f);
		}
		if (Curve.ScalingKeys.Num() > 0)
		{
			EAnimationCurvePlot::DrawVectorKeys("Scaling", Curve.ScalingKeys, Duration, Width, 60.0f);
		}
	}

	void FClipEditorPanel::DrawInspector(FClipDocument& InDocument)
	{
		FImGuiIdScope InspectorScope("ClipInspector");
		ImGui::TextDisabled("Inspector");
		ImGui::Separator();
		if ((InDocument.Selection.Is(EEditorSelectionType::NOTIFY))
			&& (m_SelectedNotifyIndex >= 0))
		{
			DrawNotifyInspector(InDocument, m_SelectedNotifyIndex);
		}
		else if (InDocument.Selection.Is(EEditorSelectionType::CURVE))
		{
			const ESkeletonAnimationClip* Clip = GetSelectedClip(InDocument);
			if ((Clip) && (m_SelectedCurveIndex >= 0) && (m_SelectedCurveIndex < Clip->AnimationCurves.Num()))
			{
				const EBoneAnimationCurve& Curve = Clip->AnimationCurves[m_SelectedCurveIndex];
				ImGui::Text("Curve: %s", *Curve.BoneName);
				ImGui::TextDisabled("Keys: %d", CurveKeyCount(Curve));
				ImGui::TextDisabled("Use the Curves tab for the plot.");
			}
		}
		else
		{
			DrawClipInspector(InDocument);
		}
	}

	void FClipEditorPanel::DrawClipInspector(FClipDocument& InDocument)
	{
		const ESkeletonAnimationClip* Clip = GetSelectedClip(InDocument);
		if (!Clip)
		{
			return;
		}
		ImGui::Text("Clip: %s", *Clip->ClipName);
		ImGui::Text("Duration: %.3fs", ClipDurationSeconds(*Clip));
		ImGui::Text("Curves: %d", Clip->AnimationCurves.Num());
		ImGui::Text("Notifies: %d", InDocument.GetDraftNotifies()[m_SelectedClipIndex].Num());
		if (ImGui::Button("Add Notify", ImVec2(-FLT_MIN, 0.0f)))
		{
			AddNotify(InDocument);
			return;
		}
		ImGui::Separator();
		const TArray<FAnimNotifyEvent>& Notifies = InDocument.GetDraftNotifies()[m_SelectedClipIndex];
		for (INT32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); NotifyIndex++)
		{
			const FAnimNotifyEvent& Notify = Notifies[NotifyIndex];
			ImGui::PushID(NotifyIndex);
			CHAR Label[192] = "";
			::snprintf(Label, sizeof(Label), "%s  @ %.3fs%s", *Notify.NotifyName, Notify.Time
				, (Notify.bIsState != FALSE ? "  [state]" : ""));
			if (ImGui::Selectable(Label, (NotifyIndex == m_SelectedNotifyIndex)))
			{
				m_SelectedNotifyIndex = NotifyIndex;
				InDocument.Selection.Type = EEditorSelectionType::NOTIFY;
				InDocument.Selection.ObjectIndex = m_SelectedClipIndex;
				InDocument.Selection.SubIndex = NotifyIndex;
				ResetNotifyBuffers();
			}
			ImGui::PopID();
		}
	}

	void FClipEditorPanel::DrawNotifyInspector(FClipDocument& InDocument, INT32 InNotifyIndex)
	{
		const TArray<FAnimNotifyEvent>& Notifies = InDocument.GetDraftNotifies()[m_SelectedClipIndex];
		if ((InNotifyIndex < 0) || (InNotifyIndex >= Notifies.Num()))
		{
			return;
		}
		FAnimNotifyEvent Notify = Notifies[InNotifyIndex];
		const EString BufferSource = EString("clip:") + ToString(m_SelectedClipIndex)
			+ EString(":") + ToString(InNotifyIndex);
		if (m_NotifyBufferSource != BufferSource)
		{
			m_NotifyNameBuffer.SetText(Notify.NotifyName);
			m_NotifyClassBuffer.SetText(Notify.NotifyClassName);
			m_NotifyAssetBuffer.SetText(Notify.NotifyAssetName);
			m_NotifyPayloadBuffer.SetText(Notify.PayloadJson);
			m_NotifyBufferSource = BufferSource;
		}

		ImGui::Text("Notify: %s", *Notify.NotifyName);
		ImGui::SetNextItemWidth(-FLT_MIN);
		m_NotifyNameBuffer.InputText("##notify_name");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Notify.NotifyName = m_NotifyNameBuffer.ToString();
			CommitSelectedNotify(InDocument, Notify, EString("Notify name updated."));
			return;
		}

		const TArray<EString> ClassNames = EAnimNotifyRegistry::GetRegisteredClassNames();
		// A resolved notify asset owns class/state; editing the class here would be overwritten
		// when the asset is reloaded. Show it read-only and offer an explicit "clear reference".
		const BOOL32 bUsesNotifyAsset = (Notify.NotifyAssetName.Length() > 0u);
		if (bUsesNotifyAsset != FALSE)
		{
			ImGui::Text("Class: %s (from asset)", ((Notify.NotifyClassName.Length() > 0u)
				? *Notify.NotifyClassName : "None"));
		}
		else
		{
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::BeginCombo("##notify_class", ((Notify.NotifyClassName.Length() > 0u)
				? *Notify.NotifyClassName : "None")))
			{
				BOOL32 bClassCommitted = FALSE;
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
						// The class defines the event kind: state classes implement Begin/Tick/End,
						// instant classes implement Notify. Sync the flag before Compile() checks it.
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
						bClassCommitted = TRUE;
						break;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
				if (bClassCommitted != FALSE)
				{
					CommitSelectedNotify(InDocument, Notify, EString("Notify class updated."));
					return;
				}
			}
		}

		ImGui::SetNextItemWidth(-FLT_MIN);
		m_NotifyAssetBuffer.InputText("##notify_asset");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Notify.NotifyAssetName = m_NotifyAssetBuffer.ToString();
			CommitSelectedNotify(InDocument, Notify, EString("Notify asset updated."));
			return;
		}
		if ((bUsesNotifyAsset != FALSE) && (ImGui::Button("Clear Asset Reference")))
		{
			Notify.NotifyAssetName = EString();
			Notify.ResetCompiledNotify();
			Notify.Compile();
			CommitSelectedNotify(InDocument, Notify, EString("Notify asset reference cleared."));
			return;
		}

		FLOAT Time = Notify.Time;
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputFloat("##notify_time", &Time, 0.0f, 0.0f, "%.3f");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Notify.Time = EMath::Clamp(Time, 0.0f, GetSelectedClipDuration(InDocument));
			CommitSelectedNotify(InDocument, Notify, EString("Notify time updated."));
			return;
		}

		// The class is the single source of truth for instant vs state. The old independent
		// checkbox could produce a state event with an instant class (or vice versa), which
		// Compile() then disabled.
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
			ImGui::TextDisabled("Select the class again to synchronize the event kind.");
		}
		if (Notify.bIsState != FALSE)
		{
			FLOAT Duration = Notify.StateDuration;
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::InputFloat("##notify_duration", &Duration, 0.0f, 0.0f, "%.3f");
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				Notify.StateDuration = EMath::Max(Duration, 0.0f);
				Notify.bStateDurationOverride = TRUE;
				CommitSelectedNotify(InDocument, Notify, EString("Notify duration updated."));
				return;
			}
		}

		ImGui::SetNextItemWidth(-FLT_MIN);
		m_NotifyPayloadBuffer.InputText("##notify_payload");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			Notify.PayloadJson = m_NotifyPayloadBuffer.ToString();
			Notify.bPayloadOverride = TRUE;
			CommitSelectedNotify(InDocument, Notify, EString("Notify payload updated."));
			return;
		}

		if (ImGui::Button("Delete Notify", ImVec2(-FLT_MIN, 0.0f)))
		{
			DeleteSelectedNotify(InDocument);
		}
	}

	void FClipEditorPanel::DrawBottomPanel(FClipDocument& InDocument, FLOAT InTotalHeight)
	{
		const FLOAT MinBottom = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
		FEditorSplitter::HorizontalInverted("##clip_output_split", m_BottomHeight
			, MinBottom, 0.0f, InTotalHeight);
		if (ImGui::BeginChild("##clip_output", ImVec2(0.0f, m_BottomHeight), true))
		{
			if (ImGui::BeginTabBar("##clip_output_tabs"))
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

	void FClipEditorPanel::DrawDiagnostics(FClipDocument& InDocument)
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
				: ImVec4(1.0f, 0.8f, 0.35f, 1.0f);
			ImGui::PushID(DiagnosticIndex);
			ImGui::TextColored(Color, "%s", *Diagnostic.Message);
			ImGui::SameLine();
			if (ImGui::SmallButton("Locate"))
			{
				if ((Diagnostic.ObjectType == EEditorSelectionType::NOTIFY)
					&& (Diagnostic.ObjectIndex >= 0))
				{
					m_SelectedClipIndex = Diagnostic.ObjectIndex;
					m_SelectedNotifyIndex = Diagnostic.SubIndex;
					InDocument.Selection.Type = EEditorSelectionType::NOTIFY;
					InDocument.Selection.ObjectIndex = Diagnostic.ObjectIndex;
					InDocument.Selection.SubIndex = Diagnostic.SubIndex;
					ResetNotifyBuffers();
				}
			}
			ImGui::PopID();
		}
	}

	void FClipEditorPanel::DrawSource(FClipDocument& InDocument)
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
				}, EString("Notify source applied."));
			if (InDocument.LastError.Length() <= 0u)
			{
				m_SourceBuffer.SetText(InDocument.DraftJson);
				ResetNotifyBuffers();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Revert Text"))
		{
			m_SourceBuffer.SetText(InDocument.DraftJson);
		}
		m_SourceBuffer.InputTextMultiline("##clip_source", ImVec2(-1.0f, -1.0f)
			, ImGuiInputTextFlags_AllowTabInput);
	}

	void FClipEditorPanel::BindPreview(FClipDocument& InDocument)
	{
		const ESkeletonAnimation* Animation = InDocument.GetAsset()->GetStoragedResource();
		if (!Animation)
		{
			return;
		}
		if ((!m_bPreviewCatalogReady) || (m_PreviewSkeletonName != Animation->GetSkeletonName()))
		{
			RefreshPreviewCatalog(Animation->GetSkeletonName());
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
		if ((m_BoundAnimationAsset != InDocument.GetAsset())
			|| (m_BoundClipIndex != m_SelectedClipIndex))
		{
			m_PreviewScene.SetAnimationAsset(InDocument.GetAsset(), m_SelectedClipIndex);
			m_BoundAnimationAsset = InDocument.GetAsset();
			m_BoundClipIndex = m_SelectedClipIndex;
		}
		m_PreviewScene.SetPlayAnimation(m_Transport.bPlaying);
		m_PreviewScene.SetPlaybackRate(m_Transport.PlayRate);
		m_PreviewScene.SetPlaybackLoopMode(m_Transport.LoopMode);
	}

	void FClipEditorPanel::RefreshPreviewCatalog(const EString& InSkeletonName)
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
	}

	void FClipEditorPanel::ApplyTransport(FClipDocument& InDocument
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
		}
	}

	void FClipEditorPanel::SyncTransport(FClipDocument& InDocument)
	{
		m_Transport.Duration = GetSelectedClipDuration(InDocument);
		if (!ImGui::IsAnyItemActive())
		{
			m_Transport.Time = m_PreviewScene.GetPlaybackTimeSeconds();
		}
		m_Transport.Time = EMath::Clamp(m_Transport.Time, 0.0f, m_Transport.Duration);
	}

	void FClipEditorPanel::RunMutation(FClipDocument& InDocument
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
		}
		else
		{
			InDocument.Commands.CancelTransaction();
			InDocument.LastError = Error;
			m_StatusMessage = Error;
			m_bStatusSucceeded = FALSE;
		}
	}

	void FClipEditorPanel::CommitSelectedNotify(FClipDocument& InDocument, const FAnimNotifyEvent& InNotify
		, const EString& InSuccessMessage)
	{
		TArray<TArray<FAnimNotifyEvent>> NewNotifies = InDocument.GetDraftNotifies();
		if ((m_SelectedClipIndex < 0) || (m_SelectedClipIndex >= NewNotifies.Num())
			|| (m_SelectedNotifyIndex < 0)
			|| (m_SelectedNotifyIndex >= NewNotifies[m_SelectedClipIndex].Num()))
		{
			return;
		}
		NewNotifies[m_SelectedClipIndex][m_SelectedNotifyIndex] = InNotify;
		NewNotifies[m_SelectedClipIndex][m_SelectedNotifyIndex].ResetCompiledNotify();
		NewNotifies[m_SelectedClipIndex][m_SelectedNotifyIndex].Compile();
		RunMutation(InDocument, [&InDocument, &NewNotifies](EString& OutError) -> BOOL32
			{
				return InDocument.CommitNotifies(NewNotifies, OutError);
			}, InSuccessMessage);
		ResetNotifyBuffers();
	}

	void FClipEditorPanel::AddNotify(FClipDocument& InDocument)
	{
		TArray<TArray<FAnimNotifyEvent>> NewNotifies = InDocument.GetDraftNotifies();
		if ((m_SelectedClipIndex < 0) || (m_SelectedClipIndex >= NewNotifies.Num()))
		{
			return;
		}
		FAnimNotifyEvent Notify;
		Notify.NotifyName = DefaultNotifyName(NewNotifies, m_SelectedClipIndex);
		const TArray<EString> ClassNames = EAnimNotifyRegistry::GetRegisteredClassNames();
		// Prefer an instant class for a new event. The registry map has no stable order, and
		// picking a state class here would create an event whose State flag and class disagree.
		EString DefaultClass;
		BOOL32 bDefaultIsState = FALSE;
		for (INT32 ClassIndex = 0; ClassIndex < ClassNames.Num(); ClassIndex++)
		{
			BOOL32 bClassIsState = FALSE;
			if ((EAnimNotifyRegistry::IsStateClass(ClassNames[ClassIndex], bClassIsState) != FALSE)
				&& (bClassIsState == FALSE))
			{
				DefaultClass = ClassNames[ClassIndex];
				break;
			}
		}
		if ((DefaultClass.Length() <= 0u) && (ClassNames.Num() > 0))
		{
			DefaultClass = ClassNames[0];
			EAnimNotifyRegistry::IsStateClass(DefaultClass, bDefaultIsState);
		}
		if (DefaultClass.Length() > 0u)
		{
			Notify.NotifyClassName = DefaultClass;
			Notify.bIsState = bDefaultIsState;
		}
		if (Notify.bIsState != FALSE)
		{
			Notify.StateDuration = 0.1f;
			Notify.bStateDurationOverride = TRUE;
		}
		Notify.Time = EMath::Clamp(m_Transport.Time, 0.0f, GetSelectedClipDuration(InDocument));
		Notify.Compile();
		NewNotifies[m_SelectedClipIndex].Add(Notify);
		RunMutation(InDocument, [&InDocument, &NewNotifies](EString& OutError) -> BOOL32
			{
				return InDocument.CommitNotifies(NewNotifies, OutError);
			}, EString("Notify added."));
		m_SelectedNotifyIndex = NewNotifies[m_SelectedClipIndex].Num() - 1;
		InDocument.Selection.Type = EEditorSelectionType::NOTIFY;
		InDocument.Selection.ObjectIndex = m_SelectedClipIndex;
		InDocument.Selection.SubIndex = m_SelectedNotifyIndex;
		ResetNotifyBuffers();
	}

	void FClipEditorPanel::DeleteSelectedNotify(FClipDocument& InDocument)
	{
		TArray<TArray<FAnimNotifyEvent>> NewNotifies = InDocument.GetDraftNotifies();
		if ((m_SelectedClipIndex < 0) || (m_SelectedClipIndex >= NewNotifies.Num())
			|| (m_SelectedNotifyIndex < 0)
			|| (m_SelectedNotifyIndex >= NewNotifies[m_SelectedClipIndex].Num()))
		{
			return;
		}
		NewNotifies[m_SelectedClipIndex].RemoveAt(m_SelectedNotifyIndex);
		RunMutation(InDocument, [&InDocument, &NewNotifies](EString& OutError) -> BOOL32
			{
				return InDocument.CommitNotifies(NewNotifies, OutError);
			}, EString("Notify deleted."));
		m_SelectedNotifyIndex = -1;
		InDocument.Selection.Type = EEditorSelectionType::CLIP;
		InDocument.Selection.ObjectIndex = m_SelectedClipIndex;
		ResetNotifyBuffers();
	}

	void FClipEditorPanel::ResetNotifyBuffers()
	{
		m_NotifyBufferSource = EString();
	}

}

#endif
