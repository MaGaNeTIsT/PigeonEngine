#include "ImportManager.h"
#include "Asset/Editor/FileDialogHelper/FileDialogHelper.h"
#include <Base/DataStructure/Text/Path.h>
#include <Base/RTTI/RTTIManager.h>
#if _EDITOR_ONLY
//Importer
#include "MeshImporter.h"
#include "TextureImporter.h"

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		PigeonEngine::RegisterClassType<PigeonEngine::EImportManager, PigeonEngine::EManagerBase>();
	}
	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);
};

PigeonEngine::EImportManager::EImportManager()
{
	
}
PigeonEngine::EImportManager::~EImportManager()
{
	
}

void PigeonEngine::EImportManager::Initialize()
{
}

void PigeonEngine::EImportManager::ShutDown()
{
	//delete managed data.
	for (auto Item = m_Importers.Begin(); Item != m_Importers.End(); Item++)
	{
		if (Item->second)
			delete Item->second;
	}
}

void PigeonEngine::EImportManager::ShowImporterButton()
{
	if (ImGui::Button("Import"))
	{
		m_ShowEditor = TRUE;
	}
}

void PigeonEngine::EImportManager::EditorInit()
{
	AddImporter("Mesh", new EMeshImporter());
	AddImporter("Texture", new ETextureImporter());
}

void PigeonEngine::EImportManager::EditorUpdate()
{
	for (auto Item = m_Importers.Begin(); Item != m_Importers.End(); ++Item)
	{
		Item->second->UpdateEditor();
	}
	if (m_ShowEditor)
	{
		ShowEditor();
	}
}

void PigeonEngine::EImportManager::AddImporter(EString FileExtension, IImporter* Importer)
{
	m_Importers.Add(FileExtension, Importer);
}
void PigeonEngine::EImportManager::ShowEditor()
{
	ImGui::Begin("Importer Select", &m_ShowEditor, ImGuiWindowFlags_::ImGuiWindowFlags_None);
	{
		for (auto Item = m_Importers.Begin(); Item != m_Importers.End(); Item++)
		{
			if (ImGui::Button(*Item->first,ImVec2(250,0)))
			{
				PathSet PathSet;
				PathSet.count = 0;
				OpenDialogMultiSelect(*Item->second->GetFileFilterList(), NULL, &PathSet);
				TArray<EString> Paths;
				for (INT32 i = 0; i < PathSet.count; i++)
				{
					const CHAR* Path = GetPath(&PathSet, i);
					Paths.Add(Path);
				}
				if (PathSet.count)
				{
					Item->second->CreateImportEditor(Paths);
					FreePathSet(&PathSet);
				}
				m_ShowEditor = FALSE;
			}
		}
	}
	ImGui::End();
}
#endif