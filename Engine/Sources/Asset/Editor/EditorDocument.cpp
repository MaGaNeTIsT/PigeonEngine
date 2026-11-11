#include "EditorDocument.h"

#if _EDITOR_ONLY

#include <imgui.h>

namespace PigeonEngine
{

	EEditorUnsavedChoiceType DrawEditorUnsavedChangesPopup(const CHAR* InPopupId, const CHAR* InAssetName
		, FEditorDocumentState& InOutState)
	{
		if (InOutState.HasPendingAction() == FALSE)
		{
			return EEditorUnsavedChoiceType::NONE;
		}
		if (!ImGui::IsPopupOpen(InPopupId))
		{
			ImGui::OpenPopup(InPopupId);
		}

		EEditorUnsavedChoiceType Choice = EEditorUnsavedChoiceType::NONE;
		if (ImGui::BeginPopupModal(InPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Save changes to %s?", (InAssetName ? InAssetName : ""));
			ImGui::TextDisabled("Discard restores the state captured when the asset was opened or last saved.");
			if (ImGui::Button("Save"))
			{
				Choice = EEditorUnsavedChoiceType::SAVE;
			}
			ImGui::SameLine();
			if (ImGui::Button("Discard"))
			{
				Choice = EEditorUnsavedChoiceType::DISCARD;
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				Choice = EEditorUnsavedChoiceType::CANCEL;
			}
			if (Choice != EEditorUnsavedChoiceType::NONE)
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		return Choice;
	}

}

#endif
