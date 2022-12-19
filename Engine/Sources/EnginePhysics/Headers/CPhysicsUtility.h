#pragma once
#include "../../../../Entry/EngineMain.h"
#include "../../EngineGame/Headers/CGameObject.h"
#include <Jolt/Jolt.h>

namespace PhysicsUtility
{
	JPH_INLINE CustomType::Vector3 Convert(JPH::Vec3 Vec)
	{
		return CustomType::Vector3(Vec.GetX(), Vec.GetY(), Vec.GetZ());
	}

	JPH_INLINE JPH::Vec3 Convert(CustomType::Vector3 Vec)
	{
		return JPH::Vec3(Vec.X(), Vec.Y(), Vec.Z());
	}

	JPH_INLINE CustomType::Quaternion Convert(JPH::Quat Vec)
	{
		return CustomType::Quaternion(Vec.GetX(), Vec.GetY(), Vec.GetZ(), Vec.GetW());
	}

	JPH_INLINE JPH::Quat Convert(CustomType::Quaternion Vec)
	{
		return JPH::Quat(Vec.X(), Vec.Y(), Vec.Z(), Vec.W());
	}
}