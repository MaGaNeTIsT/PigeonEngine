#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "PhysicsManagerInterface.h"

#ifndef ENABLE_JOLTPHYSICS
#define ENABLE_JOLTPHYSICS 1
#endif

namespace Jolt
{
	class CPhysics_Jolt : public PigeonEngine::IPhysicsManagerInterface
	{
	public:
		CPhysics_Jolt() {}
		~CPhysics_Jolt() {}
	};
};