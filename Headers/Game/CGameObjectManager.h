#pragma once

#include "../../Entry/MyMain.h"

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