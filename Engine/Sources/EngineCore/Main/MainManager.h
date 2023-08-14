#pragma once

#include "Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/BaseType.h>
#include <Base/Timer/Timer.h>
#include <Base/DataStructure/ObjectBase.h>

namespace PigeonEngine
{
	class EMainManager : public EManagerBase
	{

		EClass(EMainManager, EManagerBase)

	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
		void			SetInitializer(HWND hWnd);
		void			UpdateManager();
	public:
		void			Init();
		void			Uninit();
		void			Update();
		void			FixedUpdate();
		void			Draw();
#ifdef _EDITOR_ONLY
	public:
		void			EditorUpdate();
	private:
		BOOL			m_EditorOpen = FALSE;
#endif
	public:
		HWND						GetWindowHandle();
		const EBaseTimer&			GetWindowTimer();
		const EGameTimer*			GetGameTimer();
	private:
		HWND						m_HWND;
		Vector2Int					m_WindowSize;
		UINT						m_GraphicDepth;
		UINT						m_FrameRate;
		EBaseTimer					m_WindowTimer;
		BOOL						m_Windowed;
		EGameTimer*					m_GameTimer			= nullptr;
	private:
		class CPhysicsManager*		m_PhysicsManager	= nullptr;
		class RDeviceD3D11*			m_RenderDeviceD3D11	= nullptr;
#ifdef _EDITOR_ONLY
		class CImGUIManager*		m_ImGUIManager		= nullptr;
		class CAssimpManager*		m_AssimpManager		= nullptr;
#endif
	public:
		static LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		CLASS_MANAGER_SINGLETON_BODY(EMainManager)

	};
};