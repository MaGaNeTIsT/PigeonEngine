#include "AnimationMontageEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

#include <AnimationAsset/AnimationAsset.h>
#include <AnimationMontageAsset/AnimationMontageAsset.h>
#include <Base/DataStructure/Text/Path.h>

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
	}

	void EAnimMontageEditor::EditorInit()
	{
		m_ShowEditor = FALSE;
		m_Asset = nullptr;
		m_AnimationAsset = nullptr;
		m_OpenedAssetFullPath = EString();
		m_LastError = EString();
		m_JsonBuffer.Empty();
	}

	void EAnimMontageEditor::OpenMontageAsset(const EString& InAssetFullPath)
	{
		m_ShowEditor = TRUE;
		m_Asset = nullptr;
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
		LoadJsonBufferFromAsset();
	}

	void EAnimMontageEditor::EditorUpdate()
	{
		if (!m_ShowEditor)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(920.0f, 760.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(440.0f, 36.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Montage Editor", &m_ShowEditor, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (m_OpenedAssetFullPath.Length() > 0u))
			{
				OpenMontageAsset(m_OpenedAssetFullPath);
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
					}
					else
					{
						m_LastError = Error;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Save"))
			{
				if (m_Asset)
				{
					EString Error;
					const EString JsonText(m_JsonBuffer.GetData());
					if (EAnimMontageManager::GetManagerSingleton()->SaveMontageAsset(m_Asset, JsonText, Error))
					{
						m_LastError = EString();
					}
					else
					{
						m_LastError = Error;
					}
				}
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No montage selected."));

			if (m_LastError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastError);
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
					if (ImGui::TreeNode("Sections"))
					{
						for (INT32 SectionIndex = 0; SectionIndex < Montage->Sections.Num(); SectionIndex++)
						{
							const EAnimMontageSection& Section = Montage->Sections[SectionIndex];
							const EString NextName = ((Section.NextSectionName.Length() > 0u)
								? Section.NextSectionName : EString("(end)"));
							ImGui::BulletText("%s -> %s | clip: %s | slot: %s | %s | notifies: %d"
								, *Section.Name, *NextName, *Section.ClipName, *Section.SlotName
								, GetLoopModeLabel(Section.LoopMode), Section.Notifies.Num());
						}
						ImGui::TreePop();
					}

					ImGui::Separator();
					ImGui::BeginChild("MontageTimeline", ImVec2(0.0f, 210.0f), true);
					DrawTimeline(*Montage);
					ImGui::EndChild();
				}
			}

			if (m_JsonBuffer.Num() > 0)
			{
				ImGui::InputTextMultiline("JSON", m_JsonBuffer.GetData(), static_cast<int>(m_JsonBuffer.Num())
					, ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput);
			}
		}
		ImGui::End();
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

}

#endif
