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
		OpenSkeletonAssetNow(InAssetFullPath);
	}

	void ESkeletonSlotGroupEditor::OpenSkeletonAssetNow(const EString& InAssetFullPath)
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
		CaptureBaseline();
		m_Document.ClearPendingAction();
		m_Document.bDirty = FALSE;
		m_StatusMessage = EString("Loaded.");
		m_bStatusSucceeded = TRUE;
	}

	void ESkeletonSlotGroupEditor::CaptureBaseline()
	{
		m_BaselineSlotGroups.Empty();
		const ESkeleton* Skeleton = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
		if (Skeleton)
		{
			m_BaselineSlotGroups = Skeleton->GetSlotGroups();
		}
	}

	void ESkeletonSlotGroupEditor::DiscardChanges()
	{
		ESkeleton* Skeleton = (m_Asset ? const_cast<ESkeleton*>(m_Asset->GetStoragedResource()) : nullptr);
		if (Skeleton)
		{
			Skeleton->GetSlotGroups() = m_BaselineSlotGroups;
		}
		m_Document.bDirty = FALSE;
		m_StatusMessage = EString("Discarded unsaved changes.");
		m_bStatusSucceeded = TRUE;
	}

	BOOL32 ESkeletonSlotGroupEditor::SaveSkeletonAsset(EString& OutError)
	{
		OutError = EString();
		if (!m_Asset)
		{
			OutError = EString("No skeleton asset is open.");
			return FALSE;
		}
		if (!ValidateSlotGroups(OutError))
		{
			return FALSE;
		}
		if (!(ESkeletonAssetManager::GetManagerSingleton()->SaveSkeletonAsset(
			m_Asset->GetAssetPath(), m_Asset->GetAssetName(), m_Asset, OutError)))
		{
			return FALSE;
		}
		CaptureBaseline();
		m_Document.bDirty = FALSE;
		return TRUE;
	}

	BOOL32 ESkeletonSlotGroupEditor::ReloadSkeletonAssetNow(EString& OutError)
	{
		OutError = EString();
		if (!m_Asset)
		{
			OutError = EString("No skeleton asset is open.");
			return FALSE;
		}

		const ESkeletonAsset* FreshAsset = nullptr;
		if (!(ESkeletonAssetManager::GetManagerSingleton()->ReloadSkeletonAsset(
			m_Asset->GetAssetPath(), m_Asset->GetAssetName(), FreshAsset, OutError)) || (!FreshAsset))
		{
			return FALSE;
		}

		m_Asset = FreshAsset;
		CaptureBaseline();
		m_Document.ClearPendingAction();
		m_Document.bDirty = FALSE;
		return TRUE;
	}

	void ESkeletonSlotGroupEditor::HandleDocumentAction()
	{
		const EEditorUnsavedChoiceType Choice = DrawEditorUnsavedChangesPopup("##slot_group_editor_unsaved"
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
			if (!SaveSkeletonAsset(SaveError))
			{
				m_LastError = SaveError;
				m_StatusMessage = SaveError;
				m_bStatusSucceeded = FALSE;
				return;
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
			if (!ReloadSkeletonAssetNow(ReloadError))
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
			OpenSkeletonAssetNow(PendingPath);
		}
	}

	void ESkeletonSlotGroupEditor::EditorUpdate()
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

		ImGui::SetNextWindowSize(ImVec2(720.0f, 640.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(500.0f, 50.0f), ImGuiCond_FirstUseEver);
		bool bWindowOpen = true;
		ImGui::Begin("Skeleton Slot Groups", &bWindowOpen, ImGuiWindowFlags_None);
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
					if (!ReloadSkeletonAssetNow(ReloadError))
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
			if (ImGui::Button("Save"))
			{
				if (m_Asset)
				{
					EString Error;
					if (SaveSkeletonAsset(Error))
					{
						m_LastError = EString();
						m_StatusMessage = EString("Saved.");
						m_bStatusSucceeded = TRUE;
						PE_LOG_LOG(EString("Saved skeleton slot groups: ") + m_OpenedAssetFullPath);
					}
					else
					{
						m_LastError = (Error.Length() > 0u)
							? Error
							: (EString("Failed to save skeleton: ") + m_OpenedAssetFullPath);
						m_StatusMessage = m_LastError;
						m_bStatusSucceeded = FALSE;
					}
				}
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No skeleton selected."));
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
			if (!m_Asset)
			{
				ImGui::End();
				if (!bWindowOpen)
				{
					m_ShowEditor = FALSE;
				}
				return;
			}
			ESkeleton* Skeleton = const_cast<ESkeleton*>(m_Asset->GetStoragedResource());
			if (!Skeleton)
			{
				ImGui::TextDisabled("Skeleton resource is null.");
				ImGui::End();
				if (!bWindowOpen)
				{
					m_ShowEditor = FALSE;
				}
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
				m_Document.bDirty = TRUE;
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset Defaults"))
			{
				AddDefaultSlotGroups(Groups);
				m_Document.bDirty = TRUE;
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
					m_Document.bDirty = TRUE;
				}
				ImGui::SameLine();
				ImGui::TextDisabled("priority %d", GroupIndex);

				if (ImGui::Button("Up") && (GroupIndex > 0))
				{
					const ESkeletonSlotGroup Temp = Groups[GroupIndex - 1];
					Groups[GroupIndex - 1] = Groups[GroupIndex];
					Groups[GroupIndex] = Temp;
					m_Document.bDirty = TRUE;
					ImGui::PopID();
					break;
				}
				ImGui::SameLine();
				if (ImGui::Button("Down") && (GroupIndex + 1 < Groups.Num()))
				{
					const ESkeletonSlotGroup Temp = Groups[GroupIndex + 1];
					Groups[GroupIndex + 1] = Groups[GroupIndex];
					Groups[GroupIndex] = Temp;
					m_Document.bDirty = TRUE;
					ImGui::PopID();
					break;
				}
				ImGui::SameLine();
				if (ImGui::Button("Remove Group"))
				{
					Groups.RemoveAt(GroupIndex);
					m_Document.bDirty = TRUE;
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
						m_Document.bDirty = TRUE;
					}
					ImGui::SameLine();
					if (ImGui::Button("Remove"))
					{
						Group.SlotNames.RemoveAt(SlotIndex);
						m_Document.bDirty = TRUE;
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
					m_Document.bDirty = TRUE;
				}
				ImGui::Separator();
				ImGui::PopID();
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
