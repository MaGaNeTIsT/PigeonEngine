#include "../../../../Entry/EngineMain.h"
#include "../Headers/CObjectManager.h"

ULONGLONG CUniqueIDManager::m_UniqueID = 1u;
void CUniqueIDManager::Initialize()
{
	m_UniqueID = 1u;
}
ULONGLONG CUniqueIDManager::GetUniqueID()
{
	ULONGLONG temp = m_UniqueID;
	m_UniqueID += 1u;
	return temp;
}
CUniqueIDManager::CUniqueIDManager()
{
}
CUniqueIDManager::~CUniqueIDManager()
{
}