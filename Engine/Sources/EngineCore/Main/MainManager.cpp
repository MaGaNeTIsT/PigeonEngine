#include "MainManager.h"
#include "RenderThread.h"
#include "GameThreadCommandQueue.h"
#include "PigeonBase/Object/World/WorldTickManager.h"
#include <CoreMinimal.h>
#include <Config/EngineConfig.h>
#include <RenderDevice/DeviceD3D11.h>
#include <RenderDevice/DeviceD3D12.h>
#include <RHI/IRRHIDevice.h>
#include <Renderer/RenderScene.h>
#include <Renderer/SceneRenderer.h>
#include <RenderResourceLifetime.h>
#include <Development/Profiler.h>
#include <Development/GPUProfiler.h>
#include "../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"

#include "PigeonBase/Object/World/World.h"
#include "PigeonBase/Object/World/WorldManager.h"

#include "Development/Message/Message.h"

#if _EDITOR_ONLY
#include "../../../EngineThirdParty/imGUI/Headers/imGUIManager.h"
#include "../../../EngineThirdParty/assimp/Headers/assimpManager.h"
#include "../Editor/EditorManager.h"
#include "../Editor/EditorGlobalPanel.h"
#include "../Editor/EditorProfilerPanel.h"
#endif

#if _EDITOR_ONLY
#include <RenderProxy/RenderSingletonObject.h>
#endif

// Asset managers: registered here so their lifetime is pinned inside the engine's.
#include <Asset/MeshAsset/MeshAsset.h>
#include <Asset/TextureAsset/TextureAsset.h>
#include <Asset/SkeletonAsset/SkeletonAsset.h>
#include <Asset/AnimationAsset/AnimationAsset.h>
#include <Asset/AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <Asset/ShaderAsset/ShaderAsset.h>
#include <Asset/ShaderAsset/ShaderStageAsset.h>
#include <Asset/AssetLoadManager.h>
#include <Shader/EngineShaderInstanceCache.h>

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		RegisterClassType<EMainManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EMainManager::EMainManager()
	{
		m_HWND			= nullptr;
		m_WindowSize	= Vector2Int(static_cast<INT32>(EEngineSettings::ENGINE_SCREEN_WIDTH), static_cast<INT32>(EEngineSettings::ENGINE_SCREEN_HEIGHT));
		m_GraphicDepth	= 24u;
		m_FrameRate		= static_cast<UINT32>(EEngineSettings::ENGINE_UPDATE_FRAME);
		m_Windowed		= EEngineSettings::ENGINE_WINDOWED;

		if PE_CONSTEXPR (EEngineSettings::ENGINE_GRAPHICS_API == static_cast<UINT8>(ERHIBackendType::RHI_BACKEND_D3D12))
		{
			RenderDevice = RDeviceD3D12::GetDeviceSingleton();
		}
		else
		{
			RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		}

		m_PhysicsManager	= FPhysicsManager::GetManagerSingleton();
#if _EDITOR_ONLY
		m_ImGUIManager		= CImGUIManager::GetManagerSingleton();
		m_AssimpManager		= CAssimpManager::GetManagerSingleton();
		m_EditorManager     = EEditorManager::GetManagerSingleton();
		m_EditorGlobalPanel = EEditorGlobalPanel::GetManagerSingleton();
		m_EditorProfilerPanel = EEditorProfilerPanel::GetManagerSingleton();
#endif
		m_ClassTypeRegisterManager = EClassTypeRegisterManager::GetManagerSingleton();
		m_ClassFactoryRegisterManager = EClassFactoryRegisterManager::GetManagerSingleton();

		m_WorldManager = EWorldManager::GetManagerSingleton();
		m_WorldTickManager = EWorldTickManager::GetManagerSingleton();
		m_PrePhysicsFixedTickHandler = TFunction<void(FLOAT)>([this](FLOAT InDeltaTime)
		{
			if (m_WorldManager && m_WorldManager->GetWorld())
			{
				m_WorldManager->GetWorld()->FixedTick(InDeltaTime);
			}
			if (m_PhysicsManager)
			{
				m_PhysicsManager->PrePhysicsUpdate(InDeltaTime);
			}
		});
		m_PhysicsFixedTickHandler = TFunction<void(FLOAT)>([this](FLOAT InDeltaTime)
		{
			if (m_PhysicsManager)
			{
				m_PhysicsManager->FixedUpdate(InDeltaTime);
			}
		});
		m_PostPhysicsFixedTickHandler = TFunction<void(FLOAT)>([this](FLOAT InDeltaTime)
		{
			if (m_PhysicsManager)
			{
				m_PhysicsManager->PostPhysicsUpdate(InDeltaTime);
			}
		});

		m_MessageManager = EMessageManager::GetManagerSingleton();
	}
	EMainManager::~EMainManager()
	{
		if (m_GameTimer)
		{
			delete m_GameTimer;
			m_GameTimer = nullptr;
		}
	}
	LRESULT EMainManager::HandleMsg(HWND hWnd, UINT32 msg, WPARAM wParam, LPARAM lParam)
	{
		return EInput::HandleMsg(hWnd, msg, wParam, lParam);
	}
	void EMainManager::Initialize()
	{
		// Opens the window every render resource has to live inside. Set before anything can
		// create one; closed again at the very end of ShutDown, after the last resource is
		// gone.
		m_bEngineLifetimeActive = TRUE;

		m_ClassTypeRegisterManager->DoRegister();
		m_ClassFactoryRegisterManager->DoRegister();

		m_MessageManager->Initialize();

		EInput::Initialize(m_HWND);

		{
			RRHIDeviceInitDesc DeviceInitDesc;
			DeviceInitDesc.WindowHandle		= m_HWND;
			DeviceInitDesc.BackBufferWidth	= static_cast<UINT32>(m_WindowSize.x);
			DeviceInitDesc.BackBufferHeight	= static_cast<UINT32>(m_WindowSize.y);
			DeviceInitDesc.BackBufferCount	= 2u;
			DeviceInitDesc.RefreshRateHz	= m_FrameRate;
			DeviceInitDesc.bIsWindowed		= (m_Windowed != FALSE);
#if _EDITOR_ONLY
			// Surface D3D12 validation messages in the VS Output window so
			// PSO / resource creation failures are diagnosable.
			DeviceInitDesc.bEnableDebugLayer = FALSE;
#endif
			RenderDevice->Initialize(DeviceInitDesc);
		}

		GatherSupportedResolutions();

		// Registered here, before any asset can be loaded: the singleton accessor constructs
		// a manager on first use, so registering them now is what pins their lifetime inside
		// the engine's.
		RegisterAssetManager(EMeshAssetManager::GetManagerSingleton());
		RegisterAssetManager(ETextureAssetManager::GetManagerSingleton());
		RegisterAssetManager(ESkeletonAssetManager::GetManagerSingleton());
		RegisterAssetManager(EAnimationManager::GetManagerSingleton());
		RegisterAssetManager(EAnimStateMachineManager::GetManagerSingleton());
		RegisterAssetManager(EShaderStageAssetManager::GetManagerSingleton());
		RegisterAssetManager(EShaderAssetManager::GetManagerSingleton());
		// Registered last so it is torn down first: the instances it owns are released before
		// the shader assets they were assembled from, and before the render resource manager
		// drains the buffer destroys those releases enqueue.
		RegisterAssetManager(REngineShaderInstanceCache::GetManagerSingleton());
		// Registered after everything it can ask: a teardown drops the requests that are still
		// queued, and it has to do that before the managers whose assets those requests would
		// have produced are released.
		RegisterAssetManager(EAssetLoadManager::GetManagerSingleton());
		// Registered last so it is torn down first: what is still queued for this thread refers to
		// objects the teardown below is about to release, so it goes before them.
		RegisterAssetManager(EGameThreadCommandQueue::GetManagerSingleton());

		{
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check scene renderer is not null."), (!SceneRenderer));
			SceneRenderer = new RSceneRenderer();
			SceneRenderer->Initialize();
			RenderScene = SceneRenderer->GetRenderScene();
		}

		// Spawn the RenderThread worker now that all render-side state
		// (device, scene, renderer) is constructed. The worker will sit
		// idle on GameDoneEvent until the first frame's KickRender.
		{
			// The resource queue must exist before the first KickRender, because the
			// render thread drains it at the top of every frame.
			RRenderResourceManager::GetManagerSingleton()->Initialize();
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("RenderThread already created."), (!RenderThread));
			RenderThread = new RRenderThread(RenderDevice, RenderScene, SceneRenderer);
#if _EDITOR_ONLY
			// Secondary renderers (the asset previews) record before the main scene does, so
			// everything they write into their offscreen targets is already there when this
			// frame's main pass submits the ImGui overlay that samples them.
			RenderThread->SetPreSceneRenderCallback([this]() -> void
			{
				for (UINT32 RendererIndex = 0u, RendererNum = m_FrameRenderers.Num<UINT32>(); RendererIndex < RendererNum; RendererIndex++)
				{
					m_FrameRenderers[RendererIndex]->RenderFrame();
				}
			});
			RenderThread->SetAfterRenderCallback([this]() -> void
			{
				// The main window's overlay is drawn inside SceneRenderer::ImGuiPass, as part
				// of the frame's command list. What is left here is the platform windows: a
				// panel dragged out of the main window has its own swap chain, and ImGui
				// records and presents each of them through the hooks the manager registered.
				m_ImGUIManager->RenderPlatformWindows();
			});
#endif
			RenderThread->Start();
		}

#if _EDITOR_ONLY
		m_ImGUIManager->Initialize();
		m_AssimpManager->Initialize();
		m_EditorManager->Initialize();
		m_EditorGlobalPanel->Initialize();
		m_EditorProfilerPanel->Initialize();
#endif

		m_WindowTimer.Init();
		m_WorldTickManager->Initialize();
		m_WorldManager->Initialize();

		m_PhysicsManager->Initialize();
	}
	void EMainManager::ShutDown()
	{
		// Stop the RenderThread first so its WorkerEntry can no longer fire
		// AfterRenderCallback (which dereferences m_ImGUIManager) or touch
		// SceneRenderer / RenderDevice while we tear them down below.
		if (RenderThread)
		{
			RenderThread->Stop();
			delete RenderThread;
			RenderThread = nullptr;
		}

		m_PhysicsManager->ShutDown();
		m_WorldManager->ShutDown();
		m_WorldTickManager->ShutDown();

#if _EDITOR_ONLY
		m_EditorManager->ShutDown();
		m_AssimpManager->ShutDown();
		m_ImGUIManager->ShutDown();
		m_EditorGlobalPanel->ShutDown();
		m_EditorProfilerPanel->ShutDown();
#endif

		{
			// RenderThread has already been stopped at the top of ShutDown.
			RenderScene = nullptr;
			SceneRenderer->ShutDown();
			delete SceneRenderer;
			SceneRenderer = nullptr;
		}

		/*
		* Asset teardown. This runs after the render thread stopped and before the device goes
		* away, which is exactly the window where resource releases may be performed on this
		* thread: the render thread is gone (so nobody else touches the queues or the device)
		* and the device is alive (so the releases reach real GPU objects).
		*
		* Releasing a resource enqueues its destroy request; RRenderResourceManager::ShutDown
		* below drains them, so assets do not have to know which thread they are on.
		*/
		ShutDownAssetManagers();

		// Tear down the GPU profiler BEFORE the device - its heap holds
		// ID3D11Query / ID3D12QueryHeap + readback resource that must be
		// released while the device is still alive.
		EGPUProfiler::GetManagerSingleton()->ShutDown();
		EProfiler::GetManagerSingleton()->ShutDown();

		// Final drain plus the leak backstop: whatever is still registered belongs to an owner
		// that was never torn down, and the count is reported.
		RRenderResourceManager::GetManagerSingleton()->ShutDown();

		RenderDevice->Shutdown();
		EInput::ShutDown();

		m_MessageManager->ShutDown();

		// Closes the lifetime window last: everything above is still allowed to release
		// resources, anything after this point is not.
		m_bEngineLifetimeActive = FALSE;
	}
	void EMainManager::RegisterAssetManager(EManagerBase* InManager)
	{
		if (!InManager)
		{
			return;
		}
#if _EDITOR_ONLY
		Check((!m_AssetManagers.Contains(InManager)), (ENGINE_CORE_ERROR)
			, (PE_TEXT("[EMainManager::RegisterAssetManager] The manager is already registered.")));
#endif	// _EDITOR_ONLY
		m_AssetManagers.Add(InManager);
		InManager->Initialize();
	}
	void EMainManager::UnregisterAssetManager(EManagerBase* InManager)
	{
		if (!InManager)
		{
			return;
		}
		for (UINT32 ManagerIndex = 0u, ManagerNum = m_AssetManagers.Num<UINT32>(); ManagerIndex < ManagerNum; ManagerIndex++)
		{
			if (m_AssetManagers[ManagerIndex] == InManager)
			{
				m_AssetManagers.RemoveAt(ManagerIndex);
				return;
			}
		}
	}
	void EMainManager::RegisterFrameRenderer(IEngineFrameRenderer* InRenderer)
	{
		if (!InRenderer)
		{
			return;
		}
#if _EDITOR_ONLY
		Check((!m_FrameRenderers.Contains(InRenderer)), (ENGINE_CORE_ERROR)
			, (PE_TEXT("[EMainManager::RegisterFrameRenderer] The renderer is already registered.")));
#endif	// _EDITOR_ONLY
		m_FrameRenderers.Add(InRenderer);
	}
	void EMainManager::UnregisterFrameRenderer(IEngineFrameRenderer* InRenderer)
	{
		if (!InRenderer)
		{
			return;
		}
		for (UINT32 RendererIndex = 0u, RendererNum = m_FrameRenderers.Num<UINT32>(); RendererIndex < RendererNum; RendererIndex++)
		{
			if (m_FrameRenderers[RendererIndex] == InRenderer)
			{
				m_FrameRenderers.RemoveAt(RendererIndex);
				return;
			}
		}
	}
	void EMainManager::ShutDownAssetManagers()
	{
		// Reverse registration order: a manager registered later may hold references into one
		// registered earlier. The list is walked by index while it shrinks, so an Unregister
		// from a manager's own ShutDown cannot invalidate the walk.
		while (m_AssetManagers.Num<UINT32>() > 0u)
		{
			const UINT32 ManagerIndex = m_AssetManagers.Num<UINT32>() - 1u;
			EManagerBase* Manager = m_AssetManagers[ManagerIndex];
			m_AssetManagers.RemoveAt(ManagerIndex);
			if (Manager)
			{
				Manager->ShutDown();
			}
		}
	}
	void EMainManager::SetInitializer(HWND hWnd)
	{
		m_HWND = hWnd;
	}
	void EMainManager::UpdateManager()
	{
		m_WindowTimer.Update();
	}
	void EMainManager::Init()
	{
		if (m_GameTimer)
		{
			delete m_GameTimer;
			m_GameTimer = nullptr;
		}
		m_GameTimer = new EGameTimer(&(this->m_WindowTimer));

		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene can not be null."), (!!RenderScene));
		RenderScene->Init();

		m_GameTimer->Reset();
		m_WorldTickManager->Init();
		m_WorldTickManager->RegisterPrePhysicsTick(m_PrePhysicsFixedTickHandler);
		m_WorldTickManager->RegisterPhysicsTick(m_PhysicsFixedTickHandler);
		m_WorldTickManager->RegisterPostPhysicsTick(m_PostPhysicsFixedTickHandler);

		//need Init before world manager.
		m_PhysicsManager->Init();

		m_WorldManager->GetWorld()->BindRenderScene(RenderScene);
		m_WorldManager->Init();
#if _EDITOR_ONLY
		// Please ensure that this function is called at the end
		EditorInit();
#endif
	}
	void EMainManager::Uninit()
	{
		m_WorldTickManager->UnregisterPrePhysicsTick(m_PrePhysicsFixedTickHandler);
		m_WorldTickManager->UnregisterPhysicsTick(m_PhysicsFixedTickHandler);
		m_WorldTickManager->UnregisterPostPhysicsTick(m_PostPhysicsFixedTickHandler);
		m_PhysicsManager->Uninit();
		m_WorldManager->Uninit();
		m_WorldTickManager->Uninit();
		RenderScene->Uninit();

		if (m_GameTimer)
		{
			delete m_GameTimer;
			m_GameTimer = nullptr;
		}

	}
	void EMainManager::Update()
	{
		// Async with render thread
		m_GameTimer->Update();
		m_WorldManager->GetWorld()->Tick(static_cast<FLOAT>(m_GameTimer->GetDeltaTime()));
		m_PhysicsManager->Update();

		/*
		* Asset loads run here, on the game thread, which is the load thread of this build (a
		* dedicated load thread would drain the same queue from its own loop). Placed before the
		* wait rather than after it on purpose: a load can take seconds when it compiles a
		* shader, and running it while the render thread is still finishing the previous frame
		* overlaps that time with work that has to happen anyway. Everything the loads enqueue
		* for the render thread - the GPU objects of the assets that were built - is then drained
		* at the top of the frame this Update kicks off.
		*/
		EAssetLoadManager::GetManagerSingleton()->DoLoads();

		// Anything a load completion asked this thread to do runs here - the marshalling step of
		// "the asset exists now, apply it". It runs before the scene swaps below, so a component whose
		// asset was just set publishes that change as part of this frame rather than the next one.
		EGameThreadCommandQueue::GetManagerSingleton()->DoCommands();

		RenderThread->WaitForRenderIdle();

		// Retired assets are released here: the render thread has just gone idle, so no queued
		// creation command can still be reading their cold data.
		RRenderResourceManager::GetManagerSingleton()->FlushRetiredObjects();

		if (m_bPendingResize)
		{
			m_bPendingResize = FALSE;
			if (RenderDevice->ResizeSwapChain(m_PendingResizeWidth, m_PendingResizeHeight))
			{
				SceneRenderer->ResizeRenderTargets(m_PendingResizeWidth, m_PendingResizeHeight);
				m_WorldManager->GetWorld()->UpdateCameraViewports(m_PendingResizeWidth, m_PendingResizeHeight);
				const INT32 NewWidth  = static_cast<INT32>(m_PendingResizeWidth);
				const INT32 NewHeight = static_cast<INT32>(m_PendingResizeHeight);
				m_WindowSize = Vector2Int(NewWidth, NewHeight);
				EInput::Controller.UpdateWindowSize(NewWidth, NewHeight);
#if _EDITOR_ONLY
				// A window that fits in the old size has to be kept inside the new one. Without
				// this, shrinking the main window hands each panel its own platform window and
				// the layout scatters; growing again does not bring them back.
				m_ImGUIManager->ClampWindowsToMainViewport(m_PendingResizeWidth, m_PendingResizeHeight);
#endif
			}
		}

#if _EDITOR_ONLY
		m_ImGUIManager->Update();				// ImGui::NewFrame
		EditorUpdate();
		m_WorldManager->GetWorld()->EditorTick(static_cast<FLOAT>(m_GameTimer->GetDeltaTime()));
		{
			RDebugWireframePrimitiveManager* Manager = RDebugWireframePrimitiveManager::GetManagerSingleton();
			const Vector3 TempPos(-500.f, 100.f, 500.f);
			Manager->DrawSingleLine(TempPos + Vector3(100.f, -50.f, 0.f), TempPos + Vector3(100.f, -50.f, 0.f) + Vector3(0.f, 100.f, 0.f), Color4::White());
			Manager->DrawPlane(TempPos + Vector3(200.f, 0.f, 0.f), Quaternion::Identity(), 80.f, 80.f, Color4::White());
			Manager->DrawCircle(TempPos + Vector3(300.f, 0.f, 0.f), Quaternion::Identity(), 40.f, Color4::White());
			Manager->DrawCuboid(TempPos + Vector3(400.f, 0.f, 0.f), Quaternion::Identity(), 80.f, 80.f, 80.f, Color4::White());
			Manager->DrawCone(TempPos + Vector3(500.f, -50.f, 0.f), TempPos + Vector3(500.f, -50.f, 0.f) + Vector3(0.f, 100.f, 0.f), 40.f, Color4::White());
			Manager->DrawCylinder(TempPos + Vector3(600.f, -50.f, 0.f), TempPos + Vector3(600.f, -50.f, 0.f) + Vector3(0.f, 100.f, 0.f), 40.f, 40.f, Color4::White());
			Manager->DrawCapsule(TempPos + Vector3(700.f, -50.f, 0.f), TempPos + Vector3(700.f, -50.f, 0.f) + Vector3(0.f, 100.f, 0.f), 40.f, 40.f, Color4::White());
			Manager->DrawSphere(TempPos + Vector3(800.f, 0.f, 0.f), 40.f, Color4::White(), Quaternion::Identity());
		}
		m_ImGUIManager->PrepareDrawData();		// ImGui::Render - DrawData ready for the worker
#endif

		// Swap double buffer
#if _EDITOR_ONLY
		RDebugWireframePrimitiveManager::GetManagerSingleton()->SwapCommandSlots();
#endif
		RenderScene->SwapCommandSlots();
		// Secondary renderers publish here for the same reason: their back slot is written by
		// this thread and drained by the render thread, so it flips at the same boundary.
		for (UINT32 RendererIndex = 0u, RendererNum = m_FrameRenderers.Num<UINT32>(); RendererIndex < RendererNum; RendererIndex++)
		{
			m_FrameRenderers[RendererIndex]->PublishFrameCommands();
		}
		// The render resource queue needs no flip of its own any more: it is a single lock free
		// queue that any thread may write and the render thread drains at the top of the frame,
		// so a create request recorded here is already in it when the frame it belongs to starts.
		RenderThread->KickRender();
	}
	void EMainManager::FixedUpdate()
	{
		m_WorldTickManager->Tick(static_cast <DOUBLE>(1) / static_cast<DOUBLE>(PigeonEngine::EEngineSettings::ENGINE_FIXED_UPDATE_FRAME));
	}
	void EMainManager::Draw()
	{

	}

#if _EDITOR_ONLY
	void EMainManager::EditorInit()
	{

		m_EditorManager->EditorInit();
	}

	void EMainManager::EditorUpdate()
	{
		m_EditorManager->EditorUpdate();
	}
#endif

	HWND EMainManager::GetWindowHandle()
	{
		return (m_HWND);
	}
	const Vector2Int& EMainManager::GetWindowSize() const
	{
		return m_WindowSize;
	}
	const EBaseTimer& EMainManager::GetWindowTimer()
	{
		return (m_WindowTimer);
	}
	const EGameTimer* EMainManager::GetGameTimer()
	{
		return (m_GameTimer);
	}

	void EMainManager::GatherSupportedResolutions()
	{
		m_SupportedResolutions.Empty();
		DEVMODEW DevMode = {};
		DevMode.dmSize   = sizeof(DevMode);
		DWORD ModeIndex  = 0u;
		while (::EnumDisplaySettingsW(nullptr, ModeIndex, &DevMode) != FALSE)
		{
			RResolutionItem Item;
			Item.Width  = static_cast<UINT32>(DevMode.dmPelsWidth);
			Item.Height = static_cast<UINT32>(DevMode.dmPelsHeight);
			BOOL32 bFound = FALSE;
			for (const RResolutionItem& Existing : m_SupportedResolutions)
			{
				if (Existing == Item)
				{
					bFound = TRUE;
					break;
				}
			}
			if (!bFound)
			{
				m_SupportedResolutions.Add(Item);
			}
			++ModeIndex;
		}
		// Sort ascending by Width, then Height
		const INT32 Count = static_cast<INT32>(m_SupportedResolutions.Num());
		for (INT32 i = 0; i < Count - 1; ++i)
		{
			for (INT32 j = 0; j < Count - 1 - i; ++j)
			{
				const RResolutionItem& A = m_SupportedResolutions[j];
				const RResolutionItem& B = m_SupportedResolutions[j + 1];
				if (A.Width > B.Width || (A.Width == B.Width && A.Height > B.Height))
				{
					RResolutionItem Temp    = m_SupportedResolutions[j];
					m_SupportedResolutions[j]     = m_SupportedResolutions[j + 1];
					m_SupportedResolutions[j + 1] = Temp;
				}
			}
		}
	}

	void EMainManager::ApplyResolution(UINT32 InWidth, UINT32 InHeight, BOOL32 InIsFullscreen)
	{
		m_bIsFullscreen = InIsFullscreen;
		if (InIsFullscreen)
		{
			HMONITOR Monitor    = ::MonitorFromWindow(m_HWND, MONITOR_DEFAULTTONEAREST);
			MONITORINFO MonInfo = {};
			MonInfo.cbSize      = sizeof(MonInfo);
			::GetMonitorInfoW(Monitor, &MonInfo);
			::SetWindowLongPtrW(m_HWND, GWL_STYLE, WS_POPUP | WS_VISIBLE);
			::SetWindowPos(
				m_HWND,
				HWND_TOP,
				MonInfo.rcMonitor.left,
				MonInfo.rcMonitor.top,
				MonInfo.rcMonitor.right  - MonInfo.rcMonitor.left,
				MonInfo.rcMonitor.bottom - MonInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);
			m_PendingResizeWidth  = static_cast<UINT32>(MonInfo.rcMonitor.right  - MonInfo.rcMonitor.left);
			m_PendingResizeHeight = static_cast<UINT32>(MonInfo.rcMonitor.bottom - MonInfo.rcMonitor.top);
		}
		else
		{
			RECT ClientRect = { 0, 0, static_cast<LONG>(InWidth), static_cast<LONG>(InHeight) };
#if _EDITOR_ONLY
			constexpr DWORD WindowStyle = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
#else
			constexpr DWORD WindowStyle = WS_VISIBLE | (WS_OVERLAPPEDWINDOW ^ (WS_MAXIMIZEBOX | WS_THICKFRAME));
#endif
			::AdjustWindowRect(&ClientRect, WS_OVERLAPPEDWINDOW, FALSE);
			::SetWindowLongPtrW(m_HWND, GWL_STYLE, WindowStyle);
			::SetWindowPos(
				m_HWND,
				HWND_TOP,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				ClientRect.right  - ClientRect.left,
				ClientRect.bottom - ClientRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_SHOWWINDOW);
			m_PendingResizeWidth  = InWidth;
			m_PendingResizeHeight = InHeight;
		}
		m_bPendingResize = TRUE;
	}

	void EMainManager::OnWindowResized(UINT32 InNewWidth, UINT32 InNewHeight)
	{
		if (InNewWidth == 0u || InNewHeight == 0u)
		{
			return;
		}
		m_bPendingResize      = TRUE;
		m_PendingResizeWidth  = InNewWidth;
		m_PendingResizeHeight = InNewHeight;
		m_bIsFullscreen       = FALSE;
	}

	const TArray<RResolutionItem>& EMainManager::GetSupportedResolutions() const
	{
		return m_SupportedResolutions;
	}

	BOOL32 EMainManager::GetIsFullscreen() const
	{
		return m_bIsFullscreen;
	}
};
