#pragma once

#include <CoreMinimal.h>
#include "JoltIncludes.h"

namespace PigeonEngine
{
	struct AABBBox
	{
		Vector3 Min;
		Vector3 Max;
	};

	class FShape /*: public ERTTI*/
	{
		//EClass(FShape, ERTTI)
		FShape();
		virtual ~FShape() = default;
		virtual AABBBox GetWorldSpaceBounds(Quaternion Rotation, Vector3 Scale);
	};

	class FCastRay /*: public ERTTI*/
	{
		//EClass(FCastRay, ERTTI)
		FCastRay();
		virtual ~FCastRay() = default;
	};
}