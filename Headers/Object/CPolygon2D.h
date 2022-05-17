#pragma once

#include "../Game/CGameObject.h"

class CShader;

class CPolygon2D : public CGameObject
{
protected:

	ID3D11Buffer*	m_VertexBuffer	= NULL;
	CShader*		m_Shader		= NULL;

	XMFLOAT4        m_Position		= XMFLOAT4(0.f, 0.f, static_cast<FLOAT>(SCREEN_WIDTH), static_cast<FLOAT>(SCREEN_HEIGHT));

	FLOAT			m_Time;

	FLOAT			m_LightRotation;
	XMFLOAT3		m_LightDirection;
public:
	virtual void	Init();
	virtual void	Uninit();
	virtual void	Update();
	virtual void	Draw();
public:

	void			SetScreenPosition(XMFLOAT4 rect);
	XMFLOAT4		GetScreenPosition();
};

class DebugQuadAlpha : public CPolygon2D
{
public:
	virtual void	Init();
};

class QuadBefore : public CPolygon2D
{
public:
	virtual void	Init();
	virtual void	Draw();
};

class QuadAfter : public CPolygon2D
{
public:
	virtual void	Init();
};