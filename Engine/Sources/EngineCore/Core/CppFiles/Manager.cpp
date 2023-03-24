#include "../Headers/Manager.h"
#include "../../Input/Headers/Input.h"
#include "../../Base/Headers/ObjectBase.h"
#include "../../../../EngineThirdParty/assimp/Headers/assimpManager.h"
#include "../../RenderCore/Headers/DeviceD3D11.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineGame/Headers/CObjectManager.h"
#include "../../EngineGame/Headers/CScene.h"
#include "../../EngineRender/RenderBase/Headers/CRenderPipeline.h"

#include "../../EngineRender/RenderBase/Headers/CGPUQuery.h"
#include "../../EngineRender/RenderBase/Headers/CGPUQueryManager.h"

#ifdef _DEVELOPMENT_EDITOR

#include "../../../../EngineThirdParty/imGUI/Headers/imGUIManager.h"
#include "../../Development/Headers/CGPUProfiler.h"

#endif

namespace PigeonEngine
{
	EManager*	EManager::m_EngineManager	= new EManager();

	EManager::EManager()
	{
		this->m_HWND			= NULL;
		this->m_WindowSize		= Vector2Int(ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT);
		this->m_GraphicDepth	= 24u;
		this->m_FrameRate		= ENGINE_UPDATE_FRAME;
		this->m_Windowed		= ENGINE_WINDOWED;
		this->m_GameTimer		= new EGameTimer(&(this->m_WindowTimer));
		this->m_RenderPipeline	= nullptr;
		this->m_Scene			= nullptr;
	}
	EManager::~EManager()
	{
		if (this->m_GameTimer != nullptr)
		{
			delete (this->m_GameTimer);
			this->m_GameTimer = nullptr;
		}
		if (this->m_RenderPipeline != nullptr)
		{
			delete (this->m_RenderPipeline);
			this->m_RenderPipeline = nullptr;
		}
		if (this->m_Scene != nullptr)
		{
			delete (this->m_Scene);
			this->m_Scene = nullptr;
		}
	}
	LRESULT EManager::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return EInput::HandleMsg(hWnd, msg, wParam, lParam);
	}
	void EManager::Initialize(HWND hWnd)
	{
		EManager::m_EngineManager->m_HWND = hWnd;

		EUniqueIDAllocater::Initialize();
		EInput::Initialize(hWnd);
		RDeviceD3D11::Initialize();

#ifdef _DEVELOPMENT_EDITOR

		CimGUIManager::Initialize();

#endif

		CassimpManager::Initialize();

		CUniqueIDManager::Initialize();

		EManager::m_EngineManager->m_WindowTimer.Init();
	}
	void EManager::ShutDown()
	{
#ifdef _DEVELOPMENT_EDITOR
		CGPUProfilerManager::ShutDown();
#endif
		CGPUQueryManager::ShutDown();

		CShaderManager::ShutDown();
		CMeshManager::ShutDown();
		CTextureManager::ShutDown();

		CassimpManager::ShutDown();
#ifdef _DEVELOPMENT_EDITOR
		CimGUIManager::ShutDown();
#endif
		CRenderDevice::ShutDown();
		EInput::ShutDown();
		EUniqueIDAllocater::ShutDown();

		delete (EManager::m_EngineManager);
	}
	void EManager::StaticUpdate()
	{
		EManager::m_EngineManager->m_WindowTimer.Update();
	}
	void EManager::Init()
	{
		CRenderDevice::Init(
			CEManager::m_EngineManager->m_HWND,
			CEManager::m_EngineManager->m_WindowSize,
			CEManager::m_EngineManager->m_GraphicDepth,
			CEManager::m_EngineManager->m_FrameRate,
			CEManager::m_EngineManager->m_Windowed);

		if (EManager::m_EngineManager->m_Scene != NULL)
		{
			delete (EManager::m_EngineManager->m_Scene);
			EManager::m_EngineManager->m_Scene = NULL;
		}
		if (EManager::m_EngineManager->m_RenderPipeline != NULL)
		{
			delete (EManager::m_EngineManager->m_RenderPipeline);
			EManager::m_EngineManager->m_RenderPipeline = NULL;
		}
		EManager::m_EngineManager->m_RenderPipeline = new CRenderPipeline();
		EManager::m_EngineManager->m_Scene = new CScene();
		EManager::m_EngineManager->m_RenderPipeline->Init(EManager::m_EngineManager->m_Scene, EManager::m_EngineManager->m_WindowSize);
		EManager::m_EngineManager->m_Scene->Init();
		EManager::m_EngineManager->m_RenderPipeline->PostInit();
		EManager::m_EngineManager->m_GameTimer->Reset();

#ifdef _DEVELOPMENT_EDITOR
		EManager::m_EngineManager->m_EditorOpen = true;
#endif
	}
	void EManager::Uninit()
	{
		EManager::m_EngineManager->m_Scene->Uninit();
		EManager::m_EngineManager->m_RenderPipeline->Uninit();

		if (EManager::m_EngineManager->m_Scene != NULL)
		{
			delete (EManager::m_EngineManager->m_Scene);
			EManager::m_EngineManager->m_Scene = NULL;
		}
		if (EManager::m_EngineManager->m_RenderPipeline != NULL)
		{
			delete (EManager::m_EngineManager->m_RenderPipeline);
			EManager::m_EngineManager->m_RenderPipeline = NULL;
		}
		if (EManager::m_EngineManager->m_GameTimer != NULL)
		{
			delete (EManager::m_EngineManager->m_GameTimer);
			EManager::m_EngineManager->m_GameTimer = NULL;
		}

		CRenderDevice::Uninit();
	}
	void EManager::Update()
	{
		EManager::m_EngineManager->m_GameTimer->Update();
		//EInput::Update();
#ifdef _DEVELOPMENT_EDITOR
		CimGUIManager::Update();
		EManager::EditorUpdate();
#endif
		EManager::m_EngineManager->m_Scene->Update();
		EManager::m_EngineManager->m_RenderPipeline->PostUpdate();
	}
	void EManager::FixedUpdate()
	{
		EManager::m_EngineManager->m_Scene->FixedUpdate();
	}
	void EManager::Draw()
	{
		EManager::m_EngineManager->m_RenderPipeline->Render();
		CimGUIManager::Draw();
		CRenderDevice::Present();
	}
#ifdef _DEVELOPMENT_EDITOR
	void EManager::EditorUpdate()
	{
		bool editorOpen = EManager::m_EngineManager->m_EditorOpen;
		ImGui::Begin("EditorRoot", &editorOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
		ImGui::Text("Frame Rate Average %.3f ms/frame (%.1f FPS)", 1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		EManager::m_EngineManager->m_Scene->EditorUpdate();
		EManager::m_EngineManager->m_RenderPipeline->EditorUpdate();
		ImGui::End();
		EManager::m_EngineManager->m_EditorOpen = editorOpen;
	}
#endif
	HWND EManager::GetWindowHandle()
	{
		return (EManager::m_EngineManager->m_HWND);
	}
	const EBaseTimer& EManager::GetWindowTimer()
	{
		return (EManager::m_EngineManager->m_WindowTimer);
	}
	const EGameTimer* EManager::GetGameTimer()
	{
		return (EManager::m_EngineManager->m_GameTimer);
	}
	const CRenderPipeline* EManager::GetRenderPipeline()
	{
		return (EManager::m_EngineManager->m_RenderPipeline);
	}
	const CScene* EManager::GetScene()
	{
		return (EManager::m_EngineManager->m_Scene);
	}
};