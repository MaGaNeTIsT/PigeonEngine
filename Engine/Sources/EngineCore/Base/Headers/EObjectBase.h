#pragma once

#include "../../Core/Headers/EMain.h"
#include "./ERTTI.h"

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

class CObjectBase : public ERTTI
{
	CClass(CObjectBase, ERTTI);
public:
	CObjectBase();
	CObjectBase(const CObjectBase& obj);
	virtual ~CObjectBase();
public:
	const ULONGLONG& GetUniqueID()const;
public:
	BOOL operator==(const CObjectBase& obj);
	BOOL operator!=(const CObjectBase& obj);
protected:
	ULONGLONG m_UniqueID;
};