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

	class Shape : ERTTI
	{
		EClass(Shape, ERTTI)
		Shape();
		virtual ~Shape() = default;
		virtual AABBBox GetWorldSpaceBounds(Quaternion Rotation, Vector3 Scale);
	};
}