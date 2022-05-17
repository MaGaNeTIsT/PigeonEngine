#pragma once

#include "../../Entry/MyMain.h"
#include "../../Headers/Game/CGameObject.h"
#include "../../Headers/Base/CReadOBJ.h"
#include "../../Headers/Game/CCamera.h"
#include "../../Headers/Game/CDeferredBuffer.h"
#include "../../Headers/Object/CField.h"
#include "../../Headers/Object/CPolygon.h"
#include "../../Headers/Object/CSky.h"
#include "../../Headers/Object/CWaterField.h"
#include "../../Headers/Object/CPolygon2D.h"
#include "../../Headers/Object/CRayMarching.h"
#include "../../Headers/Object/CDebugScreen.h"

DebugScreen debugWnd;

enum SceneLayoutEnum
{
	SCENELAYOUT_CAMERA,
	SCENELAYOUT_TERRAIN,
	SCENELAYOUT_OPAQUE,
	SCENELAYOUT_SKY,
	SCENELAYOUT_TRANSPARENT,
	SCENELAYOUT_COUNT
};

class CScene
{
protected:
	std::list<CGameObject*>	m_GameObject[SCENELAYOUT_COUNT];
	ULONGLONG				m_GameObjectID;
public:
	CScene() { m_GameObjectID = 0; }
	virtual ~CScene(){}

	virtual void Init()
	{
		debugWnd.Init();

		AddGameObject<CCamera>(SCENELAYOUT_CAMERA);
		AddGameObject<CField>(SCENELAYOUT_TERRAIN);
		//AddGameObject<CSky>(SCENELAYOUT_SKY);
		AddGameObject<CWater>(SCENELAYOUT_TRANSPARENT);
	}

	virtual void Uninit()
	{
		for (INT i = 0; i < SCENELAYOUT_COUNT; ++i)
		{
			for (CGameObject* object : m_GameObject[i])
			{
				object->Uninit();
				delete object;
			}
			m_GameObject[i].clear();
		}

		debugWnd.Uninit();
	}

	virtual void Update()
	{
		for (INT i = 0; i < SCENELAYOUT_COUNT; ++i)
		{
			for (CGameObject* object : m_GameObject[i])
				object->Update();
		}

		debugWnd.Update();
	}

	virtual void DrawOpaque()
	{
		for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_COUNT; ++i)
		{
			for (CGameObject* object : m_GameObject[i])
			{
				if (object->GetTransparency() == CGameObject::GameObjectTransparencyEnum::GAMEOBJECT_OPAQUE)
					object->Draw();
			}
		}
	}

	virtual void DrawOpaqueDeferred()
	{
		for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_COUNT; ++i)
		{
			for (CGameObject* object : m_GameObject[i])
			{
				if (object->GetTransparency() == CGameObject::GameObjectTransparencyEnum::GAMEOBJECT_OPAQUE)
					object->DrawDeferred();
			}
		}
	}

	virtual void DrawTransparent()
	{
		for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_COUNT; ++i)
		{
			for (CGameObject* object : m_GameObject[i])
			{
				if (object->GetTransparency() == CGameObject::GameObjectTransparencyEnum::GAMEOBJECT_TRANSPARENT)
					object->Draw();
			}
		}
	}

	virtual void DrawSky()
	{
		for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_COUNT; ++i)
		{
			for (CGameObject* object : m_GameObject[i])
			{
				if (object->GetTransparency() == CGameObject::GameObjectTransparencyEnum::GAMEOBJECT_SKY)
					object->Draw();
			}
		}
	}

	virtual void DrawShadow()
	{
		for (INT i = SCENELAYOUT_TERRAIN; i < SCENELAYOUT_COUNT; ++i)
		{
			for (CGameObject* object : m_GameObject[i])
			{
				if (object->GetTransparency() == CGameObject::GameObjectTransparencyEnum::GAMEOBJECT_OPAQUE)
					object->DrawShadow();
			}
		}
	}

	virtual void Draw()
	{
		CRenderDevice::SetDepthState(CRenderDevice::DSSE_TESTENABLEWRITEENABLE);
		CRenderDevice::BeginShadow();
		CRenderDevice::ClrShadowDeferred();
		DrawShadow();



		CRenderDevice::BeginDeferred();
		CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
		DrawOpaqueDeferred();



		CRenderDevice::SetDepthState(CRenderDevice::DSSE_ALLDISABLE);
		CRenderDevice::BeginGBuffer();
		CRenderDevice::SetTexture(CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(DeferredBuffer::DEFERREDBUFFER_WORLDPOSITION), 0);
		CRenderDevice::SetTexture(CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(DeferredBuffer::DEFERREDBUFFER_WORLDNORMAL), 1);
		CRenderDevice::SetTexture(CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(DeferredBuffer::DEFERREDBUFFER_DIFFUSE), 2);
		CRenderDevice::SetTexture(CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(DeferredBuffer::DEFERREDBUFFER_SPECULAR), 3);
		CRenderDevice::SetShadowMap(4);
		CRenderDevice::GetDeferredQuadBefore()->Draw();



		CRenderDevice::SetDepthState(CRenderDevice::DSSE_TESTENABLEWRITEDISABLE);
		CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDALPHA);
		DrawSky();
		DrawTransparent();



		CRenderDevice::SetDepthState(CRenderDevice::DSSE_ALLDISABLE);
		CRenderDevice::Begin();
		CRenderDevice::SetTexture(CRenderDevice::GetDeferredBuffer()->GetDeferredShaderResourceView(DeferredBuffer::DEFERREDBUFFER_GBUFFER), 5);
		CRenderDevice::GetDeferredQuadAfter()->Draw();



		debugWnd.Draw();



		CRenderDevice::End();
	}

	template <typename T>
	T* AddGameObject(UINT layout)
	{
		T* gameObject = new T(m_GameObjectID);
		gameObject->Init();
		m_GameObject[layout].push_back(gameObject);

		m_GameObjectID += 1;

		return gameObject;
	}
};