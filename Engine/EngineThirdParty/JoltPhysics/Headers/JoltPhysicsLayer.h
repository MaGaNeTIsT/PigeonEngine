#pragma once
#include <CoreMinimal.h>
#include "PhysicsUtility.h"
#include "JoltIncludes.h"

PIGEONENGINE_NAMESPACE_BEGIN
// All Jolt symbols are in the JPH namespace
using namespace JPH;

namespace Layers
{
	static constexpr UINT8 NON_MOVING = 0;
	static constexpr UINT8 MOVING = 1;
	static constexpr UINT8 NUM_LAYERS = 2;
}

namespace BroadPhaseLayers
{
	static constexpr UINT8 NON_MOVING(0);
	static constexpr UINT8 MOVING(1);
	static constexpr UINT8 NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class CBPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
	CBPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayer(BroadPhaseLayers::NON_MOVING);
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayer(BroadPhaseLayers::MOVING);
	}

	virtual uint					GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual BroadPhaseLayer			GetBroadPhaseLayer(ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[(INT32)inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const
	{
		switch ((BroadPhaseLayer::Type)inLayer)
		{
		case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:													JPH_ASSERT(FALSE); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	BroadPhaseLayer					mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class CObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayer(BroadPhaseLayers::MOVING);
		case Layers::MOVING:
			return TRUE;
		default:
			JPH_ASSERT(FALSE);
			return FALSE;
		}
	}
};

/// Class that determines if two object layers can collide
class CObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
	virtual bool					ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return TRUE; // Moving collides with everything
		default:
			JPH_ASSERT(FALSE);
			return FALSE;
		}
	}
};
PIGEONENGINE_NAMESPACE_END