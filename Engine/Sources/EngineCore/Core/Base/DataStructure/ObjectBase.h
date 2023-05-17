#pragma once


#include <CoreMinimal.h>
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
		ULONGLONG m_UniqueID;
#ifdef _DEVELOPMENT_EDITOR
	public:
		const std::string& GetName()const;
	protected:
		std::string m_Name;
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
		ULONGLONG m_AllocUniqueID;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EUniqueIDManager)

	};

	extern ULONGLONG _AllocUniqueID();

};