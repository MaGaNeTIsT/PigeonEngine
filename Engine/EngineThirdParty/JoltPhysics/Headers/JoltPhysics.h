#pragma once
#include "JoltIncludes.h"
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