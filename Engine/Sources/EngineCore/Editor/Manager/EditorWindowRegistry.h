#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

namespace PigeonEngine
{
	/*
	* One optional docked editor window. Registration supplies the metadata and the
	* content draw callback; the registry owns Begin/End, the title-bar X, the Window
	* menu and the persisted visibility. Draw must not call Begin/End itself.
	*/
	struct FEditorWindowDesc
	{
		// Stable identifier. The settings key is Panels.<Id>.
		EString				Id;
		// ImGui window title, which is also the docking/layout key.
		EString				Title;
		// Full Window-menu path, e.g. "Window/World/Level". The last segment is the
		// menu label; empty falls back to "Window/<Title>".
		EString				MenuPath;
		BOOL32				bDefaultVisible = FALSE;
		// First-use size only. Position is deliberately never set: SetNextWindowPos
		// undocks a panel reopened from the Window menu.
		ImVec2				DefaultSize = ImVec2(0.0f, 0.0f);
		TFunction<void()>	Draw;
		// Loaded from settings in Initialize; the X and the menu keep it in sync.
		BOOL32				bVisible = FALSE;
	};

	namespace EEditorWindowRegistry
	{
		// Registers one window and its Panels.<Id> settings key. Must run before
		// EEditorSettings::Initialize(), because that is what loads the default/user files.
		void RegisterWindow(const FEditorWindowDesc& InWindow);
		// Reads the persisted visibility for every registered window.
		void Initialize();
		// Draws one registered window if it is visible. Hosts call this at their existing
		// draw point so the editor's window order stays where it was.
		void DrawWindow(const EString& InId);
		// Drops the descriptors and their captured panel callbacks. Called before the
		// panels they reference are destroyed.
		void Clear();
	}
}

#endif
