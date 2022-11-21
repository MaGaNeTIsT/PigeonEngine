#include "../Headers/CPhysicsManager.h"

CPhysicsManager* CPhysicsManager::m_PhysicsManager = new CPhysicsManager();
void CPhysicsManager::Init()
{
	Physics->Init();
}

void CPhysicsManager::Tick(const float cDeltaTime)
{
	Physics->Tick(cDeltaTime);
}

CPhysicsManager::CPhysicsManager()
{
	Physics = new CPhysics_API_Jolt;
}

CPhysicsManager::~CPhysicsManager()
{
	delete Physics;
}