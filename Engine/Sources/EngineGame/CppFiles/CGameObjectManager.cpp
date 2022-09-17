#include "../../../../Entry/EngineMain.h"
#include "../Headers/CGameObjectManager.h"

ULONGLONG CGameObjectManager::m_GameObjectID = 1u;
void CGameObjectManager::Initialize()
{
	m_GameObjectID = 1u;
}
ULONGLONG CGameObjectManager::GetGameObjectID()
{
	ULONGLONG temp = m_GameObjectID;
	m_GameObjectID += 1u;
	return temp;
}
CGameObjectManager::CGameObjectManager()
{
}
CGameObjectManager::~CGameObjectManager()
{
}