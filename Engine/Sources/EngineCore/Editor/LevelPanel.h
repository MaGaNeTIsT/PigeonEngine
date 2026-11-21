#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	class EWorldManager;

	/*
	* The Level panel: current level display, the New/Save/Save As/Reload intents, the
	* level file list with rename/duplicate/delete, and the unsaved-changes prompt. The
	* world manager owns the level lifecycle state and the request queue; this module only
	* draws, performs the file operations, and reports renames and deletions back through
	* the manager's Notify hooks. Hosted from EWorldManager::EditorUpdate.
	*/
	namespace ELevelPanel
	{
		void GenerateLevelPanel(EWorldManager* InManager);

		// Intent flags set by the world manager's request API and the editor File menu;
		// consumed by the panel's popup flow.
		void RequestOpenPopup();
		void RequestSaveAsPopup();
		void RequestUnsavedPrompt();
		void MarkLevelListDirty();
	}
}

#endif
