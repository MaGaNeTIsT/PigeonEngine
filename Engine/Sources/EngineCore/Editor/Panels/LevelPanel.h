#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	class EWorldManager;

	/*
	* The Level panel: current level display, the New/Save/Save As/Reload intents and the
	* level file list with rename/duplicate/delete. The world manager owns the level
	* lifecycle state and the request queue; this module only draws, performs the file
	* operations, and reports renames and deletions back through the manager's Notify hooks.
	* GenerateLevelPanel is the Level window content (hosted from EWorldManager::EditorUpdate);
	* DrawLevelDialogs draws the modals and is hosted by EEditorManager every frame, outside
	* any hideable panel.
	*/
	namespace ELevelPanel
	{
		void GenerateLevelPanel(EWorldManager* InManager);

		// Open Level / Save As / Unsaved Changes / Rename-Duplicate-Delete dialogs. The
		// caller must already have an ImGui window current: ImGui popup ids are relative to
		// the window that calls OpenPopup, so this module keeps one stable host rather than
		// relying on the (possibly hidden) Level window.
		void DrawLevelDialogs(EWorldManager* InManager);

		// Intent flags set by the world manager's request API and the editor File menu;
		// consumed by DrawLevelDialogs.
		void RequestOpenPopup();
		void RequestSaveAsPopup();
		void RequestUnsavedPrompt();
		void MarkLevelListDirty();
	}
}

#endif
