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