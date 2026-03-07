#include "IImporter.h"

#if _EDITOR_ONLY
void PigeonEngine::IImporter::UpdateEditor()
{
	if (NeedUpdate)
	{
		UpdateImportEditor();
	}
}
#endif