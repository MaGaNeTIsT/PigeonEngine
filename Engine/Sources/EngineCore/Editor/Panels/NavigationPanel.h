#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	class PWorld;

	/*
	* Level-scoped navigation panel. One row per navmesh actor of the open level: agent name,
	* agent size, assigned asset and the result of the last build pass. The two level actions
	* live here - build every agent, and create a navmesh actor - because navigation belongs
	* to the level, not to a selected object. Hosted from EWorldManager::EditorUpdate.
	*/
	namespace EWorldNavigationPanel
	{
		void GenerateNavigation(PWorld* InWorld);
	}
}

#endif
