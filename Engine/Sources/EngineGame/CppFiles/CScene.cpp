#include "../Headers/CScene.h"
#include "../../EngineBase/Headers/CTimer.h"
#include "../../EngineBase/Headers/CManager.h"
#include "../../EngineBase/Headers/CInput.h"
#include "../Headers/CCamera.h"
#include "../Headers/CLightType.h"
#include "../Headers/CScreenPolygon2D.h"
#include "../Headers/CPlane.h"
#include "../Headers/CCube.h"

#include "../../Development/Headers/CSceneGameObject.h"

CScene::CScene()
{
	this->m_MainCamera = NULL;
#if _DEVELOPMENT_EDITOR
	this->m_SelectedObject = NULL;
#endif
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
	CSceneGameObject* sceneGameObject = this->AddGameObject<CSceneGameObject>(SceneLayout::LAYOUT_OPAQUE);

	mainCamera->SetWorldPosition(CustomType::Vector3(0.f, 150.f, -50.f));
	mainLight->SetWorldRotation(CustomType::Quaternion(mainLight->GetRightVector(), 60.f * CustomType::CMath::GetDegToRad()));
	terrainPlane->SetMeshInfo(1000.f, 8, 3.f);
	cube->SetWorldPosition(CustomType::Vector3(0.f, 0.f, 0.f));
	cube->SetWorldScale(CustomType::Vector3(100.f, 100.f, 100.f));
	sceneGameObject->SetWorldPosition(CustomType::Vector3(0.f, 150.f, 0.f));
	sceneGameObject->SetWorldScale(CustomType::Vector3(100.f, 100.f, 100.f));

#if _DEVELOPMENT_EDITOR
	this->m_SelectedObject = sceneGameObject;
#endif
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
#if _DEVELOPMENT_EDITOR
	{
		if (this->m_SelectedObject != NULL)
		{
			this->m_SelectedObject->SelectedEditorUpdate();
		}
	}

	{
		CustomType::Vector3 cameraPos(this->m_MainCamera->GetWorldPosition());
		CustomType::Vector3 cameraDir(this->m_MainCamera->GetForwardVector());
		std::pair<INT, INT> mousePos = CInput::Controller.GetMousePosition();
		ImGui::Begin("Scene Manager");
		ImGui::Text("Mouse position : x = %d, y = %d.", mousePos.first, mousePos.second);
		ImGui::Text("Camera position :\nx = %f\ny = %f\nz = %f", cameraPos.X(), cameraPos.Y(), cameraPos.Z());
		ImGui::Text("Camera direction :\nx = %f\ny = %f\nz = %f", cameraDir.X(), cameraDir.Y(), cameraDir.Z());
		UINT index = 0u;
		for (const auto& object : this->m_Lights)
		{
			CustomType::Vector3 lightPos(object.second->GetWorldPosition());
			CustomType::Vector3 lightDir(object.second->GetForwardVector());
			ImGui::Text("Light index = %d", index);
			ImGui::Text("Light position :\nx = %f\ny = %f\nz = %f", lightPos.X(), lightPos.Y(), lightPos.Z());
			ImGui::Text("Light direction :\nx = %f\ny = %f\nz = %f", lightDir.X(), lightDir.Y(), lightDir.Z());
			index++;
		}
		ImGui::End();
	}
#endif

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