#pragma once

#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Game/CGameObject.h"

class CLight : public CGameObject
{
public:
	XMVECTOR GetDirection();
public:
	COLOR GetColor()const { return m_Color; };
private:
	BOOL		m_DirectionCalculated;
	XMFLOAT4	m_Direction;
	COLOR		m_Color;
};