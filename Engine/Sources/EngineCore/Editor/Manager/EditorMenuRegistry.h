#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	/*
	* One main-menu entry. Path is "Root/Group.../Item": the first segment is the
	* top-level menu, the last segment is the default label, and the segments between
	* them become a section header (joined with " / "). Registration order defines the
	* root, group and item order; entries of one group must be registered contiguously.
	*
	* Dynamic state is resolved while the menu is open:
	*   Execute   - action; optional for status rows
	*   GetLabel  - overrides the last path segment
	*   IsVisible - hides a row without leaving an empty section header
	*   IsEnabled - greys out the row
	*   IsChecked - makes the row checkable; Execute should toggle the underlying state
	*   bStatusOnly - renders a disabled, non-clickable row
	*/
	struct FEditorMenuItemDesc
	{
		EString					Path;
		TFunction<void()>		Execute;
		EString					Shortcut;
		TFunction<EString()>	GetLabel;
		TFunction<BOOL32()>		IsVisible;
		TFunction<BOOL32()>		IsEnabled;
		TFunction<BOOL32()>		IsChecked;
		BOOL32					bStatusOnly = FALSE;
	};

	namespace EEditorMenuRegistry
	{
		// Registers one item. Must run before the first editor frame; callbacks live
		// until Clear().
		void RegisterItem(const FEditorMenuItemDesc& InItem);
		void DrawMainMenuBar();
		// Drops every item. Called before the objects the callbacks capture are destroyed.
		void Clear();
	}
}

#endif
