#pragma once

class CGameObjectManager
{
public:
	CGameObjectManager();
	~CGameObjectManager();
public:
	static void			Init();
	static ULONGLONG	GetGameObjectID();
private:
	static ULONGLONG	m_GameObjectID;
};