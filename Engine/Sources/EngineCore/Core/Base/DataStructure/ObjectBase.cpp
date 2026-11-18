#include "ObjectBase.h"
#include <Base/RTTI/RTTIManager.h>
#include <Config/EngineConfig.h>
#if ENGINE_EDITOR_ONLY
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

#if ENGINE_DEBUG_MODE
	// File-local instance counter backing EObjectBase::GetAliveObjectCount. Plain and
	// non-atomic on purpose: engine objects are constructed and destroyed on the game
	// thread (render-side proxies are not EObjectBase).
	static UINT64 AliveObjectCount = 0u;
#endif

	EObjectBase::EObjectBase()
		: ERTTIObject()
		, UniqueID(_AllocUniqueID())
#if ENGINE_EDITOR_ONLY
		, DebugName(EEngineSettings::ENGINE_DEFAULT_NAME)
#endif
	{
#if ENGINE_DEBUG_MODE
		AliveObjectCount += 1u;
#endif
	}
	EObjectBase::~EObjectBase()
	{
#if ENGINE_DEBUG_MODE
		AliveObjectCount -= 1u;
#endif
	}
	const ObjectIdentityType& EObjectBase::GetUniqueID() const
	{
		return UniqueID;
	}
	UINT32 EObjectBase::GetGeneration() const
	{
		// IDs are never reused in this engine, so a fresh object always starts at
		// generation 1; the field exists so handle checks stay meaningful if recycling is
		// ever introduced.
		return Generation;
	}
	BOOL32 EObjectBase::operator==(const EObjectBase& obj)
	{
		return (GetUniqueID() == obj.GetUniqueID());
	}
	BOOL32 EObjectBase::operator!=(const EObjectBase& obj)
	{
		return (GetUniqueID() != obj.GetUniqueID());
	}
#if ENGINE_DEBUG_MODE
	UINT64 EObjectBase::GetAliveObjectCount()
	{
		return AliveObjectCount;
	}
#endif
#if ENGINE_EDITOR_ONLY
	const EString& EObjectBase::GetDebugName() const
	{
		return DebugName;
	}
	void EObjectBase::SetDebugName(const EString& InDebugName)
	{
		DebugName = InDebugName;
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
	ObjectIdentityType EUniqueIDManager::AllocUniqueID()
	{
		UsedAllocUniqueID += 1u;
		return UsedAllocUniqueID;
	}

	ObjectIdentityType _AllocUniqueID()
	{
		EUniqueIDManager* UniqueIDManager = EUniqueIDManager::GetManagerSingleton();
		return (UniqueIDManager->AllocUniqueID());
	}

};
