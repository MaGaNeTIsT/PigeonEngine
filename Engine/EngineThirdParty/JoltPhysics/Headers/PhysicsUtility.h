#pragma once
#include <CoreMinimal.h>
#include "JoltIncludes.h"

#define PHYSICSUTILITY_NAMESPACE_BEGIN namespace PhysicsUtility {
#define PHYSICSUTILITY_NAMESPACE_END }

#define PIGEONENGINE_NAMESPACE_BEGIN namespace PigeonEngine {
#define PIGEONENGINE_NAMESPACE_END }

PHYSICSUTILITY_NAMESPACE_BEGIN
typedef std::uint8_t UINT8;

enum class EActivate
{
	Activate,				///< Activate the body, making it part of the simulation
	DontActivate			///< Leave activation state as it is (will not deactivate an active body)
};

/// Motion type of a physics body
enum class EMotionType : UINT8
{
	Static,						///< Non movable
	Kinematic,					///< Movable using velocities only, does not respond to forces
	Dynamic,					///< Responds to forces as a normal physics object
};

static JPH::EMotionType GetMotionType(EMotionType inMotionType)
{
	switch (inMotionType)
	{
	case EMotionType::Static:
		return JPH::EMotionType::Static;
	case EMotionType::Dynamic:
		return JPH::EMotionType::Dynamic;
	case EMotionType::Kinematic:
		return JPH::EMotionType::Kinematic;
	default:
		return JPH::EMotionType::Static;
	}
}

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

JPH_INLINE BOOL32 HasTransformChanged(
	const Vector3& InPositionA,
	const Quaternion& InRotationA,
	const Vector3& InPositionB,
	const Quaternion& InRotationB,
	FLOAT InPositionTolerance = PE_KINDA_SMALL_NUMBER,
	FLOAT InRotationTolerance = PE_SMALL_NUMBER)
{
	if (Vector3::DistanceSquare(InPositionA, InPositionB) > InPositionTolerance * InPositionTolerance)
	{
		return TRUE;
	}

	const FLOAT RotationDot = EMath::Abs(InRotationA.x * InRotationB.x + InRotationA.y * InRotationB.y + InRotationA.z * InRotationB.z + InRotationA.w * InRotationB.w);
	return (1.0f - RotationDot) > InRotationTolerance;
}

JPH_INLINE float Convert2Meter(FLOAT centimeter)
{
	return centimeter * 0.01f;
}

PHYSICSUTILITY_NAMESPACE_END