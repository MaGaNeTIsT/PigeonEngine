#pragma once

#include "../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/BaseType.h>
#include <Base/TemplateHeaders.h>
#include <Base/DataStructure/Text/String.h>
#include <Base/RTTI/RTTIObject.h>

namespace PigeonEngine
{

	typedef ULONGLONG	ObjectIdentityType;

	/*
	* Stable, generational reference to an engine object. A handle stores identity only:
	* resolving it goes through the world's registries and fails once the object is gone,
	* which is what makes cross-object references (a component reading another component
	* across actors) safe under deferred destruction. The generation guards against
	* future ID reuse; IDs are currently never reused, so it is currently 1 for every
	* live object.
	*/
	struct FObjectHandle
	{
	public:
		ObjectIdentityType	UniqueID = 0u;
		UINT32				Generation = 0u;

		PE_NODISCARD BOOL32 IsValid() const { return (UniqueID != 0u) ? TRUE : FALSE; }
		BOOL32 operator==(const FObjectHandle& Other) const
		{
			return ((UniqueID == Other.UniqueID) && (Generation == Other.Generation)) ? TRUE : FALSE;
		}
		BOOL32 operator!=(const FObjectHandle& Other) const
		{
			return (!((*this) == Other)) ? TRUE : FALSE;
		}
	};

	class EObjectBase : public ERTTIObject
	{
	public:
		const ObjectIdentityType& GetUniqueID() const;
		// Generation of this object's identity. Paired with the ID in FObjectHandle so a
		// reference stays invalid if IDs are ever recycled.
		PE_NODISCARD UINT32 GetGeneration() const;
	public:
		BOOL32 operator==(const EObjectBase& obj);
		BOOL32 operator!=(const EObjectBase& obj);
#if ENGINE_DEBUG_MODE
	public:
		// Live EObjectBase instance count, for leak checks in the framework sanity
		// module. Debug builds only: a plain counter, not atomic.
		static UINT64 GetAliveObjectCount();
#endif
	private:
		ObjectIdentityType UniqueID;
		UINT32 Generation = 1u;
#if ENGINE_EDITOR_ONLY
	public:
		const EString&	GetDebugName() const;
		void			SetDebugName(const EString& InDebugName);
	protected:
		EString DebugName;
#endif

		CLASS_VIRTUAL_NOCOPY_BODY(EObjectBase)

	};

	PE_NODISCARD inline FObjectHandle MakeObjectHandle(const EObjectBase* InObject)
	{
		FObjectHandle Result;
		if (InObject)
		{
			Result.UniqueID = InObject->GetUniqueID();
			Result.Generation = InObject->GetGeneration();
		}
		return Result;
	}

#if ENGINE_EDITOR_ONLY

#define POBJ_DEBUGNAME_GET(__Ptr) \
	(__Ptr)->GetDebugName()\

#define POBJ_DEBUGNAME_SET(__Ptr, __Name) \
	(__Ptr)->SetDebugName((__Name))\

#else
#define POBJ_DEBUGNAME_GET(__Ptr)			("")
#define POBJ_DEBUGNAME_SET(__Ptr, __Name)	{}
#endif

	/*
	* Manager lifecycle. The four hooks are two different frequencies and must not be mixed:
	*   Initialize / ShutDown   engine lifetime, called once each by EMainManager.
	*   Init / Uninit           repeatable: an Uninit + Init pair re-initializes a manager
	*                           without restarting the engine.
	* A manager with nothing to do per run leaves Init / Uninit empty. The pair exists so that
	* per run work never ends up in Initialize, which is what would break the "once" contract
	* the engine relies on.
	*/
	class EManagerBase : public EObjectBase
	{
	public:
		virtual void Initialize() {}
		virtual void ShutDown() {}
		virtual void Init() {}
		virtual void Uninit() {}

		CLASS_VIRTUAL_NOCOPY_BODY(EManagerBase)

	};

	class EUniqueIDManager : public ERTTIObject
	{
	public:
		ObjectIdentityType AllocUniqueID();
	private:
		ObjectIdentityType UsedAllocUniqueID;

		CLASS_MANAGER_SINGLETON_BODY(EUniqueIDManager)

	};

	extern ObjectIdentityType _AllocUniqueID();

};
