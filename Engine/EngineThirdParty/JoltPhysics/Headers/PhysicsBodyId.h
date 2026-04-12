#pragma once
#pragma once
#include <CoreMinimal.h>
#include "PhysicsUtility.h"
#include "JoltIncludes.h"

PIGEONENGINE_NAMESPACE_BEGIN
using JPH::BodyID;

struct FPhysicsBodyId
{
	BodyID ID;

	inline BOOL32 operator == (const FPhysicsBodyId& inRHS) const
	{
		return ID == inRHS.ID;
	}

	inline BOOL32 operator != (const FPhysicsBodyId& inRHS) const
	{
		return ID != inRHS.ID;
	}

	inline BOOL32 operator < (const FPhysicsBodyId& inRHS) const
	{
		return ID < inRHS.ID;
	}

	inline BOOL32 operator > (const FPhysicsBodyId& inRHS) const
	{
		return ID > inRHS.ID;
	}

	inline UINT32 GetIndex() const
	{
		return ID.GetIndex();
	}

	inline UINT8 GetSequenceNumber() const
	{
		return ID.GetSequenceNumber();
	}

	inline UINT32 GetIndexAndSequenceNumber() const
	{
		return ID.GetIndexAndSequenceNumber();
	}

	inline BOOL32 IsInvalid() const
	{
		return ID.IsInvalid();
	}
};

PIGEONENGINE_NAMESPACE_END
