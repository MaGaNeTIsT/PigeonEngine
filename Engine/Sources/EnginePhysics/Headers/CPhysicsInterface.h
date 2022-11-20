#pragma once

class CPhysicsInterface
{
public:
	virtual void Init() = 0;
	virtual void Tick(const float cDeltaTime) = 0;
};