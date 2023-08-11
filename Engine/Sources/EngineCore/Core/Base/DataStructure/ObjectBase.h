#pragma once

#include "../../../Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/BaseType.h>
#include <Base/TemplateHeaders.h>
#include <Base/DataStructure/Text/String.h>
#include <Base/RTTI.h>

namespace PigeonEngine
{

	class EObjectBase : public ERTTI
	{
		
		EClass(EObjectBase, ERTTI)

	public:
		const ULONGLONG& GetUniqueID()const;
	public:
		BOOL operator==(const EObjectBase& obj);
		BOOL operator!=(const EObjectBase& obj);
	private:
		ULONGLONG UniqueID;
#ifdef _EDITOR_ONLY
	public:
		const EString& GetDebugName()const;
	protected:
		EString DebugName;
#endif

		CLASS_VIRTUAL_NOCOPY_BODY(EObjectBase)

	};

	class EManagerBase : public EObjectBase
	{

		EClass(EManagerBase, EObjectBase)

	public:
		virtual void Initialize() {}
		virtual void ShutDown() {}

		CLASS_VIRTUAL_NOCOPY_BODY(EManagerBase)

	};

	class EUniqueIDManager : public EManagerBase
	{

		EClass(EUniqueIDManager, EManagerBase)

	public:
		ULONGLONG AllocUniqueID();
	private:
		ULONGLONG UsedAllocUniqueID;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EUniqueIDManager)

	};

	extern ULONGLONG _AllocUniqueID();

};