#pragma once

#include "./CGameObject.h"

class CPolygon2D;

class DebugScreen : public CGameObject
{
public:
	virtual void	Init();
	virtual void	Uninit();
	virtual void	Update();
	virtual void	Draw();

	void			SetShaderResourceView(INT num, ID3D11ShaderResourceView** srv);
private:
	static const UINT				DEBUGPOLYGON_COUNT = 7;

	CPolygon2D*						m_Polygon[DEBUGPOLYGON_COUNT];
	ID3D11ShaderResourceView*		m_SRV[DEBUGPOLYGON_COUNT];
};