#pragma once

#include "Main.h"
#include "../../Base/Headers/BaseType.h"
#include "../../Base/Headers/Timer.h"

namespace PigeonEngine
{
	class EManager
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
		static const EBaseTimer&				GetWindowTimer();
		static const EGameTimer*				GetGameTimer();
		static const class CRenderPipeline*		GetRenderPipeline();
		static const class CScene*				GetScene();
	private:
		HWND						m_HWND;
		Vector2Int					m_WindowSize;
		UINT						m_GraphicDepth;
		UINT						m_FrameRate;
		EBaseTimer					m_WindowTimer;
		BOOL						m_Windowed;
		EGameTimer*					m_GameTimer;
		class CRenderPipeline*		m_RenderPipeline;
		class CScene*				m_Scene;
	private:
		EManager();
		~EManager();
	private:
		static EManager*			m_EngineManager;
	public:
		static LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	};
};