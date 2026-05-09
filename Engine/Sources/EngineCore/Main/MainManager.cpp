#include "MainManager.h"
#include "RenderThread.h"
#include "PigeonBase/Object/World/WorldTickManager.h"
#include <CoreMinimal.h>
#include <Config/EngineConfig.h>
#include <RenderDevice/DeviceD3D11.h>
#include <Renderer/RenderScene.h>
#include <Renderer/SceneRenderer.h>
#include "../../../EngineThirdParty/JoltPhysics/Headers/PhysicsManager.h"

#include "PigeonBase/Object/World/World.h"
#include "PigeonBase/Object/World/WorldManager.h"

#include "Development/Message/Message.h"

#if _EDITOR_ONLY
#include "../../../EngineThirdParty/imGUI/Headers/imGUIManager.h"
#include "../../../EngineThirdParty/assimp/Headers/assimpManager.h"
#include "../Editor/EditorManager.h"
#endif

#if _EDITOR_ONLY
#include <RenderProxy/RenderSingletonObject.h>
#endif

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
		
		RenderDevice = RDeviceD3D11::GetDeviceSingleton();

		m_PhysicsManager	= FPhysicsManager::GetManagerSingleton();
#if _EDITOR_ONLY
		m_ImGUIManager		= CImGUIManager::GetManagerSingleton();
		m_AssimpManager		= CAssimpManager::GetManagerSingleton();
		m_EditorManager     = EEditorManager::GetManagerSingleton();
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
			DeviceInitDesc.Backend			= ERHIBackendType::RHI_BACKEND_D3D11;
			DeviceInitDesc.bIsWindowed		= (m_Windowed != FALSE);
			RenderDevice->Initialize(DeviceInitDesc);
		}

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
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("RenderThread already created."), (!RenderThread));
			RenderThread = new RRenderThread(RenderDevice, RenderScene, SceneRenderer);
#if _EDITOR_ONLY
			RenderThread->SetAfterRenderCallback([this]() -> void
			{
				// PrepareDrawData (ImGui::Render) is invoked on the main thread
				// during Update() Phase 3 so the GameThread can finalize ImGui
				// frame state while we wait on the previous frame. Here on the
				// RenderThread we only replay the produced ImDrawData against
				// the D3D11 immediate context.
				m_ImGUIManager->RenderDrawData();
			});
#endif
			RenderThread->Start();
		}

#if _EDITOR_ONLY
		m_ImGUIManager->Initialize();
		m_AssimpManager->Initialize();
		m_EditorManager->Initialize();
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
#endif

		{
			// RenderThread has already been stopped at the top of ShutDown.
			RenderScene = nullptr;
			SceneRenderer->ShutDown();
			delete SceneRenderer;
			SceneRenderer = nullptr;
		}

		RenderDevice->Shutdown();
		EInput::ShutDown();

		m_MessageManager->ShutDown();
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

		RenderThread->WaitForRenderIdle();

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
		m_ImGUIManager->PrepareDrawData();		// ImGui::Render — DrawData ready for the worker
#endif

		// Swap double buffer
#if _EDITOR_ONLY
		RDebugWireframePrimitiveManager::GetManagerSingleton()->SwapCommandSlots();
#endif
		RenderScene->SwapCommandSlots();
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
	const EBaseTimer& EMainManager::GetWindowTimer()
	{
		return (m_WindowTimer);
	}
	const EGameTimer* EMainManager::GetGameTimer()
	{
		return (m_GameTimer);
	}
};