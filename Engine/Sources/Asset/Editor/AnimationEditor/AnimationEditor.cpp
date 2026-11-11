#include "AnimationEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

#include <cstdio>

#include <Animation/AnimationNotify.h>
#include <AnimationNotifyAsset/AnimationNotifyAsset.h>
#include <Animation/SkeletonBinding.h>
#include <AnimationAsset/AnimationAsset.h>
#include "../AnimationCurvePlot/AnimationCurvePlot.h"
#include <BaseAsset.h>
#include <Editor/EditorLogManager.h>
#include <Base/DataStructure/Text/Path.h>
#include <IO/FileHelper.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>

namespace PigeonEngine
{
	namespace
	{
		const CHAR* AnimationBehaviourTypeAsString(const EAnimationBehaviourType InType)
		{
			switch (InType)
			{
			case EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT:
				return "Default";
			case EAnimationBehaviourType::ANIMATION_BEHAVIOUR_CONSTANT:
				return "Constant";
			case EAnimationBehaviourType::ANIMATION_BEHAVIOUR_LINEAR:
				return "Linear";
			case EAnimationBehaviourType::ANIMATION_BEHAVIOUR_REPEAT:
				return "Repeat";
			default:
				return "Unknown";
			}
		}

		INT32 ClampSelectionIndex(const INT32 InIndex, const INT32 InCount)
		{
			if (InCount <= 0)
			{
				return 0;
			}
			if (InIndex < 0)
			{
				return 0;
			}
			if (InIndex >= InCount)
			{
				return InCount - 1;
			}
			return InIndex;
		}

		EString NormalizeAssetDirectory(const EString& InAssetDirectory)
		{
			EString Result = InAssetDirectory.Replace("\\", "/");
			if ((Result.Length() > 0u) && (Result[Result.LastIndex()] != '/'))
			{
				Result += "/";
			}
			return Result;
		}

		UINT32 GetCurveKeyCount(const EBoneAnimationCurve& InCurve)
		{
			return InCurve.PositionKeys.Num<UINT32>() + InCurve.RotationKeys.Num<UINT32>() + InCurve.ScalingKeys.Num<UINT32>();
		}

		FLOAT GetClipDurationSeconds(const ESkeletonAnimationClip& InClip)
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

		const CHAR* GetLoopModeLabel(EAnimationLoopMode InLoopMode)
		{
			switch (InLoopMode)
			{
			case EAnimationLoopMode::ANIMATION_LOOP_ONCE:
				return PE_TEXT("Once");
			case EAnimationLoopMode::ANIMATION_LOOP_REPEAT:
				return PE_TEXT("Repeat");
			case EAnimationLoopMode::ANIMATION_LOOP_PINGPONG:
				return PE_TEXT("PingPong");
			case EAnimationLoopMode::ANIMATION_LOOP_CLAMP:
				return PE_TEXT("Clamp");
			default:
				return PE_TEXT("Unknown");
			}
		}

		/*
		* Read-only clip timeline. Draws the ruler, the authored notify markers and the current
		* playhead, and turns a click/drag into OutScrubbedTime. The caller owns playback state:
		* it pauses and seeks the preview when this returns TRUE.
		*
		* Notify markers are positioned by authored time; state notifies are drawn as a range of
		* their StateDuration, instant notifies as a single line. The clip itself is the only
		* source here, so the widget works before any preview actor exists.
		*/
		BOOL32 DrawClipTimeline(const ESkeletonAnimationClip& InClip, FLOAT InCurrentTime, FLOAT& OutScrubbedTime)
		{
			const FLOAT DurationSeconds = GetClipDurationSeconds(InClip);
			if (DurationSeconds <= 0.0f)
			{
				return FALSE;
			}

			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			const ImVec2 Origin = ImGui::GetCursorScreenPos();
			const FLOAT BarWidth = EMath::Max(ImGui::GetContentRegionAvail().x - 8.0f, 120.0f);
			constexpr FLOAT RulerHeight = 16.0f;
			constexpr FLOAT BarHeight = 28.0f;
			constexpr FLOAT TotalHeight = RulerHeight + BarHeight + 4.0f;
			const ImVec2 BarMin(Origin.x, Origin.y + RulerHeight);
			const ImVec2 BarMax(BarMin.x + BarWidth, BarMin.y + BarHeight);

			ImGui::PushID(&InClip);
			ImGui::InvisibleButton("##clip_timeline", ImVec2(BarWidth, TotalHeight));
			const BOOL32 bHovered = ImGui::IsItemHovered();
			const BOOL32 bActive = ImGui::IsItemActive();
			ImGui::PopID();

			BOOL32 bScrubbed = FALSE;
			if (bActive != FALSE)
			{
				const FLOAT Alpha = EMath::Clamp((ImGui::GetIO().MousePos.x - BarMin.x) / BarWidth, 0.0f, 1.0f);
				OutScrubbedTime = Alpha * DurationSeconds;
				bScrubbed = TRUE;
			}

			DrawList->AddRectFilled(BarMin, BarMax, IM_COL32(38, 40, 46, 255), 3.0f);
			DrawList->AddRect(BarMin, BarMax, IM_COL32(90, 92, 100, 255), 3.0f);

			constexpr UINT32 DivisionNum = 10u;
			for (UINT32 DivisionIndex = 0u; DivisionIndex <= DivisionNum; DivisionIndex++)
			{
				const FLOAT Alpha = static_cast<FLOAT>(DivisionIndex) / static_cast<FLOAT>(DivisionNum);
				const FLOAT X = BarMin.x + Alpha * BarWidth;
				DrawList->AddLine(ImVec2(X, BarMin.y - 4.0f), ImVec2(X, BarMin.y + 2.0f), IM_COL32(130, 132, 140, 255));
				if ((DivisionIndex == 0u) || (DivisionIndex == (DivisionNum / 2u)) || (DivisionIndex == DivisionNum))
				{
					CHAR TimeText[32] = "";
					::snprintf(TimeText, sizeof(TimeText), "%.2fs", Alpha * DurationSeconds);
					DrawList->AddText(ImVec2(X + 2.0f, Origin.y), IM_COL32(160, 162, 170, 255), TimeText);
				}
			}

			const FAnimNotifyEvent* HoveredNotify = nullptr;
			for (INT32 NotifyIndex = 0; NotifyIndex < InClip.Notifies.Num(); NotifyIndex++)
			{
				const FAnimNotifyEvent& Notify = InClip.Notifies[NotifyIndex];
				const FLOAT NotifyAlpha = EMath::Clamp(Notify.Time / DurationSeconds, 0.0f, 1.0f);
				const FLOAT NotifyX = BarMin.x + NotifyAlpha * BarWidth;
				if (Notify.bIsState)
				{
					const FLOAT StateAlpha = EMath::Clamp(Notify.StateDuration / DurationSeconds, 0.0f, 1.0f - NotifyAlpha);
					const ImVec2 StateMin(NotifyX, BarMin.y + 5.0f);
					const ImVec2 StateMax(NotifyX + StateAlpha * BarWidth, BarMax.y - 5.0f);
					if (StateMax.x > StateMin.x)
					{
						DrawList->AddRectFilled(StateMin, StateMax, IM_COL32(70, 120, 210, 180), 2.0f);
						DrawList->AddRect(StateMin, StateMax, IM_COL32(120, 170, 240, 220), 2.0f);
					}
				}
				else
				{
					DrawList->AddLine(ImVec2(NotifyX, BarMin.y + 5.0f), ImVec2(NotifyX, BarMax.y - 5.0f)
						, IM_COL32(235, 150, 60, 255), 2.0f);
				}

				if ((bHovered != FALSE) && (EMath::Abs(ImGui::GetIO().MousePos.x - NotifyX) <= 4.0f))
				{
					HoveredNotify = &Notify;
				}
			}

			const FLOAT PlayheadAlpha = EMath::Clamp(InCurrentTime / DurationSeconds, 0.0f, 1.0f);
			const FLOAT PlayheadX = BarMin.x + PlayheadAlpha * BarWidth;
			DrawList->AddLine(ImVec2(PlayheadX, BarMin.y - 5.0f), ImVec2(PlayheadX, BarMax.y + 3.0f)
				, IM_COL32(245, 220, 90, 255), 2.0f);
			DrawList->AddTriangleFilled(ImVec2(PlayheadX, BarMax.y + 3.0f)
				, ImVec2(PlayheadX - 4.0f, BarMax.y + 8.0f), ImVec2(PlayheadX + 4.0f, BarMax.y + 8.0f)
				, IM_COL32(245, 220, 90, 255));

			if (HoveredNotify)
			{
				ImGui::SetTooltip("%s @ %.3fs", *HoveredNotify->NotifyName, HoveredNotify->Time);
			}
			return bScrubbed;
		}

		BOOL8 ReadAssetTypeFromAssetFile(const EString& InAssetFullPath, EAssetType& OutAssetType)
		{
			void* HeaderData = nullptr;
			if (!(EFileHelper::ReadFirstNumberOfBytesInFile(InAssetFullPath, HeaderData, sizeof(UINT32))) || (!HeaderData))
			{
				return FALSE;
			}

			OutAssetType = static_cast<EAssetType>(*static_cast<const UINT32*>(HeaderData));
			delete[] static_cast<CHAR*>(HeaderData);
			return TRUE;
		}

		BOOL8 ReadMeshTypeFromAssetFile(const EString& InAssetFullPath, EMeshType& OutMeshType)
		{
			void* HeaderData = nullptr;
			if (!(EFileHelper::ReadFirstNumberOfBytesInFile(InAssetFullPath, HeaderData, sizeof(UINT32) * 2u)) || (!HeaderData))
			{
				return FALSE;
			}

			const UINT32* HeaderWords = static_cast<const UINT32*>(HeaderData);
			const EAssetType AssetType = static_cast<EAssetType>(HeaderWords[0]);
			OutMeshType = static_cast<EMeshType>(HeaderWords[1]);
			delete[] static_cast<CHAR*>(HeaderData);
			return (AssetType == EAssetType::ASSET_TYPE_MESH) && (OutMeshType == EMeshType::MESH_TYPE_SKIN);
		}

		BOOL8 LoadSkinnedMeshAssetFromFullPath(const EString& InAssetFullPath, const ESkinnedMeshAsset*& OutMeshAsset)
		{
			OutMeshAsset = nullptr;
			const EString AssetName = EPath::GetFileNameWithoutExtension(InAssetFullPath);
			const EString AssetDirectory = NormalizeAssetDirectory(EPath::GetFileFolderPath(InAssetFullPath));
			return (AssetName.Length() > 0u) && (AssetDirectory.Length() > 0u) && EMeshAssetManager::GetManagerSingleton()->LoadSkinnedMeshAsset(AssetDirectory, AssetName, OutMeshAsset) && (!!OutMeshAsset);
		}

		BOOL8 LoadSkeletonAssetFromFullPath(const EString& InAssetFullPath, const ESkeletonAsset*& OutSkeletonAsset)
		{
			OutSkeletonAsset = nullptr;
			const EString AssetName = EPath::GetFileNameWithoutExtension(InAssetFullPath);
			const EString AssetDirectory = NormalizeAssetDirectory(EPath::GetFileFolderPath(InAssetFullPath));
			return (AssetName.Length() > 0u) && (AssetDirectory.Length() > 0u) && ESkeletonAssetManager::GetManagerSingleton()->LoadSkeletonAsset(AssetDirectory, AssetName, OutSkeletonAsset) && (!!OutSkeletonAsset);
		}

		template<typename TKeyType>
		BOOL8 TryGetKeyTimeRange(const TArray<TKeyType>& InKeys, FLOAT& OutStartTime, FLOAT& OutEndTime)
		{
			const INT32 KeyNum = InKeys.Num();
			if (KeyNum <= 0)
			{
				return FALSE;
			}
			OutStartTime = static_cast<FLOAT>(InKeys[0].Time);
			OutEndTime = static_cast<FLOAT>(InKeys[KeyNum - 1].Time);
			return TRUE;
		}
	}

	void EAnimationEditor::EditorInit()
	{
		ResetSelection();
		m_AnimationAsset = nullptr;
		m_OpenedAssetFullPath = EString();
		m_LastOpenError = EString();
		m_ShowEditor = FALSE;
		m_LastBindingSkeletonAsset = nullptr;
		m_BindingValid = FALSE;
		m_BindingMissingBoneNum = 0;
		m_PreviewScene.Initialize("AnimationEditorPreview", Vector3(36000.0f, 32000.0f, 32000.0f));
	}

	void EAnimationEditor::OpenAnimationAsset(const EString& InAssetFullPath)
	{
		m_ShowEditor = TRUE;
		const EString NormalizedAssetPath = InAssetFullPath.Replace("\\", "/");
		if (m_Document.bDirty)
		{
			if (NormalizedAssetPath == m_OpenedAssetFullPath)
			{
				m_Document.bReloadPending = TRUE;
			}
			else
			{
				m_Document.bOpenPending = TRUE;
				m_Document.PendingOpenPath = InAssetFullPath;
			}
			return;
		}
		OpenAnimationAssetNow(InAssetFullPath);
	}

	void EAnimationEditor::OpenAnimationAssetNow(const EString& InAssetFullPath)
	{
		m_ShowEditor = TRUE;
		m_LastOpenError = EString();

		const EString NormalizedAssetPath = InAssetFullPath.Replace("\\", "/");
		const EString AssetName = EPath::GetFileNameWithoutExtension(NormalizedAssetPath);
		const EString AssetDirectory = NormalizeAssetDirectory(EPath::GetFileFolderPath(NormalizedAssetPath));
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			m_LastOpenError = EString("Animation editor cannot parse asset path: ") + NormalizedAssetPath;
			return;
		}

		const ESkeletonAnimationAsset* LoadedAsset = nullptr;
		if (!(EAnimationManager::GetManagerSingleton()->LoadSkeletonAnimationAsset(AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			m_LastOpenError = EString("Animation editor failed to load asset: ") + NormalizedAssetPath;
			return;
		}

		m_AnimationAsset = LoadedAsset;
		m_OpenedAssetFullPath = NormalizedAssetPath;
		m_LastBindingSkeletonAsset = nullptr;
		m_BindingValid = FALSE;
		m_BindingMissingBoneNum = 0;
		m_Document.ClearPendingAction();
		m_Document.bDirty = FALSE;
		m_StatusMessage = EString("Loaded.");
		m_bStatusSucceeded = TRUE;
		ResetSelection();
		RefreshPreviewOptions();
		CaptureBaseline();
	}

	BOOL32 EAnimationEditor::SaveAnimationAsset(EString& OutError)
	{
		OutError = EString();
		if (!m_AnimationAsset)
		{
			OutError = EString("No animation asset is open.");
			return FALSE;
		}
		if (!(EAnimationManager::GetManagerSingleton()->SaveSkeletonAnimationAsset(m_AnimationAsset, OutError)))
		{
			return FALSE;
		}
		CaptureBaseline();
		m_Document.bDirty = FALSE;
		return TRUE;
	}

	BOOL32 EAnimationEditor::ReloadAnimationAssetNow(EString& OutError)
	{
		OutError = EString();
		if (!m_AnimationAsset)
		{
			OutError = EString("No animation asset is open.");
			return FALSE;
		}

		const EString AssetPath = m_AnimationAsset->GetAssetPath();
		const EString AssetName = m_AnimationAsset->GetAssetName();
		const ESkeletonAnimationAsset* FreshAsset = nullptr;
		if (!(EAnimationManager::GetManagerSingleton()->ReloadSkeletonAnimationAsset(AssetPath, AssetName, FreshAsset, OutError))
			|| (!FreshAsset))
		{
			return FALSE;
		}

		m_AnimationAsset = FreshAsset;
		m_LastBindingSkeletonAsset = nullptr;
		m_BindingValid = FALSE;
		m_BindingMissingBoneNum = 0;
		m_Document.ClearPendingAction();
		m_Document.bDirty = FALSE;
		ResetSelection();
		RefreshPreviewOptions();
		CaptureBaseline();
		return TRUE;
	}

	void EAnimationEditor::CaptureBaseline()
	{
		m_BaselineNotifies.Empty();
		const ESkeletonAnimation* Animation = (m_AnimationAsset ? m_AnimationAsset->GetStoragedResource() : nullptr);
		if (!Animation)
		{
			return;
		}
		for (INT32 ClipIndex = 0; ClipIndex < Animation->GetAnimationClipNum(); ClipIndex++)
		{
			const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
			TArray<FAnimNotifyEvent> NotifySnapshot;
			if (Clip)
			{
				NotifySnapshot = Clip->Notifies;
			}
			m_BaselineNotifies.Add(NotifySnapshot);
		}
	}

	void EAnimationEditor::DiscardChanges()
	{
		ESkeletonAnimation* Animation = (m_AnimationAsset
			? const_cast<ESkeletonAnimation*>(m_AnimationAsset->GetStoragedResource())
			: nullptr);
		if (Animation)
		{
			for (INT32 ClipIndex = 0; (ClipIndex < Animation->GetAnimationClipNum()) && (ClipIndex < m_BaselineNotifies.Num()); ClipIndex++)
			{
				ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
				if (Clip)
				{
					Clip->Notifies = m_BaselineNotifies[ClipIndex];
				}
			}
		}
		m_Document.bDirty = FALSE;
		m_StatusMessage = EString("Discarded unsaved changes.");
		m_bStatusSucceeded = TRUE;
	}

	void EAnimationEditor::HandleDocumentAction()
	{
		const EEditorUnsavedChoiceType Choice = DrawEditorUnsavedChangesPopup("##animation_editor_unsaved"
			, (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : nullptr), m_Document);
		if (Choice == EEditorUnsavedChoiceType::NONE)
		{
			return;
		}

		const BOOL8 bWasClose = m_Document.bClosePending;
		const BOOL8 bWasOpen = m_Document.bOpenPending;
		const BOOL8 bWasReload = m_Document.bReloadPending;
		const EString PendingPath = m_Document.PendingOpenPath;

		if (Choice == EEditorUnsavedChoiceType::CANCEL)
		{
			m_Document.ClearPendingAction();
			return;
		}
		if (Choice == EEditorUnsavedChoiceType::SAVE)
		{
			EString SaveError;
			if (!SaveAnimationAsset(SaveError))
			{
				m_LastOpenError = SaveError;
				m_StatusMessage = SaveError;
				m_bStatusSucceeded = FALSE;
				return;	// keep the pending action so the modal comes back next frame
			}
		}
		else
		{
			DiscardChanges();
		}

		m_Document.ClearPendingAction();
		if (bWasClose)
		{
			m_ShowEditor = FALSE;
			return;
		}
		if (bWasReload)
		{
			EString ReloadError;
			if (!ReloadAnimationAssetNow(ReloadError))
			{
				m_LastOpenError = ReloadError;
				m_StatusMessage = ReloadError;
				m_bStatusSucceeded = FALSE;
			}
			else
			{
				m_StatusMessage = EString("Reloaded from disk.");
				m_bStatusSucceeded = TRUE;
			}
			return;
		}
		if (bWasOpen)
		{
			OpenAnimationAssetNow(PendingPath);
		}
	}

	void EAnimationEditor::EditorUpdate()
	{
		if (!m_ShowEditor)
		{
			return;
		}

		HandleDocumentAction();
		if (!m_ShowEditor)
		{
			return;
		}

		const ESkeletonAnimation* Animation = nullptr;
		if (m_AnimationAsset)
		{
			Animation = m_AnimationAsset->GetStoragedResource();
		}

		ImGui::SetNextWindowSize(ImVec2(960.0f, 640.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(420.0f, 40.0f), ImGuiCond_FirstUseEver);
		bool bWindowOpen = true;
		ImGui::Begin("Animation Editor", &bWindowOpen, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (m_OpenedAssetFullPath.Length() > 0u))
			{
				if (m_Document.bDirty)
				{
					m_Document.bReloadPending = TRUE;
				}
				else
				{
					EString ReloadError;
					if (!ReloadAnimationAssetNow(ReloadError))
					{
						m_LastOpenError = ReloadError;
						m_StatusMessage = ReloadError;
						m_bStatusSucceeded = FALSE;
					}
					else
					{
						m_StatusMessage = EString("Reloaded from disk.");
						m_bStatusSucceeded = TRUE;
					}
				}
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No animation asset selected.");
			if (m_Document.bDirty)
			{
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f), "UNSAVED");
			}

			if (m_LastOpenError.Length() > 0u)
			{
				ImGui::Separator();
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastOpenError);
			}
			if (m_StatusMessage.Length() > 0u)
			{
				ImGui::Separator();
				const ImVec4 StatusColor = (m_bStatusSucceeded != FALSE)
					? ImVec4(0.45f, 0.85f, 0.45f, 1.0f)
					: ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
				ImGui::TextColored(StatusColor, "%s", *m_StatusMessage);
			}

			if (!m_AnimationAsset)
			{
				ImGui::Separator();
				ImGui::Text("No animation asset selected.");
			}
			else if (!Animation)
			{
				ImGui::Separator();
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "The selected animation asset does not contain a loaded resource.");
			}
			else
			{
				ImGui::Separator();
				DrawAssetSummary(m_AnimationAsset, Animation);
			}
		}
		ImGui::End();

		if (!bWindowOpen)
		{
			if (m_Document.bDirty)
			{
				m_Document.bClosePending = TRUE;
			}
			else
			{
				m_ShowEditor = FALSE;
				return;
			}
		}

		if ((!m_ShowEditor) || (!Animation))
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(280.0f, 520.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(420.0f, 220.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Animation Clip List", nullptr, ImGuiWindowFlags_None);
		{
			DrawClipList(Animation);
		}
		ImGui::End();

		const INT32 ClipNum = Animation->GetAnimationClipNum();
		if (ClipNum <= 0)
		{
			return;
		}

		m_SelectedClipIndex = ClampSelectionIndex(m_SelectedClipIndex, ClipNum);
		const ESkeletonAnimationClip* SelectedClip = Animation->GetAnimationClip(m_SelectedClipIndex);
		if (!SelectedClip)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(1360.0f, 40.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Animation Preview", nullptr, ImGuiWindowFlags_None);
		{
			if (m_AvailablePreviewMeshPaths.Num() > 0)
			{
				// The label has to stay alive for the whole BeginCombo call: taking the pointer
				// from a temporary EString leaves ImGui reading freed text, which flickers.
				const EString CurrentMeshName = (m_SelectedPreviewMeshIndex >= 0)
					? EPath::GetFileNameWithoutExtension(m_AvailablePreviewMeshPaths[m_SelectedPreviewMeshIndex])
					: EString("None");
				if (ImGui::BeginCombo("Preview Mesh", *CurrentMeshName))
				{
					for (INT32 MeshIndex = 0, MeshNum = m_AvailablePreviewMeshPaths.Num(); MeshIndex < MeshNum; MeshIndex++)
					{
						const BOOL8 bSelected = (m_SelectedPreviewMeshIndex == MeshIndex);
						const EString AssetName = EPath::GetFileNameWithoutExtension(m_AvailablePreviewMeshPaths[MeshIndex]);
						if (ImGui::Selectable(*AssetName, bSelected))
						{
							m_SelectedPreviewMeshIndex = MeshIndex;
							UpdatePreviewScene();
						}
						if (bSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "No skinned mesh asset found in current folder.");
			}

			if (m_AvailablePreviewSkeletonPaths.Num() > 0)
			{
				const EString CurrentSkeletonName = (m_SelectedPreviewSkeletonIndex >= 0)
					? EPath::GetFileNameWithoutExtension(m_AvailablePreviewSkeletonPaths[m_SelectedPreviewSkeletonIndex])
					: EString("None");
				if (ImGui::BeginCombo("Preview Skeleton", *CurrentSkeletonName))
				{
					for (INT32 SkeletonIndex = 0, SkeletonNum = m_AvailablePreviewSkeletonPaths.Num(); SkeletonIndex < SkeletonNum; SkeletonIndex++)
					{
						const BOOL8 bSelected = (m_SelectedPreviewSkeletonIndex == SkeletonIndex);
						const EString AssetName = EPath::GetFileNameWithoutExtension(m_AvailablePreviewSkeletonPaths[SkeletonIndex]);
						if (ImGui::Selectable(*AssetName, bSelected))
						{
							m_SelectedPreviewSkeletonIndex = SkeletonIndex;
							UpdatePreviewScene();
						}
						if (bSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "No skeleton asset found in current folder.");
			}

			UpdatePreviewScene();

			ImGui::Checkbox("Auto Rotate", reinterpret_cast<bool*>(&m_PreviewAutoRotate));
			ImGui::SameLine();
			ImGui::Checkbox("Play", reinterpret_cast<bool*>(&m_PreviewPlayAnimation));
			ImGui::SliderFloat("Play Rate", &m_PreviewPlaybackRate, 0.0f, 3.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::BeginCombo("Loop Mode", GetLoopModeLabel(m_PreviewLoopMode)))
			{
				const EAnimationLoopMode LoopModes[4] = {
					EAnimationLoopMode::ANIMATION_LOOP_ONCE,
					EAnimationLoopMode::ANIMATION_LOOP_REPEAT,
					EAnimationLoopMode::ANIMATION_LOOP_PINGPONG,
					EAnimationLoopMode::ANIMATION_LOOP_CLAMP
				};
				for (UINT32 ModeIndex = 0u; ModeIndex < 4u; ModeIndex++)
				{
					const BOOL32 bSelected = (m_PreviewLoopMode == LoopModes[ModeIndex]);
					if (ImGui::Selectable(GetLoopModeLabel(LoopModes[ModeIndex]), bSelected))
					{
						m_PreviewLoopMode = LoopModes[ModeIndex];
						m_PreviewScene.SetPlaybackLoopMode(m_PreviewLoopMode);
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			const FLOAT ClipDurationSeconds = m_PreviewScene.GetClipDurationSeconds();
			if (ClipDurationSeconds > 0.0f)
			{
				const FLOAT TicksPerSecond = ((SelectedClip) && (SelectedClip->TicksPerSecond > 1e-3f))
					? SelectedClip->TicksPerSecond
					: EEngineSettings::ENGINE_ANIMATION_DEFAULT_TICKS_PER_SECOND;
				const FLOAT FrameDeltaSeconds = ((TicksPerSecond > 0.0f) ? (1.0f / TicksPerSecond) : 0.0f);

				FLOAT PlaybackTimeSeconds = m_PreviewScene.GetPlaybackTimeSeconds();
				ImGui::SetNextItemWidth(90.0f);
				ImGui::InputFloat("Time (s)", &PlaybackTimeSeconds, 0.0f, 0.0f, "%.3f");
				const BOOL32 bTimeCommitted = (ImGui::IsItemDeactivatedAfterEdit() != FALSE) ? TRUE : FALSE;
				ImGui::SameLine();
				ImGui::TextDisabled("/ %.3fs", ClipDurationSeconds);
				if (bTimeCommitted != FALSE)
				{
					// The timeline is the scrub surface; the numeric field is for exact entry.
					m_PreviewPlayAnimation = FALSE;
					m_PreviewScene.SetPlayAnimation(FALSE);
					m_PreviewScene.SetPlaybackTimeSeconds(PlaybackTimeSeconds);
				}

				if (ImGui::Button("-1 Frame"))
				{
					m_PreviewPlayAnimation = FALSE;
					m_PreviewScene.SetPlayAnimation(FALSE);
					m_PreviewScene.SetPlaybackTimeSeconds(m_PreviewScene.GetPlaybackTimeSeconds() - FrameDeltaSeconds);
				}
				ImGui::SameLine();
				if (ImGui::Button("+1 Frame"))
				{
					m_PreviewPlayAnimation = FALSE;
					m_PreviewScene.SetPlayAnimation(FALSE);
					m_PreviewScene.SetPlaybackTimeSeconds(m_PreviewScene.GetPlaybackTimeSeconds() + FrameDeltaSeconds);
				}

				if (SelectedClip)
				{
					ImGui::Separator();
					ImGui::Text("Timeline  %.3f / %.3f s"
						, m_PreviewScene.GetPlaybackTimeSeconds(), ClipDurationSeconds);
					FLOAT ScrubbedTime = 0.0f;
					if (DrawClipTimeline(*SelectedClip, m_PreviewScene.GetPlaybackTimeSeconds(), ScrubbedTime))
					{
						m_PreviewPlayAnimation = FALSE;
						m_PreviewScene.SetPlayAnimation(FALSE);
						m_PreviewScene.SetPlaybackTimeSeconds(ScrubbedTime);
					}
				}
			}

			m_PreviewScene.SetAutoRotate(m_PreviewAutoRotate);
			m_PreviewScene.SetPlayAnimation(m_PreviewPlayAnimation);
			m_PreviewScene.SetPlaybackRate(m_PreviewPlaybackRate);

			ImGui::Separator();
			ImGui::Text("Binding: %s", (m_BindingValid ? "OK" : "Failed"));
			ImGui::Text("Missing Bones: %d", m_BindingMissingBoneNum);

			ImGui::Separator();
			ImVec2 AvailableRegion = ImGui::GetContentRegionAvail();
			const FLOAT PreviewWidth = EMath::Max(AvailableRegion.x, 64.0f);
			const FLOAT PreviewHeight = EMath::Max(AvailableRegion.y, 64.0f);
			m_PreviewScene.UpdateViewportSize(static_cast<UINT32>(PreviewWidth), static_cast<UINT32>(PreviewHeight));
			m_PreviewScene.Tick(ImGui::GetIO().DeltaTime);
			if (ImTextureID PreviewTexture = m_PreviewScene.GetTextureId())
			{
				ImGui::Image(PreviewTexture, ImVec2(PreviewWidth, PreviewHeight));
			}
			else
			{
				ImGui::TextWrapped("No preview to show yet.");
			}
		}
		ImGui::End();

		ImGui::SetNextWindowSize(ImVec2(420.0f, 320.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(720.0f, 220.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Animation Clip Details", nullptr, ImGuiWindowFlags_None);
		{
			// The editor edits the notify list in place; playback still only reads the clip.
			DrawClipDetails(*const_cast<ESkeletonAnimationClip*>(SelectedClip));
		}
		ImGui::End();

		ImGui::SetNextWindowSize(ImVec2(280.0f, 300.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(1148.0f, 220.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Animation Curve List", nullptr, ImGuiWindowFlags_None);
		{
			DrawCurveList(*SelectedClip);
		}
		ImGui::End();

		const INT32 CurveNum = SelectedClip->AnimationCurves.Num();
		if (CurveNum <= 0)
		{
			return;
		}

		m_SelectedCurveIndex = ClampSelectionIndex(m_SelectedCurveIndex, CurveNum);
		ImGui::SetNextWindowSize(ImVec2(420.0f, 300.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(1148.0f, 540.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Animation Curve Details", nullptr, ImGuiWindowFlags_None);
		{
			DrawCurveDetails(SelectedClip->AnimationCurves[m_SelectedCurveIndex]);
		}
		ImGui::End();
	}

	void EAnimationEditor::ResetSelection()
	{
		m_SelectedClipIndex = 0;
		m_SelectedCurveIndex = 0;
	}

	void EAnimationEditor::RefreshPreviewOptions()
	{
		m_AvailablePreviewMeshPaths.Empty();
		m_AvailablePreviewSkeletonPaths.Empty();
		m_PreviewMeshAsset = nullptr;
		m_PreviewSkeletonAsset = nullptr;
		m_SelectedPreviewMeshIndex = -1;
		m_SelectedPreviewSkeletonIndex = -1;
		m_LastBindingSkeletonAsset = nullptr;
		m_BindingValid = FALSE;
		m_BindingMissingBoneNum = 0;

		if (m_OpenedAssetFullPath.Length() <= 0u)
		{
			m_PreviewScene.ClearPreview();
			return;
		}

		// Mesh and skeleton assets live in their own engine asset folders. The animation asset
		// only names the skeleton it was authored against, so scanning the animation's own
		// folder would only find animation assets and the preview would have nothing to draw.
		TArray<EString> FolderPaths;
		TArray<EString> FilePaths;
		{
			TArray<EString> MeshFilePaths;
			EFileHelper::ScanDirectory(NormalizeAssetDirectory(EBaseSettings::ENGINE_MESH_PATH), FolderPaths, MeshFilePaths, TRUE);
			FilePaths.Append(MeshFilePaths);
		}
		{
			TArray<EString> SkeletonFilePaths;
			EFileHelper::ScanDirectory(NormalizeAssetDirectory(EBaseSettings::ENGINE_SKELETON_PATH), FolderPaths, SkeletonFilePaths, TRUE);
			FilePaths.Append(SkeletonFilePaths);
		}

		for (INT32 FileIndex = 0, FileNum = FilePaths.Num(); FileIndex < FileNum; FileIndex++)
		{
			const EString AssetPath = FilePaths[FileIndex].Replace("\\", "/");
			if (!(AssetPath.Contains(EEngineSettings::ENGINE_ASSET_NAME_TYPE)))
			{
				continue;
			}

			EAssetType AssetType = EAssetType::ASSET_TYPE_UNKNOWN;
			if (!(ReadAssetTypeFromAssetFile(AssetPath, AssetType)))
			{
				continue;
			}

			if (AssetType == EAssetType::ASSET_TYPE_SKELETON)
			{
				m_AvailablePreviewSkeletonPaths.Add(AssetPath);
			}
			else if (AssetType == EAssetType::ASSET_TYPE_MESH)
			{
				EMeshType MeshType = EMeshType::MESH_TYPE_COUNT;
				if (ReadMeshTypeFromAssetFile(AssetPath, MeshType) && (MeshType == EMeshType::MESH_TYPE_SKIN))
				{
					m_AvailablePreviewMeshPaths.Add(AssetPath);
				}
			}
		}

		if (m_AvailablePreviewMeshPaths.Num() > 0)
		{
			m_SelectedPreviewMeshIndex = 0;
		}
		if (m_AvailablePreviewSkeletonPaths.Num() > 0)
		{
			m_SelectedPreviewSkeletonIndex = 0;

			// Prefer the skeleton the asset records over whichever file happened to be first.
			const ESkeletonAnimation* Animation = (m_AnimationAsset ? m_AnimationAsset->GetStoragedResource() : nullptr);
			const EString BindSkeletonName = (Animation ? Animation->GetSkeletonName() : EString());
			if ((BindSkeletonName.Length() > 0u) && (BindSkeletonName != EString(EEngineSettings::ENGINE_DEFAULT_NAME)))
			{
				for (INT32 SkeletonIndex = 0; SkeletonIndex < m_AvailablePreviewSkeletonPaths.Num(); SkeletonIndex++)
				{
					if (EPath::GetFileNameWithoutExtension(m_AvailablePreviewSkeletonPaths[SkeletonIndex]) == BindSkeletonName)
					{
						m_SelectedPreviewSkeletonIndex = SkeletonIndex;
						break;
					}
				}
			}
		}
	}

	void EAnimationEditor::UpdatePreviewScene()
	{
		m_PreviewMeshAsset = nullptr;
		m_PreviewSkeletonAsset = nullptr;
		if ((m_SelectedPreviewMeshIndex >= 0) && (m_SelectedPreviewMeshIndex < m_AvailablePreviewMeshPaths.Num()))
		{
			LoadSkinnedMeshAssetFromFullPath(m_AvailablePreviewMeshPaths[m_SelectedPreviewMeshIndex], m_PreviewMeshAsset);
		}
		if ((m_SelectedPreviewSkeletonIndex >= 0) && (m_SelectedPreviewSkeletonIndex < m_AvailablePreviewSkeletonPaths.Num()))
		{
			LoadSkeletonAssetFromFullPath(m_AvailablePreviewSkeletonPaths[m_SelectedPreviewSkeletonIndex], m_PreviewSkeletonAsset);
		}

		// Rebuild the compatibility report only when the skeleton selection changed. Bind
		// resolves every curve once and reports the curves the target skeleton cannot serve.
		if (m_PreviewSkeletonAsset != m_LastBindingSkeletonAsset)
		{
			m_LastBindingSkeletonAsset = m_PreviewSkeletonAsset;
			m_BindingValid = FALSE;
			m_BindingMissingBoneNum = 0;
			if ((m_PreviewSkeletonAsset) && (m_AnimationAsset))
			{
				ESkeletonBinding Binding;
				m_BindingValid = Binding.Bind(m_PreviewSkeletonAsset->GetStoragedResource(), m_AnimationAsset->GetStoragedResource());
				m_BindingMissingBoneNum = Binding.GetMissingBoneNum();
			}
		}

		m_PreviewScene.SetAutoRotate(m_PreviewAutoRotate);
		m_PreviewScene.SetPlayAnimation(m_PreviewPlayAnimation);
		m_PreviewScene.SetPlaybackRate(m_PreviewPlaybackRate);
		m_PreviewScene.SetPlaybackLoopMode(m_PreviewLoopMode);

		if ((m_PreviewMeshAsset) && (m_PreviewSkeletonAsset))
		{
			m_PreviewScene.SetSkinnedMesh(m_PreviewMeshAsset, m_PreviewSkeletonAsset);
		}
		else
		{
			// Keep the selected clip even without a preview mesh: the timeline and duration
			// read from the scene, and the user may only want to inspect the curve data.
			m_PreviewScene.SetSkinnedMesh(nullptr, nullptr);
		}
		const BOOL32 bHasSelectedClip = ((m_AnimationAsset) && (m_SelectedClipIndex >= 0)) ? TRUE : FALSE;
		m_PreviewScene.SetAnimationAsset(bHasSelectedClip ? m_AnimationAsset : nullptr
			, bHasSelectedClip ? m_SelectedClipIndex : -1);
	}

	void EAnimationEditor::DrawAssetSummary(const ESkeletonAnimationAsset* InAsset, const ESkeletonAnimation* InAnimation) const
	{
		UINT32 TotalCurveNum = 0u;
		UINT32 TotalKeyNum = 0u;
		const TArray<ESkeletonAnimationClip>& AnimationClips = InAnimation->GetAnimationClips();
		for (INT32 ClipIndex = 0, ClipNum = AnimationClips.Num(); ClipIndex < ClipNum; ClipIndex++)
		{
			const ESkeletonAnimationClip& Clip = AnimationClips[ClipIndex];
			TotalCurveNum += Clip.AnimationCurves.Num<UINT32>();
			for (INT32 CurveIndex = 0, CurveNum = Clip.AnimationCurves.Num(); CurveIndex < CurveNum; CurveIndex++)
			{
				TotalKeyNum += GetCurveKeyCount(Clip.AnimationCurves[CurveIndex]);
			}
		}

		const BOOL8 bHasSkeletonName = ((InAnimation->GetSkeletonName().Length() > 0u)
			&& (InAnimation->GetSkeletonName() != EString(EEngineSettings::ENGINE_DEFAULT_NAME))) ? TRUE : FALSE;
		const BOOL8 bHasRootBoneName = ((InAnimation->GetRootBoneName().Length() > 0u)
			&& (InAnimation->GetRootBoneName() != EString(EEngineSettings::ENGINE_DEFAULT_NAME))) ? TRUE : FALSE;

		ImGui::Text("Asset Name: %s", *InAsset->GetAssetName());
		ImGui::Text("Asset Path: %s", *InAsset->GetAssetPath());
		ImGui::Text("Animation Name: %s", *InAnimation->GetAnimationName());
		ImGui::Text("Skeleton: %s", (bHasSkeletonName ? *InAnimation->GetSkeletonName() : "None"));
		ImGui::Text("Root Bone: %s", (bHasRootBoneName ? *InAnimation->GetRootBoneName() : "None"));
		ImGui::Text("Clip Count: %d", InAnimation->GetAnimationClipNum());
		ImGui::Text("Curve Count: %u", TotalCurveNum);
		ImGui::Text("Key Count: %u", TotalKeyNum);
		ImGui::Text("Resource Valid: %s", InAnimation->IsResourceValid() ? "True" : "False");
	}

	void EAnimationEditor::DrawClipList(const ESkeletonAnimation* InAnimation)
	{
		const TArray<ESkeletonAnimationClip>& AnimationClips = InAnimation->GetAnimationClips();
		const INT32 ClipNum = AnimationClips.Num();
		m_SelectedClipIndex = ClampSelectionIndex(m_SelectedClipIndex, ClipNum);

		ImGui::Text("Clips");
		ImGui::Separator();
		for (INT32 ClipIndex = 0; ClipIndex < ClipNum; ClipIndex++)
		{
			const ESkeletonAnimationClip& Clip = AnimationClips[ClipIndex];
			const BOOL8 bIsSelected = (m_SelectedClipIndex == ClipIndex);
			ImGui::PushID(ClipIndex);
			if (ImGui::Selectable(*Clip.ClipName, bIsSelected))
			{
				m_SelectedClipIndex = ClipIndex;
				m_SelectedCurveIndex = 0;
			}
			ImGui::PopID();
		}
	}

	void EAnimationEditor::DrawClipDetails(ESkeletonAnimationClip& InClip)
	{
		UINT32 TotalKeyNum = 0u;
		for (INT32 CurveIndex = 0, CurveNum = InClip.AnimationCurves.Num(); CurveIndex < CurveNum; CurveIndex++)
		{
			TotalKeyNum += GetCurveKeyCount(InClip.AnimationCurves[CurveIndex]);
		}

		const FLOAT ClipDurationSeconds = GetClipDurationSeconds(InClip);

		ImGui::Text("Clip Name: %s", *InClip.ClipName);
		ImGui::Text("Duration: %.3f", InClip.Duration);
		ImGui::Text("Ticks Per Second: %.3f", InClip.TicksPerSecond);
		ImGui::Text("Length (Seconds): %.3f", ClipDurationSeconds);
		ImGui::Text("Curve Count: %d", InClip.AnimationCurves.Num());
		ImGui::Text("Key Count: %u", TotalKeyNum);

		if (ImGui::Button("Save Animation Asset"))
		{
			if (m_AnimationAsset)
			{
				EString SaveError;
				if (SaveAnimationAsset(SaveError))
				{
					m_LastOpenError = EString();
					m_StatusMessage = EString("Saved.");
					m_bStatusSucceeded = TRUE;
					PE_LOG_LOG(EString("Saved animation asset: ") + m_AnimationAsset->GetAssetFullPathName());
				}
				else
				{
					m_LastOpenError = SaveError;
					m_StatusMessage = SaveError;
					m_bStatusSucceeded = FALSE;
					PE_LOG_WARN(SaveError);
				}
			}
		}

		DrawNotifyEditor(InClip);
	}

	void EAnimationEditor::DrawNotifyEditor(ESkeletonAnimationClip& InClip)
	{
		const FLOAT ClipDurationSeconds = GetClipDurationSeconds(InClip);

		ImGui::Separator();
		ImGui::Text("Notifies");
		ImGui::SameLine();
		if (ImGui::Button("Add Notify"))
		{
			FAnimNotifyEvent Notify;
			const TArray<EString> RegisteredClassNames = EAnimNotifyRegistry::GetRegisteredClassNames();
			Notify.NotifyName = EString("Notify") + ToString(InClip.Notifies.Num());
			Notify.NotifyClassName = ((RegisteredClassNames.Num() > 0) ? RegisteredClassNames[0] : EString());
			Notify.PayloadJson = EString("{}");
			Notify.bIsState = FALSE;
			Notify.StateDuration = 0.2f;
			Notify.Compile();
			InClip.Notifies.Add(Notify);
			m_Document.bDirty = TRUE;
		}

		if (InClip.Notifies.Num() <= 0)
		{
			ImGui::TextDisabled("No notifies.");
			return;
		}

		const TArray<EString> RegisteredClassNames = EAnimNotifyRegistry::GetRegisteredClassNames();
		for (INT32 NotifyIndex = 0; NotifyIndex < InClip.Notifies.Num(); NotifyIndex++)
		{
			FAnimNotifyEvent& Notify = InClip.Notifies[NotifyIndex];
			ImGui::PushID(NotifyIndex);
			ImGui::Separator();

			CHAR NameBuffer[EEngineSettings::ENGINE_ANIMATION_NOTIFY_NAME_LENGTH_MAX] = "";
			::strncpy_s(NameBuffer, sizeof(NameBuffer), *Notify.NotifyName, _TRUNCATE);
			if (ImGui::InputText("Name", NameBuffer, static_cast<int>(sizeof(NameBuffer))))
			{
				Notify.NotifyName = EString(NameBuffer);
				m_Document.bDirty = TRUE;
			}

			CHAR NotifyAssetBuffer[EEngineSettings::ENGINE_ANIMATION_NOTIFY_ASSET_NAME_LENGTH_MAX] = "";
			::strncpy_s(NotifyAssetBuffer, sizeof(NotifyAssetBuffer), *Notify.NotifyAssetName, _TRUNCATE);
			if (ImGui::InputText("Notify Asset", NotifyAssetBuffer, static_cast<int>(sizeof(NotifyAssetBuffer))))
			{
				Notify.NotifyAssetName = EString(NotifyAssetBuffer);
				Notify.bStateDurationOverride = FALSE;
				Notify.bPayloadOverride = FALSE;
				m_Document.bDirty = TRUE;
			}
			if (Notify.NotifyAssetName.Length() > 0u)
			{
				ImGui::SameLine();
				if (ImGui::Button("Resolve / Use Defaults"))
				{
					const EAnimNotifyDefinition* Definition = nullptr;
					if ((EAnimNotifyManager::GetManagerSingleton()->ResolveNotifyDefinition(Notify.NotifyAssetName, Definition))
						&& Definition)
					{
						Notify.bStateDurationOverride = FALSE;
						Notify.bPayloadOverride = FALSE;
						ApplyNotifyDefinitionToEvent(Notify, *Definition);
						m_Document.bDirty = TRUE;
					}
					else
					{
						PE_LOG_WARN(EString("Animation editor could not resolve notify asset '")
							+ Notify.NotifyAssetName + EString("'."));
					}
				}
				ImGui::TextDisabled("Asset supplies class/state/payload; editing duration or payload marks it as an event override.");
				ImGui::TextDisabled("Duration: %s; Payload: %s"
					, (Notify.bStateDurationOverride ? "event override" : "asset default")
					, (Notify.bPayloadOverride ? "event override" : "asset default"));
			}

			const CHAR* CurrentClassLabel = ((Notify.NotifyClassName.Length() > 0u) ? *Notify.NotifyClassName : "None");
			if (ImGui::BeginCombo("Class", CurrentClassLabel))
			{
				if (ImGui::Selectable("None", (Notify.NotifyClassName.Length() == 0u)))
				{
					Notify.NotifyClassName = EString();
					Notify.ResetCompiledNotify();
					m_Document.bDirty = TRUE;
				}
				for (INT32 ClassIndex = 0; ClassIndex < RegisteredClassNames.Num(); ClassIndex++)
				{
					const BOOL8 bSelected = (Notify.NotifyClassName == RegisteredClassNames[ClassIndex]);
					if (ImGui::Selectable(*RegisteredClassNames[ClassIndex], bSelected))
					{
						Notify.NotifyClassName = RegisteredClassNames[ClassIndex];
						Notify.ResetCompiledNotify();
						Notify.Compile();
						m_Document.bDirty = TRUE;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			if (ClipDurationSeconds > 0.0f)
			{
				FLOAT NotifyTime = Notify.Time;
				if (ImGui::SliderFloat("Time", &NotifyTime, 0.0f, ClipDurationSeconds, "%.3f", ImGuiSliderFlags_AlwaysClamp))
				{
					Notify.Time = NotifyTime;
					m_Document.bDirty = TRUE;
				}
			}
			else
			{
				ImGui::TextDisabled("Clip has no duration.");
			}

			bool bStateNotify = (Notify.bIsState != FALSE);
			if (ImGui::Checkbox("State Notify", &bStateNotify))
			{
				Notify.bIsState = (bStateNotify ? TRUE : FALSE);
				if (Notify.bIsState && (Notify.StateDuration <= 0.0f))
				{
					Notify.StateDuration = 0.2f;
				}
				Notify.ResetCompiledNotify();
				Notify.Compile();
				m_Document.bDirty = TRUE;
			}
			if (Notify.bIsState)
			{
				const FLOAT MaxStateDuration = ((ClipDurationSeconds > 0.001f) ? ClipDurationSeconds : 1.0f);
				FLOAT StateDuration = EMath::Clamp(Notify.StateDuration, 0.001f, MaxStateDuration);
				if (ImGui::SliderFloat("State Duration", &StateDuration, 0.001f, MaxStateDuration, "%.3f", ImGuiSliderFlags_AlwaysClamp))
				{
					Notify.StateDuration = StateDuration;
					Notify.bStateDurationOverride = TRUE;
					m_Document.bDirty = TRUE;
				}
				if ((Notify.NotifyClassName.Length() > 0u) && (!Notify.CachedNotify))
				{
					ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.25f, 1.0f)
						, "This class is not registered as a state notify; the event will not fire.");
				}
			}

			CHAR PayloadBuffer[EEngineSettings::ENGINE_ANIMATION_NOTIFY_PAYLOAD_LENGTH_MAX] = "";
			::strncpy_s(PayloadBuffer, sizeof(PayloadBuffer), *Notify.PayloadJson, _TRUNCATE);
			if (ImGui::InputTextMultiline("Payload", PayloadBuffer, static_cast<int>(sizeof(PayloadBuffer)), ImVec2(-1.0f, 70.0f)))
			{
				Notify.PayloadJson = EString(PayloadBuffer);
				Notify.bPayloadOverride = TRUE;
				// A notify class may parse its payload when it is created; rebuild the cached
				// object so the edit takes effect immediately.
				Notify.ResetCompiledNotify();
				Notify.Compile();
				m_Document.bDirty = TRUE;
			}

			if (ImGui::Button("Remove"))
			{
				InClip.Notifies.RemoveAt(NotifyIndex);
				m_Document.bDirty = TRUE;
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}
	}

	void EAnimationEditor::DrawCurveList(const ESkeletonAnimationClip& InClip)
	{
		const INT32 CurveNum = InClip.AnimationCurves.Num();
		m_SelectedCurveIndex = ClampSelectionIndex(m_SelectedCurveIndex, CurveNum);

		ImGui::Text("Bone Curves");
		ImGui::Separator();
		for (INT32 CurveIndex = 0; CurveIndex < CurveNum; CurveIndex++)
		{
			const EBoneAnimationCurve& Curve = InClip.AnimationCurves[CurveIndex];
			const BOOL8 bIsSelected = (m_SelectedCurveIndex == CurveIndex);
			ImGui::PushID(CurveIndex);
			if (ImGui::Selectable(*Curve.BoneName, bIsSelected))
			{
				m_SelectedCurveIndex = CurveIndex;
			}
			ImGui::PopID();
		}
	}

	void EAnimationEditor::DrawCurveDetails(const EBoneAnimationCurve& InCurve) const
	{
		FLOAT TimeStart = 0.0f;
		FLOAT TimeEnd = 0.0f;

		ImGui::Text("Bone Name: %s", *InCurve.BoneName);
		ImGui::Text("Pre State: %s", AnimationBehaviourTypeAsString(InCurve.PreState));
		ImGui::Text("Post State: %s", AnimationBehaviourTypeAsString(InCurve.PostState));
		ImGui::Text("Position Keys: %d", InCurve.PositionKeys.Num());
		if (TryGetKeyTimeRange(InCurve.PositionKeys, TimeStart, TimeEnd))
		{
			ImGui::Text("Position Range: %.3f -> %.3f", TimeStart, TimeEnd);
		}
		ImGui::Text("Rotation Keys: %d", InCurve.RotationKeys.Num());
		if (TryGetKeyTimeRange(InCurve.RotationKeys, TimeStart, TimeEnd))
		{
			ImGui::Text("Rotation Range: %.3f -> %.3f", TimeStart, TimeEnd);
		}
		ImGui::Text("Scaling Keys: %d", InCurve.ScalingKeys.Num());
		if (TryGetKeyTimeRange(InCurve.ScalingKeys, TimeStart, TimeEnd))
		{
			ImGui::Text("Scaling Range: %.3f -> %.3f", TimeStart, TimeEnd);
		}
		ImGui::Text("Total Keys: %u", GetCurveKeyCount(InCurve));

		FLOAT CurveTimeMax = 0.0f;
		FLOAT RangeStart = 0.0f;
		FLOAT RangeEnd = 0.0f;
		if (TryGetKeyTimeRange(InCurve.PositionKeys, RangeStart, RangeEnd))
		{
			CurveTimeMax = EMath::Max(CurveTimeMax, RangeEnd);
		}
		if (TryGetKeyTimeRange(InCurve.RotationKeys, RangeStart, RangeEnd))
		{
			CurveTimeMax = EMath::Max(CurveTimeMax, RangeEnd);
		}
		if (TryGetKeyTimeRange(InCurve.ScalingKeys, RangeStart, RangeEnd))
		{
			CurveTimeMax = EMath::Max(CurveTimeMax, RangeEnd);
		}
		if (CurveTimeMax > 0.0f)
		{
			ImGui::Separator();
			ImGui::TextDisabled("Curve Preview (read-only)");
			const FLOAT PlotWidth = EMath::Max(ImGui::GetContentRegionAvail().x - 8.0f, 120.0f);
			EAnimationCurvePlot::DrawVectorKeys("Position", InCurve.PositionKeys, CurveTimeMax, PlotWidth, 62.0f);
			EAnimationCurvePlot::DrawRotationKeys(InCurve.RotationKeys, CurveTimeMax, PlotWidth, 62.0f);
			EAnimationCurvePlot::DrawVectorKeys("Scaling", InCurve.ScalingKeys, CurveTimeMax, PlotWidth, 62.0f);
		}
	}
}

#endif