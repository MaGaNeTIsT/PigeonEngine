#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY
#include <Reflection/Property.h>

namespace PigeonEngine
{
	/*
	* Reflection-driven property grid used by the World Details panel and by the per-component
	* GenerateComponentDetail implementations. Reflection supplies the value type and the
	* getter/setter pair; this module only maps that to ImGui widgets. Components with a
	* CUSTOM property (physics shapes, water parameters, ...) keep their hand-written panel.
	*/
	namespace EPropertyEditor
	{
		// Called after an edit is committed with the object, its runtime class hash, the
		// property name and the values before/after. The command stack listens to this to
		// record SetProperty commands; without a callback the edit is simply applied.
		using FOnPropertyCommitted = TFunction<void(void*, const size_t&, const EString&, const FPropertyValue&, const FPropertyValue&)>;

		void SetPropertyCommittedCallback(const FOnPropertyCommitted& InCallback);
		// Asset picker shared by the reflection grid and hand-written component panels. The
		// names are scanned from the asset family directory and refreshed periodically, so a
		// newly imported or baked asset appears without restarting the editor.
		BOOL32 DrawAssetCombo(const CHAR* InLabel, const EPropertyAssetType InAssetType, EString& InOutAssetName);
		// Reports an edit that a custom control already applied, so it enters the same undo
		// history as a grid edit.
		void NotifyPropertyChanged(void* InObject, const size_t& InClassHash, const EString& InPropertyName,
			const FPropertyValue& InBefore, const FPropertyValue& InAfter);
		// Draws every editable reflected property of the object. Returns TRUE when any value
		// was applied this frame.
		BOOL32 DrawProperties(void* InObject, const size_t& InClassHash);
	}
}
#endif
