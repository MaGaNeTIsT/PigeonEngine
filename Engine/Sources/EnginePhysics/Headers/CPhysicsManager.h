#pragma once

//DXProject includes
#include "../../../../Entry/EngineMain.h"
#include "CPhysics_APi_Jolt.h"


class CPhysicsManager
{
public:
	void Init();
	void Tick(const float cDeltaTime);
private:
	CPhysicsManager();
	virtual ~CPhysicsManager();

private:
	CPhysics_API_Jolt* Physics;

public://static funcs
	static CPhysicsManager* GetPhysicsManager() { return m_PhysicsManager; }
	static void Uninit() { delete m_PhysicsManager; }

private:
	static CPhysicsManager* m_PhysicsManager;
};