#include "AnimationMontageEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

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
	}

	void EAnimMontageEditor::EditorInit()
	{
		m_ShowEditor = FALSE;
		m_Asset = nullptr;
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
