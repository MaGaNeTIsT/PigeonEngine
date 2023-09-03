#include "ObjectBase.h"
#include <Config/EngineConfig.h>
#if _EDITOR_ONLY
#include "../../../../Development/Config/EngineDebugName.h"
#endif

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<EObjectBase, ERTTIObject>();
		RegisterClassType<EManagerBase, EObjectBase>();
		RegisterClassType<EUniqueIDManager, ERTTIObject>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EObjectBase::EObjectBase()
		: UniqueID(_AllocUniqueID())
#if _EDITOR_ONLY
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
	BOOL32 EObjectBase::operator==(const EObjectBase& obj)
	{
		return (GetUniqueID() == obj.GetUniqueID());
	}
	BOOL32 EObjectBase::operator!=(const EObjectBase& obj)
	{
		return (GetUniqueID() != obj.GetUniqueID());
	}
#if _EDITOR_ONLY
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