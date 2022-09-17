#pragma once

#include "../../EngineRender/RenderBase/Headers/CStructCommon.h"
#include "./CGameObject.h"

class CTexture2D;

class CCube : public CGameObject
{
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
	virtual void	FixedUpdate()override;
	virtual void	Draw()override;
	virtual void	DrawExtra()override;
protected:
	virtual void	PrepareDraw()override;
public:
	CCube();
	virtual ~CCube();
protected:
	CustomStruct::ConstantBufferPerDraw m_ConstantBuffer;
	CTexture2D*	m_AlbedoTexture;
	CTexture2D*	m_NormalTexture;
	CTexture2D*	m_PropertyTexture;
};