#pragma once

#include "../../EngineRender/RenderBase/Headers/CRenderStructCommon.h"
#include "./CGameObject.h"

class CTexture2D;

class CCube : public CGameObject
{
public:
	virtual void	Update()override;
public:
	CCube();
	virtual ~CCube();
};