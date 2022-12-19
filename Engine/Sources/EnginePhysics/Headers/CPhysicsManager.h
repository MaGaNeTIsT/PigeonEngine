#pragma once

//DXProject includes
#include "../../../../Entry/EngineMain.h"
#include "CPhysics_API_Jolt.h"

typedef CPhysics_API_Jolt CPhysics;

class CPhysicsManager : public CPhysics
{
public:
	using CPhysics::CreateAndAddBody;
	using CPhysics::CreateBodyCreationSettings;
public:
	void Init() override;
	void Uninit() { Release(); };
	void Tick(const float cDeltaTime) override;
private:
	CPhysicsManager();
	virtual ~CPhysicsManager();
//static funcs
private:
	static CPhysicsManager* m_PhysicsManager;
	static void Release() { delete m_PhysicsManager; }
public:
	static CPhysicsManager* GetPhysicsManager() { return m_PhysicsManager; }
};