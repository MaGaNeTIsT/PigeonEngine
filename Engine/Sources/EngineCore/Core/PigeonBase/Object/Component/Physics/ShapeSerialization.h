#pragma once

#include <CoreMinimal.h>
#include <Reflection/Archive.h>

namespace PigeonEngine
{
	class FShape;

	/*
	* Shared physics-shape encoding for PPhysicsComponent and PTriggerComponent. Shapes
	* are value objects with public parameters; the type tag keeps the file readable and a
	* reader constructs the concrete shape through its normal constructor.
	*
	* PhysicsMaterial pointers are not serialized (there is no named material asset yet);
	* loaded shapes use the default material.
	*/
	BOOL32 SerializePhysicsShape(EArchive& InArchive, const CHAR* InName, FShape*& InOutShape);
}
