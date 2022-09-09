#pragma once

#include "../../../../Entry/EngineMain.h"
#include "./CGameObject.h"
#include "../../EngineRender/RenderFeatures/Headers/CGTAOComputeShader.h"
#include "../../EngineRender/RenderFeatures/Headers/CHZBBuffer.h"
#include "../../Development/Headers/CDebugScreen.h"

class CScene
{
public:
	enum SceneLayoutEnum
	{
		SCENELAYOUT_CAMERA,
		SCENELAYOUT_LIGHT,
		SCENELAYOUT_TERRAIN,
		SCENELAYOUT_OPAQUE,
		SCENELAYOUT_TRANSPARENT,
		SCENELAYOUT_SKY,
		SCENELAYOUT_COUNT
	};
protected:
	CDebugScreen								m_DebugScreen;
	CGTAOComputeShader							m_GTAOComputeShader;
	CHZBBuffer									m_HZB;
	CustomStruct::ConstantBufferPerFrame		m_ConstantBufferData;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_ConstantBuffer;
protected:
	std::map<ULONGLONG, CGameObject*>			m_GameObject[SCENELAYOUT_COUNT];
public:
	CScene();
	virtual ~CScene();
public:
	template <typename T>
	T*				AddGameObject(const UINT& layout);
	template <typename T>
	T*				GetGameObjectFirst(const UINT& layout);
	template <typename T>
	T*				GetGameObjectByIndex(const UINT& layout, const UINT& idx);
	template <typename T>
	std::vector<T*>	GetGameObjectAll(const UINT& layout);
public:
	virtual void	Init();
	virtual void	Uninit();
	virtual void	Update();
	virtual void	FixedUpdate();
	virtual void	Draw();
protected:
	virtual void	DrawOpaqueDeferred();
	virtual void	DrawOpaqueForward();
	virtual void	DrawTransparent();
	virtual void	DrawSky();
	virtual void	DrawShadow();
	virtual void	PrepareDraw();
};