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
#include "../../EngineGame/Headers/CScreenPolygon2D.h"

CManager* CManager::m_Manager = new CManager();
CManager::CManager()
{
	this->m_HWND = NULL;
	this->m_WindowSize = CustomType::Vector2Int(ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT);
	this->m_GraphicDepth = 24u;
	this->m_FrameRate = ENGINE_UPDATE_FRAME;
	this->m_Windowed = ENGINE_WINDOWED;
	this->m_GameTimer = new CGameTimer(&(this->m_WindowTimer));
}
CManager::CManager(const CManager& manager)
{
	if (this->m_GameTimer != NULL)
	{
		delete (this->m_GameTimer);
		this->m_GameTimer = NULL;
	}
	this->m_HWND = manager.m_HWND;
	this->m_WindowSize = manager.m_WindowSize;
	this->m_GraphicDepth = manager.m_GraphicDepth;
	this->m_FrameRate = manager.m_FrameRate;
	this->m_Windowed = manager.m_Windowed;
	this->m_WindowTimer = manager.m_WindowTimer;
	this->m_GameTimer = new CGameTimer(&(this->m_WindowTimer));
}
CManager::~CManager()
{
	if (this->m_GameTimer != NULL)
	{
		delete (this->m_GameTimer);
		this->m_GameTimer = NULL;
	}
	if (this->m_Scene != NULL)
	{
		delete (this->m_Scene);
		this->m_Scene = NULL;
	}
}
void CManager::Initialize(HWND hWnd)
{
	m_Manager->m_HWND = hWnd;

	CInput::Init();
	CRenderDevice::Initialize();
	CimGUIManager::Initialize();
	CGameObjectManager::Init();

	m_Manager->m_WindowTimer.Init();
}
void CManager::ShutDown()
{
	CShaderManager::Uninit();
	CMeshManager::Uninit();
	CTextureManager::Uninit();

	CimGUIManager::ShutDown();
	CRenderDevice::ShutDown();
	CInput::Uninit();

	delete (m_Manager);
}
void CManager::StaticUpdate()
{
	m_Manager->m_WindowTimer.Update();
}
void CManager::Init()
{
	CRenderDevice::Init(this->m_HWND, this->m_WindowSize, this->m_GraphicDepth, this->m_FrameRate, this->m_Windowed);

	if (this->m_Scene != NULL)
	{
		delete (this->m_Scene);
		this->m_Scene = NULL;
	}
	this->m_Scene = new CScene();
	this->m_Scene->Init();
	this->m_GameTimer->Reset();
}
void CManager::Uninit()
{
	this->m_Scene->Uninit();
	if (this->m_Scene != NULL)
	{
		delete (this->m_Scene);
		this->m_Scene = NULL;
	}

	CRenderDevice::Uninit();
}
void CManager::Update()
{
	CInput::Update();
	this->m_GameTimer->Update();
	CimGUIManager::Update();
	this->m_Scene->Update();
}
void CManager::FixedUpdate()
{
	this->m_Scene->FixedUpdate();
}
void CManager::Draw()
{
	this->m_Scene->Draw();
	CimGUIManager::Draw();
	CRenderDevice::Present();
}
void CManager::CalculateFrameStats()
{
	static UINT		frameCnt	= 0u;
	static FLOAT	timeElapsed	= 0.f;

	FLOAT totT = static_cast<FLOAT>(this->m_GameTimer->GetClockTime());
	frameCnt++;
	if ((totT - timeElapsed) >= 1.f)
	{
		FLOAT fps = static_cast<FLOAT>(frameCnt);
		FLOAT mspf = 1000.f / fps;
		std::strstream outs;
		outs << "Engine_D3D11" << " FPS: " << fps << " Frame Time: " << mspf << "(ms)" << std::ends;
		SetWindowText(this->m_HWND, outs.str());
		frameCnt = 0u;
		timeElapsed += 1.f;
	}
}