#pragma once
#include "Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/BaseType.h>
#include <Base/Timer/Timer.h>
#include <Base/DataStructure/ObjectBase.h>

namespace PigeonEngine
{
	struct RResolutionItem
	{
		UINT32 Width;
		UINT32 Height;
		BOOL32 operator==(const RResolutionItem& InOther) const
		{
			return (Width == InOther.Width && Height == InOther.Height);
		}
	};

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
#if _EDITOR_ONLY
		void            EditorInit();
		void            EditorUpdate();
#endif

	public:
		HWND						GetWindowHandle();
		const Vector2Int&			GetWindowSize() const;
		const EBaseTimer&			GetWindowTimer();
		const EGameTimer*			GetGameTimer();

	public:
		void							GatherSupportedResolutions();
		void							ApplyResolution(UINT32 InWidth, UINT32 InHeight, BOOL32 InIsFullscreen);
		void							OnWindowResized(UINT32 InNewWidth, UINT32 InNewHeight);
		const TArray<RResolutionItem>&	GetSupportedResolutions() const;
		BOOL32							GetIsFullscreen() const;

	private:
		HWND						m_HWND;
		Vector2Int					m_WindowSize;
		UINT32						m_GraphicDepth;
		UINT32						m_FrameRate;
		EBaseTimer					m_WindowTimer;
		BOOL32						m_Windowed;
		EGameTimer*					m_GameTimer			= nullptr;
	private:
		TArray<RResolutionItem>		m_SupportedResolutions;
		BOOL32						m_bPendingResize      = FALSE;
		UINT32						m_PendingResizeWidth  = 0u;
		UINT32						m_PendingResizeHeight = 0u;
		BOOL32						m_bIsFullscreen       = FALSE;
	private:
		class FPhysicsManager*				m_PhysicsManager				= nullptr;

		// Render region START
		class IRRHIDevice*					RenderDevice					= nullptr;
		class RScene*						RenderScene						= nullptr;
		class RSceneRenderer*				SceneRenderer					= nullptr;
		class RRenderThread*				RenderThread					= nullptr;
		// Render region END

#if _EDITOR_ONLY
		class CImGUIManager*				m_ImGUIManager					= nullptr;
		class CAssimpManager*				m_AssimpManager					= nullptr;
		class EEditorManager*               m_EditorManager					= nullptr;
		class EEditorGlobalPanel*			m_EditorGlobalPanel				= nullptr;
#endif
		class EClassTypeRegisterManager*	m_ClassTypeRegisterManager		= nullptr;
		class EClassFactoryRegisterManager*	m_ClassFactoryRegisterManager	= nullptr;

		class EWorldManager*                m_WorldManager					= nullptr;
		class EWorldTickManager*			m_WorldTickManager				= nullptr;
		TFunction<void(FLOAT)>				m_PrePhysicsFixedTickHandler;
		TFunction<void(FLOAT)>				m_PhysicsFixedTickHandler;
		TFunction<void(FLOAT)>				m_PostPhysicsFixedTickHandler;

		class EMessageManager*				m_MessageManager = nullptr;

	public:
		static LRESULT HandleMsg(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam);

		CLASS_MANAGER_SINGLETON_BODY(EMainManager)

	};
};