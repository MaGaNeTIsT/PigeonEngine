#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../RenderBase/Headers/CRenderDevice.h"

class CHZBBuffer
{
public:

public:
	void	Init(const CustomType::Vector2Int& bufferSize, const CustomType::Vector2Int& pipelineSize);
	void	Uninit();
	void	Update();
	void	Draw(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& sceneDepth);
protected:
	void	PrepareDraw(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& sceneDepth);
public:
	CHZBBuffer();
	~CHZBBuffer();
protected:
	class CScreenPolygon2D*						m_Polygon2D;

	CustomType::Vector2Int						m_BufferSize;
	CustomType::Vector2Int						m_PipelineSize;
	INT											m_HZBMipLevels;
	CRenderDevice::RenderTexture2DViewInfo		m_HZBBuffer;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_BuildHZBComputeShader;
};