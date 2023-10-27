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

	public:
		void            EditorInit();
		void            EditorUpdate();
	public:
		HWND						GetWindowHandle();
		const EBaseTimer&			GetWindowTimer();
		const EGameTimer*			GetGameTimer();
	private:
		HWND						m_HWND;
		Vector2Int					m_WindowSize;
		UINT32						m_GraphicDepth;
		UINT32						m_FrameRate;
		EBaseTimer					m_WindowTimer;
		BOOL32						m_Windowed;
		EGameTimer*					m_GameTimer			= nullptr;
	private:
		class FPhysicsManager*				m_PhysicsManager				= nullptr;

		// Render region START
		class RDeviceD3D11*					m_RenderDeviceD3D11				= nullptr;
		class RScene*						RenderScene						= nullptr;
		class RSceneRenderer*				SceneRenderer					= nullptr;
		// Render region END

#if _EDITOR_ONLY
		class CImGUIManager*				m_ImGUIManager					= nullptr;
		class CAssimpManager*				m_AssimpManager					= nullptr;
		class EEditorManager*               m_EditorManager					= nullptr;
#endif
		class EClassTypeRegisterManager*	m_ClassTypeRegisterManager		= nullptr;
		class EClassFactoryRegisterManager*	m_ClassFactoryRegisterManager	= nullptr;

		class EWorldManager*                m_WorldManager					= nullptr;
		
	public:
		static LRESULT HandleMsg(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam);

		CLASS_MANAGER_SINGLETON_BODY(EMainManager)

	};
};