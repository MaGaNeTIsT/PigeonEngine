#pragma once

class CUniqueIDManager
{
public:
	CUniqueIDManager();
	~CUniqueIDManager();
public:
	static void			Initialize();
	static ULONGLONG	GetUniqueID();
private:
	static ULONGLONG	m_UniqueID;
};

class CObjectBase
{
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