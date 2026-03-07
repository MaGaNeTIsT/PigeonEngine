#include "SkeletonImporter.h"
#include "SkeletonAsset/SkeletonAsset.h"
#if _EDITOR_ONLY
void PigeonEngine::ESkeletonImporter::CreateImportEditor(TArray<EString> Paths)
{
	NeedUpdate = TRUE;
	InitializeEditor = FALSE;
	m_Paths.Empty();
	for (UINT32 i = 0, n = Paths.Num<UINT32>(); i < n; i++)
	{
		m_Paths.Add(Paths[i].Replace("\\", "/"));
	}
}

void PigeonEngine::ESkeletonImporter::UpdateImportEditor()
{
	static CHAR CopyFileFolder[PigeonEngine::EPath::MAX_PATH_LENGTH] = "";
	static BOOL8 CheckCopyFile = FALSE;

	if (!InitializeEditor)
	{
		::memset(CopyFileFolder, 0, EPath::MAX_PATH_LENGTH);
		CheckCopyFile = FALSE;
		InitializeEditor = TRUE;
	}

	ImGui::Begin("Skeleton Importer", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
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

		ImGui::Checkbox("CopyFile", &CheckCopyFile);
		if (CheckCopyFile)
		{
			ImGui::PushItemWidth(250);
			ImGui::InputText("Copy File Path", CopyFileFolder, EPath::MAX_PATH_LENGTH, ImGuiInputTextFlags_CharsNoBlank);
		}

		if (ImGui::Button("Import"))
		{
			EString FilePath;
			EString FileName;
			EString FileExtension;

			for (UINT32 i = 0, n = m_Paths.Num<UINT32>(); i < n; i++)
			{
				FilePath = EPath::GetFileFolderPath(m_Paths[i]) + "/";
				FileName = EPath::GetFileNameWithoutExtension(m_Paths[i]);
				FileExtension = EPath::GetExtension(m_Paths[i]);
				const ESkeletonAsset* Asset = NULL;
				TryLoadSkeleton(EBaseSettings::ENGINE_SKELETON_PATH, FileName, Asset, &FilePath, &FileName, &FileExtension);

				//CopyFile
				if (Asset && CheckCopyFile)
				{
					EString CopyPath = EPath::Combine(EString(EBaseSettings::ENGINE_ASSET_DIRECTORY), EString(CopyFileFolder), EPath::GetFileNameWithExtension(m_Paths[i]));
					if (!EFileHelper::CopyFileToNewPath(m_Paths[i], CopyPath))
					{
						PE_CHECK((ENGINE_ASSET_ERROR), ("Skeleton Importer : copy file error!"), (FALSE));
					}
				}
			}

			NeedUpdate = FALSE;
		}
	}
	ImGui::End();
}
#endif