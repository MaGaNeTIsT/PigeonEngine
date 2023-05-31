#include "ObjectBase.h"

namespace PigeonEngine
{

	EObjectBase::EObjectBase()
		: UniqueID(_AllocUniqueID())
#ifdef _EDITOR_ONLY
		, DebugName(ENGINE_DEFAULT_NAME)
#endif
	{
	}
	EObjectBase::~EObjectBase()
	{
	}
	const ULONGLONG& EObjectBase::GetUniqueID()const
	{
		return UniqueID;
	}
	BOOL EObjectBase::operator==(const EObjectBase& obj)
	{
		return (GetUniqueID() == obj.GetUniqueID());
	}
	BOOL EObjectBase::operator!=(const EObjectBase& obj)
	{
		return (GetUniqueID() != obj.GetUniqueID());
	}
#ifdef _EDITOR_ONLY
	const EString& EObjectBase::GetDebugName()const
	{
		return DebugName;
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
#ifdef _EDITOR_ONLY
		DebugName = ENGINE_UNIQUEID_MANAGER_NAME;
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