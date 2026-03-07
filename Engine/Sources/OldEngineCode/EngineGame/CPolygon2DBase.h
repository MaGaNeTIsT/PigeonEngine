/*
#pragma once

#include "../../../../Entry/EngineMain.h"
#include "./CGameObject.h"

class CPolygon2DBase : public CGameObject
{
public:
	void			SetScreenPosition(const CustomType::Vector4& rect);
	const CustomType::Vector4& GetScreenPosition()const { return m_2DPosition; }
public:
	CPolygon2DBase(const BOOL32& active, const class CScene* scene);
	virtual ~CPolygon2DBase();
protected:
	CustomType::Vector4		m_2DPosition;
};
*/