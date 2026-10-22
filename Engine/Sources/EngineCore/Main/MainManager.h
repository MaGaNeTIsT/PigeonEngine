#pragma once
#include "Main.h"
#include "FrameRenderer.h"
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
		/*
		* Asset manager registration. A registered manager is initialized during
		* EMainManager::Initialize and shut down in reverse registration order during
		* ShutDown, which is what gives every asset an owner that is torn down before the
		* device goes away.
		*
		* Registration is explicit rather than done by the manager's own constructor: it keeps
		* the construction of every manager inside the engine lifetime, so a manager can never
		* be built - and therefore never create shared state - before Main starts.
		*/
		void			RegisterAssetManager(EManagerBase* InManager);
		void			UnregisterAssetManager(EManagerBase* InManager);
		/*
		* Secondary frame renderers. See IEngineFrameRenderer for what the two calls mean and
		* when they run. The editor asset previews register here in Initialize and unregister
		* in Shutdown, so the registry never outlives the renderer it names.
		*/
		void			RegisterFrameRenderer(IEngineFrameRenderer* InRenderer);
		void			UnregisterFrameRenderer(IEngineFrameRenderer* InRenderer);
		/*
		* TRUE between Initialize and the end of ShutDown. Render resources assert on it: a
		* resource may not outlive EMainManager. This is an assertion only, it never changes
		* behaviour - releasing a resource is the same call inside and outside the window.
		*/
		PE_NODISCARD BOOL32	IsEngineLifetimeActive() const { return m_bEngineLifetimeActive; }
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
		// Active RHI device (D3D11 or D3D12 depending on ENGINE_GRAPHICS_API).
		class IRRHIDevice*			GetRenderDevice() const { return RenderDevice; }
		// The frame renderer. Proxies reach the pipeline state cache and the per frame
		// parameter block through it, so neither has to be a singleton of its own.
		class RSceneRenderer*		GetSceneRenderer() const { return SceneRenderer; }
		// The renderer that is recording passes right now. The main scene and the editor's
		// asset previews are recorded one after another on the render thread, and a proxy
		// binds the blocks of whichever renderer is drawing it. Reading the main renderer
		// unconditionally is wrong for a preview: the main renderer's current view is null
		// until the main scene records, so the preview's ViewParams would never be bound and
		// every vertex would collapse into one point - a black panel with nothing reported.
		// Nothing published falls back to the main scene, which is the only renderer there is
		// for the runtime build and for the frames before the first Render call.
		class RSceneRenderer*		GetActiveSceneRenderer() const { return (ActiveSceneRenderer != nullptr) ? ActiveSceneRenderer : SceneRenderer; }
		void						SetActiveSceneRenderer(class RSceneRenderer* InRenderer) { ActiveSceneRenderer = InRenderer; }

		// Phase 2 transition: the per-frame command list set by SceneRenderer
		// at the start of Render() and cleared at the end. Proxies pick it up
		// during their Bind / Draw without changing all signatures.
		class IRCommandList*		GetActiveCommandList() const { return ActiveCommandList; }
		void						SetActiveCommandList(class IRCommandList* InCmdList) { ActiveCommandList = InCmdList; }

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
		class IRCommandList*				ActiveCommandList				= nullptr;
		class RScene*						RenderScene						= nullptr;
		class RSceneRenderer*				SceneRenderer					= nullptr;
		class RSceneRenderer*				ActiveSceneRenderer				= nullptr;
		class RRenderThread*				RenderThread					= nullptr;
		// Render region END

#if _EDITOR_ONLY
		class CImGUIManager*				m_ImGUIManager					= nullptr;
		class CAssimpManager*				m_AssimpManager					= nullptr;
		class EEditorManager*               m_EditorManager					= nullptr;
		class EEditorGlobalPanel*			m_EditorGlobalPanel				= nullptr;
		class EEditorProfilerPanel*			m_EditorProfilerPanel			= nullptr;
#endif
		class EClassTypeRegisterManager*	m_ClassTypeRegisterManager		= nullptr;
		class EClassFactoryRegisterManager*	m_ClassFactoryRegisterManager	= nullptr;

		class EWorldManager*                m_WorldManager					= nullptr;
		class EWorldTickManager*			m_WorldTickManager				= nullptr;
		TFunction<void(FLOAT)>				m_PrePhysicsFixedTickHandler;
		TFunction<void(FLOAT)>				m_PhysicsFixedTickHandler;
		TFunction<void(FLOAT)>				m_PostPhysicsFixedTickHandler;

		class EMessageManager*				m_MessageManager = nullptr;

		/*
		* Asset managers, in registration order. Shut down in reverse: a manager registered
		* later may hold references into one registered earlier.
		*/
		TArray<class EManagerBase*>			m_AssetManagers;
		// Secondary frame renderers, in registration order. Read on the render thread inside
		// the frame callbacks and written on the game thread between frames, never at the same
		// time - the frame boundary is a handshake between the two threads.
		TArray<IEngineFrameRenderer*>		m_FrameRenderers;
		BOOL32								m_bEngineLifetimeActive = FALSE;

		// Releases every registered manager. Split out so Initialize's failure path and
		// ShutDown run the same teardown.
		void			ShutDownAssetManagers();

	public:
		static LRESULT HandleMsg(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam);

		CLASS_MANAGER_SINGLETON_BODY(EMainManager)

	};
};
