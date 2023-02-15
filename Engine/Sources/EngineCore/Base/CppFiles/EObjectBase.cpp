#include "../Headers/EObjectBase.h"

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