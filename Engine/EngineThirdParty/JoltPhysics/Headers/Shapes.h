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

	class FShape : ERTTI
	{
		EClass(FShape, ERTTI)
		FShape();
		virtual ~FShape() = default;
		virtual AABBBox GetWorldSpaceBounds(Quaternion Rotation, Vector3 Scale);
	};

	class FCastRay : ERTTI
	{
		EClass(FCastRay, ERTTI)
		FCastRay();
		virtual ~FCastRay() = default;
	};
}