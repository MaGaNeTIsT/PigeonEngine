#include "ObjectBase.h"
#include <Config/EngineConfig.h>
#ifdef _EDITOR_ONLY
#include "../../../../Development/Config/EngineDebugName.h"
#endif

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

	EUniqueIDManager::EUniqueIDManager() : UsedAllocUniqueID(0u)
	{
	}
	EUniqueIDManager::~EUniqueIDManager()
	{
	}
	ULONGLONG EUniqueIDManager::AllocUniqueID()
	{
		UsedAllocUniqueID += 1u;
		return UsedAllocUniqueID;
	}

	ULONGLONG _AllocUniqueID()
	{
		EUniqueIDManager* UniqueIDManager = EUniqueIDManager::GetManagerSingleton();
		return (UniqueIDManager->AllocUniqueID());
	}

};