#include "PNGImporter.h"
#include "TextureAsset/TextureAsset.h"
#include "Base/DataStructure/Text/Path.h"
#if _EDITOR_ONLY
void PigeonEngine::EPNGImporter::CreateImportEditor(TArray<EString> Paths)
{
	NeedUpdate = TRUE;
	InitializeEditor = TRUE;
	m_Paths = Paths;
}

void PigeonEngine::EPNGImporter::UpdateImportEditor()
{
	//Combo Item
	static const CHAR* items[] = { "Texture2D", "TextureCube" };
	static const CHAR* cube_items[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };

	//Initialize Data
	static INT32 item_current_idx = 0;
	static INT32 cube_item_current_idx = 0;
	static TMap<const CHAR*, EString> CubeMap;
	static CHAR InputData[PigeonEngine::EPath::MAX_PATH_LENGTH] = "";
	if (InitializeEditor)
	{
		item_current_idx = 0;
		cube_item_current_idx = 0;
		::memset(InputData, 0, EPath::MAX_PATH_LENGTH);
		CubeMap.Clear();
		//TODO : does this necessary?
		//const CHAR* cube_items[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
		//for (UINT32 i = 0; i < m_Paths.Length(); i++)
		//{
		//	CubeMap.Add(cube_items[i % 6], m_Paths[i]);
		//}
		InitializeEditor = FALSE;
	}

	ImGui::Begin("PNGImporter", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
	{
		const CHAR* combo_preview_value = items[item_current_idx];
		ImGui::PushItemWidth(120);
		if (ImGui::BeginCombo("Import Type", combo_preview_value))
		{
			for (INT32 n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				const BOOL8 is_selected = (item_current_idx == n);
				if (ImGui::Selectable(items[n], is_selected))
					item_current_idx = n;

				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (item_current_idx == 0)//Texture2D
		{
			for (UINT32 i = 0; i < m_Paths.Length(); i++)
			{
				ImGui::Text("FilePath:");
				ImGui::SameLine();
				{
					EString FileName = EPath::GetFileNameWithExtension(m_Paths[i]);
					ImGui::Text(*FileName);
				}
			}
		}
		else if (item_current_idx == 1)//TextureCube
		{
			for (UINT32 i = 0; i < m_Paths.Length(); i++)
			{
				const CHAR* cube_combo_preview_value = cube_items[cube_item_current_idx];
				ImGui::PushItemWidth(50);
				if (ImGui::BeginCombo("Cube Type", cube_combo_preview_value))
				{
					for (INT32 n = 0; n < IM_ARRAYSIZE(cube_items); n++)
					{
						const BOOL8 is_selected = (cube_item_current_idx == n);
						if (ImGui::Selectable(cube_items[n], is_selected))
						{
							cube_item_current_idx = n;
							CubeMap.Add(cube_items[cube_item_current_idx], m_Paths[i]);
						}

						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				ImGui::SameLine();
				{
					ImGui::Text(":");
				}
				ImGui::SameLine();
				{
					EString FileName = EPath::GetFileNameWithExtension(m_Paths[i]);
					ImGui::Text(*FileName);
				}
			}
		}
		ImGui::Checkbox("CopyFile", &CheckCopyFile);
		if (CheckCopyFile)
		{
			ImGui::SameLine();
			{
				ImGui::PushItemWidth(250);
				ImGui::InputText(";", InputData, EPath::MAX_PATH_LENGTH, ImGuiInputTextFlags_CharsNoBlank);
			}
		}
		if (ImGui::Button("Import"))
		{
			//Handle Path.
			if (item_current_idx == 0)//Texture2D
			{
				for (UINT32 i = 0; i < m_Paths.Length(); i++)
				{
					const ETexture2DAsset* Asset = NULL;
					EString FileName = EPath::GetFileNameWithoutExtension(m_Paths[i]);
					TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, FileName, Asset, &EPath::GetRootPath(m_Paths[i]), &FileName, &m_Extension);
				}
			}
			else if (item_current_idx == 1)//TextureCube
			{
				const ETextureCubeAsset* Asset = NULL;
				TArray<EString> Extensions;
				Extensions.Add(m_Extension);
				TArray<EString> FileNames;
				for (UINT32 i = 0; i < m_Paths.Length(); i++)
				{
					FileNames.Add(EPath::GetFileNameWithoutExtension(m_Paths[i]));
				}
				TArray<EString> Paths;
				EString CubeMapPath;
				if (CubeMap.FindValue("+X", CubeMapPath)) Paths.Add(CubeMapPath);
				if (CubeMap.FindValue("-X", CubeMapPath)) Paths.Add(CubeMapPath);
				if (CubeMap.FindValue("+Y", CubeMapPath)) Paths.Add(CubeMapPath);
				if (CubeMap.FindValue("-Y", CubeMapPath)) Paths.Add(CubeMapPath);
				if (CubeMap.FindValue("+Z", CubeMapPath)) Paths.Add(CubeMapPath);
				if (CubeMap.FindValue("-Z", CubeMapPath)) Paths.Add(CubeMapPath);
				TryLoadTextureCube(EBaseSettings::ENGINE_TEXTURE_PATH, FileNames[0], Asset, &Paths, &FileNames, &Extensions);
			}

			if (CheckCopyFile)
			{
				//CopyFile
				for (UINT32 i = 0; i < m_Paths.Length(); i++)
				{
					
				}
			}
			NeedUpdate = FALSE;
		}
	}
	ImGui::End();
}
#endif