#pragma once
#include <CoreMinimal.h>
#include "JoltIncludes.h"
#include "JoltPhysicsLayer.h"
#include "JoltPhysicsListener.h"
#include "../../../Sources/EngineCore/PhysicsCore/PhysicsConfig/PhysicsConfig.h"

PIGEONENGINE_NAMESPACE_BEGIN
using JPH::BodyInterface;
using JPH::JobSystemThreadPool;
using JPH::PhysicsSystem;
using JPH::TempAllocatorImpl;

struct FPhysicsData
{
	//Physics Parameters
	PhysicsSystem* PhysicsSystem;
	BodyInterface* BodyInterface;
	TempAllocatorImpl* TempAllocator;
	JobSystemThreadPool* JobSystem;
	CBPLayerInterfaceImpl* BPLayerInterface;
	CObjectLayerPairFilterImpl* ObjectLayerPairFilterImpl;
	CObjectVsBroadPhaseLayerFilterImpl* ObjectVsBroadPhaseLayerFilterImpl;
	FBodyActivationListener* BodyActivationListener;
	FContactListener* ContactListener;
	FSoftBodyContactListener* SoftBodyContactListener;
};
PIGEONENGINE_NAMESPACE_END