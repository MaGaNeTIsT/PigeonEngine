#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class PWorld;

	// The water sample's world position. The demo level places its visible ground plane under
	// this location, so the surface sits on a floor instead of floating over the void.
	extern const Vector3 GWaterSceneLocation;

	/*
	* Builds the water simulation sample: one simulation body and one static mesh surface
	* that reads it. The build spans frames because the mesh import and the material assembly
	* are asynchronous, so repeated calls retry until the sample is complete.
	*/
	extern void	BuildDemoWaterScene(PWorld& InWorld);

}	// namespace PigeonEngine
