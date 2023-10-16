#include "DeviceD3D11.h"

namespace PigeonEngine
{
	void RDeviceD3D11::SetInitializeData(HWND hWnd, const Vector2Int& bufferSize, UINT32 bufferDepth, UINT32 frameNum, BOOL32 windowed)
	{
		if (bufferSize.x < 1 || bufferSize.y < 1 || !(bufferDepth == 24u || bufferDepth == 32u) || frameNum < 2u)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Render device init failed. Invalid arguments"));
			return;
		}
		UINT32 bufferWidth = static_cast<UINT32>(bufferSize.x);
		UINT32 bufferHeight = static_cast<UINT32>(bufferSize.y);
		HRESULT hr = S_FALSE;
		DXGI_SWAP_CHAIN_DESC sd;
		{
			::ZeroMemory(&sd, sizeof(sd));
			sd.BufferCount = 1u;
			sd.BufferDesc.Width = bufferWidth;
			sd.BufferDesc.Height = bufferHeight;
			sd.BufferDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
			sd.BufferDesc.RefreshRate.Numerator = frameNum;
			sd.BufferDesc.RefreshRate.Denominator = 1u;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.OutputWindow = hWnd;
			sd.SampleDesc.Count = 1u;
			sd.SampleDesc.Quality = 0u;
			sd.Windowed = windowed;
		}
		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1, };
		hr = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0u,
			featureLevels,
			7u,
			D3D11_SDK_VERSION,
			&sd,
			m_SwapChain.ReleaseAndGetAddressOf(),
			m_Device.ReleaseAndGetAddressOf(),
			&(m_FeatureLevel),
			m_ImmediateContext.ReleaseAndGetAddressOf());	//D3D11_CREATE_DEVICE_FLAG
		if (FAILED(hr) || !(m_FeatureLevel == D3D_FEATURE_LEVEL_11_1 || m_FeatureLevel == D3D_FEATURE_LEVEL_11_0))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create D3D11 device failed."));
			return;
		}
		m_Viewport.Width = static_cast<FLOAT>(bufferWidth);
		m_Viewport.Height = static_cast<FLOAT>(bufferHeight);
		m_Viewport.MinDepth = 0.f;
		m_Viewport.MaxDepth = 1.f;
		m_Viewport.TopLeftX = 0.f;
		m_Viewport.TopLeftY = 0.f;
		ID3D11Texture2D* pBackBuffer = nullptr;
		hr = m_SwapChain->GetBuffer(0u, __uuidof(ID3D11Texture2D), (LPVOID*)(&pBackBuffer));
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Failed to gather swap chain back buffer."));
			return;
		}
		hr = m_Device->CreateRenderTargetView(pBackBuffer, nullptr, m_RenderTargetView.ReleaseAndGetAddressOf());
		pBackBuffer->Release();
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create swap chain back buffer RTV failed."));
			return;
		}
		{
			D3D11_TEXTURE2D_DESC td;
			{
				::ZeroMemory(&td, sizeof(td));
				td.Width = sd.BufferDesc.Width;
				td.Height = sd.BufferDesc.Height;
				td.MipLevels = 1u;
				td.ArraySize = 1u;
				if (bufferDepth == 24u) { td.Format = DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS; }
				else if (bufferDepth == 32u) { td.Format = DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS; }
				td.SampleDesc = sd.SampleDesc;
				td.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
				td.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL;
				td.CPUAccessFlags = 0u;
				td.MiscFlags = 0u;
			}
			hr = m_Device->CreateTexture2D(&td, nullptr, m_DepthTexture.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create main depth buffer failed."));
				return;
			}
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
			{
				::ZeroMemory(&dsvd, sizeof(dsvd));
				if (bufferDepth == 24u) { dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT; }
				else if (bufferDepth == 32u) { dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT; }
				dsvd.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvd.Flags = 0u;	//D3D11_DSV_FLAG
			}
			hr = m_Device->CreateDepthStencilView(m_DepthTexture.Get(), &dsvd, m_DepthStencilView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create main depth buffer DSV failed."));
				return;
			}
		}
	}
	void RDeviceD3D11::Initialize()
	{
	}
	void RDeviceD3D11::ShutDown()
	{
		//if (m_RenderTargetView)
		//{
		//	m_RenderTargetView->Release();
		//	m_RenderTargetView = nullptr;
		//}
		//if (m_DepthStencilView)
		//{
		//	m_DepthStencilView->Release();
		//	m_DepthStencilView = nullptr;
		//}
		//if (m_DepthTexture)
		//{
		//	m_DepthTexture->Release();
		//	m_DepthTexture = nullptr;
		//}
		//if (m_SwapChain)
		//{
		//	m_SwapChain->Release();
		//	m_SwapChain = nullptr;
		//}
		//if (m_ImmediateContext)
		//{
		//	m_ImmediateContext->Release();
		//	m_ImmediateContext = nullptr;
		//}
		//if (m_Device)
		//{
		//	m_Device->Release();
		//	m_Device = nullptr;
		//}
	}
	BOOL32 RDeviceD3D11::CreateVertexShaderResource(const void* inCSO, const ULONG& inSize, RVertexShaderResource& outShaderResource, const RInputLayoutDesc* inLayouts, const UINT32& inLayoutNum)
	{
		{
			HRESULT hr = m_Device->CreateVertexShader(inCSO, inSize, nullptr, outShaderResource.Shader.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create vertex shader resource failed."));
				return FALSE;
			}
		}
		{
			if (inLayouts == nullptr)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create vertex shader which needs fitted input layouts."));
				return FALSE;
			}
			TArray<D3D11_INPUT_ELEMENT_DESC> tempLayouts;
			tempLayouts.Resize(inLayoutNum);
			for (UINT32 i = 0u; i < inLayoutNum; i++)
			{
				RDeviceD3D11::TranslateInputLayoutDesc(tempLayouts[i], inLayouts[i]);
			}
			HRESULT hr = m_Device->CreateInputLayout(tempLayouts.RawData(), inLayoutNum, inCSO, inSize, outShaderResource.InputLayout.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create vertex shader input layout failed."));
				return FALSE;
			}
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreatePixelShaderResource(const void* inCSO, const ULONG& inSize, RPixelShaderResource& outShaderResource)
	{
		{
			HRESULT hr = m_Device->CreatePixelShader(inCSO, inSize, nullptr, outShaderResource.Shader.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{

				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create pixel shader resource failed."));
				return FALSE;
			}
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateComputeShaderResource(const void* inCSO, const ULONG& inSize, RComputeShaderResource& outShaderResource)
	{
		{
			HRESULT hr = m_Device->CreateComputeShader(inCSO, inSize, nullptr, outShaderResource.Shader.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create compute shader resource failed."));
				return FALSE;
			}
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateBuffer(RBufferResource& buffer, const RBufferDesc& bufferDesc, const RSubresourceDataDesc* subData)
	{
		return CreateBuffer(buffer.Buffer, bufferDesc, subData);
	}
	BOOL32 RDeviceD3D11::CreateStructuredBuffer(RStructuredBuffer& output, const RStructuredBufferDesc& structuredBufferDesc, const RSubresourceDataDesc* subData)
	{
		if (structuredBufferDesc.NumElements < 1u || structuredBufferDesc.FirstElement >= structuredBufferDesc.NumElements || structuredBufferDesc.StructureSize < 4u)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create structured buffer resource check failed (error description)."));
			return FALSE;
		}
		output.ReleaseRenderResource();
		{
			output.AccessMapRead = (structuredBufferDesc.AccessFlags & RCPUAccessFlagType::CPU_ACCESS_READ) != 0;
			output.AccessMapWrite = (structuredBufferDesc.AccessFlags & RCPUAccessFlagType::CPU_ACCESS_WRITE) != 0;
			if (structuredBufferDesc.GPUWritable && output.AccessMapWrite)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create structured buffer resource check failed (error access ability)."));
				return FALSE;
			}
			D3D11_BUFFER_DESC bd;
			::ZeroMemory(&bd, sizeof(bd));
			bd.ByteWidth = structuredBufferDesc.StructureSize * structuredBufferDesc.NumElements;
			bd.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
			if (!(structuredBufferDesc.GPUWritable) && !(output.AccessMapWrite))
			{
				if (structuredBufferDesc.AccessFlags == RCPUAccessFlagType::CPU_ACCESS_DEFAULT)
				{
					bd.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
				}
				else
				{
					bd.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
				}
			}
			else if (!(structuredBufferDesc.GPUWritable) && output.AccessMapWrite)
			{
				bd.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
			}
			else if (structuredBufferDesc.GPUWritable && !(output.AccessMapWrite))
			{
				bd.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
				bd.BindFlags = bd.BindFlags | D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS;
			}
			TranslateCPUAccessFlag(bd.CPUAccessFlags, structuredBufferDesc.AccessFlags);
			bd.MiscFlags = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			bd.StructureByteStride = structuredBufferDesc.StructureSize;
			HRESULT hr = S_FALSE;
			if (subData)
			{
				D3D11_SUBRESOURCE_DATA sd;
				::ZeroMemory(&sd, sizeof(sd));
				sd.pSysMem = subData->pSysMem;
				sd.SysMemPitch = subData->SysMemPitch;
				sd.SysMemSlicePitch = subData->SysMemSlicePitch;
				hr = m_Device->CreateBuffer(&bd, &sd, output.Buffer.ReleaseAndGetAddressOf());
			}
			else
			{
				hr = m_Device->CreateBuffer(&bd, nullptr, output.Buffer.ReleaseAndGetAddressOf());
			}
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create structured buffer resource failed."));
				return FALSE;
			}
		}
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
			::ZeroMemory(&srvd, sizeof(srvd));
			srvd.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_BUFFER;
			srvd.Buffer.FirstElement = structuredBufferDesc.FirstElement;
			srvd.Buffer.NumElements = structuredBufferDesc.NumElements;
			HRESULT hr = m_Device->CreateShaderResourceView(output.Buffer.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create structured buffer resource SRV failed."));
				return FALSE;
			}
		}
		{
			if (structuredBufferDesc.GPUWritable)
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
				::ZeroMemory(&uavd, sizeof(uavd));
				uavd.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
				uavd.ViewDimension = D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_BUFFER;
				uavd.Buffer.FirstElement = structuredBufferDesc.FirstElement;
				uavd.Buffer.NumElements = structuredBufferDesc.NumElements;
				RDeviceD3D11::TranslateUAVFlag(uavd.Buffer.Flags, structuredBufferDesc.UAVFlags);
				HRESULT hr = m_Device->CreateUnorderedAccessView(output.Buffer.Get(), &uavd, output.UnorderedAccessView.ReleaseAndGetAddressOf());
				if (FAILED(hr))
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create structured buffer resource UAV failed."));
					return FALSE;
				}
			}
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateRenderTexture2D(RRenderTexture2D& output, const RTextureDesc& textureDesc)
	{
		BOOL32 isDepthFormat = textureDesc.Depth == 16u || textureDesc.Depth == 24u || textureDesc.Depth == 32u;
		if (!isDepthFormat && textureDesc.Depth != 0u)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D depth check failed (wrong depth format used)."));
			return FALSE;
		}
		output.ReleaseRenderResource();
		D3D11_TEXTURE2D_DESC td;
		{
			UINT32 tBindFlags = 0u;
			DXGI_FORMAT tFormat = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			if (isDepthFormat)
			{
				if (textureDesc.Depth == 16u)
				{
					PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Depth texture with 16 bits can not hold stencil buffer."), (!(textureDesc.HasStencil)));
					tFormat = DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS;
				}
				else if (textureDesc.Depth == 24u)
				{
					tFormat = DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS;
				}
				else if (textureDesc.Depth == 32u)
				{
					if (textureDesc.HasStencil)
					{
						tFormat = DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS;
					}
					else
					{
						tFormat = DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS;
					}
				}
				tBindFlags = D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
			}
			else
			{
				RDeviceD3D11::TranslateResourceFormat(tFormat, textureDesc.BufferFormat);
				RDeviceD3D11::TranslateBindFlag(tBindFlags, textureDesc.BindFlags);
			}
			::ZeroMemory(&td, sizeof(td));
			td.Width = textureDesc.Width;
			td.Height = textureDesc.Height;
			td.MipLevels = textureDesc.MipLevels;
			td.ArraySize = textureDesc.ArraySize;
			td.Format = tFormat;
			td.SampleDesc.Count = textureDesc.SampleDesc.Count;
			td.SampleDesc.Quality = textureDesc.SampleDesc.Quality;
			td.BindFlags = tBindFlags;
			RDeviceD3D11::TranslateUsageFlag(td.Usage, textureDesc.UsageFlag);
			RDeviceD3D11::TranslateCPUAccessFlag(td.CPUAccessFlags, textureDesc.CPUAccessFlags);
			RDeviceD3D11::TranslateResourceMiscFlag(td.MiscFlags, textureDesc.MiscFlags);
		}
		HRESULT hr = m_Device->CreateTexture2D(&td, nullptr, output.Buffer.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D resource failed."));
			return FALSE;
		}
		{
			BOOL32 needSRV = (textureDesc.BindFlags & RBindFlagType::BIND_SHADER_RESOURCE) != 0;
			if (needSRV || isDepthFormat)
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
				{
					::ZeroMemory(&srvd, sizeof(srvd));
					if (isDepthFormat)
					{
						if (textureDesc.Depth == 16u)
						{
							srvd.Format = DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT;
						}
						else if (textureDesc.Depth == 24u)
						{
							srvd.Format = DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
						}
						else if (textureDesc.Depth == 32u)
						{
							if (textureDesc.HasStencil)
							{
								srvd.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
							}
							else
							{
								srvd.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
							}
						}
					}
					else
					{
						RDeviceD3D11::TranslateResourceFormat(srvd.Format, textureDesc.SRVFormat);
					}
					srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
					srvd.Texture2D.MostDetailedMip = 0u;
					srvd.Texture2D.MipLevels = td.MipLevels;
				}
				hr = m_Device->CreateShaderResourceView(output.Buffer.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
				if (FAILED(hr))
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D resource SRV failed."));
					return FALSE;
				}
			}
		}
		{
			BOOL32 needUAV = (textureDesc.BindFlags & RBindFlagType::BIND_UNORDERED_ACCESS) != 0;
			if (needUAV && !isDepthFormat)
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
				::ZeroMemory(&uavd, sizeof(uavd));
				RDeviceD3D11::TranslateResourceFormat(uavd.Format, textureDesc.UAVFormat);
				uavd.ViewDimension = D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_TEXTURE2D;
				uavd.Texture2D.MipSlice = 0u;
				hr = m_Device->CreateUnorderedAccessView(output.Buffer.Get(), &uavd, output.UnorderedAccessView.ReleaseAndGetAddressOf());
				if (FAILED(hr))
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D resource UAV failed."));
					return FALSE;
				}
			}
		}
		{
			BOOL32 needRTV = (textureDesc.BindFlags & RBindFlagType::BIND_RENDER_TARGET) != 0;
			if (needRTV && !isDepthFormat)
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvd;
				{
					::ZeroMemory(&rtvd, sizeof(rtvd));
					RDeviceD3D11::TranslateResourceFormat(rtvd.Format, textureDesc.RTVFormat);
					rtvd.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;
					rtvd.Texture2D.MipSlice = 0u;
				}
				hr = m_Device->CreateRenderTargetView(output.Buffer.Get(), &rtvd, output.RenderTargetView.ReleaseAndGetAddressOf());
				if (FAILED(hr))
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D resource RTV failed."));
					return FALSE;
				}
			}
		}
		if (isDepthFormat)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
			{
				::ZeroMemory(&dsvd, sizeof(dsvd));
				if (textureDesc.Depth == 16u)
				{
					dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D16_UNORM;
				}
				else if (textureDesc.Depth == 24u)
				{
					dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
				}
				else if (textureDesc.Depth == 32u)
				{
					if (textureDesc.HasStencil)
					{
						dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
					}
					else
					{
						dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
					}
				}
				dsvd.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvd.Flags = 0u;	//D3D11_DSV_FLAG
				dsvd.Texture2D.MipSlice = 0u;
			}
			hr = m_Device->CreateDepthStencilView(output.Buffer.Get(), &dsvd, output.DepthStencilView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D resource DSV failed."));
				return FALSE;
			}
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateRenderTexture3D(RRenderTexture3D& output, const RTextureDesc& textureDesc)
	{
		if (textureDesc.Width < 1u || textureDesc.Height < 1u || textureDesc.Depth < 1u)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture3D check failed (wrong size used)."));
			return FALSE;
		}
		output.ReleaseRenderResource();
		D3D11_TEXTURE3D_DESC td;
		{
			::ZeroMemory(&td, sizeof(td));
			td.Width = textureDesc.Width;
			td.Height = textureDesc.Height;
			td.Depth = textureDesc.Depth;
			td.MipLevels = textureDesc.MipLevels;
			RDeviceD3D11::TranslateResourceFormat(td.Format, textureDesc.BufferFormat);
			RDeviceD3D11::TranslateUsageFlag(td.Usage, textureDesc.UsageFlag);
			RDeviceD3D11::TranslateBindFlag(td.BindFlags, textureDesc.BindFlags);
			RDeviceD3D11::TranslateCPUAccessFlag(td.CPUAccessFlags, textureDesc.CPUAccessFlags);
			RDeviceD3D11::TranslateResourceMiscFlag(td.MiscFlags, textureDesc.MiscFlags);
		}
		HRESULT hr = m_Device->CreateTexture3D(&td, nullptr, output.Buffer.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture3D resource failed."));
			return FALSE;
		}
		{
			BOOL32 needSRV = (textureDesc.BindFlags & RBindFlagType::BIND_SHADER_RESOURCE) != 0;
			if (needSRV)
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
				{
					::ZeroMemory(&srvd, sizeof(srvd));
					RDeviceD3D11::TranslateResourceFormat(srvd.Format, textureDesc.SRVFormat);
					srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE3D;
					srvd.Texture3D.MostDetailedMip = 0u;
					srvd.Texture3D.MipLevels = td.MipLevels;
				}
				hr = m_Device->CreateShaderResourceView(output.Buffer.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
				if (FAILED(hr))
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture3D resource SRV failed."));
					return FALSE;
				}
			}
		}
		{
			BOOL32 needUAV = (textureDesc.BindFlags & RBindFlagType::BIND_UNORDERED_ACCESS) != 0;
			if (needUAV)
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
				::ZeroMemory(&uavd, sizeof(uavd));
				RDeviceD3D11::TranslateResourceFormat(uavd.Format, textureDesc.UAVFormat);
				uavd.ViewDimension = D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_TEXTURE3D;
				uavd.Texture3D.MipSlice = 0u;
				uavd.Texture3D.FirstWSlice = 0u;
				uavd.Texture3D.WSize = textureDesc.Depth;
				hr = m_Device->CreateUnorderedAccessView(output.Buffer.Get(), &uavd, output.UnorderedAccessView.ReleaseAndGetAddressOf());
				if (FAILED(hr))
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture3D resource UAV failed."));
					return FALSE;
				}
			}
		}
		{
			BOOL32 needRTV = (textureDesc.BindFlags & RBindFlagType::BIND_RENDER_TARGET) != 0;
			if (needRTV)
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvd;
				{
					::ZeroMemory(&rtvd, sizeof(rtvd));
					RDeviceD3D11::TranslateResourceFormat(rtvd.Format, textureDesc.RTVFormat);
					rtvd.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE3D;
					rtvd.Texture3D.MipSlice = 0u;
					rtvd.Texture3D.FirstWSlice = 0u;
					rtvd.Texture3D.WSize = textureDesc.Depth;
				}
				hr = m_Device->CreateRenderTargetView(output.Buffer.Get(), &rtvd, output.RenderTargetView.ReleaseAndGetAddressOf());
				if (FAILED(hr))
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture3D resource RTV failed."));
					return FALSE;
				}
			}
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateTexture2D(RTexture2DResource& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData)
	{
		output.ReleaseRenderResource();
		D3D11_TEXTURE2D_DESC td;
		{
			::ZeroMemory(&td, sizeof(td));
			td.Width = textureDesc.Width;
			td.Height = textureDesc.Height;
			td.MipLevels = textureDesc.MipLevels;
			td.ArraySize = textureDesc.ArraySize;
			RDeviceD3D11::TranslateResourceFormat(td.Format, textureDesc.BufferFormat);
			td.SampleDesc.Count = textureDesc.SampleDesc.Count;
			td.SampleDesc.Quality = textureDesc.SampleDesc.Quality;
			td.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
			RDeviceD3D11::TranslateUsageFlag(td.Usage, textureDesc.UsageFlag);
			RDeviceD3D11::TranslateCPUAccessFlag(td.CPUAccessFlags, textureDesc.CPUAccessFlags);
			RDeviceD3D11::TranslateResourceMiscFlag(td.MiscFlags, textureDesc.MiscFlags);
		}
		HRESULT hr = S_FALSE;
		if (subData)
		{
			D3D11_SUBRESOURCE_DATA subresourceData;
			subresourceData.pSysMem = subData->pSysMem;
			subresourceData.SysMemPitch = subData->SysMemPitch;
			subresourceData.SysMemSlicePitch = subData->SysMemSlicePitch;

			hr = m_Device->CreateTexture2D(&td, &subresourceData, output.Buffer.ReleaseAndGetAddressOf());
		}
		else
		{
			hr = m_Device->CreateTexture2D(&td, nullptr, output.Buffer.ReleaseAndGetAddressOf());
		}
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture2D resource failed."));
			return FALSE;
		}
		D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
		{
			::ZeroMemory(&srvd, sizeof(srvd));
			RDeviceD3D11::TranslateResourceFormat(srvd.Format, textureDesc.SRVFormat);
			srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
			srvd.Texture2D.MostDetailedMip = 0u;
			srvd.Texture2D.MipLevels = td.MipLevels;
		}
		hr = m_Device->CreateShaderResourceView(output.Buffer.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture2D resource SRV failed."));
			return FALSE;
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateTextureCube(RTextureCubeResource& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData)
	{
		if (textureDesc.Width != textureDesc.Height)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture cube check failed (size of texture cube must be a square)."));
			return FALSE;
		}
		output.ReleaseRenderResource();
		{
			D3D11_TEXTURE2D_DESC td;
			{
				::ZeroMemory(&td, sizeof(td));
				td.Width = textureDesc.Width;
				td.Height = textureDesc.Height;
				td.MipLevels = textureDesc.MipLevels;
				td.ArraySize = 6u;	//Texture cube has fixed number(6) of textures.
				RDeviceD3D11::TranslateResourceFormat(td.Format, textureDesc.BufferFormat);
				td.SampleDesc.Count = textureDesc.SampleDesc.Count;
				td.SampleDesc.Quality = textureDesc.SampleDesc.Quality;
				RDeviceD3D11::TranslateUsageFlag(td.Usage, textureDesc.UsageFlag);
				td.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
				RDeviceD3D11::TranslateCPUAccessFlag(td.CPUAccessFlags, textureDesc.CPUAccessFlags);
				td.MiscFlags = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_TEXTURECUBE;
				if (textureDesc.MipLevels == 0u || textureDesc.MipLevels > 1u)
				{
					td.MiscFlags |= D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_GENERATE_MIPS;
				}
			}
			HRESULT hr = S_FALSE;
			if (subData != nullptr)
			{
				D3D11_SUBRESOURCE_DATA sd[6u];
				for (UINT32 i = 0u; i < 6u; i++)
				{
					::ZeroMemory(&(sd[i]), sizeof(sd[i]));
					sd[i].pSysMem = subData[i].pSysMem;
					sd[i].SysMemPitch = subData[i].SysMemPitch;
					sd[i].SysMemSlicePitch = subData[i].SysMemSlicePitch;
				}
				hr = m_Device->CreateTexture2D(&td, sd, output.Buffer.ReleaseAndGetAddressOf());
			}
			else
			{
				hr = m_Device->CreateTexture2D(&td, nullptr, output.Buffer.ReleaseAndGetAddressOf());
			}
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture cube resource failed."));
				return FALSE;
			}
		}
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
			{
				::ZeroMemory(&srvd, sizeof(srvd));
				RDeviceD3D11::TranslateResourceFormat(srvd.Format, textureDesc.SRVFormat);
				srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURECUBE;
				srvd.Texture2D.MostDetailedMip = 0u;
				srvd.Texture2D.MipLevels = textureDesc.MipLevels;
			}
			HRESULT hr = m_Device->CreateShaderResourceView(output.Buffer.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture cube resource SRV failed."));
				return FALSE;
			}
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::LoadVertexShader(const EString& name, RVertexShaderResource& outShaderResource, const RInputLayoutDesc* inLayouts, const UINT32& inLayoutNum)
	{
		LONG fsize;
		BYTE* buffer;
		FILE* file = nullptr;
		{
			fopen_s(&file, *name, "rb");
			if (!file)
			{
#if _EDITOR_ONLY
				{
					EString debugInfo("Can not open file path : ");
					debugInfo = debugInfo + name;
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*debugInfo));
				}
#endif
				return FALSE;
			}
			fsize = _filelength(_fileno(file));
			buffer = new BYTE[fsize];
			fread_s(buffer, fsize, fsize, 1u, file);
			fclose(file);
		}
		{
			HRESULT hr = m_Device->CreateVertexShader(static_cast<void*>(buffer), fsize, nullptr, outShaderResource.Shader.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				delete[]buffer;
#if _EDITOR_ONLY
				{
					EString debugInfo("Create vertex shader resource failed. path : ");
					debugInfo = debugInfo + name;
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*debugInfo));
				}
#endif
				return FALSE;
			}
		}
		{
			if (inLayouts == nullptr)
			{
				delete[]buffer;
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create vertex shader which needs fitted input layouts."));
				return FALSE;
			}
			TArray<D3D11_INPUT_ELEMENT_DESC> tempLayouts;
			tempLayouts.Resize(inLayoutNum);
			for (UINT32 i = 0u; i < inLayoutNum; i++)
			{
				RDeviceD3D11::TranslateInputLayoutDesc(tempLayouts[i], inLayouts[i]);
			}
			HRESULT hr = m_Device->CreateInputLayout(tempLayouts.RawData(), inLayoutNum, static_cast<void*>(buffer), fsize, outShaderResource.InputLayout.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				delete[]buffer;
#if _EDITOR_ONLY
				{
					EString debugInfo("Create vertex shader input layout failed. path : ");
					debugInfo = debugInfo + name;
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*debugInfo));
				}
#endif
				return FALSE;
			}
		}
		delete[]buffer;
		return TRUE;
	}
	BOOL32 RDeviceD3D11::LoadPixelShader(const EString& name, RPixelShaderResource& outShaderResource)
	{
		LONG fsize;
		BYTE* buffer;
		FILE* file = nullptr;
		{
			fopen_s(&file, *name, "rb");
			if (!file)
			{
#if _EDITOR_ONLY
				{
					EString debugInfo("Can not open file path : ");
					debugInfo = debugInfo + name;
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*debugInfo));
				}
#endif
				return FALSE;
			}
			fsize = _filelength(_fileno(file));
			buffer = new BYTE[fsize];
			fread_s(buffer, fsize, fsize, 1u, file);
			fclose(file);
		}
		{
			HRESULT hr = m_Device->CreatePixelShader(static_cast<void*>(buffer), fsize, nullptr, outShaderResource.Shader.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				delete[]buffer;
#if _EDITOR_ONLY
				{
					EString debugInfo("Create pixel shader resource failed. path : ");
					debugInfo = debugInfo + name;
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*debugInfo));
				}
#endif
				return FALSE;
			}
		}
		delete[]buffer;
		return TRUE;
	}
	BOOL32 RDeviceD3D11::LoadComputeShader(const EString& name, RComputeShaderResource& outShaderResource)
	{
		LONG fsize;
		BYTE* buffer;
		FILE* file = nullptr;
		{
			fopen_s(&file, *name, "rb");
			if (!file)
			{
#if _EDITOR_ONLY
				{
					EString debugInfo("Can not open file path : ");
					debugInfo = debugInfo + name;
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*debugInfo));
				}
#endif
				return FALSE;
			}
			fsize = _filelength(_fileno(file));
			buffer = new BYTE[fsize];
			fread_s(buffer, fsize, fsize, 1u, file);
			fclose(file);
		}
		{
			HRESULT hr = m_Device->CreateComputeShader(static_cast<void*>(buffer), fsize, nullptr, outShaderResource.Shader.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				delete[]buffer;
#if _EDITOR_ONLY
				{
					EString debugInfo("Create compute shader resource failed. path : ");
					debugInfo = debugInfo + name;
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), (*debugInfo));
				}
#endif
				return FALSE;
			}
		}
		delete[]buffer;
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const RBufferDesc& bufferDesc, const RSubresourceDataDesc* subData)
	{
		D3D11_BUFFER_DESC bd;
		{
			::ZeroMemory(&bd, sizeof(bd));
			bd.ByteWidth = bufferDesc.ByteWidth;
			RDeviceD3D11::TranslateUsageFlag(bd.Usage, bufferDesc.Usage);
			RDeviceD3D11::TranslateBindFlag(bd.BindFlags, bufferDesc.BindFlags);
			RDeviceD3D11::TranslateCPUAccessFlag(bd.CPUAccessFlags, bufferDesc.CPUAccessFlags);
			RDeviceD3D11::TranslateResourceMiscFlag(bd.MiscFlags, bufferDesc.MiscFlags);
			bd.StructureByteStride = bufferDesc.StructureByteStride;
		}
		HRESULT hr = S_FALSE;
		if (subData)
		{
			D3D11_SUBRESOURCE_DATA sd;
			{
				::ZeroMemory(&sd, sizeof(sd));
				sd.pSysMem = subData->pSysMem;
				sd.SysMemPitch = subData->SysMemPitch;
				sd.SysMemSlicePitch = subData->SysMemSlicePitch;
			}
			hr = m_Device->CreateBuffer(&bd, &sd, buffer.ReleaseAndGetAddressOf());
		}
		else
		{
			hr = m_Device->CreateBuffer(&bd, nullptr, buffer.ReleaseAndGetAddressOf());
		}
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create buffer resource failed."));
			return FALSE;
		}
		return TRUE;
	}
	void RDeviceD3D11::UploadBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& dstResource, const void* srcData, UINT32 srcRowPitch, UINT32 srcDepthPitch, UINT32 dstSubresource, const D3D11_BOX* dstBox)
	{
		m_ImmediateContext->UpdateSubresource(dstResource.Get(), dstSubresource, dstBox, srcData, srcRowPitch, srcDepthPitch);
	}
	void RDeviceD3D11::UploadResource(const Microsoft::WRL::ComPtr<ID3D11Texture2D>& dstResource, const void* srcData, UINT32 srcRowPitch, UINT32 srcDepthPitch, UINT32 dstSubresource, const D3D11_BOX* dstBox)
	{
		m_ImmediateContext->UpdateSubresource(dstResource.Get(), dstSubresource, dstBox, srcData, srcRowPitch, srcDepthPitch);
	}
	void RDeviceD3D11::Present(const UINT32& syncInterval)
	{
		HRESULT hr = m_SwapChain->Present(syncInterval, 0u);	//DXGI_PRESENT
	}
	void RDeviceD3D11::SetDefaultDepthStencilState()
	{
		m_ImmediateContext->OMSetDepthStencilState(nullptr, 0x0u);
	}
	void RDeviceD3D11::SetDepthStencilState(const Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const UINT32& stencilRef)
	{
		m_ImmediateContext->OMSetDepthStencilState(dss.Get(), stencilRef);
	}
	void RDeviceD3D11::SetDefaultBlendState()
	{
		m_ImmediateContext->OMSetBlendState(nullptr, nullptr, 0xffffffffu);
	}
	void RDeviceD3D11::SetBlendState(const Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const Color4& blendFactor, const UINT32& sampleMask)
	{
		m_ImmediateContext->OMSetBlendState(bs.Get(), blendFactor.rgba, sampleMask);
	}
	void RDeviceD3D11::SetNoRenderTarget()
	{
		ID3D11RenderTargetView* rtvs[] = { nullptr };
		m_ImmediateContext->OMSetRenderTargets(1u, rtvs, nullptr);
	}
	void RDeviceD3D11::SetRenderTargetOnly(const RRenderTexture2D& InRTV)
	{
		m_ImmediateContext->OMSetRenderTargets(1u, InRTV.RenderTargetView.GetAddressOf(), nullptr);
	}
	void RDeviceD3D11::SetDepthStencilOnly(const RRenderTexture2D& InDSV)
	{
		ID3D11RenderTargetView* rtvs[] = { nullptr };
		m_ImmediateContext->OMSetRenderTargets(1u, rtvs, InDSV.DepthStencilView.Get());
	}
	void RDeviceD3D11::SetRenderTarget(const RRenderTexture2D& InRTV, const RRenderTexture2D& InDSV)
	{
		m_ImmediateContext->OMSetRenderTargets(1u, InRTV.RenderTargetView.GetAddressOf(), InDSV.DepthStencilView.Get());
	}
	void RDeviceD3D11::SetRenderTargets(const RRenderTexture2D* InRTVs, const UINT32& InRTVNum)
	{
		TArray<ID3D11RenderTargetView*> UsedRTVs;
		UsedRTVs.Resize(InRTVNum);
		for (UINT32 i = 0u; i < InRTVNum; i++)
		{
			UsedRTVs[i] = (InRTVs[i]).RenderTargetView.Get();
		}
		m_ImmediateContext->OMSetRenderTargets(InRTVNum, UsedRTVs.RawData(), nullptr);
	}
	void RDeviceD3D11::SetRenderTargets(const RRenderTexture2D* InRTVs, const UINT32& InRTVNum, const RRenderTexture2D& InDSV)
	{
		TArray<ID3D11RenderTargetView*> UsedRTVs;
		UsedRTVs.Resize(InRTVNum);
		for (UINT32 i = 0u; i < InRTVNum; i++)
		{
			UsedRTVs[i] = (InRTVs[i]).RenderTargetView.Get();
		}
		m_ImmediateContext->OMSetRenderTargets(InRTVNum, UsedRTVs.RawData(), InDSV.DepthStencilView.Get());
	}
	void RDeviceD3D11::SetRasterizerState(const Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs)
	{
		m_ImmediateContext->RSSetState(rs.Get());
	}
	void RDeviceD3D11::SetViewport(const EViewport& viewport)
	{
		D3D11_VIEWPORT vp = {
			viewport.TopLeftX, viewport.TopLeftY,
			viewport.Width, viewport.Height,
			viewport.MinDepth, viewport.MaxDepth };
		m_ImmediateContext->RSSetViewports(1u, &vp);
	}
	void RDeviceD3D11::SetViewports(const EViewport* viewports, const UINT32& viewportNum)
	{
		TArray<D3D11_VIEWPORT> vps;
		vps.Resize(viewportNum);
		for (UINT32 i = 0u; i < viewportNum; i++)
		{
			vps[i].TopLeftX = viewports[i].TopLeftX;
			vps[i].TopLeftY = viewports[i].TopLeftY;
			vps[i].Width = viewports[i].Width;
			vps[i].Height = viewports[i].Height;
			vps[i].MinDepth = viewports[i].MinDepth;
			vps[i].MaxDepth = viewports[i].MaxDepth;
		}
		m_ImmediateContext->RSSetViewports(viewportNum, vps.RawData());
	}
	void RDeviceD3D11::SetNoVSShader()
	{
		m_ImmediateContext->VSSetShader(nullptr, nullptr, 0u);
	}
	void RDeviceD3D11::SetNoPSShader()
	{
		m_ImmediateContext->PSSetShader(nullptr, nullptr, 0u);
	}
	void RDeviceD3D11::SetNoCSShader()
	{
		m_ImmediateContext->CSSetShader(nullptr, nullptr, 0u);
	}
	void RDeviceD3D11::SetVSShader(const Microsoft::WRL::ComPtr<ID3D11VertexShader>& vs)
	{
		m_ImmediateContext->VSSetShader(vs.Get(), nullptr, 0u);
	}
	void RDeviceD3D11::SetPSShader(const Microsoft::WRL::ComPtr<ID3D11PixelShader>& ps)
	{
		m_ImmediateContext->PSSetShader(ps.Get(), nullptr, 0u);
	}
	void RDeviceD3D11::SetCSShader(const Microsoft::WRL::ComPtr<ID3D11ComputeShader>& cs)
	{
		m_ImmediateContext->CSSetShader(cs.Get(), nullptr, 0u);
	}
	void RDeviceD3D11::BindVSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT32& startSlot)
	{
		m_ImmediateContext->VSSetSamplers(startSlot, 1u, ss.GetAddressOf());
	}
	void RDeviceD3D11::BindVSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT32& startSlot, const UINT32& ssNum)
	{
		TArray<ID3D11SamplerState*> sampler;
		sampler.Resize(ssNum);
		for (UINT32 i = 0u; i < ssNum; i++)
		{
			sampler[i] = ss[i].Get();
		}
		m_ImmediateContext->VSSetSamplers(startSlot, ssNum, sampler.RawData());
	}
	void RDeviceD3D11::BindPSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT32& startSlot)
	{
		m_ImmediateContext->PSSetSamplers(startSlot, 1u, ss.GetAddressOf());
	}
	void RDeviceD3D11::BindPSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT32& startSlot, const UINT32& ssNum)
	{
		TArray<ID3D11SamplerState*> sampler;
		sampler.Resize(ssNum);
		for (UINT32 i = 0u; i < ssNum; i++)
		{
			sampler[i] = ss[i].Get();
		}
		m_ImmediateContext->PSSetSamplers(startSlot, ssNum, sampler.RawData());
	}
	void RDeviceD3D11::BindCSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT32& startSlot)
	{
		m_ImmediateContext->CSSetSamplers(startSlot, 1u, ss.GetAddressOf());
	}
	void RDeviceD3D11::BindCSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT32& startSlot, const UINT32& ssNum)
	{
		TArray<ID3D11SamplerState*> sampler;
		sampler.Resize(ssNum);
		for (UINT32 i = 0u; i < ssNum; i++)
		{
			sampler[i] = ss[i].Get();
		}
		m_ImmediateContext->CSSetSamplers(startSlot, ssNum, sampler.RawData());
	}
	void RDeviceD3D11::BindVSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT32& startSlot)
	{
		m_ImmediateContext->VSSetConstantBuffers(startSlot, 1u, buffer.GetAddressOf());
	}
	void RDeviceD3D11::BindVSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT32& startSlot, const UINT32& bufferNum)
	{
		TArray<ID3D11Buffer*> buffers;
		buffers.Resize(bufferNum);
		for (UINT32 i = 0u; i < bufferNum; i++)
		{
			buffers[i] = buffer[i].Get();
		}
		m_ImmediateContext->VSSetConstantBuffers(startSlot, bufferNum, buffers.RawData());
	}
	void RDeviceD3D11::BindPSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT32& startSlot)
	{
		m_ImmediateContext->PSSetConstantBuffers(startSlot, 1u, buffer.GetAddressOf());
	}
	void RDeviceD3D11::BindPSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT32& startSlot, const UINT32& bufferNum)
	{
		TArray<ID3D11Buffer*> buffers;
		buffers.Resize(bufferNum);
		for (UINT32 i = 0u; i < bufferNum; i++)
		{
			buffers[i] = buffer[i].Get();
		}
		m_ImmediateContext->PSSetConstantBuffers(startSlot, bufferNum, buffers.RawData());
	}
	void RDeviceD3D11::BindCSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT32& startSlot)
	{
		m_ImmediateContext->CSSetConstantBuffers(startSlot, 1u, buffer.GetAddressOf());
	}
	void RDeviceD3D11::BindCSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT32& startSlot, const UINT32& bufferNum)
	{
		TArray<ID3D11Buffer*> buffers;
		buffers.Resize(bufferNum);
		for (UINT32 i = 0u; i < bufferNum; i++)
		{
			buffers[i] = buffer[i].Get();
		}
		m_ImmediateContext->CSSetConstantBuffers(startSlot, bufferNum, buffers.RawData());
	}
	void RDeviceD3D11::BindVSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT32& startSlot)
	{
		m_ImmediateContext->VSSetShaderResources(startSlot, 1u, srv.GetAddressOf());
	}
	void RDeviceD3D11::BindVSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT32& startSlot, const UINT32& srvNum)
	{
		TArray<ID3D11ShaderResourceView*> srvs;
		srvs.Resize(srvNum);
		for (UINT32 i = 0u; i < srvNum; i++)
		{
			srvs[i] = srv[i].Get();
		}
		m_ImmediateContext->VSSetShaderResources(startSlot, srvNum, srvs.RawData());
	}
	void RDeviceD3D11::BindPSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT32& startSlot)
	{
		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, srv.GetAddressOf());
	}
	void RDeviceD3D11::BindPSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT32& startSlot, const UINT32& srvNum)
	{
		TArray<ID3D11ShaderResourceView*> srvs;
		srvs.Resize(srvNum);
		for (UINT32 i = 0u; i < srvNum; i++)
		{
			srvs[i] = srv[i].Get();
		}
		m_ImmediateContext->PSSetShaderResources(startSlot, srvNum, srvs.RawData());
	}
	void RDeviceD3D11::BindCSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT32& startSlot)
	{
		m_ImmediateContext->CSSetShaderResources(startSlot, 1u, srv.GetAddressOf());
	}
	void RDeviceD3D11::BindCSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT32& startSlot, const UINT32& srvNum)
	{
		TArray<ID3D11ShaderResourceView*> srvs;
		srvs.Resize(srvNum);
		for (UINT32 i = 0u; i < srvNum; i++)
		{
			srvs[i] = srv[i].Get();
		}
		m_ImmediateContext->CSSetShaderResources(startSlot, srvNum, srvs.RawData());
	}
	void RDeviceD3D11::BindNoCSUnorderedAccessView(const UINT32& startSlot)
	{
		ID3D11UnorderedAccessView* uavs[] = { nullptr };
		m_ImmediateContext->CSSetUnorderedAccessViews(startSlot, 1u, uavs, nullptr);
	}
	void RDeviceD3D11::BindCSUnorderedAccessView(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const UINT32& startSlot)
	{
		m_ImmediateContext->CSSetUnorderedAccessViews(startSlot, 1u, uav.GetAddressOf(), nullptr);
	}
	void RDeviceD3D11::BindCSUnorderedAccessViews(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>* uav, const UINT32& startSlot, const UINT32& uavNum)
	{
		TArray<ID3D11UnorderedAccessView*> uavs;
		uavs.Resize(uavNum);
		for (UINT32 i = 0u; i < uavNum; i++)
		{
			uavs[i] = uav[i].Get();
		}
		m_ImmediateContext->CSSetUnorderedAccessViews(startSlot, uavNum, uavs.RawData(), nullptr);
	}
	void RDeviceD3D11::CopyTexture2DResource(const Microsoft::WRL::ComPtr<ID3D11Texture2D>& src, const Microsoft::WRL::ComPtr<ID3D11Texture2D>& dst)
	{
		m_ImmediateContext->CopyResource(dst.Get(), src.Get());
	}
	void RDeviceD3D11::ClearRenderTargetView(const RRenderTexture2D& InRTV, const Color4& InClearColor)
	{
		m_ImmediateContext->ClearRenderTargetView(InRTV.RenderTargetView.Get(), InClearColor.rgba);
	}
	void RDeviceD3D11::ClearDepthStencilView(const RRenderTexture2D& InDSV, const UINT32 InFlags, const FLOAT InDepth, const UINT32 InStencil)
	{
		UINT32 UsedClearFlags = 0u;
		RDeviceD3D11::TranslateClearDepthStencilFlag(UsedClearFlags, InFlags);
		UINT8 UsedClearStencil = static_cast<UINT8>(EMath::Min(InStencil, 0xffu));
		FLOAT UsedClearDepth = EMath::Clamp(InDepth, 0.f, 1.f);
		m_ImmediateContext->ClearDepthStencilView(InDSV.DepthStencilView.Get(), UsedClearFlags, UsedClearDepth, UsedClearStencil);
	}
	void RDeviceD3D11::ClearUnorderedAccessViewFloat(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const Color4& clearValue)
	{
		m_ImmediateContext->ClearUnorderedAccessViewFloat(uav.Get(), clearValue.rgba);
	}
	void RDeviceD3D11::ClearUnorderedAccessViewUint(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const Vector4Int& clearValue)
	{
		UINT32 values[4] = { static_cast<UINT32>(clearValue.x), static_cast<UINT32>(clearValue.y), static_cast<UINT32>(clearValue.z), static_cast<UINT32>(clearValue.w) };
		m_ImmediateContext->ClearUnorderedAccessViewUint(uav.Get(), values);
	}
	void RDeviceD3D11::SetInputLayoutAndVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout, const Microsoft::WRL::ComPtr<ID3D11Buffer>& vb, const UINT32& stride, const UINT32& offset)
	{
		m_ImmediateContext->IASetInputLayout(layout.Get());
		//D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT 
		m_ImmediateContext->IASetVertexBuffers(0u, 1u, vb.GetAddressOf(), &stride, &offset);
	}
	void RDeviceD3D11::SetInputLayout(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout)
	{
		m_ImmediateContext->IASetInputLayout(layout.Get());
	}
	void RDeviceD3D11::SetNoInputLayout()
	{
		m_ImmediateContext->IASetInputLayout(nullptr);
	}
	void RDeviceD3D11::SetVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& InBuffer, UINT32 InStride, UINT32 InOffset, UINT32 InStartSlot, UINT32 InBufferNum)
	{
		//D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT 
		m_ImmediateContext->IASetVertexBuffers(InStartSlot, InBufferNum, InBuffer.GetAddressOf(), &InStride, &InOffset);
	}
	void RDeviceD3D11::SetNoVertexBuffer()
	{
		ID3D11Buffer* Buffers[] = { nullptr };
		UINT32 Stride = 0u, Offset = 0u;
		//D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT 
		m_ImmediateContext->IASetVertexBuffers(0u, 1u, Buffers, &Stride, &Offset);
	}
	void RDeviceD3D11::SetIndexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& InBuffer, UINT32 InOffset, RFormatType InFormat)
	{
		DXGI_FORMAT IndexFormat = DXGI_FORMAT_UNKNOWN;
		RDeviceD3D11::TranslateResourceFormat(IndexFormat, InFormat);
		m_ImmediateContext->IASetIndexBuffer(InBuffer.Get(), IndexFormat, InOffset);
	}
	void RDeviceD3D11::SetNoIndexBuffer()
	{
		DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;
		m_ImmediateContext->IASetIndexBuffer(nullptr, IndexFormat, 0u);
	}
	void RDeviceD3D11::SetPrimitiveTopology(RPrimitiveTopologyType topology)
	{
		D3D_PRIMITIVE_TOPOLOGY output = D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		TranslatePrimitiveTopology(output, topology);
		m_ImmediateContext->IASetPrimitiveTopology(output);
	}
	void RDeviceD3D11::Draw(const UINT32& vertexCount, const UINT32& startVertexLocation)
	{
		m_ImmediateContext->Draw(vertexCount, startVertexLocation);
	}
	void RDeviceD3D11::DrawIndexed(const UINT32& indexCount, const UINT32& startIndexLocation, const INT32& baseVertexLocation)
	{
		m_ImmediateContext->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
	}
	void RDeviceD3D11::DrawIndexedInstance(const UINT32& instanceCount, const UINT32& indexCountPerInstance, const UINT32& startInstanceLocation, const UINT32& startIndexLocation, const INT32& BaseVertexLocation)
	{
		m_ImmediateContext->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, BaseVertexLocation, startInstanceLocation);
	}
	void RDeviceD3D11::Dispatch(const UINT32& x, const UINT32& y, const UINT32& z)
	{
		m_ImmediateContext->Dispatch(x, y, z);
	}
	void RDeviceD3D11::DispatchIndirect(const Microsoft::WRL::ComPtr<ID3D11Buffer>& arg, const UINT32& alignedByteOffsetForArgs)
	{
		m_ImmediateContext->DispatchIndirect(arg.Get(), alignedByteOffsetForArgs);
	}
	BOOL32 RDeviceD3D11::CreateDepthStencilState(Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const RDepthState& depthState, const RStencilState* stencilState)
	{
		D3D11_DEPTH_STENCIL_DESC dsd;
		{
			::ZeroMemory(&dsd, sizeof(dsd));
			RDeviceD3D11::TranslateDepthStencilState(dsd, depthState, stencilState);
		}
		HRESULT hr = m_Device->CreateDepthStencilState(&dsd, dss.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create depth stencil state failed."));
			return FALSE;
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateBlendState(Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const RBlendState* blendStates, const UINT32& blendStateNum)
	{
		if (!blendStates || blendStateNum == 0u)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create blend state check failed."));
			return FALSE;
		}
		D3D11_BLEND_DESC bd;
		{
			::ZeroMemory(&bd, sizeof(bd));
			bd.AlphaToCoverageEnable = FALSE;
			bd.IndependentBlendEnable = FALSE;
			if (blendStateNum > 1u)
			{
				bd.IndependentBlendEnable = TRUE;
			}
			for (UINT32 i = 0u; i < blendStateNum && i < 8u; i++)
			{
				RDeviceD3D11::TranslateBlendState(bd.RenderTarget[i], blendStates[i]);
			}
		}
		HRESULT hr = m_Device->CreateBlendState(&bd, bs.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create blend state failed."));
			return FALSE;
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateRasterizerState(Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs, const RRasterizerState& rasterizerState)
	{
		D3D11_RASTERIZER_DESC rd;
		{
			::ZeroMemory(&rd, sizeof(rd));
			RDeviceD3D11::TranslateRasterizerState(rd.CullMode, rd.FillMode, rasterizerState);
			rd.FrontCounterClockwise = FALSE;
			rd.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
			rd.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
			rd.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			rd.DepthClipEnable = TRUE;
			rd.ScissorEnable = FALSE;
			rd.MultisampleEnable = FALSE;
			rd.AntialiasedLineEnable = FALSE;
		}
		HRESULT hr = m_Device->CreateRasterizerState(&rd, rs.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create rasterizer state failed."));
			return FALSE;
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const RSamplerState& samplerState)
	{
		D3D11_SAMPLER_DESC sd;
		{
			::ZeroMemory(&sd, sizeof(sd));
			RDeviceD3D11::TranslateSamplerState(sd, samplerState);
		}
		HRESULT hr = m_Device->CreateSamplerState(&sd, ss.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create sampler state failed."));
			return FALSE;
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateQuery(Microsoft::WRL::ComPtr<ID3D11Query>& q, const RQueryDesc& queryDesc)
	{
		D3D11_QUERY_DESC qd;
		{
			::ZeroMemory(&qd, sizeof(qd));
			RDeviceD3D11::TranslateQueryDesc(qd, queryDesc);
		}
		HRESULT hr = m_Device->CreateQuery(&qd, q.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create query failed."));
			return FALSE;
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::GetData(ID3D11Asynchronous* pAsync, void* output, const UINT32& size, RAsyncGetDataFlagType flag)
	{
		UINT32 getDataFlag = 0u;
		RDeviceD3D11::TranslateGetDataFlag(getDataFlag, flag);
		HRESULT hr = m_ImmediateContext->GetData(pAsync, output, size, getDataFlag);
		return (!(FAILED(hr)));
	}
	void RDeviceD3D11::Begin(ID3D11Asynchronous* pAsync)
	{
		m_ImmediateContext->Begin(pAsync);
	}
	void RDeviceD3D11::End(ID3D11Asynchronous* pAsync)
	{
		m_ImmediateContext->End(pAsync);
	}
	BOOL32 RDeviceD3D11::Map(const RStructuredBuffer& input, const UINT32& indexSubResource, RResourceMapType mapType, RResourceMapFlagType mapFlag, RMappedResource& output)
	{
		{
			if (!(input.AccessMapRead || input.AccessMapWrite))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Mapping resource buffer check failed (error input access flag)."));
				return FALSE;
			}
			BOOL32 needRead = mapType == RResourceMapType::RESOURCE_MAP_READ || mapType == RResourceMapType::RESOURCE_MAP_READ_WRITE;
			BOOL32 needWrite = mapType == RResourceMapType::RESOURCE_MAP_WRITE || mapType == RResourceMapType::RESOURCE_MAP_READ_WRITE || mapType == RResourceMapType::RESOURCE_MAP_WRITE_DISCARD || mapType == RResourceMapType::RESOURCE_MAP_WRITE_NO_OVERWRITE;
			if (input.AccessMapRead != needRead || input.AccessMapWrite != needWrite)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Mapping resource buffer check failed (error access flag)."));
				return FALSE;
			}
		}
		D3D11_MAPPED_SUBRESOURCE ms;
		{
			UINT32 d3dMapFlag; D3D11_MAP d3dMapType;
			::ZeroMemory(&ms, sizeof(ms));
			RDeviceD3D11::TranslateResourceMapType(d3dMapType, mapType);
			RDeviceD3D11::TranslateResourceMapFlag(d3dMapFlag, mapFlag);
			HRESULT hr = m_ImmediateContext->Map(
				input.Buffer.Get(),
				indexSubResource,
				d3dMapType,
				d3dMapFlag,
				&ms);
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Mapping resource buffer access failed."));
				return FALSE;
			}
		}
		output.pData = ms.pData;
		output.RowPitch = ms.RowPitch;
		output.DepthPitch = ms.DepthPitch;
		return TRUE;
	}
	void RDeviceD3D11::Unmap(const RStructuredBuffer& input, const UINT32& indexSubResource)
	{
		m_ImmediateContext->Unmap(input.Buffer.Get(), indexSubResource);
	}
	void RDeviceD3D11::TranslateBindFlag(UINT32& output, UINT8 input)
	{
		static RBindFlagType bindFlagTypes[] =
		{
			RBindFlagType::BIND_VERTEX_BUFFER,
			RBindFlagType::BIND_INDEX_BUFFER,
			RBindFlagType::BIND_CONSTANT_BUFFER,
			RBindFlagType::BIND_SHADER_RESOURCE,
			RBindFlagType::BIND_STREAM_OUTPUT,
			RBindFlagType::BIND_RENDER_TARGET,
			RBindFlagType::BIND_DEPTH_STENCIL,
			RBindFlagType::BIND_UNORDERED_ACCESS
		};
		static UINT32 d3dBindFlagTypes[] =
		{
			D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE,
			D3D11_BIND_FLAG::D3D11_BIND_STREAM_OUTPUT,
			D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET,
			D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL,
			D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS
		};
		output = 0u;
		if (input == RBindFlagType::BIND_NONE)
		{
			return;
		}
		for (UINT32 i = 0u; i < 8u; i++)
		{
			if ((input & bindFlagTypes[i]) != 0)
			{
				output |= d3dBindFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateUsageFlag(D3D11_USAGE& output, RUsageFlagType input)
	{
		static TMap<RUsageFlagType, D3D11_USAGE> usageFlagMap =
		{
			{ RUsageFlagType::USAGE_DEFAULT		, D3D11_USAGE::D3D11_USAGE_DEFAULT },
			{ RUsageFlagType::USAGE_IMMUTABLE	, D3D11_USAGE::D3D11_USAGE_IMMUTABLE },
			{ RUsageFlagType::USAGE_DYNAMIC		, D3D11_USAGE::D3D11_USAGE_DYNAMIC },
			{ RUsageFlagType::USAGE_STAGING		, D3D11_USAGE::D3D11_USAGE_STAGING }
		};
		output = usageFlagMap[input];
	}
	void RDeviceD3D11::TranslateCPUAccessFlag(UINT32& output, RCPUAccessFlagType input)
	{
		static RCPUAccessFlagType CPUAccessFlagTypes[] =
		{
			RCPUAccessFlagType::CPU_ACCESS_READ,
			RCPUAccessFlagType::CPU_ACCESS_WRITE
		};
		static UINT32 d3dCPUAccessFlagTypes[] =
		{
			D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ,
			D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE
		};
		output = 0u;
		if (input == RCPUAccessFlagType::CPU_ACCESS_DEFAULT)
		{
			return;
		}
		for (UINT32 i = 0u; i < 2u; i++)
		{
			if ((input & CPUAccessFlagTypes[i]) != 0)
			{
				output |= d3dCPUAccessFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateResourceMiscFlag(UINT32& output, RResourceMiscFlagType input)
	{
		static RResourceMiscFlagType miscFlagTypes[] =
		{
			RResourceMiscFlagType::RESOURCE_MISC_GENERATE_MIPS,
			RResourceMiscFlagType::RESOURCE_MISC_TEXTURECUBE,
			RResourceMiscFlagType::RESOURCE_MISC_DRAWINDIRECT_ARGS,
			RResourceMiscFlagType::RESOURCE_MISC_BUFFER_STRUCTURED,
			RResourceMiscFlagType::RESOURCE_MISC_RESOURCE_CLAMP
		};
		static UINT32 d3dMiscFlagTypes[] =
		{
			D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_GENERATE_MIPS,
			D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_TEXTURECUBE,
			D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS,
			D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
			D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_RESOURCE_CLAMP
		};
		output = 0u;
		if (input == RResourceMiscFlagType::RESOURCE_MISC_NONE)
		{
			return;
		}
		for (UINT32 i = 0u; i < 5u; i++)
		{
			if ((input & miscFlagTypes[i]) != 0)
			{
				output |= d3dMiscFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateResourceFormat(DXGI_FORMAT& output, RFormatType input)
	{
		static TMap<RFormatType, DXGI_FORMAT> formatMap =
		{
			{ RFormatType::FORMAT_UNKNOWN						, DXGI_FORMAT::DXGI_FORMAT_UNKNOWN						},
			{ RFormatType::FORMAT_R32G32B32A32_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_TYPELESS		},
			{ RFormatType::FORMAT_R32G32B32A32_FLOAT			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT			},
			{ RFormatType::FORMAT_R32G32B32A32_UINT 			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT			},
			{ RFormatType::FORMAT_R32G32B32A32_SINT 			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT			},
			{ RFormatType::FORMAT_R32G32B32_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_TYPELESS			},
			{ RFormatType::FORMAT_R32G32B32_FLOAT				, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT				},
			{ RFormatType::FORMAT_R32G32B32_UINT				, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT				},
			{ RFormatType::FORMAT_R32G32B32_SINT				, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT				},
			{ RFormatType::FORMAT_R16G16B16A16_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_TYPELESS		},
			{ RFormatType::FORMAT_R16G16B16A16_FLOAT			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT			},
			{ RFormatType::FORMAT_R16G16B16A16_UNORM			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UNORM			},
			{ RFormatType::FORMAT_R16G16B16A16_UINT				, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT			},
			{ RFormatType::FORMAT_R16G16B16A16_SNORM			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SNORM			},
			{ RFormatType::FORMAT_R16G16B16A16_SINT				, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SINT			},
			{ RFormatType::FORMAT_R32G32_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_R32G32_TYPELESS				},
			{ RFormatType::FORMAT_R32G32_FLOAT					, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT					},
			{ RFormatType::FORMAT_R32G32_UINT					, DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT					},
			{ RFormatType::FORMAT_R32G32_SINT					, DXGI_FORMAT::DXGI_FORMAT_R32G32_SINT					},
			{ RFormatType::FORMAT_R10G10B10A2_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_TYPELESS			},
			{ RFormatType::FORMAT_R10G10B10A2_UNORM				, DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM     		},
			{ RFormatType::FORMAT_R10G10B10A2_UINT				, DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UINT      		},
			{ RFormatType::FORMAT_R11G11B10_FLOAT				, DXGI_FORMAT::DXGI_FORMAT_R11G11B10_FLOAT       		},
			{ RFormatType::FORMAT_R8G8B8A8_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_TYPELESS     		},
			{ RFormatType::FORMAT_R8G8B8A8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM        		},
			{ RFormatType::FORMAT_R8G8B8A8_UNORM_SRGB			, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB   		},
			{ RFormatType::FORMAT_R8G8B8A8_UINT					, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT         		},
			{ RFormatType::FORMAT_R8G8B8A8_SNORM				, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SNORM        		},
			{ RFormatType::FORMAT_R8G8B8A8_SINT					, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SINT         		},
			{ RFormatType::FORMAT_R16G16_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_R16G16_TYPELESS       		},
			{ RFormatType::FORMAT_R16G16_FLOAT					, DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT          		},
			{ RFormatType::FORMAT_R16G16_UNORM					, DXGI_FORMAT::DXGI_FORMAT_R16G16_UNORM          		},
			{ RFormatType::FORMAT_R16G16_UINT 					, DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT           		},
			{ RFormatType::FORMAT_R16G16_SNORM					, DXGI_FORMAT::DXGI_FORMAT_R16G16_SNORM          		},
			{ RFormatType::FORMAT_R16G16_SINT 					, DXGI_FORMAT::DXGI_FORMAT_R16G16_SINT           		},
			{ RFormatType::FORMAT_R32_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS          		},
			{ RFormatType::FORMAT_R32_FLOAT						, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT             		},
			{ RFormatType::FORMAT_R32_UINT 						, DXGI_FORMAT::DXGI_FORMAT_R32_UINT              		},
			{ RFormatType::FORMAT_R32_SINT 						, DXGI_FORMAT::DXGI_FORMAT_R32_SINT              		},
			{ RFormatType::FORMAT_R24G8_TYPELESS 				, DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS        		},
			{ RFormatType::FORMAT_R8G8_TYPELESS 				, DXGI_FORMAT::DXGI_FORMAT_R8G8_TYPELESS     			},
			{ RFormatType::FORMAT_R8G8_UNORM					, DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM        			},
			{ RFormatType::FORMAT_R8G8_UINT 					, DXGI_FORMAT::DXGI_FORMAT_R8G8_UINT         			},
			{ RFormatType::FORMAT_R8G8_SNORM					, DXGI_FORMAT::DXGI_FORMAT_R8G8_SNORM        			},
			{ RFormatType::FORMAT_R8G8_SINT 					, DXGI_FORMAT::DXGI_FORMAT_R8G8_SINT         			},
			{ RFormatType::FORMAT_R16_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS      			},
			{ RFormatType::FORMAT_R16_FLOAT						, DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT         			},
			{ RFormatType::FORMAT_R16_UNORM						, DXGI_FORMAT::DXGI_FORMAT_R16_UNORM         			},
			{ RFormatType::FORMAT_R16_UINT 						, DXGI_FORMAT::DXGI_FORMAT_R16_UINT          			},
			{ RFormatType::FORMAT_R16_SNORM						, DXGI_FORMAT::DXGI_FORMAT_R16_SNORM         			},
			{ RFormatType::FORMAT_R16_SINT 						, DXGI_FORMAT::DXGI_FORMAT_R16_SINT          			},
			{ RFormatType::FORMAT_R8_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_R8_TYPELESS       			},
			{ RFormatType::FORMAT_R8_UNORM						, DXGI_FORMAT::DXGI_FORMAT_R8_UNORM          			},
			{ RFormatType::FORMAT_R8_UINT 						, DXGI_FORMAT::DXGI_FORMAT_R8_UINT           			},
			{ RFormatType::FORMAT_R8_SNORM						, DXGI_FORMAT::DXGI_FORMAT_R8_SNORM          			},
			{ RFormatType::FORMAT_R8_SINT 						, DXGI_FORMAT::DXGI_FORMAT_R8_SINT           			},
			{ RFormatType::FORMAT_A8_UNORM						, DXGI_FORMAT::DXGI_FORMAT_A8_UNORM          			},
			{ RFormatType::FORMAT_R1_UNORM						, DXGI_FORMAT::DXGI_FORMAT_R1_UNORM          			},
			{ RFormatType::FORMAT_R9G9B9E5_SHAREDEXP			, DXGI_FORMAT::DXGI_FORMAT_R9G9B9E5_SHAREDEXP			},
			{ RFormatType::FORMAT_R8G8_B8G8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_R8G8_B8G8_UNORM				},
			{ RFormatType::FORMAT_G8R8_G8B8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_G8R8_G8B8_UNORM				},
			{ RFormatType::FORMAT_BC1_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS					},
			{ RFormatType::FORMAT_BC1_UNORM						, DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM					},
			{ RFormatType::FORMAT_BC1_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB				},
			{ RFormatType::FORMAT_BC2_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_BC2_TYPELESS					},
			{ RFormatType::FORMAT_BC2_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM					},
			{ RFormatType::FORMAT_BC2_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB				},
			{ RFormatType::FORMAT_BC3_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS					},
			{ RFormatType::FORMAT_BC3_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM					},
			{ RFormatType::FORMAT_BC3_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB				},
			{ RFormatType::FORMAT_BC4_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS					},
			{ RFormatType::FORMAT_BC4_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM					},
			{ RFormatType::FORMAT_BC4_SNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM					},
			{ RFormatType::FORMAT_BC5_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS					},
			{ RFormatType::FORMAT_BC5_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM					},
			{ RFormatType::FORMAT_BC5_SNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM					},
			{ RFormatType::FORMAT_B5G6R5_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_B5G6R5_UNORM					},
			{ RFormatType::FORMAT_B5G5R5A1_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B5G5R5A1_UNORM				},
			{ RFormatType::FORMAT_B8G8R8A8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM				},
			{ RFormatType::FORMAT_B8G8R8X8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM				},
			{ RFormatType::FORMAT_R10G10B10_XR_BIAS_A2_UNORM	, DXGI_FORMAT::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM	},
			{ RFormatType::FORMAT_B8G8R8A8_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_TYPELESS  			},
			{ RFormatType::FORMAT_B8G8R8A8_UNORM_SRGB 			, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB			},
			{ RFormatType::FORMAT_B8G8R8X8_TYPELESS 			, DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_TYPELESS  			},
			{ RFormatType::FORMAT_B8G8R8X8_UNORM_SRGB 			, DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB			},
			{ RFormatType::FORMAT_BC6H_TYPELESS 				, DXGI_FORMAT::DXGI_FORMAT_BC6H_TYPELESS      			},
			{ RFormatType::FORMAT_BC6H_UF16						, DXGI_FORMAT::DXGI_FORMAT_BC6H_UF16          			},
			{ RFormatType::FORMAT_BC6H_SF16						, DXGI_FORMAT::DXGI_FORMAT_BC6H_SF16          			},
			{ RFormatType::FORMAT_BC7_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC7_TYPELESS       			},
			{ RFormatType::FORMAT_BC7_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM          			},
			{ RFormatType::FORMAT_BC7_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB     			},
			{ RFormatType::FORMAT_AYUV 							, DXGI_FORMAT::DXGI_FORMAT_AYUV               			},
			{ RFormatType::FORMAT_Y410 							, DXGI_FORMAT::DXGI_FORMAT_Y410               			},
			{ RFormatType::FORMAT_Y416 							, DXGI_FORMAT::DXGI_FORMAT_Y416               			},
			{ RFormatType::FORMAT_NV12 							, DXGI_FORMAT::DXGI_FORMAT_NV12               			},
			{ RFormatType::FORMAT_P010 							, DXGI_FORMAT::DXGI_FORMAT_P010               			},
			{ RFormatType::FORMAT_P016 							, DXGI_FORMAT::DXGI_FORMAT_P016               			},
			{ RFormatType::FORMAT_420_OPAQUE 					, DXGI_FORMAT::DXGI_FORMAT_420_OPAQUE         			},
			{ RFormatType::FORMAT_YUY2 							, DXGI_FORMAT::DXGI_FORMAT_YUY2               			},
			{ RFormatType::FORMAT_Y210 							, DXGI_FORMAT::DXGI_FORMAT_Y210               			},
			{ RFormatType::FORMAT_Y216 							, DXGI_FORMAT::DXGI_FORMAT_Y216               			},
			{ RFormatType::FORMAT_NV11 							, DXGI_FORMAT::DXGI_FORMAT_NV11               			},
			{ RFormatType::FORMAT_AI44 							, DXGI_FORMAT::DXGI_FORMAT_AI44               			},
			{ RFormatType::FORMAT_IA44 							, DXGI_FORMAT::DXGI_FORMAT_IA44               			},
			{ RFormatType::FORMAT_P8							, DXGI_FORMAT::DXGI_FORMAT_P8                 			},
			{ RFormatType::FORMAT_A8P8 							, DXGI_FORMAT::DXGI_FORMAT_A8P8               			},
			{ RFormatType::FORMAT_B4G4R4A4_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B4G4R4A4_UNORM     			},
			{ RFormatType::FORMAT_P208							, DXGI_FORMAT::DXGI_FORMAT_P208							},
			{ RFormatType::FORMAT_V208							, DXGI_FORMAT::DXGI_FORMAT_V208							},
			{ RFormatType::FORMAT_V408							, DXGI_FORMAT::DXGI_FORMAT_V408							},
			{ RFormatType::FORMAT_FORCE_UINT					, DXGI_FORMAT::DXGI_FORMAT_FORCE_UINT					}
		};
		output = formatMap[input];
	}
	void RDeviceD3D11::TranslateUAVFlag(UINT32& output, RUAVFlagType input)
	{
		static RUAVFlagType UAVFlagTypes[] =
		{
			RUAVFlagType::UAV_FLAG_RAW,
			RUAVFlagType::UAV_FLAG_APPEND,
			RUAVFlagType::UAV_FLAG_COUNTER
		};
		static UINT32 d3dUAVFlagTypes[] =
		{
			D3D11_BUFFER_UAV_FLAG::D3D11_BUFFER_UAV_FLAG_RAW,
			D3D11_BUFFER_UAV_FLAG::D3D11_BUFFER_UAV_FLAG_APPEND,
			D3D11_BUFFER_UAV_FLAG::D3D11_BUFFER_UAV_FLAG_COUNTER
		};
		output = 0u;
		if (input == RUAVFlagType::UAV_FLAG_NONE)
		{
			return;
		}
		for (UINT32 i = 0u; i < 2u; i++)
		{
			if ((input & UAVFlagTypes[i]) != 0)
			{
				output |= d3dUAVFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateComparisonFunction(D3D11_COMPARISON_FUNC& output, RComparisonFunctionType input)
	{
		static TMap<RComparisonFunctionType, D3D11_COMPARISON_FUNC> comparisonFuncMap =
		{
			{ RComparisonFunctionType::COMPARISON_NEVER			, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER },
			{ RComparisonFunctionType::COMPARISON_LESS			, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS },
			{ RComparisonFunctionType::COMPARISON_EQUAL			, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_EQUAL },
			{ RComparisonFunctionType::COMPARISON_LESS_EQUAL	, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL },
			{ RComparisonFunctionType::COMPARISON_GREATER		, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_GREATER },
			{ RComparisonFunctionType::COMPARISON_NOT_EQUAL		, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NOT_EQUAL },
			{ RComparisonFunctionType::COMPARISON_GREATER_EQUAL	, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_GREATER_EQUAL },
			{ RComparisonFunctionType::COMPARISON_ALWAYS		, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS }
		};
		output = comparisonFuncMap[input];
	}
	void RDeviceD3D11::TranslateStencilStateType(D3D11_DEPTH_STENCILOP_DESC& output, const RStencilStateType& input)
	{
		static TMap<RStencilOperationType, D3D11_STENCIL_OP> stencilOpMap =
		{
			{ RStencilOperationType::STENCIL_OP_KEEP		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP },
			{ RStencilOperationType::STENCIL_OP_ZERO		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_ZERO },
			{ RStencilOperationType::STENCIL_OP_REPLACE		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_REPLACE },
			{ RStencilOperationType::STENCIL_OP_INCR_SAT	, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INCR_SAT },
			{ RStencilOperationType::STENCIL_OP_DECR_SAT	, D3D11_STENCIL_OP::D3D11_STENCIL_OP_DECR_SAT },
			{ RStencilOperationType::STENCIL_OP_INVERT		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INVERT },
			{ RStencilOperationType::STENCIL_OP_INCR		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INCR },
			{ RStencilOperationType::STENCIL_OP_DECR		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_DECR }
		};
		output.StencilFailOp = stencilOpMap[input.StencilFailOp];
		output.StencilDepthFailOp = stencilOpMap[input.StencilDepthFailOp];
		output.StencilPassOp = stencilOpMap[input.StencilPassOp];
		RDeviceD3D11::TranslateComparisonFunction(output.StencilFunc, input.StencilFunc);
	}
	void RDeviceD3D11::TranslateDepthStencilState(D3D11_DEPTH_STENCIL_DESC& output, const RDepthState& depthInput, const RStencilState* stencilInput)
	{
		static TMap<RDepthWriteMaskType, D3D11_DEPTH_WRITE_MASK> depthWriteMap =
		{
			{ RDepthWriteMaskType::DEPTH_WRITE_MASK_ZERO	, D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ZERO },
			{ RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL		, D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL }
		};
		output.DepthEnable = depthInput.DepthEnable;
		output.DepthWriteMask = depthWriteMap[depthInput.DepthWriteMask];
		RDeviceD3D11::TranslateComparisonFunction(output.DepthFunc, depthInput.DepthFunc);
		output.StencilEnable = FALSE;
		if (stencilInput)
		{
			output.StencilEnable = TRUE;
			output.StencilReadMask = static_cast<UINT8>(stencilInput->StencilReadMask & 0xffu);
			output.StencilWriteMask = static_cast<UINT8>(stencilInput->StencilWriteMask & 0xffu);
			RDeviceD3D11::TranslateStencilStateType(output.FrontFace, stencilInput->FrontFace);
			RDeviceD3D11::TranslateStencilStateType(output.BackFace, stencilInput->BackFace);
		}
	}
	void RDeviceD3D11::TranslateBlendState(D3D11_RENDER_TARGET_BLEND_DESC& output, const RBlendState& input)
	{
		static TMap<RBlendOptionType, D3D11_BLEND> blendMap =
		{
			{ RBlendOptionType::BLEND_ZERO				, D3D11_BLEND::D3D11_BLEND_ZERO },
			{ RBlendOptionType::BLEND_ONE				, D3D11_BLEND::D3D11_BLEND_ONE },
			{ RBlendOptionType::BLEND_SRC_COLOR			, D3D11_BLEND::D3D11_BLEND_SRC_COLOR },
			{ RBlendOptionType::BLEND_INV_SRC_COLOR		, D3D11_BLEND::D3D11_BLEND_INV_SRC_COLOR },
			{ RBlendOptionType::BLEND_SRC_ALPHA			, D3D11_BLEND::D3D11_BLEND_SRC_ALPHA },
			{ RBlendOptionType::BLEND_INV_SRC_ALPHA		, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA },
			{ RBlendOptionType::BLEND_DEST_ALPHA		, D3D11_BLEND::D3D11_BLEND_DEST_ALPHA },
			{ RBlendOptionType::BLEND_INV_DEST_ALPHA	, D3D11_BLEND::D3D11_BLEND_INV_DEST_ALPHA },
			{ RBlendOptionType::BLEND_DEST_COLOR		, D3D11_BLEND::D3D11_BLEND_DEST_COLOR },
			{ RBlendOptionType::BLEND_INV_DEST_COLOR	, D3D11_BLEND::D3D11_BLEND_INV_DEST_COLOR },
			{ RBlendOptionType::BLEND_SRC_ALPHA_SAT		, D3D11_BLEND::D3D11_BLEND_SRC_ALPHA_SAT },
			{ RBlendOptionType::BLEND_BLEND_FACTOR		, D3D11_BLEND::D3D11_BLEND_BLEND_FACTOR },
			{ RBlendOptionType::BLEND_INV_BLEND_FACTOR	, D3D11_BLEND::D3D11_BLEND_INV_BLEND_FACTOR }
		};
		static TMap<RBlendOperationType, D3D11_BLEND_OP> blendOpMap =
		{
			{ RBlendOperationType::BLEND_OP_ADD				, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD },
			{ RBlendOperationType::BLEND_OP_SUBTRACT		, D3D11_BLEND_OP::D3D11_BLEND_OP_SUBTRACT },
			{ RBlendOperationType::BLEND_OP_REV_SUBTRACT	, D3D11_BLEND_OP::D3D11_BLEND_OP_REV_SUBTRACT },
			{ RBlendOperationType::BLEND_OP_MIN				, D3D11_BLEND_OP::D3D11_BLEND_OP_MIN },
			{ RBlendOperationType::BLEND_OP_MAX				, D3D11_BLEND_OP::D3D11_BLEND_OP_MAX }
		};
		static RColorWriteMaskType colorWriteMaskTypes[] =
		{
			RColorWriteMaskType::COLOR_WRITE_MASK_RED,
			RColorWriteMaskType::COLOR_WRITE_MASK_GREEN,
			RColorWriteMaskType::COLOR_WRITE_MASK_BLUE,
			RColorWriteMaskType::COLOR_WRITE_MASK_ALPHA
		};
		static D3D11_COLOR_WRITE_ENABLE d3dColorWriteMaskTypes[] =
		{
			D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_RED,
			D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_GREEN,
			D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_BLUE,
			D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALPHA
		};
		output.BlendEnable = input.BlendEnable;
		output.SrcBlend = blendMap[input.SrcBlend];
		output.DestBlend = blendMap[input.DstBlend];
		output.BlendOp = blendOpMap[input.BlendOp];
		output.SrcBlendAlpha = blendMap[input.SrcBlendAlpha];
		output.DestBlendAlpha = blendMap[input.DstBlendAlpha];
		output.BlendOpAlpha = blendOpMap[input.BlendOpAlpha];
		if (input.RenderTargetWriteMask == (RColorWriteMaskType::COLOR_WRITE_MASK_ALL))
		{
			output.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;
		}
		else
		{
			output.RenderTargetWriteMask = 0u;
			for (UINT32 i = 0u; i < 4u; i++)
			{
				if ((input.RenderTargetWriteMask & colorWriteMaskTypes[i]) != 0)
				{
					output.RenderTargetWriteMask |= d3dColorWriteMaskTypes[i];
				}
			}
		}
	}
	void RDeviceD3D11::TranslateRasterizerState(D3D11_CULL_MODE& outputCull, D3D11_FILL_MODE& outputFill, const RRasterizerState& input)
	{
		static TMap<RCullModeType, D3D11_CULL_MODE> cullMap =
		{
			{ RCullModeType::CULL_NONE	, D3D11_CULL_MODE::D3D11_CULL_NONE },
			{ RCullModeType::CULL_FRONT	, D3D11_CULL_MODE::D3D11_CULL_FRONT },
			{ RCullModeType::CULL_BACK	, D3D11_CULL_MODE::D3D11_CULL_BACK }
		};
		static TMap<RFillModeType, D3D11_FILL_MODE> fillMap =
		{
			{ RFillModeType::FILL_WIREFRAME	, D3D11_FILL_MODE::D3D11_FILL_WIREFRAME },
			{ RFillModeType::FILL_SOLID		, D3D11_FILL_MODE::D3D11_FILL_SOLID }
		};
		outputCull = cullMap[input.CullMode];
		outputFill = fillMap[input.FillMode];
	}
	void RDeviceD3D11::TranslateSamplerState(D3D11_SAMPLER_DESC& output, const RSamplerState& input)
	{
		static TMap<RFilterType, D3D11_FILTER> filterMap =
		{
			{ RFilterType::FILTER_POINT			, D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_POINT },
			{ RFilterType::FILTER_LINEAR		, D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR },
			{ RFilterType::FILTER_ANISOTROPIC	, D3D11_FILTER::D3D11_FILTER_ANISOTROPIC }
		};
		static TMap<RTextureAddressModeType, D3D11_TEXTURE_ADDRESS_MODE> textureAddressModeMap =
		{
			{ RTextureAddressModeType::TEXTURE_ADDRESS_WRAP			, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP },
			{ RTextureAddressModeType::TEXTURE_ADDRESS_MIRROR		, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_MIRROR },
			{ RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP		, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP },
			{ RTextureAddressModeType::TEXTURE_ADDRESS_BORDER		, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER },
			{ RTextureAddressModeType::TEXTURE_ADDRESS_MIRROR_ONCE	, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_MIRROR_ONCE }
		};
		output.Filter = filterMap[input.Filter];
		output.AddressU = textureAddressModeMap[input.AddressU];
		output.AddressV = textureAddressModeMap[input.AddressV];
		output.AddressW = textureAddressModeMap[input.AddressW];
		output.MipLODBias = input.MipLODBias;
		output.MaxAnisotropy = input.MaxAnisotropy;
		RDeviceD3D11::TranslateComparisonFunction(output.ComparisonFunc, input.ComparisonFunc);
		output.BorderColor[0] = input.BorderColor.r;
		output.BorderColor[1] = input.BorderColor.g;
		output.BorderColor[2] = input.BorderColor.b;
		output.BorderColor[3] = input.BorderColor.a;
		output.MinLOD = input.MinLOD;
		output.MaxLOD = input.MaxLOD;
	}
	void RDeviceD3D11::TranslateClearDepthStencilFlag(UINT32& output, const UINT32& input)
	{
		static RClearDepthStencilFlagType clearDepthStencilFlagTypes[] =
		{
			RClearDepthStencilFlagType::CLEAR_DEPTH,
			RClearDepthStencilFlagType::CLEAR_STENCIL
		};
		static UINT32 d3dClearDepthStencilFlagTypes[] =
		{
			D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH,
			D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL,
		};
		output = 0u;
		if (input == RClearDepthStencilFlagType::CLEAR_NONE)
		{
			return;
		}
		for (UINT32 i = 0u; i < 2u; i++)
		{
			if ((input & clearDepthStencilFlagTypes[i]) != 0)
			{
				output |= d3dClearDepthStencilFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslatePrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY& output, RPrimitiveTopologyType input)
	{
		static TMap<RPrimitiveTopologyType, D3D_PRIMITIVE_TOPOLOGY> primitiveTopologyMap =
		{
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_UNDEFINED		, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED },
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_POINTLIST		, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST },
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINELIST		, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINELIST },
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINESTRIP		, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP },
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST	, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST },
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLESTRIP	, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP }
		};
		output = primitiveTopologyMap[input];
	}
	void RDeviceD3D11::TranslateQueryDesc(D3D11_QUERY_DESC& output, const RQueryDesc& input)
	{
		static TMap<RQueryType, D3D11_QUERY> queryTypeMap =
		{
			{ RQueryType::QUERY_EVENT							, D3D11_QUERY::D3D11_QUERY_EVENT },
			{ RQueryType::QUERY_OCCLUSION						, D3D11_QUERY::D3D11_QUERY_OCCLUSION },
			{ RQueryType::QUERY_TIMESTAMP						, D3D11_QUERY::D3D11_QUERY_TIMESTAMP },
			{ RQueryType::QUERY_TIMESTAMP_DISJOINT				, D3D11_QUERY::D3D11_QUERY_TIMESTAMP_DISJOINT },
			{ RQueryType::QUERY_PIPELINE_STATISTICS				, D3D11_QUERY::D3D11_QUERY_PIPELINE_STATISTICS },
			{ RQueryType::QUERY_OCCLUSION_PREDICATE				, D3D11_QUERY::D3D11_QUERY_OCCLUSION_PREDICATE },
			{ RQueryType::QUERY_SO_STATISTICS					, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS },
			{ RQueryType::QUERY_SO_OVERFLOW_PREDICATE			, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE },
			{ RQueryType::QUERY_SO_STATISTICS_STREAM0			, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM0 },
			{ RQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM0	, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM0 },
			{ RQueryType::QUERY_SO_STATISTICS_STREAM1			, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM1 },
			{ RQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM1	, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM1 },
			{ RQueryType::QUERY_SO_STATISTICS_STREAM2			, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM2 },
			{ RQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM2	, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM2 },
			{ RQueryType::QUERY_SO_STATISTICS_STREAM3			, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM3 },
			{ RQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM3	, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM3 }
		};
		static RQueryMiscFlagType queryMiscFlagTypes[] =
		{
			RQueryMiscFlagType::QUERY_MISC_PREDICATEHINT
		};
		static UINT32 d3dQueryMiscFlagTypes[] =
		{
			D3D11_QUERY_MISC_FLAG::D3D11_QUERY_MISC_PREDICATEHINT
		};
		output.Query = queryTypeMap[input.QueryType];
		output.MiscFlags = 0u;
		if (input.MiscFlags == RQueryMiscFlagType::QUERY_MISC_DEFAULT)
		{
			return;
		}
		for (UINT32 i = 0u; i < 1u; i++)
		{
			if ((input.MiscFlags & queryMiscFlagTypes[i]) != 0)
			{
				output.MiscFlags |= d3dQueryMiscFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateGetDataFlag(UINT32& output, RAsyncGetDataFlagType input)
	{
		static RAsyncGetDataFlagType asyncGetDataFlagTypes[] =
		{
			RAsyncGetDataFlagType::ASYNC_GETDATA_DONOTFLUSH
		};
		static UINT32 d3dAsyncGetDataFlagTypes[] =
		{
			D3D11_ASYNC_GETDATA_FLAG::D3D11_ASYNC_GETDATA_DONOTFLUSH
		};
		output = 0u;
		if (input == RAsyncGetDataFlagType::ASYNC_GETDATA_DEFAULT)
		{
			return;
		}
		for (UINT32 i = 0u; i < 1u; i++)
		{
			if ((input & asyncGetDataFlagTypes[i]) != 0)
			{
				output |= d3dAsyncGetDataFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateResourceMapType(D3D11_MAP& output, RResourceMapType input)
	{
		static TMap<RResourceMapType, D3D11_MAP> resourceMapTypes =
		{
			{ RResourceMapType::RESOURCE_MAP_READ				, D3D11_MAP::D3D11_MAP_READ },
			{ RResourceMapType::RESOURCE_MAP_WRITE				, D3D11_MAP::D3D11_MAP_WRITE },
			{ RResourceMapType::RESOURCE_MAP_READ_WRITE			, D3D11_MAP::D3D11_MAP_READ_WRITE },
			{ RResourceMapType::RESOURCE_MAP_WRITE_DISCARD		, D3D11_MAP::D3D11_MAP_WRITE_DISCARD },
			{ RResourceMapType::RESOURCE_MAP_WRITE_NO_OVERWRITE	, D3D11_MAP::D3D11_MAP_WRITE_NO_OVERWRITE }
		};
		output = resourceMapTypes[input];
	}
	void RDeviceD3D11::TranslateResourceMapFlag(UINT32& output, RResourceMapFlagType input)
	{
		static TMap<RResourceMapFlagType, UINT32> resourceMapFlagTypes =
		{
			{ RResourceMapFlagType::RESOURCE_MAP_FLAG_NONE			, 0u },
			{ RResourceMapFlagType::RESOURCE_MAP_FLAG_DO_NOT_WAIT	, D3D11_MAP_FLAG::D3D11_MAP_FLAG_DO_NOT_WAIT }
		};
		output = resourceMapFlagTypes[input];
	}
	void RDeviceD3D11::TranslateInputLayoutDesc(D3D11_INPUT_ELEMENT_DESC& output, const RInputLayoutDesc& input)
	{
		static TMap<RInputClassificationType, D3D11_INPUT_CLASSIFICATION> classificationMap =
		{
			{ RInputClassificationType::INPUT_PER_VERTEX_DATA	, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA },
			{ RInputClassificationType::INPUT_PER_INSTANCE_DATA	, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA }
		};
		::ZeroMemory(&output, sizeof(output));

		enum ESemanticIndexType : UINT8
		{
			SEMANTIC_INDEX_PSIZE		= 0,
			SEMANTIC_INDEX_POSITIONT,
			SEMANTIC_INDEX_BINORMAL,
			SEMANTIC_INDEX_BLENDWEIGHT,
			SEMANTIC_INDEX_BLENDINDICES,
			SEMANTIC_INDEX_COLOR,
			SEMANTIC_INDEX_TANGENT,
			SEMANTIC_INDEX_NORMAL,
			SEMANTIC_INDEX_POSITION,
			SEMANTIC_INDEX_TEXCOORD,
			SEMANTIC_INDEX_COUNT
		};
		static const CHAR* StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_COUNT] =
		{
			"PSIZE",
			"POSITIONT",
			"BINORMAL",
			"BLENDWEIGHT",
			"BLENDINDICES",
			"COLOR",
			"TANGENT",
			"NORMAL",
			"POSITION",
			"TEXCOORD"
		};

		auto TranslateMemberToFormat = [](const RInputLayoutDesc& InDesc)->DXGI_FORMAT
		{
			CheckSlow(((InDesc.MemberFormat >= RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT) && (InDesc.MemberFormat < RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_COUNT)), (ENGINE_RENDER_CORE_ERROR));
			CheckSlow(((InDesc.MemberStride > 0u) && (InDesc.MemberStride <= 4u) && (InDesc.MemberStride != 3u)), (ENGINE_RENDER_CORE_ERROR));
			CheckSlow(((InDesc.MemberNum > 0u) && (InDesc.MemberNum <= 4u)), (ENGINE_RENDER_CORE_ERROR));
			if (InDesc.MemberNum == 1u)
			{
				if (InDesc.MemberStride == 1u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8_SNORM;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8_UINT;
						}
					}
				}
				else if (InDesc.MemberStride == 2u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
						}
					}
				}
				else if (InDesc.MemberStride == 4u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
						}
					}
				}
			}
			else if (InDesc.MemberNum == 2u)
			{
				if (InDesc.MemberStride == 1u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8_SNORM;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8_UINT;
						}
					}
				}
				else if (InDesc.MemberStride == 2u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT;
						}
					}
				}
				else if (InDesc.MemberStride == 4u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
						}
					}
				}
			}
			else if (InDesc.MemberNum == 3u)
			{
				if (InDesc.MemberStride == 1u)
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Engine does not support [rgb8 type] this format."));
				}
				else if (InDesc.MemberStride == 2u)
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Engine does not support [rgb16 type] this format."));
				}
				else if (InDesc.MemberStride == 4u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT;
						}
					}
				}
			}
			else if (InDesc.MemberNum == 4u)
			{
				if (InDesc.MemberStride == 1u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SNORM;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
						}
					}
				}
				else if (InDesc.MemberStride == 2u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
						}
					}
				}
				else if (InDesc.MemberStride == 4u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT;
						}
					}
				}
			}
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Can not find this input layout member format."));
			return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		};

		if (input.SemanticName == RShaderSemanticType::SHADER_SEMANTIC_NONE) { return; }
		{
			EString strSemanticName; UINT32 semanticName = input.SemanticName;
			output.SemanticIndex = semanticName & 0xfu;
			output.Format = TranslateMemberToFormat(input);
			if ((semanticName >> 15) & 0x1u)
			{
				//SHADER_SEMANTIC_TEXCOORD[n]
				//strSemanticName = "TEXCOORD";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_TEXCOORD];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
			}
			else if ((semanticName >> 14) & 0x1u)
			{
				//SHADER_SEMANTIC_POSITION[n]
				//strSemanticName = "POSITION";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_POSITION];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 13) & 0x1u)
			{
				//SHADER_SEMANTIC_NORMAL[n]
				//strSemanticName = "NORMAL";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_NORMAL];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 12) & 0x1u)
			{
				//SHADER_SEMANTIC_TANGENT[n]
				//strSemanticName = "TANGENT";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_TANGENT];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 11) & 0x1u)
			{
				//SHADER_SEMANTIC_COLOR[n]
				//strSemanticName = "COLOR";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_COLOR];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 10) & 0x1u)
			{
				//SHADER_SEMANTIC_BLENDINDICES[n]
				//strSemanticName = "BLENDINDICES";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_BLENDINDICES];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
			}
			else if ((semanticName >> 9) & 0x1u)
			{
				//SHADER_SEMANTIC_BLENDWEIGHT[n]
				//strSemanticName = "BLENDWEIGHT";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_BLENDWEIGHT];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 8) & 0x1u)
			{
				//SHADER_SEMANTIC_BINORMAL[n]
				//strSemanticName = "BINORMAL";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_BINORMAL];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 7) & 0x1u)
			{
				//SHADER_SEMANTIC_POSITIONT[n]
				//strSemanticName = "POSITIONT";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_POSITIONT];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 6) & 0x1u)
			{
				//SHADER_SEMANTIC_PSIZE[n]
				//strSemanticName = "PSIZE";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_PSIZE];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
		}
		output.InputSlot = input.InputSlot;
		output.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	//0u
		output.InputSlotClass = classificationMap[input.InputSlotClass];
		output.InstanceDataStepRate = input.InstanceDataStepRate;
	}
	void RDeviceD3D11::ClearFinalOutput()
	{
		m_ImmediateContext->ClearRenderTargetView(m_RenderTargetView.Get(), Color4(0.5f, 0.5f, 0.5f, 1.f).rgba);
		m_ImmediateContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL, 1.f, 0xffu);
	}
	void RDeviceD3D11::SetFinalOutput()
	{
		m_ImmediateContext->OMSetRenderTargets(1u, m_RenderTargetView.GetAddressOf(), m_DepthStencilView.Get());
		m_ImmediateContext->RSSetViewports(1u, &(m_Viewport));
	}
 };