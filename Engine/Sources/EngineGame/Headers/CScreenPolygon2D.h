#pragma once

#include "../../../../Entry/EngineMain.h"
#include "./CPolygon2DBase.h"

class CScreenPolygon2D : public CPolygon2DBase
{
public:
	virtual void	Update()override;
public:
	CScreenPolygon2D(const std::string& pixelShaderName, CustomType::Vector4 screenPosition = CustomType::Vector4(0, 0, ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT));
	virtual ~CScreenPolygon2D();
};