#include "../Headers/ObjectBase.h"

namespace PigeonEngine
{

	EObjectBase::EObjectBase()
		: m_UniqueID(_AllocUniqueID())
#ifdef _DEVELOPMENT_EDITOR
		, m_Name(ENGINE_DEFAULT_NAME)
#endif
	{
	}
	EObjectBase::~EObjectBase()
	{
	}
	const ULONGLONG& EObjectBase::GetUniqueID()const
	{
		return m_UniqueID;
	}
	BOOL EObjectBase::operator==(const EObjectBase& obj)
	{
		return (GetUniqueID() == obj.GetUniqueID());
	}
	BOOL EObjectBase::operator!=(const EObjectBase& obj)
	{
		return (GetUniqueID() != obj.GetUniqueID());
	}
#ifdef _DEVELOPMENT_EDITOR
	const std::string& EObjectBase::GetName()const
	{
		return m_Name;
	}
#endif

	EManagerBase::EManagerBase()
	{
	}
	EManagerBase::~EManagerBase()
	{
	}

	EUniqueIDManager::EUniqueIDManager() : m_AllocUniqueID(0u)
	{
#ifdef _DEVELOPMENT_EDITOR
		m_Name = ENGINE_UNIQUEID_MANAGER_NAME;
#endif
	}
	EUniqueIDManager::~EUniqueIDManager()
	{
	}
	ULONGLONG EUniqueIDManager::AllocUniqueID()
	{
		m_AllocUniqueID += 1u;
		return m_AllocUniqueID;
	}

	ULONGLONG _AllocUniqueID()
	{
		EUniqueIDManager* UniqueIDManager = EUniqueIDManager::GetManagerSingleton();
		return (UniqueIDManager->AllocUniqueID());
	}

};