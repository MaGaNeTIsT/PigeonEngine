#include "../Headers/CScene.h"
#include "../../EngineBase/Headers/CTimer.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineBase/Headers/CInput.h"
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
	//CPlane* terrainPlane = this->AddGameObject<CPlane>(SceneLayout::LAYOUT_TERRAIN);
	//CCube* cube = this->AddGameObject<CCube>(SceneLayout::LAYOUT_OPAQUE);
	//CPlane* testPlane = this->AddGameObject<CPlane>(SceneLayout::LAYOUT_OPAQUE);

	mainCamera->SetPosition(CustomType::Vector3(0.f, 0.6f, -3.f));
	mainLight->SetRotation(CustomType::Quaternion(mainLight->GetRightVector(), 30.f * CustomType::CMath::GetDegToRad()));
	//terrainPlane->SetMeshInfo(100.f, 50, 50.f);
	//cube->SetPosition(CustomType::Vector3(0.f, 0.f, -70.f));
	//cube->SetScale(CustomType::Vector3(150.f, 150.f, 10.f));
	//testPlane->SetPosition(CustomType::Vector3(0.f, 0.5f, -4.f));
	//testPlane->SetScale(CustomType::Vector3(3.f, 3.f, 3.f));

	{
		//static_cast<UINT>(::time(NULL));
		::srand(12415u);

		const INT randomMax = 3000;
		const FLOAT rangePos = 50.f;
		//const FLOAT rangePos = 200.f;
		const FLOAT baseScale = 0.05f;
		const FLOAT rangeScale = 0.15f;
		const UINT countModel = 4u;
		//const UINT countModel = 200u;
		const BOOL showOBB = FALSE;

		INT random[3] = { 0, 0, 0 };
		FLOAT t[3] = { 0.f, 0.f, 0.f };
		for (UINT i = 0u; i < countModel; i++)
		{
			CTestModel* model = this->AddGameObject<CTestModel>(SceneLayout::LAYOUT_OPAQUE);
			random[0] = rand() % randomMax;
			t[0] = static_cast<FLOAT>(random[0]) / static_cast<FLOAT>((randomMax - 1));
			FLOAT scale = baseScale + t[0] * rangeScale;
			model->SetScale(CustomType::Vector3(scale, scale, scale));
			random[0] = rand() % randomMax;
			random[1] = rand() % randomMax;
			random[2] = rand() % randomMax;
			t[0] = static_cast<FLOAT>(random[0]) / static_cast<FLOAT>((randomMax - 1));
			t[1] = static_cast<FLOAT>(random[1]) / static_cast<FLOAT>((randomMax - 1));
			t[2] = static_cast<FLOAT>(random[2]) / static_cast<FLOAT>((randomMax - 1));
			model->SetPosition(CustomType::Vector3((t[0] - 0.5f) * rangePos, (t[1] - 0.5f) * rangePos, (t[2] - 0.5f) * rangePos));
			if (showOBB)
			{
				CCube* cube = this->AddGameObject<CCube>(SceneLayout::LAYOUT_OPAQUE);
				CustomType::Vector3 min, max;
				model->GetAABBBoundingBox(min, max);
				cube->SetPosition(min + ((max - min) * 0.5f));
				cube->SetScale(max - min);
			}
		}
	}
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