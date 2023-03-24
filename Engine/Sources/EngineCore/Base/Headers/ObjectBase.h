#pragma once

#include "../../Core/Headers/Main.h"
#include "RTTI.h"

class EUniqueIDAllocater
{
public:
	static ULONGLONG	GetUniqueID();
public:
	static void			Initialize();
	static void			ShutDown();
private:
	EUniqueIDAllocater();
	~EUniqueIDAllocater();
private:
	static EUniqueIDAllocater*		m_UniqueIDAllocater;
	ULONGLONG						m_UniqueID;
};

class EObjectBase : public ERTTI
{
	EClass(EObjectBase, ERTTI);
public:
	EObjectBase();
	EObjectBase(const EObjectBase& obj);
	virtual ~EObjectBase();
public:
	const ULONGLONG& GetUniqueID()const { return m_UniqueID; }
public:
	BOOL operator==(const EObjectBase& obj);
	BOOL operator!=(const EObjectBase& obj);
protected:
	ULONGLONG m_UniqueID;
#ifdef _DEVELOPMENT_EDITOR
public:
	const std::string& GetName()const { return m_Name; }
protected:
	std::string m_Name;
#endif
};