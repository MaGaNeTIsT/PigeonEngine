#include "AnimationMontageEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

#include <AnimationAsset/AnimationAsset.h>
#include <AnimationMontageAsset/AnimationMontageAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <RapidJson/Headers/JsonObject.h>
#include "../AnimationCurvePlot/AnimationCurvePlot.h"
#include <Base/DataStructure/Text/Path.h>

#include <cfloat>

namespace PigeonEngine
{

	namespace
	{
		// Montage JSON grows with sections and notify lists; give the buffer room so the
		// author can add a chain without the ImGui input reallocating every frame.
		constexpr INT32 MontageEditorBufferHeadroom = 24 * 1024;

		const CHAR* GetLoopModeLabel(const EAnimationLoopMode InLoopMode)
		{
			switch (InLoopMode)
			{
			case ANIMATION_LOOP_ONCE:
				return "Once";
			case ANIMATION_LOOP_REPEAT:
				return "Repeat";
			case ANIMATION_LOOP_PINGPONG:
				return "PingPong";
			case ANIMATION_LOOP_CLAMP:
				return "Clamp";
			default:
				return "Unknown";
			}
		}

		FLOAT GetClipDurationSeconds(const ESkeletonAnimationClip* InClip)
		{
			if ((!InClip) || (InClip->Duration <= 0.0f))
			{
				return 0.0f;
			}
			const FLOAT TicksPerSecond = ((InClip->TicksPerSecond > 0.001f)
				? InClip->TicksPerSecond : EEngineSettings::ENGINE_ANIMATION_DEFAULT_TICKS_PER_SECOND);
			return (InClip->Duration / TicksPerSecond);
		}

		// GetObjectArrayField hands out heap copies; this guard owns them for the duration of
		// one structural edit, including objects appended for a new section.
		struct FJsonObjectArrayGuard
		{
			explicit FJsonObjectArrayGuard(TArray<CJsonObject*>& InObjects) : Objects(InObjects) {}
			~FJsonObjectArrayGuard()
			{
				for (INT32 Index = 0; Index < Objects.Num(); Index++)
				{
					delete Objects[Index];
				}
				Objects.Empty();
			}
			TArray<CJsonObject*>& Objects;
		};

		EString GetJsonStringField(CJsonObject& InObject, const EString& InFieldName)
		{
			EString Result;
			InObject.GetStringField(InFieldName, Result);
			return Result;
		}

	}

	void EAnimMontageEditor::EditorInit()
	{
		m_ShowEditor = FALSE;
		m_Asset = nullptr;
		m_AnimationAsset = nullptr;
		m_SkeletonAsset = nullptr;
		m_OpenedAssetFullPath = EString();
		m_LastError = EString();
		m_JsonBuffer.Empty();
		m_SelectedCurveSectionIndex = 0;
		m_SelectedCurveIndex = 0;
	}

	void EAnimMontageEditor::OpenMontageAsset(const EString& InAssetFullPath)
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
		OpenMontageAssetNow(InAssetFullPath);
	}

	void EAnimMontageEditor::OpenMontageAssetNow(const EString& InAssetFullPath)
	{
		m_ShowEditor = TRUE;
		m_LastError = EString();

		const EString NormalizedAssetPath = InAssetFullPath.Replace("\\", "/");
		const EString AssetName = EPath::GetFileNameWithoutExtension(NormalizedAssetPath);
		EString AssetDirectory = EPath::GetFileFolderPath(NormalizedAssetPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			m_LastError = EString("Montage editor cannot parse asset path: ") + NormalizedAssetPath;
			return;
		}

		const EAnimMontageAsset* LoadedAsset = nullptr;
		if (!(EAnimMontageManager::GetManagerSingleton()->LoadMontageAsset(AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			m_LastError = EString("Montage editor failed to load asset: ") + NormalizedAssetPath;
			return;
		}

		m_Asset = LoadedAsset;
		m_OpenedAssetFullPath = NormalizedAssetPath;
		m_SelectedCurveSectionIndex = 0;
		m_SelectedCurveIndex = 0;
		m_AnimationAsset = nullptr;
		const EAnimMontage* Montage = m_Asset->GetStoragedResource();
		if ((Montage) && (Montage->AnimationAssetName.Length() > 0u))
		{
			const ESkeletonAnimationAsset* LoadedAnimation = nullptr;
			if (EAnimationManager::GetManagerSingleton()->LoadSkeletonAnimationAsset(
				EBaseSettings::ENGINE_ANIMATION_PATH, Montage->AnimationAssetName, LoadedAnimation))
			{
				m_AnimationAsset = LoadedAnimation;
			}
		}
		if (Montage)
		{
			RefreshSkeletonAsset(*Montage);
		}
		LoadJsonBufferFromAsset();
		CaptureBaseline();
		m_Document.ClearPendingAction();
		m_Document.bDirty = FALSE;
		m_StatusMessage = EString("Loaded.");
		m_bStatusSucceeded = TRUE;
	}

	void EAnimMontageEditor::CaptureBaseline()
	{
		m_BaselineJson = EString();
		const EAnimMontage* Montage = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
		if (Montage)
		{
			m_BaselineJson = Montage->SourceJson;
		}
	}

	BOOL32 EAnimMontageEditor::DiscardChanges()
	{
		if (m_Asset && (m_BaselineJson.Length() > 0u))
		{
			EString Error;
			if (!EAnimMontageManager::GetManagerSingleton()->RebuildMontageAsset(m_Asset, m_BaselineJson, Error))
			{
				m_LastError = Error;
				m_StatusMessage = Error;
				m_bStatusSucceeded = FALSE;
				return FALSE;
			}
		}
		m_JsonBuffer.Empty();
		const EString Baseline = m_BaselineJson;
		if (Baseline.Length() > 0u)
		{
			const INT32 BufferLength = static_cast<INT32>(Baseline.Length()) + MontageEditorBufferHeadroom + 1;
			m_JsonBuffer.SetNum(BufferLength);
			::memset(m_JsonBuffer.GetData(), 0, static_cast<size_t>(BufferLength));
			::memcpy_s(m_JsonBuffer.GetData(), static_cast<rsize_t>(BufferLength), *Baseline, Baseline.Length());
		}
		m_Document.bDirty = FALSE;
		m_StatusMessage = EString("Discarded unsaved changes.");
		m_bStatusSucceeded = TRUE;
		return TRUE;
	}

	BOOL32 EAnimMontageEditor::SaveMontageAsset(EString& OutError)
	{
		OutError = EString();
		if (!m_Asset)
		{
			OutError = EString("No montage asset is open.");
			return FALSE;
		}
		const EString JsonText(m_JsonBuffer.GetData());
		if (!EAnimMontageManager::GetManagerSingleton()->SaveMontageAsset(m_Asset, JsonText, OutError))
		{
			return FALSE;
		}
		m_BaselineJson = JsonText;
		m_Document.bDirty = FALSE;
		return TRUE;
	}

	BOOL32 EAnimMontageEditor::ReloadMontageAssetNow(EString& OutError)
	{
		OutError = EString();
		if (!m_Asset)
		{
			OutError = EString("No montage asset is open.");
			return FALSE;
		}

		const EAnimMontageAsset* FreshAsset = nullptr;
		if (!(EAnimMontageManager::GetManagerSingleton()->ReloadMontageAsset(
			m_Asset->GetAssetPath(), m_Asset->GetAssetName(), FreshAsset, OutError)) || (!FreshAsset))
		{
			return FALSE;
		}

		m_Asset = FreshAsset;
		m_AnimationAsset = nullptr;
		const EAnimMontage* Montage = m_Asset->GetStoragedResource();
		if ((Montage) && (Montage->AnimationAssetName.Length() > 0u))
		{
			const ESkeletonAnimationAsset* LoadedAnimation = nullptr;
			if (EAnimationManager::GetManagerSingleton()->LoadSkeletonAnimationAsset(
				EBaseSettings::ENGINE_ANIMATION_PATH, Montage->AnimationAssetName, LoadedAnimation))
			{
				m_AnimationAsset = LoadedAnimation;
			}
		}
		if (Montage)
		{
			RefreshSkeletonAsset(*Montage);
		}
		m_SelectedCurveSectionIndex = 0;
		m_SelectedCurveIndex = 0;
		LoadJsonBufferFromAsset();
		CaptureBaseline();
		m_Document.ClearPendingAction();
		m_Document.bDirty = FALSE;
		return TRUE;
	}

	void EAnimMontageEditor::HandleDocumentAction()
	{
		const EEditorUnsavedChoiceType Choice = DrawEditorUnsavedChangesPopup("##montage_editor_unsaved"
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
			if (!SaveMontageAsset(SaveError))
			{
				m_LastError = SaveError;
				m_StatusMessage = SaveError;
				m_bStatusSucceeded = FALSE;
				return;
			}
		}
		else
		{
			if (!DiscardChanges())
			{
				return;
			}
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
			if (!ReloadMontageAssetNow(ReloadError))
			{
				m_LastError = ReloadError;
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
			OpenMontageAssetNow(PendingPath);
		}
	}

	void EAnimMontageEditor::EditorUpdate()
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

		ImGui::SetNextWindowSize(ImVec2(920.0f, 760.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(440.0f, 36.0f), ImGuiCond_FirstUseEver);
		bool bWindowOpen = true;
		ImGui::Begin("Montage Editor", &bWindowOpen, ImGuiWindowFlags_None);
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
					if (!ReloadMontageAssetNow(ReloadError))
					{
						m_LastError = ReloadError;
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
			if (ImGui::Button("Apply"))
			{
				if (m_Asset)
				{
					EString Error;
					const EString JsonText(m_JsonBuffer.GetData());
					if (EAnimMontageManager::GetManagerSingleton()->RebuildMontageAsset(m_Asset, JsonText, Error))
					{
						m_LastError = EString();
						m_Document.bDirty = TRUE;
						m_StatusMessage = EString("Applied to the live asset. Save to write it to disk.");
						m_bStatusSucceeded = TRUE;
					}
					else
					{
						m_LastError = Error;
						m_StatusMessage = Error;
						m_bStatusSucceeded = FALSE;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Save"))
			{
				if (m_Asset)
				{
					EString Error;
					if (SaveMontageAsset(Error))
					{
						m_LastError = EString();
						m_StatusMessage = EString("Saved.");
						m_bStatusSucceeded = TRUE;
					}
					else
					{
						m_LastError = Error;
						m_StatusMessage = Error;
						m_bStatusSucceeded = FALSE;
					}
				}
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No montage selected."));
			if (m_Document.bDirty)
			{
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f), "UNSAVED");
			}

			if (m_LastError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastError);
			}
			if (m_StatusMessage.Length() > 0u)
			{
				const ImVec4 StatusColor = (m_bStatusSucceeded != FALSE)
					? ImVec4(0.45f, 0.85f, 0.45f, 1.0f)
					: ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
				ImGui::TextColored(StatusColor, "%s", *m_StatusMessage);
			}

			ImGui::Separator();
			if (m_Asset)
			{
				const EAnimMontage* Montage = m_Asset->GetStoragedResource();
				if (Montage)
				{
					ImGui::Text("Name: %s", *Montage->Name);
					ImGui::Text("Skeleton: %s", *Montage->SkeletonName);
					ImGui::Text("Animation: %s", *Montage->AnimationAssetName);
					ImGui::Text("Slot groups: %d  Sections: %d", Montage->SlotGroups.Num(), Montage->Sections.Num());

					if (ImGui::TreeNode("Slot groups"))
					{
						for (INT32 GroupIndex = 0; GroupIndex < Montage->SlotGroups.Num(); GroupIndex++)
						{
							const EAnimMontageSlotGroup& Group = Montage->SlotGroups[GroupIndex];
							ImGui::BulletText("%s (%d slots)", *Group.Name, Group.SlotNames.Num());
						}
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("Sections (editable)"))
					{
						DrawEditableSections(*Montage);
						ImGui::TreePop();
					}

					ImGui::Separator();
					ImGui::BeginChild("MontageTimeline", ImVec2(0.0f, 210.0f), true);
					DrawTimeline(*Montage);
					ImGui::EndChild();

					ImGui::Separator();
					ImGui::BeginChild("MontageCurvePreview", ImVec2(0.0f, 240.0f), true);
					DrawCurvePreview(*Montage);
					ImGui::EndChild();
				}
			}

			if (m_JsonBuffer.Num() > 0)
			{
				if (ImGui::InputTextMultiline("JSON", m_JsonBuffer.GetData(), static_cast<int>(m_JsonBuffer.Num())
					, ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput))
				{
					m_Document.bDirty = TRUE;
				}
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
			}
		}
	}

	void EAnimMontageEditor::DrawTimeline(const EAnimMontage& InMontage)
	{
		ImGui::TextDisabled("Timeline (read-only; edit the JSON below)");
		ImGui::SameLine();
		ImGui::TextDisabled("| blue = state range, orange = instant notify");
		if (InMontage.Sections.Num() <= 0)
		{
			ImGui::TextDisabled("No sections.");
			return;
		}

		const ESkeletonAnimation* Animation = (m_AnimationAsset ? m_AnimationAsset->GetStoragedResource() : nullptr);
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		constexpr FLOAT LabelWidth = 180.0f;
		constexpr FLOAT RowHeight = 28.0f;

		for (INT32 SectionIndex = 0; SectionIndex < InMontage.Sections.Num(); SectionIndex++)
		{
			const EAnimMontageSection& Section = InMontage.Sections[SectionIndex];
			const ESkeletonAnimationClip* Clip = (Animation ? Animation->GetAnimationClip(Section.ClipName) : nullptr);
			const FLOAT DurationSeconds = GetClipDurationSeconds(Clip);

			ImGui::Text("%s", *Section.Name);
			ImGui::SameLine(LabelWidth);

			const ImVec2 BarMin = ImGui::GetCursorScreenPos();
			const FLOAT BarWidth = EMath::Max(ImGui::GetContentRegionAvail().x - 70.0f, 80.0f);
			const ImVec2 BarMax(BarMin.x + BarWidth, BarMin.y + 24.0f);
			DrawList->AddRectFilled(BarMin, BarMax, IM_COL32(38, 40, 46, 255), 3.0f);
			DrawList->AddRect(BarMin, BarMax, IM_COL32(90, 92, 100, 255), 3.0f);

			if (DurationSeconds > 0.0f)
			{
				const FLOAT InvDuration = 1.0f / DurationSeconds;
				for (const FAnimNotifyEvent& Notify : Section.Notifies)
				{
					if (!Notify.bIsState)
					{
						continue;
					}
					const FLOAT Start = EMath::Clamp(Notify.Time * InvDuration, 0.0f, 1.0f);
					const FLOAT End = EMath::Clamp((Notify.Time + EMath::Max(Notify.StateDuration, 0.001f)) * InvDuration, 0.0f, 1.0f);
					const FLOAT X0 = BarMin.x + Start * BarWidth;
					const FLOAT X1 = BarMin.x + End * BarWidth;
					if (X1 > X0)
					{
						DrawList->AddRectFilled(ImVec2(X0, BarMin.y + 3.0f), ImVec2(X1, BarMax.y - 3.0f)
							, IM_COL32(70, 170, 255, 120), 2.0f);
						DrawList->AddRect(ImVec2(X0, BarMin.y + 3.0f), ImVec2(X1, BarMax.y - 3.0f)
							, IM_COL32(110, 200, 255, 220), 2.0f);
					}
				}
				for (const FAnimNotifyEvent& Notify : Section.Notifies)
				{
					if (Notify.bIsState)
					{
						continue;
					}
					const FLOAT T = EMath::Clamp(Notify.Time * InvDuration, 0.0f, 1.0f);
					const FLOAT X = BarMin.x + T * BarWidth;
					DrawList->AddLine(ImVec2(X, BarMin.y + 2.0f), ImVec2(X, BarMax.y - 2.0f)
						, IM_COL32(255, 190, 60, 255), 2.0f);
					DrawList->AddCircleFilled(ImVec2(X, BarMin.y + 4.0f), 3.0f, IM_COL32(255, 210, 100, 255));
				}

				CHAR DurationText[64] = "";
				::sprintf_s(DurationText, sizeof(DurationText), "%.2fs", DurationSeconds);
				DrawList->AddText(ImVec2(BarMax.x + 6.0f, BarMin.y + 4.0f), IM_COL32(180, 180, 180, 255), DurationText);
			}
			else
			{
				DrawList->AddText(ImVec2(BarMin.x + 8.0f, BarMin.y + 5.0f)
					, IM_COL32(220, 120, 120, 255), "clip duration unavailable");
			}

			ImGui::Dummy(ImVec2(0.0f, RowHeight));
		}
	}

	void EAnimMontageEditor::DrawCurvePreview(const EAnimMontage& InMontage)
	{
		ImGui::TextDisabled("Curve preview (read-only)");
		if ((!m_AnimationAsset) || (!m_AnimationAsset->GetStoragedResource()) || (InMontage.Sections.Num() <= 0))
		{
			ImGui::TextDisabled("Animation asset or sections are unavailable.");
			return;
		}
		const ESkeletonAnimation* Animation = m_AnimationAsset->GetStoragedResource();
		m_SelectedCurveSectionIndex = EMath::Clamp(m_SelectedCurveSectionIndex, 0, InMontage.Sections.Num() - 1);
		const EString CurrentSectionName = InMontage.Sections[m_SelectedCurveSectionIndex].Name;
		if (ImGui::BeginCombo("Section", *CurrentSectionName))
		{
			for (INT32 SectionIndex = 0; SectionIndex < InMontage.Sections.Num(); SectionIndex++)
			{
				const BOOL8 bSelected = (SectionIndex == m_SelectedCurveSectionIndex);
				if (ImGui::Selectable(*InMontage.Sections[SectionIndex].Name, bSelected))
				{
					m_SelectedCurveSectionIndex = SectionIndex;
					m_SelectedCurveIndex = 0;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		const EAnimMontageSection& Section = InMontage.Sections[m_SelectedCurveSectionIndex];
		const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(Section.ClipName);
		if (!Clip)
		{
			ImGui::TextDisabled("Clip '%s' is unavailable.", *Section.ClipName);
			return;
		}
		const INT32 CurveNum = Clip->AnimationCurves.Num();
		if (CurveNum <= 0)
		{
			ImGui::TextDisabled("This clip has no bone curves.");
			return;
		}
		m_SelectedCurveIndex = EMath::Clamp(m_SelectedCurveIndex, 0, CurveNum - 1);
		const EString CurrentCurveName = Clip->AnimationCurves[m_SelectedCurveIndex].BoneName;
		if (ImGui::BeginCombo("Bone Curve", *CurrentCurveName))
		{
			for (INT32 CurveIndex = 0; CurveIndex < CurveNum; CurveIndex++)
			{
				const BOOL8 bSelected = (CurveIndex == m_SelectedCurveIndex);
				if (ImGui::Selectable(*Clip->AnimationCurves[CurveIndex].BoneName, bSelected))
				{
					m_SelectedCurveIndex = CurveIndex;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		const EBoneAnimationCurve& Curve = Clip->AnimationCurves[m_SelectedCurveIndex];
		FLOAT TimeMax = Clip->Duration;
		for (INT32 KeyIndex = 0; KeyIndex < Curve.PositionKeys.Num(); KeyIndex++)
		{
			TimeMax = EMath::Max(TimeMax, Curve.PositionKeys[KeyIndex].Time);
		}
		for (INT32 KeyIndex = 0; KeyIndex < Curve.RotationKeys.Num(); KeyIndex++)
		{
			TimeMax = EMath::Max(TimeMax, Curve.RotationKeys[KeyIndex].Time);
		}
		for (INT32 KeyIndex = 0; KeyIndex < Curve.ScalingKeys.Num(); KeyIndex++)
		{
			TimeMax = EMath::Max(TimeMax, Curve.ScalingKeys[KeyIndex].Time);
		}
		if (TimeMax <= 0.0f)
		{
			TimeMax = 1.0f;
		}

		const FLOAT PlotWidth = EMath::Max(ImGui::GetContentRegionAvail().x, 120.0f);
		EAnimationCurvePlot::DrawVectorKeys("Position", Curve.PositionKeys, TimeMax, PlotWidth, 62.0f);
		EAnimationCurvePlot::DrawRotationKeys(Curve.RotationKeys, TimeMax, PlotWidth, 62.0f);
		EAnimationCurvePlot::DrawVectorKeys("Scaling", Curve.ScalingKeys, TimeMax, PlotWidth, 62.0f);
	}

	void EAnimMontageEditor::LoadJsonBufferFromAsset()
	{
		m_JsonBuffer.Empty();
		if (!m_Asset)
		{
			return;
		}
		const EAnimMontage* Montage = m_Asset->GetStoragedResource();
		if (!Montage)
		{
			return;
		}

		const EString& SourceJson = Montage->SourceJson;
		const INT32 BufferLength = static_cast<INT32>(SourceJson.Length()) + MontageEditorBufferHeadroom + 1;
		m_JsonBuffer.SetNum(BufferLength);
		::memset(m_JsonBuffer.GetData(), 0, static_cast<size_t>(BufferLength));
		if (SourceJson.Length() > 0u)
		{
			::memcpy_s(m_JsonBuffer.GetData(), static_cast<rsize_t>(BufferLength), *SourceJson, SourceJson.Length());
		}
	}

	void EAnimMontageEditor::ReplaceJsonBuffer(const EString& InJsonText)
	{
		m_JsonBuffer.Empty();
		const INT32 BufferLength = static_cast<INT32>(InJsonText.Length()) + MontageEditorBufferHeadroom + 1;
		m_JsonBuffer.SetNum(BufferLength);
		::memset(m_JsonBuffer.GetData(), 0, static_cast<size_t>(BufferLength));
		if (InJsonText.Length() > 0u)
		{
			::memcpy_s(m_JsonBuffer.GetData(), static_cast<rsize_t>(BufferLength), *InJsonText, InJsonText.Length());
		}
	}

	BOOL32 EAnimMontageEditor::CommitJsonText(const EString& InJsonText)
	{
		EString Error;
		if (!(EAnimMontageManager::GetManagerSingleton()->RebuildMontageAsset(m_Asset, InJsonText, Error)))
		{
			m_LastError = Error;
			m_StatusMessage = Error;
			m_bStatusSucceeded = FALSE;
			return FALSE;
		}
		ReplaceJsonBuffer(InJsonText);
		m_LastError = EString();
		m_Document.bDirty = TRUE;
		return TRUE;
	}

	BOOL32 EAnimMontageEditor::UpdateMontageSection(INT32 InSectionIndex, const TFunction<void(CJsonObject&)>& InMutator)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		CJsonObject Root(EString(m_JsonBuffer.GetData()));
		if (!Root.IsValid())
		{
			m_LastError = EString("Montage JSON is not a valid object.");
			return FALSE;
		}
		TArray<CJsonObject*> Sections;
		if (!(Root.GetObjectArrayField(PE_TEXT("Sections"), Sections)))
		{
			m_LastError = EString("Montage JSON has no sections array.");
			return FALSE;
		}
		FJsonObjectArrayGuard SectionGuard(Sections);
		if ((InSectionIndex < 0) || (InSectionIndex >= Sections.Num()) || (!Sections[InSectionIndex]))
		{
			m_LastError = EString("Montage section index is out of range.");
			return FALSE;
		}
		InMutator(*Sections[InSectionIndex]);
		Root.SetObjectArrayField(PE_TEXT("Sections"), Sections);
		return CommitJsonText(Root.ToString());
	}

	BOOL32 EAnimMontageEditor::AddMontageSection()
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		CJsonObject Root(EString(m_JsonBuffer.GetData()));
		if (!Root.IsValid())
		{
			m_LastError = EString("Montage JSON is not a valid object.");
			return FALSE;
		}
		TArray<CJsonObject*> Sections;
		if (!(Root.GetObjectArrayField(PE_TEXT("Sections"), Sections)))
		{
			m_LastError = EString("Montage JSON has no sections array.");
			return FALSE;
		}
		FJsonObjectArrayGuard SectionGuard(Sections);

		INT32 Suffix = Sections.Num();
		EString NewName;
		BOOL32 bNameExists = TRUE;
		while (bNameExists != FALSE)
		{
			bNameExists = FALSE;
			NewName = EString(PE_TEXT("Section")) + ToString(Suffix++);
			for (INT32 SectionIndex = 0; SectionIndex < Sections.Num(); SectionIndex++)
			{
				if ((Sections[SectionIndex]) && (GetJsonStringField(*Sections[SectionIndex], PE_TEXT("Name")) == NewName))
				{
					bNameExists = TRUE;
					break;
				}
			}
		}

		EString ClipName;
		const ESkeletonAnimation* Animation = (m_AnimationAsset ? m_AnimationAsset->GetStoragedResource() : nullptr);
		if ((Animation) && (Animation->GetAnimationClipNum() > 0))
		{
			const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(0);
			if (Clip)
			{
				ClipName = Clip->ClipName;
			}
		}
		EString SlotName;
		const ESkeleton* Skeleton = (m_SkeletonAsset ? m_SkeletonAsset->GetStoragedResource() : nullptr);
		if (Skeleton)
		{
			const TArray<ESkeletonSlotGroup>& Groups = Skeleton->GetSlotGroups();
			if ((Groups.Num() > 0) && (Groups[0].SlotNames.Num() > 0))
			{
				SlotName = Groups[0].SlotNames[0];
			}
		}

		CJsonObject* NewSection = new CJsonObject();
		NewSection->SetStringField(PE_TEXT("Name"), NewName);
		NewSection->SetStringField(PE_TEXT("Clip"), ClipName);
		NewSection->SetStringField(PE_TEXT("Slot"), SlotName);
		NewSection->SetStringField(PE_TEXT("Next"), EString());
		NewSection->SetStringField(PE_TEXT("Loop"), EString(PE_TEXT("Once")));
		NewSection->SetFloatField(PE_TEXT("Rate"), 1.0f);
		NewSection->SetFloatField(PE_TEXT("BlendIn"), 0.1f);
		NewSection->SetFloatField(PE_TEXT("BlendOut"), 0.2f);
		NewSection->SetBoolField(PE_TEXT("Additive"), FALSE);
		Sections.Add(NewSection);

		Root.SetObjectArrayField(PE_TEXT("Sections"), Sections);
		return CommitJsonText(Root.ToString());
	}

	BOOL32 EAnimMontageEditor::RemoveMontageSection(INT32 InSectionIndex)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		CJsonObject Root(EString(m_JsonBuffer.GetData()));
		if (!Root.IsValid())
		{
			m_LastError = EString("Montage JSON is not a valid object.");
			return FALSE;
		}
		TArray<CJsonObject*> Sections;
		if (!(Root.GetObjectArrayField(PE_TEXT("Sections"), Sections)))
		{
			m_LastError = EString("Montage JSON has no sections array.");
			return FALSE;
		}
		FJsonObjectArrayGuard SectionGuard(Sections);
		if ((InSectionIndex < 0) || (InSectionIndex >= Sections.Num()) || (!Sections[InSectionIndex]))
		{
			m_LastError = EString("Montage section index is out of range.");
			return FALSE;
		}
		if (Sections.Num() <= 1)
		{
			m_LastError = EString("A montage needs at least one section.");
			return FALSE;
		}

		const EString RemovedName = GetJsonStringField(*Sections[InSectionIndex], PE_TEXT("Name"));
		delete Sections[InSectionIndex];
		Sections.RemoveAt(InSectionIndex);
		// A chain that pointed at the removed section now ends instead of referencing a name
		// that no longer exists. The builder would reject the dangling reference.
		for (INT32 SectionIndex = 0; SectionIndex < Sections.Num(); SectionIndex++)
		{
			if ((Sections[SectionIndex]) && (GetJsonStringField(*Sections[SectionIndex], PE_TEXT("Next")) == RemovedName))
			{
				Sections[SectionIndex]->SetStringField(PE_TEXT("Next"), EString());
			}
		}

		Root.SetObjectArrayField(PE_TEXT("Sections"), Sections);
		return CommitJsonText(Root.ToString());
	}

	BOOL32 EAnimMontageEditor::UpdateBoneMask(INT32 InSectionIndex, const TArray<EString>& InBones, FLOAT InWeight
		, const EString& InMaskName)
	{
		return UpdateMontageSection(InSectionIndex, [InBones, InWeight, InMaskName](CJsonObject& InSection)
			{
				CJsonObject MaskObject;
				InSection.GetObjectField(PE_TEXT("BoneMask"), MaskObject);
				MaskObject.SetStringField(PE_TEXT("Name"), InMaskName);
				MaskObject.SetStringArrayField(PE_TEXT("Bones"), InBones);
				MaskObject.SetFloatField(PE_TEXT("Weight"), InWeight);
				InSection.SetObjectField(PE_TEXT("BoneMask"), &MaskObject);
			});
	}

	BOOL32 EAnimMontageEditor::UpdateMontageNotify(INT32 InSectionIndex, INT32 InNotifyIndex
		, const TFunction<void(CJsonObject&)>& InMutator)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		const EAnimMontage* Montage = m_Asset->GetStoragedResource();
		if ((!Montage) || (InSectionIndex < 0) || (InSectionIndex >= Montage->Sections.Num()))
		{
			return FALSE;
		}
		const EAnimMontageSection& Section = Montage->Sections[InSectionIndex];
		if ((InNotifyIndex < 0) || (InNotifyIndex >= Section.Notifies.Num()))
		{
			return FALSE;
		}

		return UpdateMontageSection(InSectionIndex, [InNotifyIndex, &InMutator](CJsonObject& InSection)
			{
				TArray<CJsonObject*> Notifies;
				if (!(InSection.GetObjectArrayField(PE_TEXT("Notifies"), Notifies)))
				{
					return;
				}
				FJsonObjectArrayGuard NotifyGuard(Notifies);
				if ((InNotifyIndex < 0) || (InNotifyIndex >= Notifies.Num()) || (!Notifies[InNotifyIndex]))
				{
					return;
				}
				InMutator(*Notifies[InNotifyIndex]);
				InSection.SetObjectArrayField(PE_TEXT("Notifies"), Notifies);
			});
	}

	BOOL32 EAnimMontageEditor::AddMontageNotify(INT32 InSectionIndex)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		const EAnimMontage* Montage = m_Asset->GetStoragedResource();
		if ((!Montage) || (InSectionIndex < 0) || (InSectionIndex >= Montage->Sections.Num()))
		{
			return FALSE;
		}

		return UpdateMontageSection(InSectionIndex, [](CJsonObject& InSection)
			{
				TArray<CJsonObject*> Notifies;
				InSection.GetObjectArrayField(PE_TEXT("Notifies"), Notifies);
				FJsonObjectArrayGuard NotifyGuard(Notifies);

				INT32 Suffix = Notifies.Num();
				EString NewName;
				BOOL32 bNameExists = TRUE;
				while (bNameExists != FALSE)
				{
					bNameExists = FALSE;
					NewName = EString(PE_TEXT("Notify")) + ToString(Suffix++);
					for (INT32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); NotifyIndex++)
					{
						if ((Notifies[NotifyIndex]) && (GetJsonStringField(*Notifies[NotifyIndex], PE_TEXT("Name")) == NewName))
						{
							bNameExists = TRUE;
							break;
						}
					}
				}

				const TArray<EString> RegisteredClassNames = EAnimNotifyRegistry::GetRegisteredClassNames();
				// Prefer an instant class for a new event; the registry order is not stable and
				// a state class must not be written with State=false.
				EString DefaultClass;
				BOOL32 bDefaultIsState = FALSE;
				for (INT32 ClassIndex = 0; ClassIndex < RegisteredClassNames.Num(); ClassIndex++)
				{
					BOOL32 bClassIsState = FALSE;
					if ((EAnimNotifyRegistry::IsStateClass(RegisteredClassNames[ClassIndex], bClassIsState) != FALSE)
						&& (bClassIsState == FALSE))
					{
						DefaultClass = RegisteredClassNames[ClassIndex];
						break;
					}
				}
				if ((DefaultClass.Length() <= 0u) && (RegisteredClassNames.Num() > 0))
				{
					DefaultClass = RegisteredClassNames[0];
					EAnimNotifyRegistry::IsStateClass(DefaultClass, bDefaultIsState);
				}
				CJsonObject* NewNotify = new CJsonObject();
				NewNotify->SetStringField(PE_TEXT("Name"), NewName);
				NewNotify->SetStringField(PE_TEXT("Class"), DefaultClass);
				NewNotify->SetFloatField(PE_TEXT("Time"), 0.0f);
				NewNotify->SetBoolField(PE_TEXT("State"), bDefaultIsState);
				if (bDefaultIsState != FALSE)
				{
					NewNotify->SetFloatField(PE_TEXT("Duration"), 0.1f);
				}
				NewNotify->SetStringField(PE_TEXT("Payload"), EString(PE_TEXT("{}")));
				Notifies.Add(NewNotify);
				InSection.SetObjectArrayField(PE_TEXT("Notifies"), Notifies);
			});
	}

	BOOL32 EAnimMontageEditor::RemoveMontageNotify(INT32 InSectionIndex, INT32 InNotifyIndex)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		const EAnimMontage* Montage = m_Asset->GetStoragedResource();
		if ((!Montage) || (InSectionIndex < 0) || (InSectionIndex >= Montage->Sections.Num()))
		{
			return FALSE;
		}
		const EAnimMontageSection& Section = Montage->Sections[InSectionIndex];
		if ((InNotifyIndex < 0) || (InNotifyIndex >= Section.Notifies.Num()))
		{
			return FALSE;
		}

		return UpdateMontageSection(InSectionIndex, [InNotifyIndex](CJsonObject& InSection)
			{
				TArray<CJsonObject*> Notifies;
				if (!(InSection.GetObjectArrayField(PE_TEXT("Notifies"), Notifies)))
				{
					return;
				}
				FJsonObjectArrayGuard NotifyGuard(Notifies);
				if ((InNotifyIndex < 0) || (InNotifyIndex >= Notifies.Num()) || (!Notifies[InNotifyIndex]))
				{
					return;
				}
				delete Notifies[InNotifyIndex];
				Notifies.RemoveAt(InNotifyIndex);
				InSection.SetObjectArrayField(PE_TEXT("Notifies"), Notifies);
			});
	}

	BOOL32 EAnimMontageEditor::DrawSectionNotifies(const EAnimMontageSection& InSection, INT32 InSectionIndex)
	{
		if (ImGui::Button("Add Notify"))
		{
			if (AddMontageNotify(InSectionIndex))
			{
				return TRUE;
			}
		}
		if (InSection.Notifies.Num() <= 0)
		{
			ImGui::TextDisabled("No notifies.");
			return FALSE;
		}

		const ESkeletonAnimation* Animation = (m_AnimationAsset ? m_AnimationAsset->GetStoragedResource() : nullptr);
		const ESkeletonAnimationClip* Clip = (Animation ? Animation->GetAnimationClip(InSection.ClipName) : nullptr);
		const FLOAT ClipDurationSeconds = GetClipDurationSeconds(Clip);
		const TArray<EString> RegisteredClassNames = EAnimNotifyRegistry::GetRegisteredClassNames();
		BOOL32 bPendingAddNotify = FALSE;
		INT32 PendingRemoveNotifyIndex = -1;

		for (INT32 NotifyIndex = 0; NotifyIndex < InSection.Notifies.Num(); NotifyIndex++)
		{
			const FAnimNotifyEvent& Notify = InSection.Notifies[NotifyIndex];
			ImGui::PushID(NotifyIndex);
			ImGui::Separator();
			ImGui::Text("Name: %s", *Notify.NotifyName);
			if (ImGui::BeginPopupContextItem("##notify_menu"))
			{
				if (ImGui::MenuItem("Add Notify"))
				{
					bPendingAddNotify = TRUE;
				}
				if (ImGui::MenuItem("Remove Notify"))
				{
					PendingRemoveNotifyIndex = NotifyIndex;
				}
				ImGui::EndPopup();
			}
			if (Notify.NotifyAssetName.Length() > 0u)
			{
				ImGui::TextDisabled("Asset: %s", *Notify.NotifyAssetName);
			}

			const CHAR* CurrentClassLabel = ((Notify.NotifyClassName.Length() > 0u) ? *Notify.NotifyClassName : "None");
				if (ImGui::BeginCombo("Class", CurrentClassLabel))
				{
					if (ImGui::Selectable("None", (Notify.NotifyClassName.Length() == 0u)))
					{
						if (UpdateMontageNotify(InSectionIndex, NotifyIndex, [](CJsonObject& InNotify)
							{
								InNotify.SetStringField(PE_TEXT("Class"), EString());
								InNotify.SetBoolField(PE_TEXT("State"), FALSE);
							}))
						{
							ImGui::EndCombo();
							ImGui::PopID();
							return TRUE;
						}
					}
					for (INT32 ClassIndex = 0; ClassIndex < RegisteredClassNames.Num(); ClassIndex++)
					{
						BOOL32 bClassIsState = FALSE;
						const BOOL32 bKnownClass = EAnimNotifyRegistry::IsStateClass(
							RegisteredClassNames[ClassIndex], bClassIsState);
						EString ClassLabel = RegisteredClassNames[ClassIndex];
						if (bKnownClass != FALSE)
						{
							ClassLabel += ((bClassIsState != FALSE) ? EString(" (State)") : EString(" (Instant)"));
						}
						const BOOL32 bSelected = (Notify.NotifyClassName == RegisteredClassNames[ClassIndex]);
						if (ImGui::Selectable(*ClassLabel, bSelected))
						{
							const EString NewClassName = RegisteredClassNames[ClassIndex];
							if (UpdateMontageNotify(InSectionIndex, NotifyIndex
								, [NewClassName, bKnownClass, bClassIsState](CJsonObject& InNotify)
								{
									InNotify.SetStringField(PE_TEXT("Class"), NewClassName);
									if (bKnownClass != FALSE)
									{
										// Class kind is authoritative; keep the serialized State flag in
										// sync instead of letting the old checkbox disagree with it.
										InNotify.SetBoolField(PE_TEXT("State"), bClassIsState);
										if (bClassIsState != FALSE)
										{
											FLOAT ExistingDuration = 0.0f;
											InNotify.GetFloatField(PE_TEXT("Duration"), ExistingDuration);
											InNotify.SetFloatField(PE_TEXT("Duration"), EMath::Max(ExistingDuration, 0.1f));
										}
									}
								}))
							{
								ImGui::EndCombo();
								ImGui::PopID();
								return TRUE;
							}
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
					if (UpdateMontageNotify(InSectionIndex, NotifyIndex, [NotifyTime](CJsonObject& InNotify)
						{
							InNotify.SetFloatField(PE_TEXT("Time"), NotifyTime);
						}))
					{
						ImGui::PopID();
						return TRUE;
					}
				}
			}

			// Class is the single source of truth for the event kind now. Keep a read-only
			// indicator so legacy assets with a mismatched State flag are visible but cannot
			// create another invalid combination.
			if (Notify.CachedNotify)
			{
				ImGui::Text("Kind: %s", (Notify.bIsState != FALSE) ? "State" : "Instant");
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f)
					, "Class '%s' is missing or does not match its State flag.", CurrentClassLabel);
				ImGui::TextDisabled("Select the class again to synchronize the event kind.");
			}
			if (Notify.bIsState)
			{
				const FLOAT MaxDuration = ((ClipDurationSeconds > 0.001f) ? ClipDurationSeconds : 1.0f);
				FLOAT StateDuration = EMath::Clamp(Notify.StateDuration, 0.001f, MaxDuration);
				if (ImGui::SliderFloat("State Duration", &StateDuration, 0.001f, MaxDuration, "%.3f", ImGuiSliderFlags_AlwaysClamp))
				{
					if (UpdateMontageNotify(InSectionIndex, NotifyIndex, [StateDuration](CJsonObject& InNotify)
						{
							InNotify.SetFloatField(PE_TEXT("Duration"), StateDuration);
						}))
					{
						ImGui::PopID();
						return TRUE;
					}
				}
			}
			if (Notify.PayloadJson.Length() > 0u)
			{
				ImGui::TextDisabled("Payload (read-only): %.120s", *Notify.PayloadJson);
			}

			if (ImGui::Button("Remove Notify"))
			{
				if (RemoveMontageNotify(InSectionIndex, NotifyIndex))
				{
					ImGui::PopID();
					return TRUE;
				}
			}
			ImGui::PopID();
		}

		if (bPendingAddNotify != FALSE)
		{
			if (AddMontageNotify(InSectionIndex))
			{
				return TRUE;
			}
		}
		if (PendingRemoveNotifyIndex >= 0)
		{
			if (RemoveMontageNotify(InSectionIndex, PendingRemoveNotifyIndex))
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	void EAnimMontageEditor::RefreshSkeletonAsset(const EAnimMontage& InMontage)
	{
		m_SkeletonAsset = nullptr;
		if (InMontage.SkeletonName.Length() <= 0u)
		{
			return;
		}
		const ESkeletonAsset* LoadedSkeleton = nullptr;
		if (ESkeletonAssetManager::GetManagerSingleton()->LoadSkeletonAsset(
			EBaseSettings::ENGINE_SKELETON_PATH, InMontage.SkeletonName, LoadedSkeleton))
		{
			m_SkeletonAsset = LoadedSkeleton;
		}
	}

	void EAnimMontageEditor::DrawEditableSections(const EAnimMontage& InMontage)
	{
		if (ImGui::Button("Add Section"))
		{
			if (AddMontageSection())
			{
				return;
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Slot names come from the skeleton; boneMask/notifies stay untouched.");

		const ESkeleton* Skeleton = (m_SkeletonAsset ? m_SkeletonAsset->GetStoragedResource() : nullptr);
		// Section context actions rebuild the live montage; defer them until the loop ends.
		BOOL32 bPendingAddSection = FALSE;
		INT32 PendingRemoveSectionIndex = -1;
		INT32 PendingAddNotifySectionIndex = -1;
		for (INT32 SectionIndex = 0; SectionIndex < InMontage.Sections.Num(); SectionIndex++)
		{
			const EAnimMontageSection& Section = InMontage.Sections[SectionIndex];
			ImGui::PushID(SectionIndex);
			ImGui::Separator();

			ImGui::Text("%s", *Section.Name);
			if (ImGui::BeginPopupContextItem("##section_menu"))
			{
				if (ImGui::MenuItem("Add Section"))
				{
					bPendingAddSection = TRUE;
				}
				if (ImGui::MenuItem("Remove Section", nullptr, false, (InMontage.Sections.Num() > 1)))
				{
					PendingRemoveSectionIndex = SectionIndex;
				}
				if (ImGui::MenuItem("Add Notify"))
				{
					PendingAddNotifySectionIndex = SectionIndex;
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Remove Section"))
			{
				if (RemoveMontageSection(SectionIndex))
				{
					ImGui::PopID();
					return;
				}
			}

			const CHAR* CurrentClipLabel = ((Section.ClipName.Length() > 0u) ? *Section.ClipName : "None");
			if (ImGui::BeginCombo("Clip", CurrentClipLabel))
			{
				const ESkeletonAnimation* Animation = (m_AnimationAsset ? m_AnimationAsset->GetStoragedResource() : nullptr);
				if (Animation)
				{
					for (INT32 ClipIndex = 0; ClipIndex < Animation->GetAnimationClipNum(); ClipIndex++)
					{
						const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
						if (!Clip)
						{
							continue;
						}
						const BOOL32 bSelected = (Section.ClipName == Clip->ClipName);
						if (ImGui::Selectable(*Clip->ClipName, bSelected))
						{
							const EString NewClipName = Clip->ClipName;
							if (UpdateMontageSection(SectionIndex, [NewClipName](CJsonObject& InSection)
								{
									InSection.SetStringField(PE_TEXT("Clip"), NewClipName);
								}))
							{
								ImGui::EndCombo();
								ImGui::PopID();
								return;
							}
						}
						if (bSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
				}
				ImGui::EndCombo();
			}

			const CHAR* CurrentSlotLabel = ((Section.SlotName.Length() > 0u) ? *Section.SlotName : "None");
			if (ImGui::BeginCombo("Slot", CurrentSlotLabel))
			{
				if (Skeleton)
				{
					const TArray<ESkeletonSlotGroup>& Groups = Skeleton->GetSlotGroups();
					for (INT32 GroupIndex = 0; GroupIndex < Groups.Num(); GroupIndex++)
					{
						const ESkeletonSlotGroup& Group = Groups[GroupIndex];
						for (INT32 SlotIndex = 0; SlotIndex < Group.SlotNames.Num(); SlotIndex++)
						{
							const EString& SlotName = Group.SlotNames[SlotIndex];
							const BOOL32 bSelected = (Section.SlotName == SlotName);
							if (ImGui::Selectable(*SlotName, bSelected))
							{
								const EString NewSlotName = SlotName;
								if (UpdateMontageSection(SectionIndex, [NewSlotName](CJsonObject& InSection)
									{
										InSection.SetStringField(PE_TEXT("Slot"), NewSlotName);
									}))
								{
									ImGui::EndCombo();
									ImGui::PopID();
									return;
								}
							}
							if (bSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
					}
				}
				else
				{
					ImGui::TextDisabled("Skeleton '%s' is not loaded.", *InMontage.SkeletonName);
				}
				ImGui::EndCombo();
			}

			const CHAR* CurrentNextLabel = ((Section.NextSectionName.Length() > 0u) ? *Section.NextSectionName : "(end)");
			if (ImGui::BeginCombo("Next", CurrentNextLabel))
			{
				if (ImGui::Selectable("(end)", (Section.NextSectionName.Length() == 0u)))
				{
					if (UpdateMontageSection(SectionIndex, [](CJsonObject& InSection)
						{
							InSection.SetStringField(PE_TEXT("Next"), EString());
						}))
					{
						ImGui::EndCombo();
						ImGui::PopID();
						return;
					}
				}
				for (INT32 NextIndex = 0; NextIndex < InMontage.Sections.Num(); NextIndex++)
				{
					if (NextIndex == SectionIndex)
					{
						continue;
					}
					const EAnimMontageSection& NextSection = InMontage.Sections[NextIndex];
					const BOOL32 bSelected = (Section.NextSectionName == NextSection.Name);
					if (ImGui::Selectable(*NextSection.Name, bSelected))
					{
						const EString NewNextName = NextSection.Name;
						if (UpdateMontageSection(SectionIndex, [NewNextName](CJsonObject& InSection)
							{
								InSection.SetStringField(PE_TEXT("Next"), NewNextName);
							}))
						{
							ImGui::EndCombo();
							ImGui::PopID();
							return;
						}
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			const EAnimationLoopMode LoopModes[4] = {
				EAnimationLoopMode::ANIMATION_LOOP_ONCE,
				EAnimationLoopMode::ANIMATION_LOOP_REPEAT,
				EAnimationLoopMode::ANIMATION_LOOP_PINGPONG,
				EAnimationLoopMode::ANIMATION_LOOP_CLAMP
			};
			if (ImGui::BeginCombo("Loop", GetLoopModeLabel(Section.LoopMode)))
			{
				for (UINT32 ModeIndex = 0u; ModeIndex < 4u; ModeIndex++)
				{
					const BOOL32 bSelected = (Section.LoopMode == LoopModes[ModeIndex]);
					if (ImGui::Selectable(GetLoopModeLabel(LoopModes[ModeIndex]), bSelected))
					{
						const EString NewLoopText(GetLoopModeLabel(LoopModes[ModeIndex]));
						if (UpdateMontageSection(SectionIndex, [NewLoopText](CJsonObject& InSection)
							{
								InSection.SetStringField(PE_TEXT("Loop"), NewLoopText);
							}))
						{
							ImGui::EndCombo();
							ImGui::PopID();
							return;
						}
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			FLOAT PlayRate = Section.PlayRate;
			if (ImGui::SliderFloat("Rate", &PlayRate, 0.01f, 4.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
			{
				if (UpdateMontageSection(SectionIndex, [PlayRate](CJsonObject& InSection)
					{
						InSection.SetFloatField(PE_TEXT("Rate"), PlayRate);
					}))
				{
					ImGui::PopID();
					return;
				}
			}
			FLOAT BlendIn = Section.BlendInSeconds;
			if (ImGui::SliderFloat("Blend In", &BlendIn, 0.0f, 2.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
			{
				if (UpdateMontageSection(SectionIndex, [BlendIn](CJsonObject& InSection)
					{
						InSection.SetFloatField(PE_TEXT("BlendIn"), BlendIn);
					}))
				{
					ImGui::PopID();
					return;
				}
			}
			FLOAT BlendOut = Section.BlendOutSeconds;
			if (ImGui::SliderFloat("Blend Out", &BlendOut, 0.0f, 2.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
			{
				if (UpdateMontageSection(SectionIndex, [BlendOut](CJsonObject& InSection)
					{
						InSection.SetFloatField(PE_TEXT("BlendOut"), BlendOut);
					}))
				{
					ImGui::PopID();
					return;
				}
			}
			bool bAdditive = (Section.bAdditive != FALSE);
			if (ImGui::Checkbox("Additive", &bAdditive))
			{
				if (UpdateMontageSection(SectionIndex, [bAdditive](CJsonObject& InSection)
					{
						InSection.SetBoolField(PE_TEXT("Additive"), (bAdditive ? TRUE : FALSE));
					}))
				{
					ImGui::PopID();
					return;
				}
			}

			if (ImGui::TreeNode("Bone Mask"))
			{
				ImGui::TextDisabled("Empty list = full body.");
				const EString MaskName = ((Section.BoneMask.Name.Length() > 0u)
					? Section.BoneMask.Name : EString(PE_TEXT("Mask")));
				FLOAT MaskWeight = Section.BoneMask.Weight;
				if (ImGui::SliderFloat("Weight", &MaskWeight, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				{
					if (UpdateBoneMask(SectionIndex, Section.BoneMask.BoneNames, MaskWeight, MaskName))
					{
						ImGui::TreePop();
						ImGui::PopID();
						return;
					}
				}

				if (Skeleton)
				{
					ImGui::BeginChild("BoneMaskList", ImVec2(0.0f, 150.0f), true);
					const TArray<EBoneData>& Bones = Skeleton->GetBonePart();
					for (INT32 BoneIndex = 0; BoneIndex < Bones.Num(); BoneIndex++)
					{
						const EBoneData& Bone = Bones[BoneIndex];
						bool bChecked = (Section.BoneMask.BoneNames.Contains(Bone.Name) != FALSE);
						if (ImGui::Checkbox(*Bone.Name, &bChecked))
						{
							TArray<EString> NewBones;
							for (INT32 ExistingIndex = 0; ExistingIndex < Section.BoneMask.BoneNames.Num(); ExistingIndex++)
							{
								const EString& ExistingName = Section.BoneMask.BoneNames[ExistingIndex];
								if (ExistingName != Bone.Name)
								{
									NewBones.Add(ExistingName);
								}
							}
							if (bChecked)
							{
								NewBones.Add(Bone.Name);
							}
							if (UpdateBoneMask(SectionIndex, NewBones, Section.BoneMask.Weight, MaskName))
							{
								ImGui::EndChild();
								ImGui::TreePop();
								ImGui::PopID();
								return;
							}
						}
					}
					ImGui::EndChild();
				}
				else
				{
					ImGui::TextDisabled("Skeleton is not loaded.");
				}
				ImGui::TreePop();
			}

			if (DrawSectionNotifies(Section, SectionIndex))
			{
				ImGui::PopID();
				return;
			}
			ImGui::PopID();
		}

		if (bPendingAddSection != FALSE)
		{
			AddMontageSection();
			return;
		}
		if (PendingRemoveSectionIndex >= 0)
		{
			RemoveMontageSection(PendingRemoveSectionIndex);
			return;
		}
		if (PendingAddNotifySectionIndex >= 0)
		{
			AddMontageNotify(PendingAddNotifySectionIndex);
			return;
		}
	}

}

#endif
