/*
#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "./CMeshComponent.h"

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
	struct SkyBoxConstantBuffer
	{
		CustomType::Vector4 Parameter;
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
	virtual ~CSkyBox();
protected:
	SkyBoxInfo									m_SkyBoxInfo;
	class CVertexShader*						m_VertexShader;
	class CPixelShader*							m_PixelShader;
	class CTextureCube*							m_CubeMap;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_ConstantBuffer;
	SkyBoxConstantBuffer						m_ConstantData;
protected:
	static const CBaseMesh<UINT32>*				m_FullScreenMesh;
};
*/