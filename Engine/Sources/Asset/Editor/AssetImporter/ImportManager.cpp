#include "ImportManager.h"
#include "Asset/Editor/FileDialogHelper/FileDialogHelper.h"
#include <Base/DataStructure/Text/Path.h>
#include <Base/RTTI/RTTIManager.h>
#if ENGINE_EDITOR_ONLY
//Importer
#include "MeshImporter.h"
#include "TextureImporter.h"
#include "SkeletonImporter.h"
#include "AnimationImporter.h"
#include "AnimationStateMachineImporter.h"
#include "AnimationMontageImporter.h"
#include "AnimationNotifyImporter.h"

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		PigeonEngine::RegisterClassType<PigeonEngine::EImportManager, PigeonEngine::EManagerBase>();
	}
	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

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
	for (TMap<EString, IImporter*>::TIterator Item = Importers.Begin(); Item != Importers.End(); Item++)
	{
		if (Item->second)
			delete Item->second;
	}
	bContentBrowserRefreshRequested = FALSE;
}

void PigeonEngine::EImportManager::ShowImporterButton()
{
	if (ImGui::Button("Import"))
	{
		bShowEditor = TRUE;
	}
}

void PigeonEngine::EImportManager::NotifyImportFinished()
{
	bContentBrowserRefreshRequested = TRUE;
}

BOOL32 PigeonEngine::EImportManager::ConsumeContentBrowserRefreshRequest()
{
	const BOOL32 bRequested = bContentBrowserRefreshRequested;
	bContentBrowserRefreshRequested = FALSE;
	return bRequested;
}

void PigeonEngine::EImportManager::EditorInit()
{
	AddImporter("Mesh", new EMeshImporter());
	AddImporter("Texture", new ETextureImporter());
	AddImporter("Skeleton", new ESkeletonImporter());
	AddImporter("Animation", new EAnimationImporter());
	AddImporter("StateMachine", new EAnimStateMachineImporter());
	AddImporter("Montage", new EAnimMontageImporter());
	AddImporter("Notify", new EAnimNotifyImporter());
}

void PigeonEngine::EImportManager::EditorUpdate()
{
	for (TMap<EString, IImporter*>::TIterator Item = Importers.Begin(); Item != Importers.End(); Item++)
	{
		Item->second->UpdateEditor();
	}
	if (bShowEditor)
	{
		ShowEditor();
	}
}

void PigeonEngine::EImportManager::AddImporter(EString FileExtension, IImporter* Importer)
{
	Importers.Add(FileExtension, Importer);
}
void PigeonEngine::EImportManager::ShowEditor()
{
	ImGui::Begin("Importer Select", &bShowEditor, ImGuiWindowFlags_::ImGuiWindowFlags_None);
	{
		for (TMap<EString, IImporter*>::TIterator Item = Importers.Begin(); Item != Importers.End(); Item++)
		{
			if (ImGui::Button(*Item->first,ImVec2(250,0)))
			{
				PathSet SelectedPaths;
				SelectedPaths.count = 0;
				EString FilterList = Item->second->GetFileFilterList();
				OpenDialogMultiSelect(*FilterList, NULL, &SelectedPaths);
				TArray<EString> Paths;
				for (SIZE_T PathIndex = 0u, PathNum = SelectedPaths.count; PathIndex < PathNum; PathIndex++)
				{
					const CHAR* Path = GetPath(&SelectedPaths, PathIndex);
					Paths.Add(Path);
				}
				if (SelectedPaths.count)
				{
					Item->second->CreateImportEditor(Paths);
					FreePathSet(&SelectedPaths);
				}
				bShowEditor = FALSE;
			}
		}
	}
	ImGui::End();
}

}	// namespace PigeonEngine
#endif
