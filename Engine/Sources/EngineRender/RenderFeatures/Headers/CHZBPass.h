#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../RenderBase/Headers/CRenderDevice.h"

class CHZBPass
{
public:
	UINT	GetHZBBufferNum() { return static_cast<UINT>(this->m_HZBBuffers.size()); }
	void	GetHZBBufferByMipIndex(CRenderDevice::RenderTexture2DViewInfo& buffer, const UINT& idx);
	void	GetHZBBuffers(std::vector<CRenderDevice::RenderTexture2DViewInfo>& buffers);
	void	GetHZBSizeByMipIndex(CustomType::Vector2Int& size, const UINT& idx);
	void	GetHZBSizes(std::vector<CustomType::Vector2Int>& sizes);
public:
	void	Init(const CustomType::Vector2Int& pipelineSize);
	void	Uninit();
	void	Update();
	void	ComputeHZB(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& sceneDepth);
	void	DrawDebug();
private:
	void	CalculateHZBLevels();
	void	InitHZBBuffers();
public:
	CHZBPass();
	~CHZBPass();
private:
	class CScreenPolygon2D*		m_Polygon2D;
	BOOL						m_DebugType;
	INT							m_DebugLevel;

	Microsoft::WRL::ComPtr<ID3D11ComputeShader>		m_BuildHZBComputeShader;
	CustomType::Vector2Int							m_PipelineSize;

	std::vector<CRenderDevice::RenderTexture2DViewInfo>		m_HZBBuffers;
	std::vector<CustomType::Vector2Int>						m_HZBSizes;
	INT														m_HZBMipLevels;
};