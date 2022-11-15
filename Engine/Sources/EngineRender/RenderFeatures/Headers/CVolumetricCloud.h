#pragma once

#include "../../RenderBase/Headers/CRenderStructCommon.h"
#include "../../../EngineGame/Headers/CGameObject.h"

class CVolumetricClouds : public CGameObject
{
public:

public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
	virtual void	Draw()override;
	virtual void	DrawExtra()override;
protected:
	virtual void	PrepareDraw()override;
public:

public:
	CVolumetricClouds();
	virtual ~CVolumetricClouds();
protected:

protected:

};