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
template <typename T>
T* CScene::AddGameObject(const UINT& layout)
{
	CGameObject* gameObject = new T();
	gameObject->SetScene(this);
	gameObject->Active();
	gameObject->Init();
	ULONGLONG id = gameObject->GetGameObjectID();
	(this->m_GameObject[layout])[id] = gameObject;
	return  (reinterpret_cast<T*>(gameObject));
}
template <typename T>
T* CScene::GetGameObjectFirst(const UINT& layout)const
{
	if (this->m_GameObject[layout].size() < 1)
		return NULL;
	for (const auto& obj : (this->m_GameObject[layout]))
	{
		if ((obj.second) != NULL)
		{
			if (typeid(*(obj.second)) == typeid(T))
				return (reinterpret_cast<T*>(obj.second));
		}
	}
	return NULL;
}
template <typename T>
T* CScene::GetGameObjectByIndex(const UINT& layout, const UINT& idx)const
{
	if (this->m_GameObject[layout].size() < 1)
		return NULL;
	UINT number = idx;
	for (const auto& obj : (this->m_GameObject[layout]))
	{
		if ((obj.second) != NULL)
		{
			if (typeid(*(obj.second)) == typeid(T))
			{
				if (number == 0)
					return (reinterpret_cast<T*>(obj.second));
				else
					number -= 1;
			}
		}
	}
	return NULL;
}
template <typename T>
std::vector<T*> CScene::GetGameObjectAll(const UINT& layout)const
{
	std::vector<T*> listObj;
	if (this->m_GameObject[layout].size() < 1)
		return listObj;
	for (const auto& obj : (this->m_GameObject[layout]))
	{
		if ((obj.second) != NULL)
		{
			if (typeid(*(obj.second)) == typeid(T))
				listObj.push_back((reinterpret_cast<T*>(obj.second)));
		}
	}
	return listObj;
}
void CScene::Init()
{
	CCamera* mainCamera = this->AddGameObject<CCamera>(SceneLayout::LAYOUT_CAMERA);

	CLight* mainLight = this->AddGameObject<CLight>(SceneLayout::LAYOUT_LIGHT);
	CPlane* terrainPlane = this->AddGameObject<CPlane>(SceneLayout::LAYOUT_TERRAIN);
	CCube* cube = this->AddGameObject<CCube>(SceneLayout::LAYOUT_OPAQUE);
	CTestModel* sphere = this->AddGameObject<CTestModel>(SceneLayout::LAYOUT_OPAQUE);
	CPlane* testPlane = this->AddGameObject<CPlane>(SceneLayout::LAYOUT_OPAQUE);

	mainCamera->SetPosition(CustomType::Vector3(0.f, 0.6f, -3.f));
	mainLight->SetRotation(CustomType::Quaternion(mainLight->GetRightVector(), 30.f * CustomType::CMath::GetDegToRad()));
	terrainPlane->SetMeshInfo(100.f, 50, 50.f);
	cube->SetPosition(CustomType::Vector3(0.f, 0.5f, 0.f));
	sphere->SetPosition(CustomType::Vector3(0.f, 0.5f, 2.f));
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