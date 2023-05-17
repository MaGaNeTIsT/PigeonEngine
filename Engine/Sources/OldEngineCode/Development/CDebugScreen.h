#pragma once

#include "../../../../Entry/EngineMain.h"

#ifdef _DEVELOPMENT_EDITOR

#include "../../EngineBase/Headers/CBaseType.h"

class CDebugScreen
{
public:
	void	SetShadowMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadow);
public:
	void	Init(const CustomType::Vector2Int& pipelineSize);
	void	Uninit();
	void	Update();
	void	Draw();
protected:
	void	PrepareDraw();
public:
	CDebugScreen();
	~CDebugScreen();
protected:
	static UINT DEBUGPOLYGON_COUNT;
protected:
	std::vector<class CScreenPolygon2D*> m_Polygons;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_SRVs;
};

#endif