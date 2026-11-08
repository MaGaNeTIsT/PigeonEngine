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

#if ENGINE_EDITOR_ONLY
#include "../../../EngineThirdParty/imGUI/Headers/imGUIManager.h"
#include "../../../EngineThirdParty/assimp/Headers/assimpManager.h"
#include "../Editor/EditorManager.h"
#include "../Editor/EditorGlobalPanel.h"
#include "../Editor/EditorProfilerPanel.h"
#endif

#if ENGINE_EDITOR_ONLY
#include <RenderProxy/RenderSingletonObject.h>
#endif

// Asset managers: registered here so their lifetime is pinned inside the engine's.
#include <Asset/MeshAsset/MeshAsset.h>
#include <Asset/TextureAsset/TextureAsset.h>
#include <Asset/SkeletonAsset/SkeletonAsset.h>
#include <Asset/AnimationAsset/AnimationAsset.h>
#include <Asset/AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <Asset/AnimationMontageAsset/AnimationMontageAsset.h>
#include <Asset/AnimationNotifyAsset/AnimationNotifyAsset.h>
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
		WindowHandle			= nullptr;
		WindowSize	= Vector2Int(static_cast<INT32>(EEngineSettings::ENGINE_SCREEN_WIDTH), static_cast<INT32>(EEngineSettings::ENGINE_SCREEN_HEIGHT));
		GraphicDepth	= 24u;
		FrameRate		= static_cast<UINT32>(EEngineSettings::ENGINE_UPDATE_FRAME);
		bWindowed		= EEngineSettings::ENGINE_WINDOWED;

		if PE_CONSTEXPR (EEngineSettings::ENGINE_GRAPHICS_API == static_cast<UINT8>(ERHIBackendType::RHI_BACKEND_D3D12))
		{
			RenderDevice = RDeviceD3D12::GetDeviceSingleton();
		}
		else
		{
			RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		}

		PhysicsManager	= FPhysicsManager::GetManagerSingleton();
#if ENGINE_EDITOR_ONLY
		ImGUIManager		= CImGUIManager::GetManagerSingleton();
		AssimpManager		= CAssimpManager::GetManagerSingleton();
		EditorManager     = EEditorManager::GetManagerSingleton();
		EditorGlobalPanel = EEditorGlobalPanel::GetManagerSingleton();
		EditorProfilerPanel = EEditorProfilerPanel::GetManagerSingleton();
#endif
		ClassTypeRegisterManager = EClassTypeRegisterManager::GetManagerSingleton();
		ClassFactoryRegisterManager = EClassFactoryRegisterManager::GetManagerSingleton();

		WorldManager = EWorldManager::GetManagerSingleton();
		WorldTickManager = EWorldTickManager::GetManagerSingleton();
		PrePhysicsFixedTickHandler = TFunction<void(FLOAT)>([this](FLOAT InDeltaTime)
		{
			if (WorldManager && WorldManager->GetWorld())
			{
				WorldManager->GetWorld()->FixedTick(InDeltaTime);
			}
			if (PhysicsManager)
			{
				PhysicsManager->PrePhysicsUpdate(InDeltaTime);
			}
		});
		PhysicsFixedTickHandler = TFunction<void(FLOAT)>([this](FLOAT InDeltaTime)
		{
			if (PhysicsManager)
			{
				PhysicsManager->FixedUpdate(InDeltaTime);
			}
		});
		PostPhysicsFixedTickHandler = TFunction<void(FLOAT)>([this](FLOAT InDeltaTime)
		{
			if (PhysicsManager)
			{
				PhysicsManager->PostPhysicsUpdate(InDeltaTime);
			}
		});

		MessageManager = EMessageManager::GetManagerSingleton();
	}
	EMainManager::~EMainManager()
	{
		if (GameTimer)
		{
			delete GameTimer;
			GameTimer = nullptr;
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
		bEngineLifetimeActive = TRUE;

		ClassTypeRegisterManager->DoRegister();
		ClassFactoryRegisterManager->DoRegister();

		MessageManager->Initialize();

		EInput::Initialize(WindowHandle);

		{
			RRHIDeviceInitDesc DeviceInitDesc;
			DeviceInitDesc.WindowHandle		= WindowHandle;
			DeviceInitDesc.BackBufferWidth	= static_cast<UINT32>(WindowSize.x);
			DeviceInitDesc.BackBufferHeight	= static_cast<UINT32>(WindowSize.y);
			DeviceInitDesc.BackBufferCount	= 2u;
			DeviceInitDesc.RefreshRateHz	= FrameRate;
			DeviceInitDesc.bIsWindowed		= (bWindowed != FALSE);
#if ENGINE_EDITOR_ONLY
			// Opt-in through the engine setting: the validation layer turns every recorded
			// command into a checked CPU call, which is exactly what makes a PSO / resource
			// failure readable - and what makes a frame with a shadow pass unusably slow. The
			// device drains the layer's queue into the engine log where a call fails
			// (RDeviceD3D12::LogD3D12DebugMessages), and turns the layer's own break off so a
			// message does not stop the run before the engine's failure path sees it.
			DeviceInitDesc.bEnableDebugLayer = EEngineSettings::ENGINE_RHI_ENABLE_DEBUG_LAYER;
#endif
			RenderDevice->Initialize(DeviceInitDesc);
		}

		// Explicit even though the GPU timestamp heap is created lazily on the
		// first BeginFrame: this is the point where both profilers get a clean
		// slate, and the render thread that feeds them starts further down.
		EProfiler::GetManagerSingleton()->Initialize();
		EGPUProfiler::GetManagerSingleton()->Initialize();

		GatherSupportedResolutions();

		// Registered here, before any asset can be loaded: the singleton accessor constructs
		// a manager on first use, so registering them now is what pins their lifetime inside
		// the engine's.
		RegisterAssetManager(EMeshAssetManager::GetManagerSingleton());
		RegisterAssetManager(ETextureAssetManager::GetManagerSingleton());
		RegisterAssetManager(ESkeletonAssetManager::GetManagerSingleton());
		RegisterAssetManager(EAnimationManager::GetManagerSingleton());
		RegisterAssetManager(EAnimStateMachineManager::GetManagerSingleton());
		RegisterAssetManager(EAnimMontageManager::GetManagerSingleton());
		RegisterAssetManager(EAnimNotifyManager::GetManagerSingleton());
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
#if ENGINE_EDITOR_ONLY
			// Secondary renderers (the asset previews) record before the main scene does, so
			// everything they write into their offscreen targets is already there when this
			// frame's main pass submits the ImGui overlay that samples them.
			RenderThread->SetPreSceneRenderCallback([this]() -> void
			{
				for (UINT32 RendererIndex = 0u, RendererNum = FrameRenderers.Num<UINT32>(); RendererIndex < RendererNum; RendererIndex++)
				{
					FrameRenderers[RendererIndex]->RenderFrame();
				}
			});
			RenderThread->SetAfterRenderCallback([this]() -> void
			{
				// The main window's overlay is drawn inside SceneRenderer::ImGuiPass, as part
				// of the frame's command list. What is left here is the platform windows: a
				// panel dragged out of the main window has its own swap chain, and ImGui
				// records and presents each of them through the hooks the manager registered.
				ImGUIManager->RenderPlatformWindows();
			});
#endif
			RenderThread->Start();
		}

#if ENGINE_EDITOR_ONLY
		ImGUIManager->Initialize();
		AssimpManager->Initialize();
		EditorManager->Initialize();
		EditorGlobalPanel->Initialize();
		EditorProfilerPanel->Initialize();
#endif

		WindowTimer.Init();
		WorldTickManager->Initialize();
		WorldManager->Initialize();

		PhysicsManager->Initialize();
	}
	void EMainManager::ShutDown()
	{
		// Stop the RenderThread first so its WorkerEntry can no longer fire
		// AfterRenderCallback (which dereferences ImGUIManager) or touch
		// SceneRenderer / RenderDevice while we tear them down below.
		if (RenderThread)
		{
			RenderThread->Stop();
			delete RenderThread;
			RenderThread = nullptr;
		}

		PhysicsManager->ShutDown();
		WorldManager->ShutDown();
		WorldTickManager->ShutDown();

#if ENGINE_EDITOR_ONLY
		EditorManager->ShutDown();
		AssimpManager->ShutDown();
		ImGUIManager->ShutDown();
		EditorGlobalPanel->ShutDown();
		EditorProfilerPanel->ShutDown();
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

		MessageManager->ShutDown();

		// Closes the lifetime window last: everything above is still allowed to release
		// resources, anything after this point is not.
		bEngineLifetimeActive = FALSE;
	}
	void EMainManager::RegisterAssetManager(EManagerBase* InManager)
	{
		if (!InManager)
		{
			return;
		}
#if ENGINE_EDITOR_ONLY
		Check((!AssetManagers.Contains(InManager)), (ENGINE_CORE_ERROR)
			, (PE_TEXT("[EMainManager::RegisterAssetManager] The manager is already registered.")));
#endif	// ENGINE_EDITOR_ONLY
		AssetManagers.Add(InManager);
		InManager->Initialize();
	}
	void EMainManager::UnregisterAssetManager(EManagerBase* InManager)
	{
		if (!InManager)
		{
			return;
		}
		for (UINT32 ManagerIndex = 0u, ManagerNum = AssetManagers.Num<UINT32>(); ManagerIndex < ManagerNum; ManagerIndex++)
		{
			if (AssetManagers[ManagerIndex] == InManager)
			{
				AssetManagers.RemoveAt(ManagerIndex);
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
#if ENGINE_EDITOR_ONLY
		Check((!FrameRenderers.Contains(InRenderer)), (ENGINE_CORE_ERROR)
			, (PE_TEXT("[EMainManager::RegisterFrameRenderer] The renderer is already registered.")));
#endif	// ENGINE_EDITOR_ONLY
		FrameRenderers.Add(InRenderer);
	}
	void EMainManager::UnregisterFrameRenderer(IEngineFrameRenderer* InRenderer)
	{
		if (!InRenderer)
		{
			return;
		}
		for (UINT32 RendererIndex = 0u, RendererNum = FrameRenderers.Num<UINT32>(); RendererIndex < RendererNum; RendererIndex++)
		{
			if (FrameRenderers[RendererIndex] == InRenderer)
			{
				FrameRenderers.RemoveAt(RendererIndex);
				return;
			}
		}
	}
	void EMainManager::NotifyFrameRenderersShaderReplaced(const EName& InShaderName)
	{
		for (UINT32 RendererIndex = 0u, RendererNum = FrameRenderers.Num<UINT32>(); RendererIndex < RendererNum; RendererIndex++)
		{
			FrameRenderers[RendererIndex]->OnShaderAssetReplaced(InShaderName);
		}
	}
	void EMainManager::ShutDownAssetManagers()
	{
		// Reverse registration order: a manager registered later may hold references into one
		// registered earlier. The list is walked by index while it shrinks, so an Unregister
		// from a manager's own ShutDown cannot invalidate the walk.
		while (AssetManagers.Num<UINT32>() > 0u)
		{
			const UINT32 ManagerIndex = AssetManagers.Num<UINT32>() - 1u;
			EManagerBase* Manager = AssetManagers[ManagerIndex];
			AssetManagers.RemoveAt(ManagerIndex);
			if (Manager)
			{
				Manager->ShutDown();
			}
		}
	}
	void EMainManager::SetInitializer(HWND hWnd)
	{
		WindowHandle = hWnd;
	}
	void EMainManager::UpdateManager()
	{
		WindowTimer.Update();
	}
	void EMainManager::Init()
	{
		if (GameTimer)
		{
			delete GameTimer;
			GameTimer = nullptr;
		}
		GameTimer = new EGameTimer(&(this->WindowTimer));

		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene can not be null."), (!!RenderScene));
		RenderScene->Init();

		GameTimer->Reset();
		WorldTickManager->Init();
		WorldTickManager->RegisterPrePhysicsTick(PrePhysicsFixedTickHandler);
		WorldTickManager->RegisterPhysicsTick(PhysicsFixedTickHandler);
		WorldTickManager->RegisterPostPhysicsTick(PostPhysicsFixedTickHandler);

		//need Init before world manager.
		PhysicsManager->Init();

		WorldManager->GetWorld()->BindRenderScene(RenderScene);
		WorldManager->Init();
#if ENGINE_EDITOR_ONLY
		// Please ensure that this function is called at the end
		EditorInit();
#endif
	}
	void EMainManager::Uninit()
	{
		// The last Update kicked a frame and does not wait for it. That frame records draws from
		// the proxies and assets released below, so it has to retire before any of them is
		// destroyed. Waiting here rather than relying on ShutDown is what closes the window:
		// ShutDown stops the render thread, but it runs after this method has already torn the
		// world and the render scene down.
		if (RenderThread)
		{
			RenderThread->WaitForRenderIdle();
		}

		WorldTickManager->UnregisterPrePhysicsTick(PrePhysicsFixedTickHandler);
		WorldTickManager->UnregisterPhysicsTick(PhysicsFixedTickHandler);
		WorldTickManager->UnregisterPostPhysicsTick(PostPhysicsFixedTickHandler);
		PhysicsManager->Uninit();
		WorldManager->Uninit();
		WorldTickManager->Uninit();
		RenderScene->Uninit();

		if (GameTimer)
		{
			delete GameTimer;
			GameTimer = nullptr;
		}

	}
	void EMainManager::Update()
	{
		// Whole-frame wall clock, including the WaitForRenderIdle below. This is
		// the number the panel's FPS is computed from, so it has to cover the
		// hand-off to the render thread rather than the game work alone.
		PE_PROFILE_SCOPE(PROFILER_SCOPE_GAME_THREAD);

		// Async with render thread
		GameTimer->Update();
		WorldManager->GetWorld()->Tick(static_cast<FLOAT>(GameTimer->GetDeltaTime()));
		{
			// Physics runs on the game thread today, so this bracket nests inside
			// GameThread. The scope stays on the call, not on the thread: moving
			// physics to its own loop keeps this measurement without a rewrite.
			PE_PROFILE_SCOPE(PROFILER_SCOPE_PHYSICS_THREAD);
			PhysicsManager->Update();
		}

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

		{
			// The wait is backpressure, not game work: the previous render frame has to finish
			// before this thread may swap the command slots. Kept as its own child scope so the
			// panel can separate "the game is slow" from "the render thread is the frame rate",
			// which is what a GameThread total that exceeds RenderThread would otherwise hide.
			PE_PROFILE_SCOPE(PROFILER_SCOPE_GAME_WAIT_RENDER);
			RenderThread->WaitForRenderIdle();
		}

		// Retired assets are released here: the render thread has just gone idle, so no queued
		// creation command can still be reading their cold data.
		RRenderResourceManager::GetManagerSingleton()->FlushRetiredObjects();

		if (bPendingResize)
		{
			bPendingResize = FALSE;
			if (RenderDevice->ResizeSwapChain(PendingResizeWidth, PendingResizeHeight))
			{
				SceneRenderer->ResizeRenderTargets(PendingResizeWidth, PendingResizeHeight);
				WorldManager->GetWorld()->UpdateCameraViewports(PendingResizeWidth, PendingResizeHeight);
				const INT32 NewWidth  = static_cast<INT32>(PendingResizeWidth);
				const INT32 NewHeight = static_cast<INT32>(PendingResizeHeight);
				WindowSize = Vector2Int(NewWidth, NewHeight);
				EInput::Controller.UpdateWindowSize(NewWidth, NewHeight);
#if ENGINE_EDITOR_ONLY
				// A window that fits in the old size has to be kept inside the new one. Without
				// this, shrinking the main window hands each panel its own platform window and
				// the layout scatters; growing again does not bring them back.
				ImGUIManager->ClampWindowsToMainViewport(PendingResizeWidth, PendingResizeHeight);
#endif
			}
		}

#if ENGINE_EDITOR_ONLY
		ImGUIManager->Update();				// ImGui::NewFrame
		EditorUpdate();
		WorldManager->GetWorld()->EditorTick(static_cast<FLOAT>(GameTimer->GetDeltaTime()));
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
		ImGUIManager->PrepareDrawData();		// ImGui::Render - DrawData ready for the worker
#endif

		// Swap double buffer
#if ENGINE_EDITOR_ONLY
		RDebugWireframePrimitiveManager::GetManagerSingleton()->SwapCommandSlots();
#endif
		RenderScene->SwapCommandSlots();
		// Secondary renderers publish here for the same reason: their back slot is written by
		// this thread and drained by the render thread, so it flips at the same boundary.
		for (UINT32 RendererIndex = 0u, RendererNum = FrameRenderers.Num<UINT32>(); RendererIndex < RendererNum; RendererIndex++)
		{
			FrameRenderers[RendererIndex]->PublishFrameCommands();
		}
		// The render resource queue needs no flip of its own any more: it is a single lock free
		// queue that any thread may write and the render thread drains at the top of the frame,
		// so a create request recorded here is already in it when the frame it belongs to starts.
		RenderThread->KickRender();
	}
	void EMainManager::FixedUpdate()
	{
		WorldTickManager->Tick(static_cast <DOUBLE>(1) / static_cast<DOUBLE>(PigeonEngine::EEngineSettings::ENGINE_FIXED_UPDATE_FRAME));
	}
	void EMainManager::Draw()
	{

	}

#if ENGINE_EDITOR_ONLY
	void EMainManager::EditorInit()
	{

		EditorManager->EditorInit();
	}

	void EMainManager::EditorUpdate()
	{
		EditorManager->EditorUpdate();
	}
#endif

	HWND EMainManager::GetWindowHandle()
	{
		return (WindowHandle);
	}
	const Vector2Int& EMainManager::GetWindowSize() const
	{
		return WindowSize;
	}
	const EBaseTimer& EMainManager::GetWindowTimer()
	{
		return (WindowTimer);
	}
	const EGameTimer* EMainManager::GetGameTimer()
	{
		return (GameTimer);
	}

	void EMainManager::GatherSupportedResolutions()
	{
		SupportedResolutions.Empty();
		DEVMODEW DevMode = {};
		DevMode.dmSize   = sizeof(DevMode);
		DWORD ModeIndex  = 0u;
		while (::EnumDisplaySettingsW(nullptr, ModeIndex, &DevMode) != FALSE)
		{
			RResolutionItem Item;
			Item.Width  = static_cast<UINT32>(DevMode.dmPelsWidth);
			Item.Height = static_cast<UINT32>(DevMode.dmPelsHeight);
			BOOL32 bFound = FALSE;
			for (const RResolutionItem& Existing : SupportedResolutions)
			{
				if (Existing == Item)
				{
					bFound = TRUE;
					break;
				}
			}
			if (!bFound)
			{
				SupportedResolutions.Add(Item);
			}
			++ModeIndex;
		}
		// Sort ascending by Width, then Height
		const INT32 Count = static_cast<INT32>(SupportedResolutions.Num());
		for (INT32 i = 0; i < Count - 1; ++i)
		{
			for (INT32 j = 0; j < Count - 1 - i; ++j)
			{
				const RResolutionItem& A = SupportedResolutions[j];
				const RResolutionItem& B = SupportedResolutions[j + 1];
				if (A.Width > B.Width || (A.Width == B.Width && A.Height > B.Height))
				{
					RResolutionItem Temp    = SupportedResolutions[j];
					SupportedResolutions[j]     = SupportedResolutions[j + 1];
					SupportedResolutions[j + 1] = Temp;
				}
			}
		}
	}

	void EMainManager::ApplyResolution(UINT32 InWidth, UINT32 InHeight, BOOL32 InIsFullscreen)
	{
		bIsFullscreen = InIsFullscreen;
		if (InIsFullscreen)
		{
			HMONITOR Monitor    = ::MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTONEAREST);
			MONITORINFO MonInfo = {};
			MonInfo.cbSize      = sizeof(MonInfo);
			::GetMonitorInfoW(Monitor, &MonInfo);
			::SetWindowLongPtrW(WindowHandle, GWL_STYLE, WS_POPUP | WS_VISIBLE);
			::SetWindowPos(
				WindowHandle,
				HWND_TOP,
				MonInfo.rcMonitor.left,
				MonInfo.rcMonitor.top,
				MonInfo.rcMonitor.right  - MonInfo.rcMonitor.left,
				MonInfo.rcMonitor.bottom - MonInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);
			PendingResizeWidth  = static_cast<UINT32>(MonInfo.rcMonitor.right  - MonInfo.rcMonitor.left);
			PendingResizeHeight = static_cast<UINT32>(MonInfo.rcMonitor.bottom - MonInfo.rcMonitor.top);
		}
		else
		{
			RECT ClientRect = { 0, 0, static_cast<LONG>(InWidth), static_cast<LONG>(InHeight) };
#if ENGINE_EDITOR_ONLY
			constexpr DWORD WindowStyle = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
#else
			constexpr DWORD WindowStyle = WS_VISIBLE | (WS_OVERLAPPEDWINDOW ^ (WS_MAXIMIZEBOX | WS_THICKFRAME));
#endif
			::AdjustWindowRect(&ClientRect, WS_OVERLAPPEDWINDOW, FALSE);
			::SetWindowLongPtrW(WindowHandle, GWL_STYLE, WindowStyle);
			::SetWindowPos(
				WindowHandle,
				HWND_TOP,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				ClientRect.right  - ClientRect.left,
				ClientRect.bottom - ClientRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_SHOWWINDOW);
			PendingResizeWidth  = InWidth;
			PendingResizeHeight = InHeight;
		}
		bPendingResize = TRUE;
	}

	void EMainManager::OnWindowResized(UINT32 InNewWidth, UINT32 InNewHeight)
	{
		if (InNewWidth == 0u || InNewHeight == 0u)
		{
			return;
		}
		bPendingResize      = TRUE;
		PendingResizeWidth  = InNewWidth;
		PendingResizeHeight = InNewHeight;
		bIsFullscreen       = FALSE;
	}

	const TArray<RResolutionItem>& EMainManager::GetSupportedResolutions() const
	{
		return SupportedResolutions;
	}

	BOOL32 EMainManager::GetIsFullscreen() const
	{
		return bIsFullscreen;
	}
};
