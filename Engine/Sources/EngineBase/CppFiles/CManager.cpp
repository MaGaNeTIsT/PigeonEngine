#include "../../../../Entry/EngineMain.h"
#include "../Headers/CManager.h"
#include "../Headers/CInput.h"
#include "../../../EngineThirdParty/CimGUI/Headers/CimGUIManager.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "../../EngineRender/AssetsManager/Headers/CShaderManager.h"
#include "../../EngineRender/AssetsManager/Headers/CTextureManager.h"
#include "../../EngineRender/AssetsManager/Headers/CMeshManager.h"
#include "../../EngineGame/Headers/CGameObjectManager.h"
#include "../../EngineGame/Headers/CScene.h"
#include "../../EngineRender/RenderBase/Headers/CRenderPipeline.h"

CManager* CManager::m_Manager = new CManager();

CManager::CManager()
{
	this->m_HWND			= NULL;
	this->m_WindowSize		= CustomType::Vector2Int(ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT);
	this->m_GraphicDepth	= 24u;
	this->m_FrameRate		= ENGINE_UPDATE_FRAME;
	this->m_Windowed		= ENGINE_WINDOWED;
	this->m_GameTimer		= new CGameTimer(&(this->m_WindowTimer));
}
CManager::CManager(const CManager& manager)
{
	if (this->m_GameTimer != NULL)
	{
		delete (this->m_GameTimer);
		this->m_GameTimer = NULL;
	}
	this->m_HWND			= manager.m_HWND;
	this->m_WindowSize		= manager.m_WindowSize;
	this->m_GraphicDepth	= manager.m_GraphicDepth;
	this->m_FrameRate		= manager.m_FrameRate;
	this->m_Windowed		= manager.m_Windowed;
	this->m_WindowTimer		= manager.m_WindowTimer;
	this->m_GameTimer		= new CGameTimer(&(this->m_WindowTimer));
}
CManager::~CManager()
{
	if (this->m_GameTimer != NULL)
	{
		delete (this->m_GameTimer);
		this->m_GameTimer = NULL;
	}
	if (this->m_RenderPipeline != NULL)
	{
		delete (this->m_RenderPipeline);
		this->m_RenderPipeline = NULL;
	}
	if (this->m_Scene != NULL)
	{
		delete (this->m_Scene);
		this->m_Scene = NULL;
	}
}
void CManager::Initialize(HWND hWnd)
{
	CManager::m_Manager->m_HWND = hWnd;

	CInput::Initialize();
	CRenderDevice::Initialize();
	CimGUIManager::Initialize();

	CGameObjectManager::Initialize();

	CManager::m_Manager->m_WindowTimer.Init();
}
void CManager::ShutDown()
{
	CShaderManager::ShutDown();
	CMeshManager::ShutDown();
	CTextureManager::ShutDown();

	CimGUIManager::ShutDown();
	CRenderDevice::ShutDown();
	CInput::ShutDown();

	delete (CManager::m_Manager);
}
void CManager::StaticUpdate()
{
	CManager::m_Manager->m_WindowTimer.Update();
	CInput::Update();
}
void CManager::Init()
{
	CRenderDevice::Init(
		CManager::m_Manager->m_HWND,
		CManager::m_Manager->m_WindowSize,
		CManager::m_Manager->m_GraphicDepth,
		CManager::m_Manager->m_FrameRate,
		CManager::m_Manager->m_Windowed);

	if (CManager::m_Manager->m_Scene != NULL)
	{
		delete (CManager::m_Manager->m_Scene);
		CManager::m_Manager->m_Scene = NULL;
	}
	if (CManager::m_Manager->m_RenderPipeline != NULL)
	{
		delete (CManager::m_Manager->m_RenderPipeline);
		CManager::m_Manager->m_RenderPipeline = NULL;
	}
	CManager::m_Manager->m_RenderPipeline = new CRenderPipeline();
	CManager::m_Manager->m_Scene = new CScene();
	CManager::m_Manager->m_RenderPipeline->Init(CManager::m_Manager->m_Scene, CManager::m_Manager->m_WindowSize);
	CManager::m_Manager->m_Scene->Init();
	CManager::m_Manager->m_RenderPipeline->PostInit();
	CManager::m_Manager->m_GameTimer->Reset();
}
void CManager::Uninit()
{
	CManager::m_Manager->m_Scene->Uninit();
	CManager::m_Manager->m_RenderPipeline->Uninit();

	if (CManager::m_Manager->m_Scene != NULL)
	{
		delete (CManager::m_Manager->m_Scene);
		CManager::m_Manager->m_Scene = NULL;
	}
	if (CManager::m_Manager->m_RenderPipeline != NULL)
	{
		delete (CManager::m_Manager->m_RenderPipeline);
		CManager::m_Manager->m_RenderPipeline = NULL;
	}
	if (CManager::m_Manager->m_GameTimer != NULL)
	{
		delete (CManager::m_Manager->m_GameTimer);
		CManager::m_Manager->m_GameTimer = NULL;
	}

	CRenderDevice::Uninit();
}
void CManager::Update()
{
	CManager::m_Manager->m_GameTimer->Update();
	CimGUIManager::Update();
	CManager::m_Manager->m_Scene->Update();
	CManager::m_Manager->m_RenderPipeline->PostUpdate();
}
void CManager::FixedUpdate()
{
	CManager::m_Manager->m_Scene->FixedUpdate();
}
void CManager::Draw()
{
	CManager::m_Manager->m_RenderPipeline->Render();
	CimGUIManager::Draw();
	CRenderDevice::Present();
}
void CManager::CalculateFrameStats()
{
	static UINT		frameCnt	= 0u;
	static FLOAT	timeElapsed	= 0.f;

	FLOAT totT = static_cast<FLOAT>(CManager::m_Manager->m_GameTimer->GetClockTime());
	frameCnt++;
	if ((totT - timeElapsed) >= 1.f)
	{
		FLOAT fps = static_cast<FLOAT>(frameCnt);
		FLOAT mspf = 1000.f / fps;
		std::string outs;
		outs = "Engine_D3D11";
		outs += " FPS: ";
		outs += std::to_string(fps);
		outs += " Frame Time: ";
		outs += std::to_string(mspf);
		outs += "(ms)";
		SetWindowText(CManager::m_Manager->m_HWND, outs.c_str());
		frameCnt = 0u;
		timeElapsed += 1.f;
	}
}
HWND CManager::GetWindowHandle()
{
	return (CManager::m_Manager->m_HWND);
}
const CTimer& CManager::GetWindowTimer()
{
	return (CManager::m_Manager->m_WindowTimer);
}
const CRenderPipeline* CManager::GetRenderPipeline()
{
	return (CManager::m_Manager->m_RenderPipeline);
}
const CGameTimer* CManager::GetGameTimer()
{
	return (CManager::m_Manager->m_GameTimer);
}
const CScene* CManager::GetScene()
{
	return (CManager::m_Manager->m_Scene);
}