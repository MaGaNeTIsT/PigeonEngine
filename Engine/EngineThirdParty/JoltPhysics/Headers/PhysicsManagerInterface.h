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
    virtual void SetGravity(Vector3 InGravity)				{}
    virtual Vector3 GetGravity() const						{ return Vector3(0.f, -981.f, 0.f); }
};
PIGEONENGINE_NAMESPACE_END