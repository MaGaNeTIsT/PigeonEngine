#pragma once

#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "./CGameObject.h"

class CTexture2D;

class CCube : public CGameObject
{
public:
	virtual void	Update()override;
protected:
	CTexture2D* m_AlbedoTexture;
	CTexture2D* m_NormalTexture;
	CTexture2D* m_PropertyTexture;
public:
	CCube();
	virtual ~CCube();
};