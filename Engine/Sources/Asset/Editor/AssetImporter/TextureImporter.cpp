#include "TextureImporter.h"
#include "TextureAsset/TextureAsset.h"
#include "Base/DataStructure/Text/Path.h"
#include "FileHelper.h"
#if _EDITOR_ONLY
void PigeonEngine::ETextureImporter::CreateImportEditor(TArray<EString> Paths)
{
	NeedUpdate = TRUE;
	InitializeEditor = FALSE;
	m_Paths.Empty();
	m_CubeMapIndex.Empty();
	for (UINT32 i = 0, n = Paths.Num<UINT32>(); i < n; i++)
	{
		m_Paths.Add(Paths[i].Replace("\\", "/"));
		m_CubeMapIndex.Add(0);
	}
	//m_Paths = Paths;
}

void PigeonEngine::ETextureImporter::UpdateImportEditor()
{
	//Combo Item
	static const CHAR* type_items[] = { "Texture2D", "TextureCube" };
	static const CHAR* cube_items[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };

	//Initialize Data
	static INT32 type_item_current_idx = 0;
	static TMap<const CHAR*, EString> CubeMap;
	static CHAR CopyFileFolder[PigeonEngine::EPath::MAX_PATH_LENGTH] = "";
	static CHAR CubeMapName[PigeonEngine::EPath::MAX_PATH_LENGTH] = "";
	static BOOL8 CheckCopyFile = FALSE;
	if (!InitializeEditor)
	{
		type_item_current_idx = 0;
		::memset(CopyFileFolder, 0, EPath::MAX_PATH_LENGTH);
		::memset(CubeMapName, 0, EPath::MAX_PATH_LENGTH);
		CheckCopyFile = FALSE;
		CubeMap.Empty();
		//TODO : does this necessary?
		//const CHAR* cube_items[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
		//for (UINT32 i = 0; i < m_Paths.Length(); i++)
		//{
		//	CubeMap.Add(cube_items[i % 6], m_Paths[i]);
		//}
		InitializeEditor = TRUE;
	}

	ImGui::Begin("Texture Importer", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
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
			for (UINT32 i = 0, n = m_Paths.Num<UINT32>(); i < n; i++)
			{
				ImGui::Text("FileName:");
				ImGui::SameLine();
				{
					EString FileName = EPath::GetFileNameWithExtension(m_Paths[i]);
					ImGui::Text(*FileName);
				}
			}
		}
		else if (type_item_current_idx == 1)//TextureCube
		{
			CubeMap.Empty();
			for (INT32 i = 0, p = m_Paths.Num(); i < p; i++)
			{
				const CHAR* cube_combo_preview_value = cube_items[m_CubeMapIndex[i]];
				CubeMap.Add(cube_combo_preview_value, m_Paths[i]);
				ImGui::PushItemWidth(50);
				UINT32 ToChar = i + 65u;
				EString CubeType = EString("Cube Type ") + EString((CHAR*)&(ToChar));
				if (ImGui::BeginCombo(*CubeType, cube_combo_preview_value))
				{
					for (INT32 n = 0, u = PE_ARRAYSIZE(cube_items); n < u; n++)
					{
						const BOOL8 is_selected = (m_CubeMapIndex[i] == n);
						if (ImGui::Selectable(cube_items[n], is_selected))
						{
							m_CubeMapIndex[i] = n;
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
			ImGui::PushItemWidth(250);
			ImGui::InputText("Copy File Path", CopyFileFolder, EPath::MAX_PATH_LENGTH, ImGuiInputTextFlags_CharsNoBlank);
			if (type_item_current_idx == 1)
			{
				ImGui::PushItemWidth(250);
				ImGui::InputText("Texture Cube Name", CubeMapName, EPath::MAX_PATH_LENGTH, ImGuiInputTextFlags_CharsNoBlank);
			}
		}
		if (ImGui::Button("Import"))
		{
			//Handle Path.
			if (type_item_current_idx == 0)//Texture2D
			{
				for (UINT32 i = 0, n = m_Paths.Num<UINT32>(); i < n; i++)
				{
					const ETexture2DAsset* Asset = NULL;
					EString FileName = EPath::GetFileNameWithoutExtension(m_Paths[i]);
					EString FilePath = EPath::GetFileFolderPath(m_Paths[i]) + "/";
					TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, FileName, Asset, &FilePath, &FileName, &EPath::GetExtension(m_Paths[i]));

					//CopyFile
					if (Asset && CheckCopyFile)
					{
						EString CopyPath = EPath::Combine(EString(EBaseSettings::ENGINE_ASSET_DIRECTORY), EString(CopyFileFolder), EPath::GetFileNameWithExtension(m_Paths[i]));
						//BOOL8 CopySuccess = FALSE;
						//CopyFile(*m_Paths[i], *CopyPath, CopySuccess);
						//PE_CHECK((ENGINE_ASSET_ERROR), ("Texture Importer copy file error!"), (CopySuccess));
						if (!EFileHelper::CopyFileToNewPath(m_Paths[i], CopyPath))
						{
							PE_CHECK((ENGINE_ASSET_ERROR), ("Texture Importer : copy file error!"), (FALSE));
						}
					}
				}
			}
			else if (type_item_current_idx == 1)//TextureCube
			{
				const ETextureCubeAsset* Asset = NULL;
				TArray<EString> Extensions;
				TArray<EString> FileNames;
				TArray<EString> Paths;
				EString CubeMapPath;
				if (CubeMap.FindValue("+X", CubeMapPath))
				{
					Paths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				if (CubeMap.FindValue("-X", CubeMapPath)) 
				{
					Paths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				if (CubeMap.FindValue("+Y", CubeMapPath)) 
				{
					Paths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				if (CubeMap.FindValue("-Y", CubeMapPath))
				{
					Paths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				if (CubeMap.FindValue("+Z", CubeMapPath)) 
				{
					Paths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				if (CubeMap.FindValue("-Z", CubeMapPath)) 
				{
					Paths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				TryLoadTextureCube(EBaseSettings::ENGINE_TEXTURE_PATH, CubeMapName, Asset, &Paths, &FileNames, &Extensions);

				//CopyFile
				if (Asset && CheckCopyFile)
				{
					EString CopyPath = EPath::Combine(EString(EBaseSettings::ENGINE_ASSET_DIRECTORY), EString(CopyFileFolder), EString(CubeMapName));
					for (auto Item = CubeMap.Begin(); Item != CubeMap.End(); Item++)
					{
						if (!EFileHelper::CopyFileToNewPath(Item->second, EPath::Combine(CopyPath, EPath::GetFileNameWithExtension(Item->second))))
						{
							PE_CHECK((ENGINE_ASSET_ERROR), ("Texture Importer : copy file error!"), (FALSE));
						}
					}
				}
			}
			NeedUpdate = FALSE;
		}
	}
	ImGui::End();
}
#endif