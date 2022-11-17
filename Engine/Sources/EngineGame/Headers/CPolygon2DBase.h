#pragma once

#include "./CGameObject.h"

class CPolygon2DBase : public CGameObject
{
public:
	virtual void	Init()override			{}
	virtual void	Uninit()override		{}
	virtual void	Update()override		{}
	virtual void	FixedUpdate()override	{}
public:
	void			SetScreenPosition(const CustomType::Vector4& rect);
	const CustomType::Vector4& GetScreenPosition()const { return m_2DPosition; }
public:
	CPolygon2DBase();
	virtual ~CPolygon2DBase();
protected:
	CustomType::Vector4		m_2DPosition;
};