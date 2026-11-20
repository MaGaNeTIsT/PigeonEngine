#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	class PWorld;

	/*
	* The world details panel. The host draws the generic frame - the selected actor's
	* name row, the reflection grid, and the Remove Component action - and hands the
	* per-class drawing to EEditorCustomization, which composes every editor registered
	* along the selected object's inheritance chain, base-first. Builtin editors are
	* registered once from EEditorManager::EditorInit. Hosted from
	* EWorldManager::EditorUpdate.
	*/
	namespace EWorldDetailsPanel
	{
		void RegisterBuiltinCustomEditors();
		void GenerateDetails(PWorld* InWorld);
	}
}

#endif
