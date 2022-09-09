#pragma once

#include "../../../EngineGame/Headers/CGameObject.h"

class CScreenPolygon2D;

class CHZBBuffer : public CGameObject
{
public:

public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
	virtual void	Draw()override;
protected:
	virtual void	PrepareDraw()override;
public:
	CHZBBuffer();
	virtual ~CHZBBuffer();
protected:
	CScreenPolygon2D*							m_Polygon2D;

	INT											m_HZBMipLevels;
	CRenderDevice::RenderTexture2DViewInfo		m_HZBBuffer;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_BuildHZBComputeShader;
};