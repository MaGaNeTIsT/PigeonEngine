#pragma once

#include "./CPolygon2DBase.h"

class CPolygon2D : public CPolygon2DBase
{
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
public:
	CPolygon2D();
	virtual ~CPolygon2D();
};