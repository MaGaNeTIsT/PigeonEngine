#include "../../Headers/Game/CGameObjectManager.h"

ULONGLONG CGameObjectManager::m_GameObjectID = 1;
void CGameObjectManager::Init()
{
	m_GameObjectID = 1;
}
ULONGLONG CGameObjectManager::GetGameObjectID()
{
	ULONGLONG temp = m_GameObjectID;
	m_GameObjectID += 1;
	return temp;
}
CGameObjectManager::CGameObjectManager()
{
}
CGameObjectManager::~CGameObjectManager()
{
}