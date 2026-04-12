#pragma once

PIGEONENGINE_NAMESPACE_BEGIN
class FPhysicsManagerInterface
{
public:
	FPhysicsManagerInterface()					{}
	virtual ~FPhysicsManagerInterface()			= default;
public:
	virtual void InitPhysics()					{}
	virtual void UninitPhysics()				{}
	virtual void PrePhysicsUpdate()				{}
	virtual void PhysicsUpdate()				{}
	virtual void PostPhysicsUpdate()			{}
};
PIGEONENGINE_NAMESPACE_END