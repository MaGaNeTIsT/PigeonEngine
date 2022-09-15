#pragma once

#include "./CTimer.h"
#include "./CBaseType.h"

class CScene;
class CGameTimer;

class CManager
{
public:
	static CManager*	GetManager() { return m_Manager; }
	static void			Initialize(HWND hWnd);
	static void			ShutDown();
	static void			StaticUpdate();
public:
	void				Init();
	void				Uninit();
	void				Update();
	void				FixedUpdate();
	void				Draw();

	void				CalculateFrameStats();
public:
	HWND				GetWindowHandle() { return this->m_HWND; }
	const CTimer&		GetWindowTimer()const { return (this->m_WindowTimer); }
	const CScene*		GetScene()const { return this->m_Scene; }
	const CGameTimer*	GetGameTimer()const { return (this->m_GameTimer); }
private:
	HWND					m_HWND;
	CustomType::Vector2Int	m_WindowSize;
	UINT					m_GraphicDepth;
	UINT					m_FrameRate;
	CTimer					m_WindowTimer;
	BOOL					m_Windowed;
	CGameTimer*				m_GameTimer	= NULL;
	CScene*					m_Scene		= NULL;
private:
	CManager();
	CManager(const CManager& manager);
	~CManager();
private:
	static CManager*	m_Manager;
};