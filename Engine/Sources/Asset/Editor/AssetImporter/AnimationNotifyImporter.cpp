#include "AnimationNotifyImporter.h"

#include "AnimationNotifyAsset/AnimationNotifyAsset.h"

#if ENGINE_EDITOR_ONLY

void PigeonEngine::EAnimNotifyImporter::CreateImportEditor(TArray<EString> InPaths)
{
	NeedUpdate = TRUE;
	bInitializeEditor = FALSE;
	Paths.Empty();
	for (UINT32 Index = 0; Index < InPaths.Num<UINT32>(); Index++)
	{
		Paths.Add(InPaths[Index].Replace("\\", "/"));
	}
}

void PigeonEngine::EAnimNotifyImporter::UpdateImportEditor()
{
	bInitializeEditor = TRUE;

	ImGui::Begin("Notify Importer", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
	{
		for (UINT32 Index = 0; Index < Paths.Num<UINT32>(); Index++)
		{
			ImGui::Text("FileName:");
			ImGui::SameLine();
			const EString FileName = EPath::GetFileNameWithExtension(Paths[Index]);
			ImGui::Text("%s", *FileName);
		}

		if (ImGui::Button("Import"))
		{
			BOOL32 bAnyImported = FALSE;
			for (UINT32 Index = 0; Index < Paths.Num<UINT32>(); Index++)
			{
				if (EAnimNotifyManager::GetManagerSingleton()->ImportNotify(
					EPath::GetFileNameWithoutExtension(Paths[Index]), Paths[Index],
					EBaseSettings::ENGINE_NOTIFY_PATH) != FALSE)
				{
					bAnyImported = TRUE;
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
