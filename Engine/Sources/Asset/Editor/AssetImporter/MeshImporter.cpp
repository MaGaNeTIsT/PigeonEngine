#include "MeshImporter.h"
#include "MeshAsset/MeshAsset.h"
#if _EDITOR_ONLY
void PigeonEngine::EMeshImporter::CreateImportEditor(TArray<EString> Paths)
{
	NeedUpdate = TRUE;
	InitializeEditor = TRUE;
	m_Paths.Clear();
	for (UINT i = 0; i < Paths.Length(); i++)
	{
		m_Paths.Add(Paths[i].Replace("\\", "/"));
	}
}

void PigeonEngine::EMeshImporter::UpdateImportEditor()
{
	//Combo Item
	static const CHAR* type_items[] = { "Static Mesh", "Skeletal Mesh" };
	
	//Initialize Data
	static INT32 type_item_current_idx = 0;
	if (InitializeEditor)
	{
		type_item_current_idx = 0;
		InitializeEditor = FALSE;
	}

	ImGui::Begin("Mesh Importer", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
	{
		const CHAR* combo_preview_value = type_items[type_item_current_idx];
		ImGui::PushItemWidth(120);
		if (ImGui::BeginCombo("Import Type", combo_preview_value))
		{
			for (INT32 n = 0; n < IM_ARRAYSIZE(type_items); n++)
			{
				const BOOL8 is_selected = (type_item_current_idx == n);
				if (ImGui::Selectable(type_items[n], is_selected))
					type_item_current_idx = n;

				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (type_item_current_idx == 0)//Texture2D
		{
			for (UINT32 i = 0; i < m_Paths.Length(); i++)
			{
				ImGui::Text("File:");
				ImGui::SameLine();
				{
					EString FileName = EPath::GetFileNameWithExtension(m_Paths[i]);
					ImGui::Text(*FileName);
				}
			}
		}
	}
	ImGui::End();
}
#endif