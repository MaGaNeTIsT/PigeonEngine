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

CObjectBase::CObjectBase()
{
	this->m_UniqueID = CUniqueIDManager::GetUniqueID();
}
CObjectBase::CObjectBase(const CObjectBase& obj)
{
	this->m_UniqueID = CUniqueIDManager::GetUniqueID();
}
CObjectBase::~CObjectBase()
{
}
const ULONGLONG& CObjectBase::GetUniqueID()const
{
	return (this->m_UniqueID);
}
BOOL CObjectBase::operator==(const CObjectBase& obj)
{
	return (this->m_UniqueID == obj.m_UniqueID);
}
BOOL CObjectBase::operator!=(const CObjectBase& obj)
{
	return (this->m_UniqueID != obj.m_UniqueID);
}