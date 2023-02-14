#pragma once

#include "./CEMain.h"
#include "../../Base/Headers/CEBaseType.h"
#include "../../Base/Headers/CETimer.h"

namespace PigeonEngine
{
	class CEManager
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
#ifdef _DEVELOPMENT_EDITOR
	public:
		static void		EditorUpdate();
	private:
		BOOL			m_EditorOpen;
#endif
	public:
		static HWND								GetWindowHandle();
		static const CEBaseTimer&				GetWindowTimer();
		static const CEGameTimer*				GetGameTimer();
		static const class CRenderPipeline*		GetRenderPipeline();
		static const class CScene*				GetScene();
	private:
		HWND						m_HWND;
		Vector2Int					m_WindowSize;
		UINT						m_GraphicDepth;
		UINT						m_FrameRate;
		CEBaseTimer					m_WindowTimer;
		BOOL						m_Windowed;
		CEGameTimer*				m_GameTimer;
		class CRenderPipeline*		m_RenderPipeline;
		class CScene*				m_Scene;
	private:
		CEManager();
		~CEManager();
	private:
		static CEManager*			m_EngineManager;
	public:
		static LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	};
};