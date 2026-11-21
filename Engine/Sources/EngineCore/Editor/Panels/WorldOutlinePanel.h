#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	class PActor;
	class PSceneComponent;
	class PWorld;

	/*
	* The world outline panel: the actor hierarchy with its creation menus, drag-drop
	* reparenting and the detach drop zone, plus the per-actor scene component tree.
	* The engine object classes carry no ImGui code for outlining; this module draws
	* them through their public API (selection resolution, command stack, RTTI class
	* lists). Hosted from EWorldManager::EditorUpdate.
	*/
	namespace EWorldOutlinePanel
	{
		void GenerateOutline(PWorld* InWorld);
		void GenerateComponentTree(PSceneComponent* InRoot, PWorld* InWorld);
	}
}

#endif
