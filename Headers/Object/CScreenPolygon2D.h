#pragma once

#include "../../Entry/MyMain.h"
#include "./CPolygon2DBase.h"

class CScreenPolygon2D : public CPolygon2DBase
{
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
	virtual void	Draw()override;
protected:
	virtual void	PrepareDraw()override;
public:
	CScreenPolygon2D(const std::string& vertexShaderName, const std::string& pixelShaderName, CustomType::Vector4 screenPosition = CustomType::Vector4(0, 0, ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT));
	virtual ~CScreenPolygon2D();
};