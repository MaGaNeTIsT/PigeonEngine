#include "MeshImporter.h"
#include "MeshAsset/MeshAsset.h"
#if ENGINE_EDITOR_ONLY
void PigeonEngine::EMeshImporter::CreateImportEditor(TArray<EString> InPaths)
{
	NeedUpdate = TRUE;
	bInitializeEditor = FALSE;
	Paths.Empty();
	MeshCombineChecks.Empty();
	for (UINT32 PathIndex = 0u, PathNum = InPaths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
	{
		Paths.Add(InPaths[PathIndex].Replace("\\", "/"));
		MeshCombineChecks.Add(FALSE);
	}
}

void PigeonEngine::EMeshImporter::UpdateImportEditor()
{
	//Combo Item
	static const CHAR* ImportTypeItems[] = { "Static Mesh", "Skeletal Mesh" };

	//Initialize Data
	static INT32 ImportTypeIndex = 0;
	static CHAR CopyFileFolder[PigeonEngine::EPath::MAX_PATH_LENGTH] = "";
	static BOOL8 bCopyFile = FALSE;
	if (!bInitializeEditor)
	{
		ImportTypeIndex = 0;
		::memset(CopyFileFolder, 0, EPath::MAX_PATH_LENGTH);
		bCopyFile = FALSE;
		bInitializeEditor = TRUE;
	}

	ImGui::Begin("Mesh Importer", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
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
		for (UINT32 PathIndex = 0u, PathNum = Paths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
		{
			ImGui::Text("FileName:");
			ImGui::SameLine();
			{
				EString FileName = EPath::GetFileNameWithExtension(Paths[PathIndex]);
				ImGui::Text(*FileName);
			}
			ImGui::SameLine();
			{
				BOOL8 bMeshCombineCheck = MeshCombineChecks[PathIndex];
				UINT32 ToChar = PathIndex + 49u;
				EString CombineType = EString("IsCombineSubmeshes ") + EString((CHAR*)&(ToChar));
				ImGui::Checkbox(*CombineType, &bMeshCombineCheck);
				MeshCombineChecks[PathIndex] = bMeshCombineCheck;
			}
		}

		ImGui::Checkbox("CopyFile", &bCopyFile);
		if (bCopyFile)
		{
			ImGui::PushItemWidth(250);
			ImGui::InputText("Copy File Path", CopyFileFolder, EPath::MAX_PATH_LENGTH, ImGuiInputTextFlags_CharsNoBlank);
		}

		if (ImGui::Button("Import"))
		{
			EString FilePath;
			EString FileName;
			EString FileExtension;
			BOOL32 bAnyImported = FALSE;
			if (ImportTypeIndex == 0)//Static Mesh
			{
				for (UINT32 PathIndex = 0u, PathNum = Paths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
				{
					FilePath = EPath::GetFileFolderPath(Paths[PathIndex]) + "/";
					FileName = EPath::GetFileNameWithoutExtension(Paths[PathIndex]);
					FileExtension = EPath::GetExtension(Paths[PathIndex]);
					const EStaticMeshAsset* Asset = NULL;
					TryLoadStaticMesh(EBaseSettings::ENGINE_MESH_PATH, FileName, Asset, &FilePath, &FileName, &FileExtension, MeshCombineChecks[PathIndex]);
					if (Asset != nullptr)
					{
						bAnyImported = TRUE;
					}

					//CopyFile
					if (Asset && bCopyFile)
					{
						EString CopyPath = EPath::Combine(EString(EBaseSettings::ENGINE_ASSET_DIRECTORY), EString(CopyFileFolder), EPath::GetFileNameWithExtension(Paths[PathIndex]));
						if (!EFileHelper::CopyFileToNewPath(Paths[PathIndex], CopyPath))
						{
							PE_CHECK((ENGINE_ASSET_ERROR), ("Mesh Importer : copy file error!"), (FALSE));
						}
					}
				}
			}
			else if (ImportTypeIndex == 1)//Skeletal Mesh
			{
				for (UINT32 PathIndex = 0u, PathNum = Paths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
				{
					FilePath = EPath::GetFileFolderPath(Paths[PathIndex]) + "/";
					FileName = EPath::GetFileNameWithoutExtension(Paths[PathIndex]);
					FileExtension = EPath::GetExtension(Paths[PathIndex]);
					const ESkinnedMeshAsset* Asset = NULL;
					TryLoadSkinnedMesh(EBaseSettings::ENGINE_MESH_PATH, FileName, Asset, &FilePath, &FileName, &FileExtension, MeshCombineChecks[PathIndex]);
					if (Asset != nullptr)
					{
						bAnyImported = TRUE;
					}

					//CopyFile
					if (Asset && bCopyFile)
					{
						EString CopyPath = EPath::Combine(EString(EBaseSettings::ENGINE_ASSET_DIRECTORY), EString(CopyFileFolder), EPath::GetFileNameWithExtension(Paths[PathIndex]));
						if (!EFileHelper::CopyFileToNewPath(Paths[PathIndex], CopyPath))
						{
							PE_CHECK((ENGINE_ASSET_ERROR), ("Mesh Importer : copy file error!"), (FALSE));
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
