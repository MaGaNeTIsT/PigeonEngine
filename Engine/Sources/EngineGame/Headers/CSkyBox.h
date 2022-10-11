#pragma once

#include "../../EngineRender/AssetsManager/Headers/CMesh.h"

class CSkyBox
{
public:
	struct SkyBoxInfo
	{
		SkyBoxInfo() { ::ZeroMemory(this, sizeof(*this)); }
		SkyBoxInfo(const FLOAT& radius)
		{
			Radius = radius;
		}
		FLOAT Radius;
	};
public:
	virtual void	Init(const CustomType::Vector2Int& bufferSize, const SkyBoxInfo& skyBoxInfo);
	virtual void	Uninit();
	virtual void	Update();
	virtual void	Draw();
protected:
	virtual void	PrepareDraw();
public:
	CSkyBox();
	CSkyBox(const CSkyBox& skyBox);
	virtual ~CSkyBox() {}
protected:
	SkyBoxInfo									m_SkyBoxInfo;
	std::shared_ptr<class CPixelShader>			m_PixelShader;
protected:
	static std::shared_ptr<CMesh<UINT>>			m_FullScreenMesh;
	static std::shared_ptr<class CVertexShader>	m_VertexShader;
};