#include "../../Entry/MyMain.h"
#include "../../Headers/Base/CManager.h"
#include "../../Headers/Base/CTimer.h"
#include "../../Headers/Base/CInput.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Base/CReadOBJ.h"
#include "../../Headers/Game/CScene.h"
#include "../../Headers/Object/CField.h"
#include "../../Headers/Object/CPolygon2D.h"
#include "../../Headers/Object/CPolygon.h"

CScene*		CManager::m_Scene;
CTimer		CManager::m_Timer;

void CManager::Init()
{
	CRenderDevice::Init();
	CInput::Init();

	m_Scene = new CScene();
	m_Scene->Init();

	m_Timer.Init();
}

void CManager::Uninit()
{
	m_Scene->Uninit();
	delete m_Scene;

	CInput::Uninit();
	CRenderDevice::Uninit();
}

void CManager::Update()
{
	m_Timer.Update();
	CalculateFrameStats();

	CInput::Update();

	CRenderDevice::GetDeferredQuadBefore()->Update();
	CRenderDevice::GetDeferredQuadAfter()->Update();

	m_Scene->Update();
}

void CManager::Draw()
{
	m_Scene->Draw();
}

void CManager::CalculateFrameStats()
{
	static INT frameCnt = 0;
	static FLOAT timeElapsed = 0.f;
	DOUBLE totT = m_Timer.GetClockTime();
	frameCnt++;
	if ((totT - timeElapsed) >= 1.f)
	{
		FLOAT fps = (FLOAT)frameCnt;
		FLOAT mspf = 1000.f / fps;
		std::strstream outs;
		outs << "  DirectX113DGame  " << "    FPS: " << fps
			<< "    Frame Time: " << mspf << "(ms)" << std::ends;
		SetWindowText(GetWindow(), outs.str());
		frameCnt = 0;
		timeElapsed += 1.f;
	}
}