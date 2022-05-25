#include "../../Entry/MyMain.h"
#include "../../Headers/Base/CManager.h"
#include "../../Headers/Base/CTimer.h"
#include "../../Headers/Base/CInput.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Base/CShaderManager.h"
#include "../../Headers/Base/CTextureManager.h"
#include "../../Headers/Game/CMeshManager.h"
#include "../../Headers/Game/CGameObjectManager.h"
#include "../../Headers/Game/CScene.h"
#include "../../Headers/Object/CScreenPolygon2D.h"

CScene*		CManager::m_Scene;
CGameTimer* CManager::m_Timer;

void CManager::Init()
{
	CRenderDevice::Init();
	CInput::Init();
	CGameObjectManager::Init();

	m_Timer = new CGameTimer(GetMainWindowTimer());
	m_Timer->Reset();
	m_Scene = new CScene();
	m_Scene->Init();
}

void CManager::Uninit()
{
	m_Scene->Uninit();
	delete m_Scene;
	delete m_Timer;

	CInput::Uninit();
	CShaderManager::Uninit();
	CMeshManager::Uninit();
	CTextureManager::Uninit();
	CRenderDevice::Uninit();
}

void CManager::Update()
{
	m_Timer->Update();
	CalculateFrameStats();

	CInput::Update();

	CRenderDevice::GetDeferredResolve()->Update();
	CRenderDevice::GetPostEffect()->Update();

	m_Scene->Update();
}

void CManager::Draw()
{
	m_Scene->Draw();
}

void CManager::CalculateFrameStats()
{
	static UINT frameCnt = 0;
	static FLOAT timeElapsed = 0.f;
	FLOAT totT = static_cast<FLOAT>(m_Timer->GetClockTime());

	frameCnt++;
	if ((totT - timeElapsed) >= 1.f)
	{
		FLOAT fps = static_cast<FLOAT>(frameCnt);
		FLOAT mspf = 1000.f / fps;
		std::strstream outs;
		outs << "  DirectX113DGame  " << "    FPS: " << fps
			<< "    Frame Time: " << mspf << "(ms)" << std::ends;
		SetWindowText(GetMainWindowHandle(), outs.str());
		frameCnt = 0;
		timeElapsed += 1.f;
	}
}