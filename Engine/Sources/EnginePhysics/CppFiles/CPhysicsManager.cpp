#include "../Headers/CPhysicsManager.h"

CPhysicsManager* CPhysicsManager::m_PhysicsManager = new CPhysicsManager();
void CPhysicsManager::Init()
{
	CPhysics::Init();
}

void CPhysicsManager::Tick(const float cDeltaTime)
{
	CPhysics::Tick(cDeltaTime);
}

CPhysicsManager::CPhysicsManager()
{

}

CPhysicsManager::~CPhysicsManager()
{

}