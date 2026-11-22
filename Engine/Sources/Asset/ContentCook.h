#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	class PWorld;

	/*
	* Generic content cook / validation pass.
	*
	* The engine's import cache lives under Build/Temp and is not committed. A packaged build
	* therefore has to prepare it on a build machine, and a level that references an asset which
	* was never imported has to be reported before it ships. This pass does exactly that, without
	* knowing any project content:
	*
	*   1. import every source in Assets/AssetSources.json (mesh roots expand through their
	*      scene manifests, textures load one by one);
	*   2. load every Assets/Levels/*.PLevel in the editor world and wait for its asynchronous
	*      asset requests to settle;
	*   3. report the actor count and the mesh asset names that stayed unbound;
	*   4. write Build/CookedContentReport.json and quit.
	*
	* Enable with PIGEON_COOK_CONTENT=1. Editor builds only: a runtime build has no importer.
	* It is a build/validation tool, not part of the level or runtime flow.
	*/
	namespace EContentCook
	{
		// TRUE while the cook owns the frame; FALSE when no cook was requested or it is done.
		BOOL32	Tick(PWorld& InWorld);
	}
}

#endif
