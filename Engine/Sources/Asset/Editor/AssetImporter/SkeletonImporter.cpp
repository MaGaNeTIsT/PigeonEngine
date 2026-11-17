#include "SkeletonImporter.h"
#include "SkeletonAsset/SkeletonAsset.h"
#if ENGINE_EDITOR_ONLY
void PigeonEngine::ESkeletonImporter::CreateImportEditor(TArray<EString> InPaths)
{
	NeedUpdate = TRUE;
	bInitializeEditor = FALSE;
	Paths.Empty();
	for (UINT32 PathIndex = 0u, PathNum = InPaths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
	{
		Paths.Add(InPaths[PathIndex].Replace("\\", "/"));
	}
}

void PigeonEngine::ESkeletonImporter::UpdateImportEditor()
{
	static CHAR CopyFileFolder[PigeonEngine::EPath::MAX_PATH_LENGTH] = "";
	static BOOL8 bCopyFile = FALSE;

	if (!bInitializeEditor)
	{
		::memset(CopyFileFolder, 0, EPath::MAX_PATH_LENGTH);
		bCopyFile = FALSE;
		bInitializeEditor = TRUE;
	}

	ImGui::Begin("Skeleton Importer", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
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

			for (UINT32 PathIndex = 0u, PathNum = Paths.Num<UINT32>(); PathIndex < PathNum; PathIndex++)
			{
				FilePath = EPath::GetFileFolderPath(Paths[PathIndex]) + "/";
				FileName = EPath::GetFileNameWithoutExtension(Paths[PathIndex]);
				FileExtension = EPath::GetExtension(Paths[PathIndex]);
				const ESkeletonAsset* Asset = NULL;
				TryLoadSkeleton(EBaseSettings::ENGINE_SKELETON_PATH, FileName, Asset, &FilePath, &FileName, &FileExtension);
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
						PE_CHECK((ENGINE_ASSET_ERROR), ("Skeleton Importer : copy file error!"), (FALSE));
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
