#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	class PWorld;

	/*
	* Builds the Sponza test scene.
	*
	* One actor and one static mesh component per object the source file authored, each drawn with
	* the material instance of the material that object names. The pairing between an object, the
	* mesh asset it was written to, and the material and texture files it was authored with is
	* written beside the assets at import time (the mesh importer's scene manifest), because neither
	* a mesh asset nor the PAsset layout carries a material name.
	*
	* Everything the scene needs arrives asynchronously - the import itself, then one asset load per
	* object and per texture, then the material assembly - so the whole build is a deferred step that
	* asks for what it does not have yet and runs again next frame. Nothing blocks the game thread
	* beyond the import the asset manager already runs on its load path.
	*
	* Game thread: called once from the level's UserBeginPlay. Repeated calls are a no-op.
	*/
	extern void	BuildDemoSponzaScene(PWorld& InWorld);

}	// namespace PigeonEngine
