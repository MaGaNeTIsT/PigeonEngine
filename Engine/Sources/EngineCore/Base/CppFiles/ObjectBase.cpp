#include "../Headers/ObjectBase.h"

EUniqueIDAllocater* EUniqueIDAllocater::m_UniqueIDAllocater = new EUniqueIDAllocater();
EUniqueIDAllocater::EUniqueIDAllocater()
{
	this->m_UniqueID = 1u;
}
EUniqueIDAllocater::~EUniqueIDAllocater()
{

}
void EUniqueIDAllocater::Initialize()
{
	EUniqueIDAllocater::m_UniqueIDAllocater->m_UniqueID = 1u;
}
ULONGLONG EUniqueIDAllocater::GetUniqueID()
{
	ULONGLONG temp = EUniqueIDAllocater::m_UniqueIDAllocater->m_UniqueID;
	EUniqueIDAllocater::m_UniqueIDAllocater->m_UniqueID += 1u;
	return temp;
}
void EUniqueIDAllocater::ShutDown()
{
	if (EUniqueIDAllocater::m_UniqueIDAllocater != nullptr)
	{
		delete (EUniqueIDAllocater::m_UniqueIDAllocater);
		EUniqueIDAllocater::m_UniqueIDAllocater = nullptr;
	}
}



EObjectBase::EObjectBase()
	: m_UniqueID(EUniqueIDAllocater::GetUniqueID())
#ifdef _DEVELOPMENT_EDITOR
	, m_Name(ENGINE_DEFAULT_NAME)
#endif
{
}
EObjectBase::EObjectBase(const EObjectBase& obj)
	: m_UniqueID(EUniqueIDAllocater::GetUniqueID())
#ifdef _DEVELOPMENT_EDITOR
	, m_Name(obj.GetName())
#endif
{
}
EObjectBase::~EObjectBase()
{
}
BOOL EObjectBase::operator==(const EObjectBase& obj)
{
	return (this->GetUniqueID() == obj.GetUniqueID());
}
BOOL EObjectBase::operator!=(const EObjectBase& obj)
{
	return (this->GetUniqueID() != obj.GetUniqueID());
}