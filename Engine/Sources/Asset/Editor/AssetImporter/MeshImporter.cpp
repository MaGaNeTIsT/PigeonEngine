#include "MeshImporter.h"
#include "MeshAsset/MeshAsset.h"
#if _EDITOR_ONLY
void PigeonEngine::EMeshImporter::CreateImportEditor(TArray<EString> Paths)
{
	NeedUpdate = TRUE;
	InitializeEditor = TRUE;
	m_Paths.Clear();
	m_MeshCombineCheck.Clear();
	for (UINT i = 0; i < Paths.Length(); i++)
	{
		m_Paths.Add(Paths[i].Replace("\\", "/"));
		m_MeshCombineCheck.Add(FALSE);
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
		//if (type_item_current_idx == 0)//Static Mesh
		//{
			for (UINT32 i = 0; i < m_Paths.Length(); i++)
			{
				ImGui::Text("FileName:");
				ImGui::SameLine();
				{
					EString FileName = EPath::GetFileNameWithExtension(m_Paths[i]);
					ImGui::Text(*FileName);
				}
				ImGui::SameLine();
				{
					BOOL8 MeshCombineCheck = m_MeshCombineCheck[i];
					UINT32 ToChar = i + 49u;
					EString CombineType = EString("IsCombineSubmeshes ") + EString((CHAR*)&(ToChar));
					ImGui::Checkbox(*CombineType, &MeshCombineCheck);
					m_MeshCombineCheck[i] = MeshCombineCheck;
				}
			}
		//}
		//else if (type_item_current_idx == 1)//Skeletal Mesh
		//{

		//}
		if (ImGui::Button("Import"))
		{
			EString FilePath;
			EString FileName;
			EString FileExtension;
			if (type_item_current_idx == 0)//Static Mesh
			{
				for (UINT32 i = 0; i < m_Paths.Length(); i++)
				{
					FilePath = EPath::GetFileFolderPath(m_Paths[i]);
					FileName = EPath::GetFileNameWithoutExtension(m_Paths[i]);
					FileExtension = EPath::GetExtension(m_Paths[i]);
					const EStaticMeshAsset* Asset = NULL;
					TryLoadStaticMesh(EBaseSettings::ENGINE_MESH_PATH, FileName, Asset, &FilePath, &FileName, &FileExtension, m_MeshCombineCheck[i]);
				}
			}
			else if (type_item_current_idx == 1)//Skeletal Mesh
			{
				for (UINT32 i = 0; i < m_Paths.Length(); i++)
				{
					FilePath = EPath::GetFileFolderPath(m_Paths[i]);
					FileName = EPath::GetFileNameWithoutExtension(m_Paths[i]);
					FileExtension = EPath::GetExtension(m_Paths[i]);
					const ESkinnedMeshAsset* Asset = NULL;
					TryLoadSkinnedMesh(EBaseSettings::ENGINE_MESH_PATH, FileName, Asset, &FilePath, &FileName, &FileExtension, m_MeshCombineCheck[i]);
				}
			}
			NeedUpdate = FALSE;
		}
	}
	ImGui::End();
}
#endif