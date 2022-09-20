#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "./CStructCommon.h"
#include "../Headers/CRenderDevice.h"

class CGPUCulling
{
public:
	struct PrimitivesAABBBoxInfo
	{
		FLOAT Anchor[3];
		FLOAT Dimensions[3];
		FLOAT Padding[2];
	};
public:
	void	Init(class CHZBPass* hzb);
	void	Uninit();
	void	Update();
	void	ComputeCulling();
private:
	void	ReadBackFromResource();
public:
	CGPUCulling();
	~CGPUCulling();
private:
	Microsoft::WRL::ComPtr<ID3D11ComputeShader>		m_GPUCullingComputeShader;

	CRenderDevice::RenderStructuredBufferViewInfo	m_CullingTestBuffer;
	CRenderDevice::RenderStructuredBufferViewInfo	m_CullingResultBuffer;
};