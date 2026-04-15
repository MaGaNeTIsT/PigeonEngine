#pragma once

PIGEONENGINE_NAMESPACE_BEGIN
class FPhysicsManagerInterface
{
public:
	FPhysicsManagerInterface()								{}
	virtual ~FPhysicsManagerInterface() = default;
public:
	virtual void InitPhysics()								{}
	virtual void UninitPhysics()							{}
	virtual void PrePhysicsUpdate(FLOAT DeltaTime)			{}
	virtual void PhysicsUpdate(FLOAT DeltaTime)				{}
	virtual void PostPhysicsUpdate(FLOAT DeltaTime)			{}
};
PIGEONENGINE_NAMESPACE_END