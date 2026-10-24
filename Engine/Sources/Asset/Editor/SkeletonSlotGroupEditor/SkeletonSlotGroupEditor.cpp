#include "SkeletonSlotGroupEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

#include <Base/DataStructure/Text/Path.h>
#include <Editor/EditorLogManager.h>
#include <SkeletonAsset/SkeletonAsset.h>

namespace PigeonEngine
{

	namespace
	{
		BOOL32 HasGroupName(const TArray<ESkeletonSlotGroup>& InGroups, const EString& InName)
		{
			for (INT32 Index = 0; Index < InGroups.Num(); Index++)
			{
				if (InGroups[Index].Name == InName)
				{
					return TRUE;
				}
			}
			return FALSE;
		}

		BOOL32 HasSlotName(const ESkeletonSlotGroup& InGroup, const EString& InName)
		{
			for (INT32 Index = 0; Index < InGroup.SlotNames.Num(); Index++)
			{
				if (InGroup.SlotNames[Index] == InName)
				{
					return TRUE;
				}
			}
			return FALSE;
		}

		void AddDefaultSlotGroups(TArray<ESkeletonSlotGroup>& InOutGroups)
		{
			InOutGroups.Empty();
			const CHAR* DefaultGroupNames[] = { "Default", "UpperBody", "FullBody" };
			for (const CHAR* GroupName : DefaultGroupNames)
			{
				ESkeletonSlotGroup& Group = InOutGroups.AddDefaultGetRef();
				Group.Name = EString(GroupName);
				Group.SlotNames.Add(EString(GroupName));
			}
		}
	}

	void ESkeletonSlotGroupEditor::EditorInit()
	{
		m_ShowEditor = FALSE;
		m_Asset = nullptr;
		m_OpenedAssetFullPath = EString();
		m_LastError = EString();
	}

	void ESkeletonSlotGroupEditor::OpenSkeletonAsset(const EString& InAssetFullPath)
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
			m_LastError = EString("Skeleton editor cannot parse asset path: ") + NormalizedAssetPath;
			return;
		}

		const ESkeletonAsset* LoadedAsset = nullptr;
		if (!(ESkeletonAssetManager::GetManagerSingleton()->LoadSkeletonAsset(AssetDirectory, AssetName, LoadedAsset))
			|| (!LoadedAsset))
		{
			m_LastError = EString("Skeleton editor failed to load asset: ") + NormalizedAssetPath;
			return;
		}

		m_Asset = LoadedAsset;
		m_OpenedAssetFullPath = NormalizedAssetPath;
	}

	void ESkeletonSlotGroupEditor::EditorUpdate()
	{
		if (!m_ShowEditor)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(720.0f, 640.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(500.0f, 50.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Skeleton Slot Groups", &m_ShowEditor, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (m_OpenedAssetFullPath.Length() > 0u))
			{
				OpenSkeletonAsset(m_OpenedAssetFullPath);
			}
			ImGui::SameLine();
			if (ImGui::Button("Save"))
			{
				if (m_Asset)
				{
					EString Error;
					if (!ValidateSlotGroups(Error))
					{
						m_LastError = Error;
					}
					else if (!ESkeletonAssetManager::GetManagerSingleton()->SaveSkeletonAsset(
						m_Asset->GetAssetPath(), m_Asset->GetAssetName(), m_Asset))
					{
						m_LastError = EString("Failed to save skeleton: ") + m_OpenedAssetFullPath;
					}
					else
					{
						m_LastError = EString();
						PE_LOG_LOG(EString("Saved skeleton slot groups: ") + m_OpenedAssetFullPath);
					}
				}
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No skeleton selected."));

			if (m_LastError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastError);
			}

			ImGui::Separator();
			if (!m_Asset)
			{
				ImGui::End();
				return;
			}
			ESkeleton* Skeleton = const_cast<ESkeleton*>(m_Asset->GetStoragedResource());
			if (!Skeleton)
			{
				ImGui::TextDisabled("Skeleton resource is null.");
				ImGui::End();
				return;
			}
			TArray<ESkeletonSlotGroup>& Groups = Skeleton->GetSlotGroups();

			ImGui::Text("Skeleton: %s  Bones: %d  Slot groups: %d"
				, *Skeleton->GetSkeletonName(), Skeleton->GetBoneCount(), Groups.Num());
			ImGui::TextDisabled("Group order is the blend order: top is the base, bottom is the highest overlay.");

			if (ImGui::Button("Add Group"))
			{
				INT32 Suffix = Groups.Num();
				EString NewName;
				do
				{
					NewName = EString("Group") + ToString(Suffix++);
				} while (HasGroupName(Groups, NewName));
				ESkeletonSlotGroup& NewGroup = Groups.AddDefaultGetRef();
				NewGroup.Name = NewName;
				NewGroup.SlotNames.Add(EString("Slot") + ToString(0));
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset Defaults"))
			{
				AddDefaultSlotGroups(Groups);
			}
			ImGui::Separator();

			for (INT32 GroupIndex = 0; GroupIndex < Groups.Num(); GroupIndex++)
			{
				ESkeletonSlotGroup& Group = Groups[GroupIndex];
				ImGui::PushID(GroupIndex);

				CHAR GroupNameBuffer[EEngineSettings::ENGINE_ANIMATION_SLOT_GROUP_NAME_LENGTH_MAX] = "";
				::strncpy_s(GroupNameBuffer, sizeof(GroupNameBuffer), *Group.Name, _TRUNCATE);
				if (ImGui::InputText("Group", GroupNameBuffer, sizeof(GroupNameBuffer)))
				{
					Group.Name = EString(GroupNameBuffer);
				}
				ImGui::SameLine();
				ImGui::TextDisabled("priority %d", GroupIndex);

				if (ImGui::Button("Up") && (GroupIndex > 0))
				{
					const ESkeletonSlotGroup Temp = Groups[GroupIndex - 1];
					Groups[GroupIndex - 1] = Groups[GroupIndex];
					Groups[GroupIndex] = Temp;
					ImGui::PopID();
					break;
				}
				ImGui::SameLine();
				if (ImGui::Button("Down") && (GroupIndex + 1 < Groups.Num()))
				{
					const ESkeletonSlotGroup Temp = Groups[GroupIndex + 1];
					Groups[GroupIndex + 1] = Groups[GroupIndex];
					Groups[GroupIndex] = Temp;
					ImGui::PopID();
					break;
				}
				ImGui::SameLine();
				if (ImGui::Button("Remove Group"))
				{
					Groups.RemoveAt(GroupIndex);
					ImGui::PopID();
					break;
				}

				BOOL32 bRemovedSlot = FALSE;
				for (INT32 SlotIndex = 0; SlotIndex < Group.SlotNames.Num(); SlotIndex++)
				{
					ImGui::PushID(SlotIndex);
					CHAR SlotNameBuffer[EEngineSettings::ENGINE_ANIMATION_SLOT_NAME_LENGTH_MAX] = "";
					::strncpy_s(SlotNameBuffer, sizeof(SlotNameBuffer), *Group.SlotNames[SlotIndex], _TRUNCATE);
					if (ImGui::InputText("Slot", SlotNameBuffer, sizeof(SlotNameBuffer)))
					{
						Group.SlotNames[SlotIndex] = EString(SlotNameBuffer);
					}
					ImGui::SameLine();
					if (ImGui::Button("Remove"))
					{
						Group.SlotNames.RemoveAt(SlotIndex);
						ImGui::PopID();
						bRemovedSlot = TRUE;
						break;
					}
					ImGui::PopID();
				}
				if (bRemovedSlot)
				{
					ImGui::PopID();
					break;
				}

				if (ImGui::Button("Add Slot"))
				{
					INT32 Suffix = Group.SlotNames.Num();
					EString NewSlotName;
					do
					{
						NewSlotName = EString("Slot") + ToString(Suffix++);
					} while (HasSlotName(Group, NewSlotName));
					Group.SlotNames.Add(NewSlotName);
				}
				ImGui::Separator();
				ImGui::PopID();
			}
		}
		ImGui::End();
	}

	BOOL32 ESkeletonSlotGroupEditor::ValidateSlotGroups(EString& OutError)const
	{
		OutError = EString();
		if (!m_Asset)
		{
			OutError = EString("No skeleton asset.");
			return FALSE;
		}
		const ESkeleton* Skeleton = m_Asset->GetStoragedResource();
		if (!Skeleton)
		{
			OutError = EString("Skeleton resource is null.");
			return FALSE;
		}
		const TArray<ESkeletonSlotGroup>& Groups = Skeleton->GetSlotGroups();
		if (Groups.Num() <= 0)
		{
			OutError = EString("A skeleton needs at least one slot group.");
			return FALSE;
		}

		TMap<EString, INT32> GroupNames;
		TMap<EString, INT32> SlotNames;
		for (INT32 GroupIndex = 0; GroupIndex < Groups.Num(); GroupIndex++)
		{
			const ESkeletonSlotGroup& Group = Groups[GroupIndex];
			if (Group.Name.Length() <= 0u)
			{
				OutError = EString("Slot group ") + ToString(GroupIndex) + EString(" has an empty name.");
				return FALSE;
			}
			if (GroupNames.ContainsKey(Group.Name))
			{
				OutError = EString("Duplicate slot group name: ") + Group.Name;
				return FALSE;
			}
			GroupNames.Add(Group.Name, GroupIndex);
			if (Group.SlotNames.Num() <= 0)
			{
				OutError = EString("Slot group '") + Group.Name + EString("' needs at least one slot.");
				return FALSE;
			}
			for (INT32 SlotIndex = 0; SlotIndex < Group.SlotNames.Num(); SlotIndex++)
			{
				const EString& SlotName = Group.SlotNames[SlotIndex];
				if (SlotName.Length() <= 0u)
				{
					OutError = EString("Slot group '") + Group.Name + EString("' has an empty slot name.");
					return FALSE;
				}
				if (SlotNames.ContainsKey(SlotName))
				{
					OutError = EString("Slot name is used by more than one group: ") + SlotName;
					return FALSE;
				}
				SlotNames.Add(SlotName, SlotIndex);
			}
		}
		return TRUE;
	}

}

#endif
