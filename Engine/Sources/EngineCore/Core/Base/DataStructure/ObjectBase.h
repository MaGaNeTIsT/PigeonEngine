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

	class EObjectBase : public ERTTIObject
	{
	public:
		const ObjectIdentityType& GetUniqueID()const;
	public:
		BOOL32 operator==(const EObjectBase& obj);
		BOOL32 operator!=(const EObjectBase& obj);
	private:
		ObjectIdentityType UniqueID;
#if _EDITOR_ONLY
	public:
		const EString&	GetDebugName()const;
		void			SetDebugName(const EString& InDebugName);
	protected:
		EString DebugName;
#endif

		CLASS_VIRTUAL_NOCOPY_BODY(EObjectBase)

	};

#if _EDITOR_ONLY

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
