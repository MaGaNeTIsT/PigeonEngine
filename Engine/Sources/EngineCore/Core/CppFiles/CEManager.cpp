#include "../Headers/CEManager.h"
#include "../../Input/Headers/CInput.h"
#include "../../../../EngineThirdParty/Cassimp/Headers/CassimpManager.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineGame/Headers/CObjectManager.h"
#include "../../EngineGame/Headers/CScene.h"
#include "../../EngineRender/RenderBase/Headers/CRenderPipeline.h"

#include "../../EngineRender/RenderBase/Headers/CGPUQuery.h"
#include "../../EngineRender/RenderBase/Headers/CGPUQueryManager.h"

#ifdef _DEVELOPMENT_EDITOR

#include "../../../../EngineThirdParty/CimGUI/Headers/CimGUIManager.h"
#include "../../Development/Headers/CGPUProfiler.h"

#endif

namespace PigeonEngine
{
	CEManager*	CEManager::m_EngineManager	= new CEManager();

	CEManager::CEManager()
	{
		this->m_HWND			= NULL;
		this->m_WindowSize		= Vector2Int(ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT);
		this->m_GraphicDepth	= 24u;
		this->m_FrameRate		= ENGINE_UPDATE_FRAME;
		this->m_Windowed		= ENGINE_WINDOWED;
		this->m_GameTimer		= new CEGameTimer(&(this->m_WindowTimer));
		this->m_RenderPipeline	= nullptr;
		this->m_Scene			= nullptr;
	}
	CEManager::~CEManager()
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
	LRESULT CEManager::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return CInput::HandleMsg(hWnd, msg, wParam, lParam);
	}
	void CEManager::Initialize(HWND hWnd)
	{
		CEManager::m_EngineManager->m_HWND = hWnd;

		CInput::Initialize(hWnd);
		CRenderDevice::Initialize();

#ifdef _DEVELOPMENT_EDITOR

		CimGUIManager::Initialize();

#endif

		CassimpManager::Initialize();

		CUniqueIDManager::Initialize();

		CEManager::m_EngineManager->m_WindowTimer.Init();
	}
	void CEManager::ShutDown()
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
		CInput::ShutDown();

		delete (CEManager::m_EngineManager);
	}
	void CEManager::StaticUpdate()
	{
		CEManager::m_EngineManager->m_WindowTimer.Update();
	}
	void CEManager::Init()
	{
		CRenderDevice::Init(
			CEManager::m_EngineManager->m_HWND,
			CEManager::m_EngineManager->m_WindowSize,
			CEManager::m_EngineManager->m_GraphicDepth,
			CEManager::m_EngineManager->m_FrameRate,
			CEManager::m_EngineManager->m_Windowed);

		if (CEManager::m_EngineManager->m_Scene != NULL)
		{
			delete (CEManager::m_EngineManager->m_Scene);
			CEManager::m_EngineManager->m_Scene = NULL;
		}
		if (CEManager::m_EngineManager->m_RenderPipeline != NULL)
		{
			delete (CEManager::m_EngineManager->m_RenderPipeline);
			CEManager::m_EngineManager->m_RenderPipeline = NULL;
		}
		CEManager::m_EngineManager->m_RenderPipeline = new CRenderPipeline();
		CEManager::m_EngineManager->m_Scene = new CScene();
		CEManager::m_EngineManager->m_RenderPipeline->Init(CEManager::m_EngineManager->m_Scene, CEManager::m_EngineManager->m_WindowSize);
		CEManager::m_EngineManager->m_Scene->Init();
		CEManager::m_EngineManager->m_RenderPipeline->PostInit();
		CEManager::m_EngineManager->m_GameTimer->Reset();

#ifdef _DEVELOPMENT_EDITOR
		CEManager::m_EngineManager->m_EditorOpen = true;
#endif
	}
	void CEManager::Uninit()
	{
		CEManager::m_EngineManager->m_Scene->Uninit();
		CEManager::m_EngineManager->m_RenderPipeline->Uninit();

		if (CEManager::m_EngineManager->m_Scene != NULL)
		{
			delete (CEManager::m_EngineManager->m_Scene);
			CEManager::m_EngineManager->m_Scene = NULL;
		}
		if (CEManager::m_EngineManager->m_RenderPipeline != NULL)
		{
			delete (CEManager::m_EngineManager->m_RenderPipeline);
			CEManager::m_EngineManager->m_RenderPipeline = NULL;
		}
		if (CEManager::m_EngineManager->m_GameTimer != NULL)
		{
			delete (CEManager::m_EngineManager->m_GameTimer);
			CEManager::m_EngineManager->m_GameTimer = NULL;
		}

		CRenderDevice::Uninit();
	}
	void CEManager::Update()
	{
		CEManager::m_EngineManager->m_GameTimer->Update();
		CInput::Update();
#ifdef _DEVELOPMENT_EDITOR
		CimGUIManager::Update();
		CManager::EditorUpdate();
#endif
		CEManager::m_EngineManager->m_Scene->Update();
		CEManager::m_EngineManager->m_RenderPipeline->PostUpdate();
	}
	void CEManager::FixedUpdate()
	{
		CEManager::m_EngineManager->m_Scene->FixedUpdate();
	}
	void CEManager::Draw()
	{
		CEManager::m_EngineManager->m_RenderPipeline->Render();
		CimGUIManager::Draw();
		CRenderDevice::Present();
	}
#ifdef _DEVELOPMENT_EDITOR
	void CEManager::EditorUpdate()
	{
		bool editorOpen = CEManager::m_EngineManager->m_EditorOpen;
		ImGui::Begin("EditorRoot", &editorOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
		ImGui::Text("Frame Rate Average %.3f ms/frame (%.1f FPS)", 1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		CEManager::m_EngineManager->m_Scene->EditorUpdate();
		CEManager::m_EngineManager->m_RenderPipeline->EditorUpdate();
		ImGui::End();
		CEManager::m_EngineManager->m_EditorOpen = editorOpen;
	}
#endif
	HWND CEManager::GetWindowHandle()
	{
		return (CEManager::m_EngineManager->m_HWND);
	}
	const CEBaseTimer& CEManager::GetWindowTimer()
	{
		return (CEManager::m_EngineManager->m_WindowTimer);
	}
	const CEGameTimer* CEManager::GetGameTimer()
	{
		return (CEManager::m_EngineManager->m_GameTimer);
	}
	const CRenderPipeline* CEManager::GetRenderPipeline()
	{
		return (CEManager::m_EngineManager->m_RenderPipeline);
	}
	const CScene* CEManager::GetScene()
	{
		return (CEManager::m_EngineManager->m_Scene);
	}
};