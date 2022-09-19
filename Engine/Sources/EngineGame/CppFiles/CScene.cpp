#include "../Headers/CScene.h"
#include "../../EngineBase/Headers/CTimer.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../Headers/CCamera.h"
#include "../Headers/CLight.h"
#include "../Headers/CScreenPolygon2D.h"
#include "../Headers/CPlane.h"
#include "../Headers/CCube.h"

#include "../../Development/Headers/CTestModel.h"

CScene::CScene()
{
}
CScene::~CScene()
{
}
void CScene::Init()
{
	CCamera* mainCamera = this->AddGameObject<CCamera>(SceneLayout::LAYOUT_CAMERA);

	CLight* mainLight = this->AddGameObject<CLight>(SceneLayout::LAYOUT_LIGHT);
	CPlane* terrainPlane = this->AddGameObject<CPlane>(SceneLayout::LAYOUT_TERRAIN);
	CCube* cube = this->AddGameObject<CCube>(SceneLayout::LAYOUT_OPAQUE);
	CTestModel* model = this->AddGameObject<CTestModel>(SceneLayout::LAYOUT_OPAQUE);
	CPlane* testPlane = this->AddGameObject<CPlane>(SceneLayout::LAYOUT_OPAQUE);

	mainCamera->SetPosition(CustomType::Vector3(0.f, 0.6f, -3.f));
	mainLight->SetRotation(CustomType::Quaternion(mainLight->GetRightVector(), 30.f * CustomType::CMath::GetDegToRad()));
	terrainPlane->SetMeshInfo(100.f, 50, 50.f);
	cube->SetPosition(CustomType::Vector3(0.f, 0.5f, 0.f));
	model->SetScale(CustomType::Vector3(0.03f, 0.03f, 0.03f));
	model->SetPosition(CustomType::Vector3(0.f, 5.f, 5.f));
	testPlane->SetPosition(CustomType::Vector3(0.f, 0.5f, -4.f));;
	testPlane->SetScale(CustomType::Vector3(3.f, 3.f, 3.f));;
}
void CScene::Uninit()
{
	for (INT i = 0; i < SceneLayout::LAYOUT_COUNT; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
		{
			object.second->Uninit();
			delete (object.second);
		}
		this->m_GameObject[i].clear();
	}
}
void CScene::Update()
{
	for (INT i = 0; i < SceneLayout::LAYOUT_COUNT; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
			object.second->Update();
	}
}
void CScene::FixedUpdate()
{
	for (INT i = 0; i < SceneLayout::LAYOUT_COUNT; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
			object.second->FixedUpdate();
	}
}