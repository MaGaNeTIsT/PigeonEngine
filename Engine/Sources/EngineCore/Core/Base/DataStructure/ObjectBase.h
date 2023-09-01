#pragma once

#include "../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/BaseType.h>
#include <Base/TemplateHeaders.h>
#include <Base/DataStructure/Text/String.h>
#include <Base/RTTI/RTTIObject.h>

namespace PigeonEngine
{

	class EObjectBase : public ERTTIObject
	{
	public:
		const ULONGLONG& GetUniqueID()const;
	public:
		BOOL32 operator==(const EObjectBase& obj);
		BOOL32 operator!=(const EObjectBase& obj);
	private:
		ULONGLONG UniqueID;
#if _EDITOR_ONLY
	public:
		const EString& GetDebugName()const;
	protected:
		EString DebugName;
#endif

		CLASS_VIRTUAL_NOCOPY_BODY(EObjectBase)

	};

	class EManagerBase : public EObjectBase
	{
	public:
		virtual void Initialize() {}
		virtual void ShutDown() {}

		CLASS_VIRTUAL_NOCOPY_BODY(EManagerBase)

	};

	class EUniqueIDManager : public ERTTIObject
	{
	public:
		ULONGLONG AllocUniqueID();
	private:
		ULONGLONG UsedAllocUniqueID;

		CLASS_MANAGER_SINGLETON_BODY(EUniqueIDManager)

	};

	extern ULONGLONG _AllocUniqueID();

};