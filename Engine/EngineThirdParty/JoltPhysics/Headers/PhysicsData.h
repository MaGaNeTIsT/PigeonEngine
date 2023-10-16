#pragma once
#include <CoreMinimal.h>
#include "JoltIncludes.h"
#include "../../../Sources/EngineCore/PhysicsCore/PhysicsConfig/PhysicsConfig.h"

PIGEONENGINE_NAMESPACE_BEGIN
using namespace JPH;
struct FPhysicsData
{
	//Physics Parameters
	PhysicsSystem* PhysicsSystem;
	BodyInterface* BodyInterface;
	TempAllocatorImpl* TempAllocator;
	JobSystemThreadPool* JobSystem;
	PigeonEngine::CBPLayerInterfaceImpl* BPLayerInterface;
	PigeonEngine::CObjectLayerPairFilterImpl* ObjectLayerPairFilterImpl;
	PigeonEngine::CObjectVsBroadPhaseLayerFilterImpl* ObjectVsBroadPhaseLayerFilterImpl;
	PigeonEngine::FBodyActivationListener* BodyActivationListener;
	PigeonEngine::FContactListener* ContactListener;
};
PIGEONENGINE_NAMESPACE_END