#include "../../Headers/Game/CScene.h"
#include "../../Headers/Base/CTimer.h"
#include "../../Headers/Base/CManager.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Game/CCamera.h"
#include "../../Headers/Game/CDeferredBuffer.h"
#include "../../Headers/Game/CLight.h"
#include "../../Headers/Object/CCube.h"
#include "../../Headers/Object/CScreenPolygon2D.h"

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

	ID3D11SamplerState* sceneSampler[] = {
		(CRenderDevice::GetSamplerState(CRenderDevice::SSE_CLAMP_POINT)),
		(CRenderDevice::GetSamplerState(CRenderDevice::SSE_CLAMP_LINEAR)),
		(CRenderDevice::GetSamplerState(CRenderDevice::SSE_WRAP_POINT)),
		(CRenderDevice::GetSamplerState(CRenderDevice::SSE_WRAP_LINEAR))
	};
	CRenderDevice::GetDeviceContext()->PSSetSamplers(0u, 4u, sceneSampler);


	CCamera* mainCamera = this->AddGameObject<CCamera>(SCENELAYOUT_CAMERA);
	CLight* mainLight = this->AddGameObject<CLight>(SCENELAYOUT_LIGHT);
	//this->AddGameObject<CField>(SCENELAYOUT_TERRAIN);
	this->AddGameObject<CCube>(SCENELAYOUT_OPAQUE);
	//this->AddGameObject<CSky>(SCENELAYOUT_SKY);
	//this->AddGameObject<CWater>(SCENELAYOUT_TRANSPARENT);


	this->m_DebugScreen.SetScene(this);
	this->m_DebugScreen.Init();

	mainCamera->SetPosition(CustomType::Vector3(0.f, 0.f, -3.f));
	mainLight->SetRotation(CustomType::Quaternion(mainLight->GetRightVector(), 30.f * CustomType::CMath::GetDegToRad()));
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
void CScene::DrawOpaque()
{
	for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_SKY; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
		{
			if (object.second->GetTransparency() == CGameObject::GameObjectTransparencyEnum::GAMEOBJECT_OPAQUE)
				object.second->Draw();
		}
	}
}
void CScene::DrawOpaqueDeferred()
{
	for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_SKY; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
		{
			if (object.second->GetTransparency() == CGameObject::GameObjectTransparencyEnum::GAMEOBJECT_OPAQUE)
				object.second->Draw();
		}
	}
}
void CScene::DrawTransparent()
{
	for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_SKY; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
		{
			if (object.second->GetTransparency() == CGameObject::GameObjectTransparencyEnum::GAMEOBJECT_TRANSPARENT)
				object.second->Draw();
		}
	}
}
void CScene::DrawSky()
{
	for (const auto& object : this->m_GameObject[SCENELAYOUT_SKY])
	{
		if (object.second->GetTransparency() == CGameObject::GameObjectTransparencyEnum::GAMEOBJECT_SKY)
			object.second->Draw();
	}
}
void CScene::DrawShadow()
{
	for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_SKY; ++i)
	{
		for (const auto& object : this->m_GameObject[i])
		{
			if (object.second->GetTransparency() == CGameObject::GameObjectTransparencyEnum::GAMEOBJECT_OPAQUE)
				object.second->DrawExtra();
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
	this->m_ConstantBufferData.TimeParams = XMFLOAT4(static_cast<FLOAT>(CManager::GetTimer()->GetClockTime()), static_cast<FLOAT>(CManager::GetTimer()->GetDeltaTime()), 1.f, 1.f);
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
	this->PrepareDraw();


	CRenderDevice::SetDepthState(CRenderDevice::DSSE_TESTENABLEWRITEENABLE);
	CRenderDevice::BeginShadow();
	CRenderDevice::ClrShadowDeferred();


	this->DrawShadow();


	CRenderDevice::BeginDeferred();
	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);


	this->DrawOpaqueDeferred();


	CRenderDevice::SetDepthState(CRenderDevice::DSSE_ALLDISABLE);
	CRenderDevice::BeginGBuffer();
	CRenderDevice::BindTexture(CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_WORLDNORMAL), 0);
	CRenderDevice::BindTexture(CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_ALBEDO), 1);
	CRenderDevice::BindTexture(CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_PROPERTY), 2);
	CRenderDevice::BindTexture(CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_ID), 3);
	CRenderDevice::SetShadowMap(5);
	CRenderDevice::GetDeferredResolve()->Draw();


	CRenderDevice::SetDepthState(CRenderDevice::DSSE_TESTENABLEWRITEDISABLE);
	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDALPHA);


	this->DrawSky();
	this->DrawTransparent();


	CRenderDevice::SetDepthState(CRenderDevice::DSSE_ALLDISABLE);
	CRenderDevice::Begin();
	CRenderDevice::BindTexture(CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_EXTRA), 4);
	CRenderDevice::GetPostEffect()->Draw();


	this->m_DebugScreen.Draw();
	this->DrawShadow();


	CRenderDevice::End();
}