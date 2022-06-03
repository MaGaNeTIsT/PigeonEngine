#include "../../Headers/Game/CScene.h"
#include "../../Headers/Base/CTimer.h"
#include "../../Headers/Base/CManager.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Game/CCamera.h"
#include "../../Headers/Game/CDeferredBuffer.h"
#include "../../Headers/Game/CLight.h"
#include "../../Headers/Object/CScreenPolygon2D.h"
#include "../../Headers/Object/CPlane.h"
#include "../../Headers/Object/CCube.h"

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
	gameObject->Init();
	gameObject->SetScene(this);
	this->m_GameObject[layout][gameObject->GetGameObjectID()] = gameObject;
	return  (reinterpret_cast<T*>(gameObject));
}
template <typename T>
T* CScene::GetGameObjectFirst(const UINT& layout)
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
T* CScene::GetGameObjectByIndex(const UINT& layout, const UINT& idx)
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
std::vector<T*> CScene::GetGameObjectAll(const UINT& layout)
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
	CRenderDevice::CreateConstantBuffer(this->m_ConstantBuffer, sizeof(CustomStruct::ConstantBufferPerFrame));

	CCamera* mainCamera = this->AddGameObject<CCamera>(SCENELAYOUT_CAMERA);
	CLight* mainLight = this->AddGameObject<CLight>(SCENELAYOUT_LIGHT);
	CPlane* terrainPlane = this->AddGameObject<CPlane>(SCENELAYOUT_TERRAIN);
	CCube* cube = this->AddGameObject<CCube>(SCENELAYOUT_OPAQUE);
	//this->AddGameObject<CSky>(SCENELAYOUT_SKY);
	//this->AddGameObject<CWater>(SCENELAYOUT_TRANSPARENT);

	this->m_DebugScreen.SetScene(this);
	this->m_DebugScreen.Init();

	mainCamera->SetPosition(CustomType::Vector3(0.f, 0.6f, -3.f));
	mainLight->SetRotation(CustomType::Quaternion(mainLight->GetRightVector(), 30.f * CustomType::CMath::GetDegToRad()));
	terrainPlane->SetMeshInfo(100.f, 50, 50.f);
	cube->SetPosition(CustomType::Vector3(0.f, 0.5f, 0.f));
}
void CScene::Uninit()
{
	this->m_DebugScreen.Uninit();

	for (INT i = 0; i < SCENELAYOUT_COUNT; ++i)
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
	for (INT i = 0; i < SCENELAYOUT_COUNT; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
			object.second->Update();
	}

	this->m_DebugScreen.Update();
}
void CScene::FixedUpdate()
{
	for (INT i = 0; i < SCENELAYOUT_COUNT; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
			object.second->FixedUpdate();
	}
}
void CScene::DrawOpaqueDeferred()
{
	for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_SKY; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
		{
			if (object.second->GetMeshRenderer() != NULL)
			{
				if (object.second->GetMeshRenderer()->GetRenderType() == CMeshRenderer::RENDER_TYPE_OPAQUE_DEFERRED)
					object.second->Draw();
			}
		}
	}
}
void CScene::DrawOpaqueForward()
{
	for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_SKY; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
		{
			if (object.second->GetMeshRenderer() != NULL)
			{
				if (object.second->GetMeshRenderer()->GetRenderType() == CMeshRenderer::RENDER_TYPE_OPAQUE_FORWARD)
					object.second->Draw();
			}
		}
	}
}
void CScene::DrawTransparent()
{
	for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_SKY; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
		{
			if (object.second->GetMeshRenderer() != NULL)
			{
				if (object.second->GetMeshRenderer()->GetRenderType() == CMeshRenderer::RENDER_TYPE_TRANSPARENT)
					object.second->Draw();
			}
		}
	}
}
void CScene::DrawSky()
{
	for (const auto& object : this->m_GameObject[SCENELAYOUT_SKY])
	{
		if (object.second->GetMeshRenderer() != NULL)
		{
			if (object.second->GetMeshRenderer()->GetRenderType() == CMeshRenderer::RENDER_TYPE_SKY)
				object.second->Draw();
		}
	}
}
void CScene::DrawShadow()
{
	CMeshRenderer::CRenderTypeEnum renderType;
	for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_SKY; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
		{
			if (object.second->GetMeshRenderer() != NULL)
			{
				renderType = object.second->GetMeshRenderer()->GetRenderType();
				if (renderType == CMeshRenderer::RENDER_TYPE_OPAQUE_DEFERRED || renderType == CMeshRenderer::RENDER_TYPE_OPAQUE_FORWARD)
					object.second->Draw();
			}
		}
	}
}
void CScene::PrepareDraw()
{
	CCamera* tempCamera = this->GetGameObjectFirst<CCamera>(SCENELAYOUT_CAMERA);
	this->m_ConstantBufferData.ViewMatrix = tempCamera->GetViewMatrix().GetGPUUploadFloat4x4();
	this->m_ConstantBufferData.ViewInvMatrix = tempCamera->GetViewInverseMatrix().GetGPUUploadFloat4x4();
	this->m_ConstantBufferData.ProjectionMatrix = tempCamera->GetProjectionMatrix().GetGPUUploadFloat4x4();
	this->m_ConstantBufferData.ProjectionInvMatrix = tempCamera->GetProjectionInverseMatrix().GetGPUUploadFloat4x4();
	this->m_ConstantBufferData.ViewProjectionMatrix = tempCamera->GetViewProjectionMatrix().GetGPUUploadFloat4x4();
	this->m_ConstantBufferData.ViewProjectionInvMatrix = tempCamera->GetViewProjectionInverseMatrix().GetGPUUploadFloat4x4();
	this->m_ConstantBufferData.TimeParams = XMFLOAT4(static_cast<FLOAT>(CManager::GetManager()->GetGameTimer()->GetClockTime()), static_cast<FLOAT>(CManager::GetManager()->GetGameTimer()->GetDeltaTime()), 1.f, 1.f);
	this->m_ConstantBufferData.ScreenParams = XMFLOAT4(ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT, 1.f / ENGINE_SCREEN_WIDTH, 1.f / ENGINE_SCREEN_HEIGHT);
	this->m_ConstantBufferData.CameraWorldPosition = tempCamera->GetPosition().GetXMFLOAT3();

	std::vector<CLight*> tempLightList = this->GetGameObjectAll<CLight>(SCENELAYOUT_LIGHT);
	this->m_ConstantBufferData.DirectionalLightCount = static_cast<FLOAT>(tempLightList.size());
	for (INT i = 0; i < tempLightList.size() && i < 4; i++)
	{
		this->m_ConstantBufferData.DirectionalLightData[i].Direction = tempLightList[i]->GetLightData()->Direction;
		this->m_ConstantBufferData.DirectionalLightData[i].Color = tempLightList[i]->GetLightData()->Color;
	}

	CRenderDevice::UploadConstantBuffer(this->m_ConstantBuffer, &(this->m_ConstantBufferData));
	CRenderDevice::BindConstantBuffer(this->m_ConstantBuffer, ENGINE_CONSTANT_BUFFER_PER_FRAME_START_SLOT);
}
void CScene::Draw()
{
	CRenderDevice::ResetRenderTarget();
	this->PrepareDraw();


	CRenderDevice::BeginShadow();
	this->DrawShadow();
	CRenderDevice::EndShadow();


	CRenderDevice::BeginDeferred();
	this->DrawOpaqueDeferred();
	CRenderDevice::EndDeferred();


	CRenderDevice::BeginDeferredResolve();
	CRenderDevice::SetShadowMap(ENGINE_SRV_LIGHT_SHADOW_MAP_START_SLOT);
	CRenderDevice::GetDeferredResolve()->Draw();
	CRenderDevice::EndDeferredResolve();


	CRenderDevice::BeginForward();
	this->DrawSky();
	this->DrawOpaqueForward();
	CRenderDevice::SetDepthState(CRenderDevice::DSSE_TESTENABLEWRITEDISABLE);
	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDALPHA);
	this->DrawTransparent();
	CRenderDevice::EndForward();


	CRenderDevice::BeginFinal();
	CRenderDevice::GetPostEffect()->Draw();
	this->m_DebugScreen.Draw();
	CRenderDevice::EndFinal();
}