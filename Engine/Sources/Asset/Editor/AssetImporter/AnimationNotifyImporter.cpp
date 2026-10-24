#include "AnimationNotifyImporter.h"

#include "AnimationNotifyAsset/AnimationNotifyAsset.h"

#if _EDITOR_ONLY

void PigeonEngine::EAnimNotifyImporter::CreateImportEditor(TArray<EString> Paths)
{
	NeedUpdate = TRUE;
	InitializeEditor = FALSE;
	m_Paths.Empty();
	for (UINT32 Index = 0; Index < Paths.Num<UINT32>(); Index++)
	{
		m_Paths.Add(Paths[Index].Replace("\\", "/"));
	}
}

void PigeonEngine::EAnimNotifyImporter::UpdateImportEditor()
{
	InitializeEditor = TRUE;

	ImGui::Begin("Notify Importer", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
	{
		for (UINT32 Index = 0; Index < m_Paths.Num<UINT32>(); Index++)
		{
			ImGui::Text("FileName:");
			ImGui::SameLine();
			const EString FileName = EPath::GetFileNameWithExtension(m_Paths[Index]);
			ImGui::Text("%s", *FileName);
		}

		if (ImGui::Button("Import"))
		{
			for (UINT32 Index = 0; Index < m_Paths.Num<UINT32>(); Index++)
			{
				EAnimNotifyManager::GetManagerSingleton()->ImportNotify(
					EPath::GetFileNameWithoutExtension(m_Paths[Index]), m_Paths[Index],
					EBaseSettings::ENGINE_NOTIFY_PATH);
			}
			NeedUpdate = FALSE;
		}
	}
	ImGui::End();
}

#endif
