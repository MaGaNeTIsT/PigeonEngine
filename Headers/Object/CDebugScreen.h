#pragma once

#include "../Game/CGameObject.h"

class CScreenPolygon2D;

class CDebugScreen : public CGameObject
{
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
	virtual void	Draw()override;
protected:
	virtual void	PrepareDraw()override;
public:
	CDebugScreen();
	virtual ~CDebugScreen();
protected:
	static UINT		DEBUGPOLYGON_COUNT;
protected:
	std::vector<CScreenPolygon2D*>			m_Polygons;
	std::vector<ID3D11ShaderResourceView*>	m_SRVs;
};