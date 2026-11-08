#include "TextureImporter.h"
#include "TextureAsset/TextureAsset.h"
#include "Base/DataStructure/Text/Path.h"
#include "FileHelper.h"
#if ENGINE_EDITOR_ONLY
void PigeonEngine::ETextureImporter::CreateImportEditor(TArray<EString> InPaths)
{
	NeedUpdate = TRUE;
	bInitializeEditor = FALSE;
	Paths.Empty();
	CubeMapIndex.Empty();
	for (UINT32 PathIndex = 0u, PathNum = InPaths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
	{
		Paths.Add(InPaths[PathIndex].Replace("\\", "/"));
		CubeMapIndex.Add(0);
	}
}

void PigeonEngine::ETextureImporter::UpdateImportEditor()
{
	//Combo Item
	static const CHAR* ImportTypeItems[] = { "Texture2D", "TextureCube" };
	static const CHAR* CubeFaceItems[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };

	//Initialize Data
	static INT32 ImportTypeIndex = 0;
	static TMap<const CHAR*, EString> CubeMap;
	static CHAR CopyFileFolder[PigeonEngine::EPath::MAX_PATH_LENGTH] = "";
	static CHAR CubeMapName[PigeonEngine::EPath::MAX_PATH_LENGTH] = "";
	static BOOL8 bCopyFile = FALSE;
	// Answers for images that declare no colour space of their own (TGA and friends): an image that
	// does declare one is imported by its own statement regardless of this box. Checked by default
	// because the files that need it are colour textures far more often than data textures; a
	// normal map or a mask is the case to untick it for.
	static BOOL8 bFallbackSRGB = TRUE;
	if (!bInitializeEditor)
	{
		ImportTypeIndex = 0;
		::memset(CopyFileFolder, 0, EPath::MAX_PATH_LENGTH);
		::memset(CubeMapName, 0, EPath::MAX_PATH_LENGTH);
		bCopyFile = FALSE;
		bFallbackSRGB = TRUE;
		CubeMap.Empty();
		//TODO : does this necessary?
		//const CHAR* cube_items[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
		//for (UINT32 i = 0; i < Paths.Length(); i++)
		//{
		//	CubeMap.Add(cube_items[i % 6], Paths[i]);
		//}
		bInitializeEditor = TRUE;
	}

	ImGui::Begin("Texture Importer", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
	{
		const CHAR* ImportTypePreviewName = ImportTypeItems[ImportTypeIndex];
		ImGui::PushItemWidth(120);
		if (ImGui::BeginCombo("Import Type", ImportTypePreviewName))
		{
			for (INT32 TypeIndex = 0, TypeNum = IM_ARRAYSIZE(ImportTypeItems); TypeIndex < TypeNum; TypeIndex++)
			{
				const BOOL8 bSelected = (ImportTypeIndex == TypeIndex);
				if (ImGui::Selectable(ImportTypeItems[TypeIndex], bSelected))
					ImportTypeIndex = TypeIndex;

				if (bSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (ImportTypeIndex == 0)//Texture2D
		{
			for (UINT32 PathIndex = 0u, PathNum = Paths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
			{
				ImGui::Text("FileName:");
				ImGui::SameLine();
				{
					EString FileName = EPath::GetFileNameWithExtension(Paths[PathIndex]);
					ImGui::Text(*FileName);
				}
			}
		}
		else if (ImportTypeIndex == 1)//TextureCube
		{
			CubeMap.Empty();
			for (INT32 PathIndex = 0, PathNum = Paths.Num(); PathIndex < PathNum; PathIndex++)
			{
				const CHAR* CubeFacePreviewName = CubeFaceItems[CubeMapIndex[PathIndex]];
				CubeMap.Add(CubeFacePreviewName, Paths[PathIndex]);
				ImGui::PushItemWidth(50);
				UINT32 ToChar = PathIndex + 65u;
				EString CubeType = EString("Cube Type ") + EString((CHAR*)&(ToChar));
				if (ImGui::BeginCombo(*CubeType, CubeFacePreviewName))
				{
					for (INT32 FaceIndex = 0, FaceNum = PE_ARRAYSIZE(CubeFaceItems); FaceIndex < FaceNum; FaceIndex++)
					{
						const BOOL8 bSelected = (CubeMapIndex[PathIndex] == FaceIndex);
						if (ImGui::Selectable(CubeFaceItems[FaceIndex], bSelected))
						{
							CubeMapIndex[PathIndex] = FaceIndex;
						}

						if (bSelected)
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
					EString FileName = EPath::GetFileNameWithExtension(Paths[PathIndex]);
					ImGui::Text(*FileName);
				}
			}
		}
		ImGui::Checkbox("CopyFile", &bCopyFile);
		// Untick this for data images (normal maps, masks, height fields) whose files carry no
		// colour space: with no declaration of their own they would otherwise be read as sRGB
		// colour and decoded.
		ImGui::Checkbox("Fallback sRGB (when the file declares no colour space)", &bFallbackSRGB);
		if (bCopyFile)
		{
			ImGui::PushItemWidth(250);
			ImGui::InputText("Copy File Path", CopyFileFolder, EPath::MAX_PATH_LENGTH, ImGuiInputTextFlags_CharsNoBlank);
			if (ImportTypeIndex == 1)
			{
				ImGui::PushItemWidth(250);
				ImGui::InputText("Texture Cube Name", CubeMapName, EPath::MAX_PATH_LENGTH, ImGuiInputTextFlags_CharsNoBlank);
			}
		}
		if (ImGui::Button("Import"))
		{
			BOOL32 bAnyImported = FALSE;
			//Handle Path.
			if (ImportTypeIndex == 0)//Texture2D
			{
				for (UINT32 PathIndex = 0u, PathNum = Paths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
				{
					const ETexture2DAsset* Asset = NULL;
					EString FileName = EPath::GetFileNameWithoutExtension(Paths[PathIndex]);
					EString FilePath = EPath::GetFileFolderPath(Paths[PathIndex]) + "/";
					TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, FileName, Asset, &FilePath, &FileName, &EPath::GetExtension(Paths[PathIndex]), bFallbackSRGB);
					if (Asset != nullptr)
					{
						bAnyImported = TRUE;
					}

					//CopyFile
					if (Asset && bCopyFile)
					{
						EString CopyPath = EPath::Combine(EString(EBaseSettings::ENGINE_ASSET_DIRECTORY), EString(CopyFileFolder), EPath::GetFileNameWithExtension(Paths[PathIndex]));
						//BOOL8 CopySuccess = FALSE;
						//CopyFile(*Paths[i], *CopyPath, CopySuccess);
						//PE_CHECK((ENGINE_ASSET_ERROR), ("Texture Importer copy file error!"), (CopySuccess));
						if (!EFileHelper::CopyFileToNewPath(Paths[PathIndex], CopyPath))
						{
							PE_CHECK((ENGINE_ASSET_ERROR), ("Texture Importer : copy file error!"), (FALSE));
						}
					}
				}
			}
			else if (ImportTypeIndex == 1)//TextureCube
			{
				const ETextureCubeAsset* Asset = NULL;
				TArray<EString> Extensions;
				TArray<EString> FileNames;
				TArray<EString> CubeMapPaths;
				EString CubeMapPath;
				if (CubeMap.FindValue("+X", CubeMapPath))
				{
					CubeMapPaths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				if (CubeMap.FindValue("-X", CubeMapPath)) 
				{
					CubeMapPaths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				if (CubeMap.FindValue("+Y", CubeMapPath)) 
				{
					CubeMapPaths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				if (CubeMap.FindValue("-Y", CubeMapPath))
				{
					CubeMapPaths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				if (CubeMap.FindValue("+Z", CubeMapPath)) 
				{
					CubeMapPaths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				if (CubeMap.FindValue("-Z", CubeMapPath)) 
				{
					CubeMapPaths.Add(EPath::GetFileFolderPath(CubeMapPath) + "/");
					FileNames.Add(EPath::GetFileNameWithoutExtension(CubeMapPath));
					Extensions.Add(EPath::GetExtension(CubeMapPath));
				}
				TryLoadTextureCube(EBaseSettings::ENGINE_TEXTURE_PATH, CubeMapName, Asset, &CubeMapPaths, &FileNames, &Extensions, bFallbackSRGB);
				if (Asset != nullptr)
				{
					bAnyImported = TRUE;
				}

				//CopyFile
				if (Asset && bCopyFile)
				{
					EString CopyPath = EPath::Combine(EString(EBaseSettings::ENGINE_ASSET_DIRECTORY), EString(CopyFileFolder), EString(CubeMapName));
					for (TMap<const CHAR*, EString>::TIterator Item = CubeMap.Begin(); Item != CubeMap.End(); Item++)
					{
						if (!EFileHelper::CopyFileToNewPath(Item->second, EPath::Combine(CopyPath, EPath::GetFileNameWithExtension(Item->second))))
						{
							PE_CHECK((ENGINE_ASSET_ERROR), ("Texture Importer : copy file error!"), (FALSE));
						}
					}
				}
			}
			if (bAnyImported != FALSE)
			{
				NotifyImportFinished();
			}
			NeedUpdate = FALSE;
		}
	}
	ImGui::End();
}
#endif
