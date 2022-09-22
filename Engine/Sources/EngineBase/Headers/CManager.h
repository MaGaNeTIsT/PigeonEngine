#pragma once

#include "./CTimer.h"
#include "./CBaseType.h"

class CManager
{
public:
	static void		Initialize(HWND hWnd);
	static void		ShutDown();
	static void		StaticUpdate();
public:
	static void		Init();
	static void		Uninit();
	static void		Update();
	static void		FixedUpdate();
	static void		Draw();
public:
	static void				CalculateFrameStats();
	static HWND				GetWindowHandle();
	static const CTimer&	GetWindowTimer();
public:
	static const class CRenderPipeline*	GetRenderPipeline();
	static const class CGameTimer*		GetGameTimer();
	static const class CScene*			GetScene();
private:
	HWND					m_HWND;
	CustomType::Vector2Int	m_WindowSize;
	UINT					m_GraphicDepth;
	UINT					m_FrameRate;
	CTimer					m_WindowTimer;
	BOOL					m_Windowed;
	class CGameTimer*		m_GameTimer			= NULL;
	class CRenderPipeline*	m_RenderPipeline	= NULL;
	class CScene*			m_Scene				= NULL;
private:
	CManager();
	CManager(const CManager& manager);
	~CManager();
private:
	static CManager* m_Manager;

public:
	static LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

};