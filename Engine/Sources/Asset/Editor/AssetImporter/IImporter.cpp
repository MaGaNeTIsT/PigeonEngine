#include "IImporter.h"
#include "ImportManager.h"

#if ENGINE_EDITOR_ONLY
void PigeonEngine::IImporter::UpdateEditor()
{
	if (NeedUpdate)
	{
		UpdateImportEditor();
	}
}

void PigeonEngine::IImporter::NotifyImportFinished()
{
	PigeonEngine::EImportManager::GetManagerSingleton()->NotifyImportFinished();
}
#endif
