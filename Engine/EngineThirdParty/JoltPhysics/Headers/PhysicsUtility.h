#pragma once
#include <CoreMinimal.h>
#include <Jolt/Jolt.h>

#ifndef ENABLE_JOLTPHYSICS
#define ENABLE_JOLTPHYSICS 1
#endif

#ifdef ENABLE_JOLTPHYSICS
//Jolt define
//#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
//#define JPH_USE_SSE4_1
//#define JPH_USE_SSE4_2
//#define JPH_USE_LZCNT
//#define JPH_USE_TZCNT
//#define JPH_USE_F16C
//#define JPH_USE_FMADD
//#define NDEBUG
#define JPH_PROFILE_ENABLED
#define JPH_DEBUG_RENDERER
#endif

namespace PhysicsUtility
{
	using namespace PigeonEngine;
	JPH_INLINE Vector3 Convert(JPH::Vec3 Vec)
	{
		return Vector3(Vec.GetX(), Vec.GetY(), Vec.GetZ());
	}

	JPH_INLINE Vector3 Convert2Centimeter(JPH::Vec3 Vec)
	{
		return Vector3(Vec.GetX(), Vec.GetY(), Vec.GetZ()) * 100.f;
	}

	JPH_INLINE JPH::Vec3 Convert(Vector3 Vec)
	{
		return JPH::Vec3(Vec.x, Vec.y, Vec.z);
	}

	JPH_INLINE JPH::Vec3 Convert2Meter(Vector3 Vec)
	{
		return JPH::Vec3(Vec.x, Vec.y, Vec.z) * 0.01f;
	}

	JPH_INLINE Quaternion Convert(JPH::Quat Vec)
	{
		return Quaternion(Vec.GetX(), Vec.GetY(), Vec.GetZ(), Vec.GetW());
	}

	JPH_INLINE JPH::Quat Convert(Quaternion Vec)
	{
		return JPH::Quat(Vec.x, Vec.y, Vec.z, Vec.w);
	}
}