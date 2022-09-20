#include "../Headers/CGPUCulling.h"
#include "../../AssetsManager/Headers/CShader.h"
#include "../../AssetsManager/Headers/CShaderManager.h"
#include "../../RenderFeatures/Headers/CHZBPass.h"

CGPUCulling::CGPUCulling()
{
}
CGPUCulling::~CGPUCulling()
{
}
void CGPUCulling::Init(CHZBPass* hzb)
{
	CRenderDevice::LoadComputeShader("./Engine/Assets/EngineShaders/EngineGPUCulling.cso", this->m_GPUCullingComputeShader);

	{
		const static UINT objectNum = 10u;

		{
			D3D11_BUFFER_DESC bd;
			::ZeroMemory(&bd, sizeof(bd));
			bd.ByteWidth = sizeof(PrimitivesAABBBoxInfo) * objectNum;
			bd.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
			bd.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
			bd.CPUAccessFlags = 0u;
			bd.MiscFlags = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			bd.StructureByteStride = sizeof(PrimitivesAABBBoxInfo);
			HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateBuffer(&bd, NULL, m_CullingTestBuffer.Buffer.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				//TODO
			}
			D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
			::ZeroMemory(&srvd, sizeof(srvd));
			srvd.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_BUFFER;
			//srvd.Buffer = 
			//	union
			//{
			//	UINT FirstElement;
			//	UINT ElementOffset;
			//};
			//union
			//{
			//	UINT NumElements;
			//	UINT ElementWidth;
			//};;
			//hr = CRenderDevice::m_RenderDevice->m_Device->CreateShaderResourceView(m_CullingTestBuffer.Buffer.Get(), , m_CullingTestBuffer.ShaderResourceView.ReleaseAndGetAddressOf());
		}

		{
			D3D11_BUFFER_DESC bd;
			::ZeroMemory(&bd, sizeof(bd));
			bd.ByteWidth = sizeof(UINT) * objectNum;
			bd.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
			bd.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS;
			bd.CPUAccessFlags = 0u;
			bd.MiscFlags = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			bd.StructureByteStride = sizeof(UINT);
			HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateBuffer(&bd, NULL, m_CullingResultBuffer.Buffer.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				//TODO
			}
		}
	}
}
void CGPUCulling::Uninit()
{

}
void CGPUCulling::Update()
{

}
void CGPUCulling::ComputeCulling()
{

}
void CGPUCulling::ReadBackFromResource()
{
	//CRenderDevice::m_RenderDevice->m_ImmediateContext->Map();
	//HRESULT Map(
	//	[in]            ID3D11Resource * pResource,
	//	[in]            UINT                     Subresource,
	//	[in]            D3D11_MAP                MapType,
	//	[in]            UINT                     MapFlags,
	//	[out, optional] D3D11_MAPPED_SUBRESOURCE * pMappedResource
	//);
}