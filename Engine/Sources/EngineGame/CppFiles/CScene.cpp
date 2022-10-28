#include "../Headers/CScene.h"
#include "../../EngineBase/Headers/CTimer.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineBase/Headers/CInput.h"
#include "../Headers/CCamera.h"
#include "../Headers/CLightType.h"
#include "../Headers/CScreenPolygon2D.h"
#include "../Headers/CPlane.h"
#include "../Headers/CCube.h"

#include "../../Development/Headers/CTestModel.h"

CScene::CScene()
{
	this->m_MainCamera = NULL;
}
CScene::~CScene()
{
	if (this->m_MainCamera)
	{
		this->m_MainCamera->Uninit();
		delete (this->m_MainCamera);
		this->m_MainCamera = NULL;
	}
	if (this->m_Lights.size() > 0u)
	{
		for (const auto& object : this->m_Lights)
		{
			if (object.second)
			{
				object.second->Uninit();
				delete (object.second);
			}
		}
		this->m_Lights.clear();
	}
	for (INT i = 0; i < SceneLayout::LAYOUT_COUNT; ++i)
	{
		if (this->m_GameObjects[i].size() > 0u)
		{
			for (const auto& object : this->m_GameObjects[i])
			{
				if (object.second)
				{
					object.second->Uninit();
					delete (object.second);
				}
			}
			this->m_GameObjects[i].clear();
		}
	}
}
void CScene::Init()
{
	CCamera* mainCamera = this->AddCamera<CCamera>();

	CLightDirectional* mainLight = this->AddLight<CLightDirectional>();
	CPlane* terrainPlane = this->AddGameObject<CPlane>(SceneLayout::LAYOUT_TERRAIN);
	CCube* cube = this->AddGameObject<CCube>(SceneLayout::LAYOUT_OPAQUE);

	mainCamera->SetPosition(CustomType::Vector3(0.f, 5.f, -5.f));
	mainLight->SetRotation(CustomType::Quaternion(mainLight->GetRightVector(), 90.f * CustomType::CMath::GetDegToRad()));
	terrainPlane->SetMeshInfo(100.f, 8, 3.f);
	cube->SetPosition(CustomType::Vector3(0.f, 50.f, 0.f));
	cube->SetScale(CustomType::Vector3(10.f, 10.f, 10.f));

	{
		//static_cast<UINT>(::time(NULL));
		::srand(12415u);

		const INT randomMax = 3000;
		//const FLOAT rangePos = 50.f;
		const FLOAT rangePos = 200.f;
		const FLOAT baseScale = 0.05f;
		const FLOAT rangeScale = 0.15f;
		//const UINT countModel = 4u;
		const UINT countModel = 200u;
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
	if (this->m_MainCamera)
	{
		this->m_MainCamera->Uninit();
		delete (this->m_MainCamera);
		this->m_MainCamera = NULL;
	}
	if (this->m_Lights.size() > 0u)
	{
		for (const auto& object : this->m_Lights)
		{
			if (object.second)
			{
				object.second->Uninit();
				delete (object.second);
			}
		}
		this->m_Lights.clear();
	}
	for (INT i = 0; i < SceneLayout::LAYOUT_COUNT; ++i)
	{
		if (this->m_GameObjects[i].size() > 0u)
		{
			for (const auto& object : this->m_GameObjects[i])
			{
				if (object.second)
				{
					object.second->Uninit();
					delete (object.second);
				}
			}
			this->m_GameObjects[i].clear();
		}
	}
}
void CScene::Update()
{
	this->m_MainCamera->Update();

	for (const auto& object : this->m_Lights)
	{
		object.second->Update();
	}
	for (INT i = 0; i < SceneLayout::LAYOUT_COUNT; ++i)
	{
		for (const auto& object : this->m_GameObjects[i])
		{
			object.second->Update();
		}
	}

	{
		std::pair<INT, INT> mousePos = CInput::Controller.GetMousePosition();
		ImGui::Begin("Scene Manager");
		ImGui::Text("Mouse position : x = %d, y = %d.", mousePos.first, mousePos.second);
		ImGui::End();
	}
}
void CScene::FixedUpdate()
{
	this->m_MainCamera->FixedUpdate();

	for (const auto& object : this->m_Lights)
	{
		object.second->FixedUpdate();
	}
	for (INT i = 0; i < SceneLayout::LAYOUT_COUNT; ++i)
	{
		for (const auto& object : this->m_GameObjects[i])
		{
			object.second->FixedUpdate();
		}
	}
}