#include "AnimationImporter.h"
#include "AnimationAsset/AnimationAsset.h"
#if _EDITOR_ONLY
void PigeonEngine::EAnimationImporter::CreateImportEditor(TArray<EString> Paths)
{
	NeedUpdate = TRUE;
	InitializeEditor = FALSE;
	m_Paths.Empty();
	for (UINT32 i = 0, n = Paths.Num<UINT32>(); i < n; i++)
	{
		m_Paths.Add(Paths[i].Replace("\\", "/"));
	}
}

void PigeonEngine::EAnimationImporter::UpdateImportEditor()
{
	static CHAR CopyFileFolder[PigeonEngine::EPath::MAX_PATH_LENGTH] = "";
	static BOOL8 CheckCopyFile = FALSE;

	if (!InitializeEditor)
	{
		::memset(CopyFileFolder, 0, EPath::MAX_PATH_LENGTH);
		CheckCopyFile = FALSE;
		InitializeEditor = TRUE;
	}

	ImGui::Begin("Animation Importer", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
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
			for (UINT32 i = 0, n = m_Paths.Num<UINT32>(); i < n; i++)
			{
				//Import Asset
				EAnimationManager::GetManagerSingleton()->ImportSkeletonAnimation(EPath::GetFileNameWithoutExtension(m_Paths[i]), m_Paths[i], EBaseSettings::ENGINE_ASSET_DIRECTORY);

				//CopyFile
				if (/*Asset &&*/ CheckCopyFile)
				{
					EString CopyPath = EPath::Combine(EString(EBaseSettings::ENGINE_ASSET_DIRECTORY), EString(CopyFileFolder), EPath::GetFileNameWithExtension(m_Paths[i]));
					if (!EFileHelper::CopyFileToNewPath(m_Paths[i], CopyPath))
					{
						PE_CHECK((ENGINE_ASSET_ERROR), ("Animation Importer : copy file error!"), (FALSE));
					}
				}
			}
			NeedUpdate = FALSE;
		}
	}
	ImGui::End();
}
#endif