#pragma once

#include "../../Entry/MyMain.h"
#include "../../Headers/Base/CRenderDevice.h"

class CGameObject
{
public:

	enum GameObjectTransparencyEnum
	{
		GAMEOBJECT_OPAQUE		= 0,
		GAMEOBJECT_TRANSPARENT	= 1,
		GAMEOBJECT_SKY			= 2
	};

protected:

	void SetTransparent(GameObjectTransparencyEnum transparent = GAMEOBJECT_TRANSPARENT) { m_Transparency = transparent; }

protected:

	GameObjectTransparencyEnum	m_Transparency = GAMEOBJECT_OPAQUE;

	XMFLOAT3					m_Position;
	XMFLOAT3					m_Rotation;
	XMFLOAT3					m_Scale;

public:

	GameObjectTransparencyEnum GetTransparency()const { return m_Transparency; }

	XMFLOAT3 GetPosition()const { return m_Position; }
	XMFLOAT3 GetRotation()const { return m_Rotation; }
	XMFLOAT3 GetScale()const { return m_Scale; }

	void SetPosition(const XMFLOAT3& Position) { m_Position = Position; }
	void SetRotation(const XMFLOAT3& Rotation ){ m_Rotation = Rotation; }
	void SetScale(const XMFLOAT3& Scale ){ m_Scale = Scale; }

public:

	CGameObject(){}

	virtual ~CGameObject()
	{
	}


	virtual void Uninit()
	{
	}


	virtual void Update()
	{
	}


	virtual void Draw()
	{
	}


	virtual void DrawDeferred()
	{
	}


	virtual void DrawShadow()
	{
	}
};