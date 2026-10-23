#include "AnimationStateMachineImporter.h"

#include "AnimationStateMachineAsset/AnimationStateMachineAsset.h"

#if _EDITOR_ONLY

void PigeonEngine::EAnimStateMachineImporter::CreateImportEditor(TArray<EString> Paths)
{
	NeedUpdate = TRUE;
	InitializeEditor = FALSE;
	m_Paths.Empty();
	for (UINT32 Index = 0; Index < Paths.Num<UINT32>(); Index++)
	{
		m_Paths.Add(Paths[Index].Replace("\\", "/"));
	}
}

void PigeonEngine::EAnimStateMachineImporter::UpdateImportEditor()
{
	if (!InitializeEditor)
	{
		InitializeEditor = TRUE;
	}

	ImGui::Begin("State Machine Importer", &NeedUpdate, ImGuiWindowFlags_::ImGuiWindowFlags_None);
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
				EAnimStateMachineManager::GetManagerSingleton()->ImportStateMachine(
					EPath::GetFileNameWithoutExtension(m_Paths[Index]), m_Paths[Index],
					EBaseSettings::ENGINE_STATE_MACHINE_PATH);
			}
			NeedUpdate = FALSE;
		}
	}
	ImGui::End();
}

#endif
