#include "../Headers/CRenderDevice.h"

CRenderDevice* CRenderDevice::m_RenderDevice = NULL;
void CRenderDevice::Initialize()
{
	if (CRenderDevice::m_RenderDevice == NULL)
	{
		CRenderDevice::m_RenderDevice = new CRenderDevice();
	}
}
void CRenderDevice::ShutDown()
{
	if (CRenderDevice::m_RenderDevice != NULL)
	{
		CRenderDevice::m_RenderDevice->Uninit();
		delete (CRenderDevice::m_RenderDevice);
	}
}
void CRenderDevice::Init(HWND hWnd, const CustomType::Vector2Int& bufferSize, const UINT& bufferDepth, const UINT& frameNum, const BOOL& windowed)
{
	if (bufferSize.X() < 1 || bufferSize.Y() < 1 || !(bufferDepth == 24u || bufferDepth == 32u) || frameNum < 2u)
	{
		//TODO Invalid Arguments log.
		return;
	}

	UINT bufferWidth = static_cast<UINT>(bufferSize.X());
	UINT bufferHeight = static_cast<UINT>(bufferSize.Y());
	HRESULT hr = S_FALSE;

	DXGI_SWAP_CHAIN_DESC sd;
	{
		::ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1u;
		sd.BufferDesc.Width = bufferWidth;
		sd.BufferDesc.Height = bufferHeight;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = frameNum;
		sd.BufferDesc.RefreshRate.Denominator = 1u;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hWnd;
		sd.SampleDesc.Count = 1u;
		sd.SampleDesc.Quality = 0u;
		sd.Windowed = windowed;
	}
	D3D_FEATURE_LEVEL featureLevels[4] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0 };
	hr = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		0u,
		featureLevels,
		0u,
		D3D11_SDK_VERSION,
		&sd,
		m_SwapChain.ReleaseAndGetAddressOf(),
		m_Device.ReleaseAndGetAddressOf(),
		&m_FeatureLevel,
		m_ImmediateContext.ReleaseAndGetAddressOf());	//D3D11_CREATE_DEVICE_FLAG
	if (FAILED(hr) || !(m_FeatureLevel == D3D_FEATURE_LEVEL_11_1 || m_FeatureLevel == D3D_FEATURE_LEVEL_11_0))
	{
		//TODO Create d3d device failed log.
		return;
	}

	m_Viewport.Width = static_cast<FLOAT>(bufferWidth);
	m_Viewport.Height = static_cast<FLOAT>(bufferHeight);
	m_Viewport.MinDepth = 0.f;
	m_Viewport.MaxDepth = 1.f;
	m_Viewport.TopLeftX = 0.f;
	m_Viewport.TopLeftY = 0.f;

	ID3D11Texture2D* pBackBuffer = NULL;
	hr = m_SwapChain->GetBuffer(0u, __uuidof(ID3D11Texture2D), (LPVOID*)(&pBackBuffer));
	if (FAILED(hr))
	{
		//TODO Get buffer failed from swap chain log.
		return;
	}
	hr = m_Device->CreateRenderTargetView(pBackBuffer, NULL, m_RenderTargetView.ReleaseAndGetAddressOf());
	pBackBuffer->Release();
	if (FAILED(hr))
	{
		//TODO Create buffer RTV failed from swap chain log.
		return;
	}

	{
		ID3D11Texture2D* depthBuffer = NULL;
		D3D11_TEXTURE2D_DESC td;
		{
			::ZeroMemory(&td, sizeof(td));
			td.Width = sd.BufferDesc.Width;
			td.Height = sd.BufferDesc.Height;
			td.MipLevels = 1u;
			td.ArraySize = 1u;
			if (bufferDepth == 24u)
			{
				td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			}
			else if (bufferDepth == 32u)
			{
				td.Format = DXGI_FORMAT_D32_FLOAT;
			}
			td.SampleDesc = sd.SampleDesc;
			td.Usage = D3D11_USAGE_DEFAULT;
			td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			td.CPUAccessFlags = 0u;
			td.MiscFlags = 0u;
		}
		hr = m_Device->CreateTexture2D(&td, NULL, &depthBuffer);
		if (FAILED(hr))
		{
			//TODO Create depth buffer failed log.
			return;
		}
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
		::ZeroMemory(&dsvd, sizeof(dsvd));
		dsvd.Format = td.Format;
		dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvd.Flags = 0u;
		hr = m_Device->CreateDepthStencilView(depthBuffer, &dsvd, m_DepthStencilView.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			//TODO Create depth stencil view failed log.
			return;
		}
	}

	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_WHITE]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_WHITE);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_BLACK]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_BLACK);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_GRAY]		= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_GRAY);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_RED]		= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_RED);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_GREEN]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_GREEN);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_BLUE]		= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_BLUE);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_BUMP]		= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_BUMP);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_PROPERTY]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_PROPERTY);
}
void CRenderDevice::Uninit()
{
	if (m_RenderTargetView)
	{
		m_RenderTargetView->Release();
		m_RenderTargetView = nullptr;
	}
	if (m_DepthStencilView)
	{
		m_DepthStencilView->Release();
		m_DepthStencilView = nullptr;
	}
	if (m_SwapChain)
	{
		m_SwapChain->Release();
		m_SwapChain = nullptr;
	}
	if (m_ImmediateContext)
	{
		m_ImmediateContext->Release();
		m_ImmediateContext = nullptr;
	}
	if (m_Device)
	{
		m_Device->Release();
		m_Device = nullptr;
	}
}
BOOL CRenderDevice::LoadVertexShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout, const std::vector<D3D11_INPUT_ELEMENT_DESC>* layout)
{
	FILE* file;
	LONG fsize;
	BYTE* buffer;
	{
		fopen_s(&file, name.c_str(), "rb");
		if (file == NULL)
		{
			//TODO Can not read file log.
			return FALSE;
		}
		fsize = _filelength(_fileno(file));
		buffer = new BYTE[fsize];
		fread_s(buffer, fsize, fsize, 1u, file);
		fclose(file);
	}

	{
		HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateVertexShader(static_cast<void*>(buffer), fsize, NULL, vertexShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			//TODO Create vertex shader object failed log.
			return FALSE;
		}
	}

	{
		UINT numElements = 5u;
		std::vector<D3D11_INPUT_ELEMENT_DESC> tempLayout;
		if (layout == NULL)
		{
			tempLayout.resize(5u);
			tempLayout[0] = { "POSITION", 0u, DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0u };
			tempLayout[1] = { "NORMAL",   0u, DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0u };
			tempLayout[2] = { "TANGENT",  0u, DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0u };
			tempLayout[3] = { "COLOR",    0u, DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0u };
			tempLayout[4] = { "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT,       0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0u };
		}
		else
		{
			numElements = static_cast<UINT>(layout->size());
			tempLayout.resize(layout->size());
			for (UINT i = 0u; i < numElements; i++)
				tempLayout[i] = (*layout)[i];
		}
		
		HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateInputLayout(tempLayout.data(), numElements, static_cast<void*>(buffer), fsize, inputLayout.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			//TODO Create input layout object failed log.
			return FALSE;
		}
	}

	delete[]buffer;
	return TRUE;
}
BOOL CRenderDevice::LoadPixelShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader)
{
	FILE* file;
	LONG fsize;
	BYTE* buffer;
	{
		fopen_s(&file, name.c_str(), "rb");
		if (file == NULL)
		{
			//TODO Can not read file log.
			return FALSE;
		}
		fsize = _filelength(_fileno(file));
		buffer = new BYTE[fsize];
		fread_s(buffer, fsize, fsize, 1u, file);
		fclose(file);
	}

	{
		HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreatePixelShader(static_cast<void*>(buffer), fsize, NULL, pixelShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			//TODO Create pixel shader object failed log.
			return FALSE;
		}
	}

	delete[] buffer;
	return TRUE;
}
BOOL CRenderDevice::LoadComputeShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11ComputeShader>& computeShader)
{
	FILE* file;
	LONG fsize;
	BYTE* buffer;
	{
		fopen_s(&file, name.c_str(), "rb");
		if (file == NULL)
		{
			//TODO Can not read file log.
			return FALSE;
		}
		fsize = _filelength(_fileno(file));
		buffer = new BYTE[fsize];
		fread_s(buffer, fsize, fsize, 1u, file);
		fclose(file);
	}

	{
		HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateComputeShader(static_cast<void*>(buffer), fsize, NULL, computeShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			//TODO Create compute shader object failed log.
			return FALSE;
		}
	}

	delete[] buffer;
	return TRUE;
}
BOOL CRenderDevice::CreateBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const CustomStruct::CRenderBufferDesc& bufferDesc, const CustomStruct::CRenderSubresourceData* subData)
{
	D3D11_BUFFER_DESC bd;
	{
		::ZeroMemory(&bd, sizeof(bd));
		bd.ByteWidth = bufferDesc.ByteWidth;
		CRenderDevice::TranslateUsage(bd.Usage, bufferDesc.Usage);
		CRenderDevice::TranslateBindFlag(bd.BindFlags, bufferDesc.BindFlags);
		CRenderDevice::TranslateCPUAccessFlag(bd.CPUAccessFlags, bufferDesc.CPUAccessFlags);
		CRenderDevice::TranslateResourceMiscFlag(bd.MiscFlags, bufferDesc.MiscFlags);
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
		hr = CRenderDevice::m_RenderDevice->m_Device->CreateBuffer(&bd, &sd, buffer.ReleaseAndGetAddressOf());
	}
	else
	{
		hr = CRenderDevice::m_RenderDevice->m_Device->CreateBuffer(&bd, NULL, buffer.ReleaseAndGetAddressOf());
	}
	if (FAILED(hr))
	{
		//TODO Create buffer object failed log.
		return FALSE;
	}
	return TRUE;
}
void CRenderDevice::UploadBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& dstResource, const void* srcData, UINT srcRowPitch, UINT srcDepthPitch, UINT dstSubresource, const D3D11_BOX* dstBox)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->UpdateSubresource(dstResource.Get(), dstSubresource, dstBox, srcData, srcRowPitch, srcDepthPitch);
}
void CRenderDevice::Present(const UINT& syncInterval = 0u)
{
	HRESULT hr = CRenderDevice::m_RenderDevice->m_SwapChain->Present(syncInterval, 0u);	//DXGI_PRESENT
}
void CRenderDevice::SetDepthStencilState(const Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const UINT& stencilRef)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetDepthStencilState(dss.Get(), stencilRef);
}
void CRenderDevice::SetBlendState(const Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const CustomStruct::CColor& blendFactor, const UINT& sampleMask)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetBlendState(bs.Get(), blendFactor.rgba, sampleMask);
}
void CRenderDevice::SetRasterizerState(const Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->RSSetState(rs.Get());
}
void CRenderDevice::BindVSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->VSSetSamplers(startSlot, 1u, ss.GetAddressOf());
}
void CRenderDevice::BindPSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->PSSetSamplers(startSlot, 1u, ss.GetAddressOf());
}
void CRenderDevice::BindCSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetSamplers(startSlot, 1u, ss.GetAddressOf());
}
BOOL CRenderDevice::CreateDepthStencilState(Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const CustomStruct::CRenderDepthState& depthState, const CustomStruct::CRenderStencilState* stencilState)
{
	D3D11_DEPTH_STENCIL_DESC dsd;
	{
		::ZeroMemory(&dsd, sizeof(dsd));
		CRenderDevice::TranslateDepthStencilState(dsd, depthState, stencilState);
	}
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateDepthStencilState(&dsd, dss.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		//TODO Create depth stencil state object failed log.
		return FALSE;
	}
	return TRUE;
}
BOOL CRenderDevice::CreateBlendState(Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const std::vector<CustomStruct::CRenderBlendState>& blendStates)
{
	if (blendStates.size() < 1)
	{
		//TODO Blend state info error log.
		return FALSE;
	}
	D3D11_BLEND_DESC bd;
	{
		::ZeroMemory(&bd, sizeof(bd));
		bd.AlphaToCoverageEnable = FALSE;
		bd.IndependentBlendEnable = FALSE;
		if (blendStates.size() > 1)
		{
			bd.IndependentBlendEnable = TRUE;
		}
		for (INT i = 0; i < static_cast<INT>(blendStates.size()) && i < 8; i++)
		{
			CRenderDevice::TranslateBlendState(bd.RenderTarget[i], blendStates[i]);
		}
	}
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateBlendState(&bd, bs.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		//TODO Create blend state object failed log.
		return FALSE;
	}
	return TRUE;
}
BOOL CRenderDevice::CreateRasterizerState(Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs, const CustomStruct::CRenderRasterizerState& rasterizerState)
{
	D3D11_RASTERIZER_DESC rd;
	{
		::ZeroMemory(&rd, sizeof(rd));
		CRenderDevice::TranslateRasterizerState(rd.CullMode, rd.FillMode, rasterizerState);
		rd.FrontCounterClockwise = FALSE;
		rd.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
		rd.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
		rd.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rd.DepthClipEnable = TRUE;
		rd.ScissorEnable = FALSE;
		rd.MultisampleEnable = FALSE;
		rd.AntialiasedLineEnable = FALSE;
	}
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateRasterizerState(&rd, rs.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		//TODO Create rasterizer state object failed log.
		return FALSE;
	}
	return TRUE;
}
BOOL CRenderDevice::CreateSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const CustomStruct::CRenderSamplerState& samplerState)
{
	D3D11_SAMPLER_DESC sd;
	{
		::ZeroMemory(&sd, sizeof(sd));
		CRenderDevice::TranslateSamplerState(sd, samplerState);
	}
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateSamplerState(&sd, ss.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		//TODO Create sampler state object failed log.
		return FALSE;
	}
	return TRUE;
}
D3D11_VIEWPORT CRenderDevice::GetViewport()
{
	return (CRenderDevice::m_RenderDevice->m_Viewport);
}
void CRenderDevice::TranslateBindFlag(UINT& output, CustomStruct::CRenderBindFlag input)
{
	static std::map<CustomStruct::CRenderBindFlag, D3D11_BIND_FLAG> bindFlagMap = {
		{ CustomStruct::CRenderBindFlag::BIND_VERTEX_BUFFER, D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER },
		{ CustomStruct::CRenderBindFlag::BIND_INDEX_BUFFER, D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER },
		{ CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER },
		{ CustomStruct::CRenderBindFlag::BIND_SHADER_RESOURCE, D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE },
		{ CustomStruct::CRenderBindFlag::BIND_RENDER_TARGET, D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET },
		{ CustomStruct::CRenderBindFlag::BIND_DEPTH_STENCIL, D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL },
		{ CustomStruct::CRenderBindFlag::BIND_UNORDERED_ACCESS, D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS } };

	if (input == CustomStruct::CRenderBindFlag::BIND_NONE)
	{
		output = 0u;
	}
	else
	{
		output = bindFlagMap[input];
	}
}
void CRenderDevice::TranslateUsage(D3D11_USAGE& output, CustomStruct::CRenderUsage input)
{
	static std::map<CustomStruct::CRenderUsage, D3D11_USAGE> usageMap = {
		{ CustomStruct::CRenderUsage::USAGE_DEFAULT, D3D11_USAGE::D3D11_USAGE_DEFAULT },
		{ CustomStruct::CRenderUsage::USAGE_IMMUTABLE, D3D11_USAGE::D3D11_USAGE_IMMUTABLE },
		{ CustomStruct::CRenderUsage::USAGE_DYNAMIC, D3D11_USAGE::D3D11_USAGE_DYNAMIC },
		{ CustomStruct::CRenderUsage::USAGE_STAGING, D3D11_USAGE::D3D11_USAGE_STAGING } };

	output = usageMap[input];
}
void CRenderDevice::TranslateCPUAccessFlag(UINT& output, CustomStruct::CRenderCPUAccessFlag input)
{
	static std::map<CustomStruct::CRenderCPUAccessFlag, D3D11_CPU_ACCESS_FLAG> cpuAccessFlagMap = {
		{ CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_WRITE, D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE },
		{ CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_READ, D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ } };

	if (input == CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_NONE)
	{
		output = 0u;
	}
	else
	{
		output = cpuAccessFlagMap[input];
	}
}
void CRenderDevice::TranslateResourceMiscFlag(UINT& output, CustomStruct::CRenderResourceMiscFlag input)
{
	static std::map<CustomStruct::CRenderResourceMiscFlag, D3D11_RESOURCE_MISC_FLAG> resourceMiscFlagMap = {
		{ CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_GENERATE_MIPS, D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_GENERATE_MIPS },
		{ CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_TEXTURECUBE, D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_TEXTURECUBE },
		{ CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_DRAWINDIRECT_ARGS, D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS },
		{ CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED },
		{ CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_RESOURCE_CLAMP, D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_RESOURCE_CLAMP } };

	if (input == CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_NONE)
	{
		output = 0u;
	}
	else
	{
		output = resourceMiscFlagMap[input];
	}
}
void CRenderDevice::TranslateResourceFormat(DXGI_FORMAT& output, CustomStruct::CRenderFormat input)
{
	static std::map<CustomStruct::CRenderFormat, DXGI_FORMAT> formatMap = {
		{ CustomStruct::CRenderFormat::FORMAT_UNKNOWN						, DXGI_FORMAT::DXGI_FORMAT_UNKNOWN						},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32A32_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_TYPELESS		},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32A32_FLOAT			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT			},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32A32_UINT 			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT			},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32A32_SINT 			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT			},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_TYPELESS			},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32_FLOAT				, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT				},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32_UINT				, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT				},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32_SINT				, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT				},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_TYPELESS		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_FLOAT			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT			},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_UNORM			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UNORM			},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_UINT				, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT			},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_SNORM			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SNORM			},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_SINT				, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SINT			},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_R32G32_TYPELESS				},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32_FLOAT					, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT					},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32_UINT					, DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT					},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32_SINT					, DXGI_FORMAT::DXGI_FORMAT_R32G32_SINT					},
		{ CustomStruct::CRenderFormat::FORMAT_R32G8X24_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS			},
		{ CustomStruct::CRenderFormat::FORMAT_D32_FLOAT_S8X24_UINT			, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT			},
		{ CustomStruct::CRenderFormat::FORMAT_R32_FLOAT_X8X24_TYPELESS		, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS		},
		{ CustomStruct::CRenderFormat::FORMAT_X32_TYPELESS_G8X24_UINT		, DXGI_FORMAT::DXGI_FORMAT_X32_TYPELESS_G8X24_UINT		},
		{ CustomStruct::CRenderFormat::FORMAT_R10G10B10A2_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_TYPELESS			},
		{ CustomStruct::CRenderFormat::FORMAT_R10G10B10A2_UNORM				, DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM     		},
		{ CustomStruct::CRenderFormat::FORMAT_R10G10B10A2_UINT				, DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UINT      		},
		{ CustomStruct::CRenderFormat::FORMAT_R11G11B10_FLOAT				, DXGI_FORMAT::DXGI_FORMAT_R11G11B10_FLOAT       		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_TYPELESS     		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM        		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_UNORM_SRGB			, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB   		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_UINT					, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT         		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_SNORM				, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SNORM        		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_SINT					, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SINT         		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_R16G16_TYPELESS       		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_FLOAT					, DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT          		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_UNORM					, DXGI_FORMAT::DXGI_FORMAT_R16G16_UNORM          		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_UINT 					, DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT           		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_SNORM					, DXGI_FORMAT::DXGI_FORMAT_R16G16_SNORM          		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_SINT 					, DXGI_FORMAT::DXGI_FORMAT_R16G16_SINT           		},
		{ CustomStruct::CRenderFormat::FORMAT_R32_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS          		},
		{ CustomStruct::CRenderFormat::FORMAT_D32_FLOAT						, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT             		},
		{ CustomStruct::CRenderFormat::FORMAT_R32_FLOAT						, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT             		},
		{ CustomStruct::CRenderFormat::FORMAT_R32_UINT 						, DXGI_FORMAT::DXGI_FORMAT_R32_UINT              		},
		{ CustomStruct::CRenderFormat::FORMAT_R32_SINT 						, DXGI_FORMAT::DXGI_FORMAT_R32_SINT              		},
		{ CustomStruct::CRenderFormat::FORMAT_R24G8_TYPELESS 				, DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS        		},
		{ CustomStruct::CRenderFormat::FORMAT_D24_UNORM_S8_UINT				, DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT     		},
		{ CustomStruct::CRenderFormat::FORMAT_R24_UNORM_X8_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS 		},
		{ CustomStruct::CRenderFormat::FORMAT_X24_TYPELESS_G8_UINT			, DXGI_FORMAT::DXGI_FORMAT_X24_TYPELESS_G8_UINT  		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_TYPELESS 				, DXGI_FORMAT::DXGI_FORMAT_R8G8_TYPELESS     			},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_UNORM					, DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM        			},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_UINT 					, DXGI_FORMAT::DXGI_FORMAT_R8G8_UINT         			},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_SNORM					, DXGI_FORMAT::DXGI_FORMAT_R8G8_SNORM        			},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_SINT 					, DXGI_FORMAT::DXGI_FORMAT_R8G8_SINT         			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS      			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_FLOAT						, DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT         			},
		{ CustomStruct::CRenderFormat::FORMAT_D16_UNORM						, DXGI_FORMAT::DXGI_FORMAT_D16_UNORM         			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_UNORM						, DXGI_FORMAT::DXGI_FORMAT_R16_UNORM         			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_UINT 						, DXGI_FORMAT::DXGI_FORMAT_R16_UINT          			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_SNORM						, DXGI_FORMAT::DXGI_FORMAT_R16_SNORM         			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_SINT 						, DXGI_FORMAT::DXGI_FORMAT_R16_SINT          			},
		{ CustomStruct::CRenderFormat::FORMAT_R8_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_R8_TYPELESS       			},
		{ CustomStruct::CRenderFormat::FORMAT_R8_UNORM						, DXGI_FORMAT::DXGI_FORMAT_R8_UNORM          			},
		{ CustomStruct::CRenderFormat::FORMAT_R8_UINT 						, DXGI_FORMAT::DXGI_FORMAT_R8_UINT           			},
		{ CustomStruct::CRenderFormat::FORMAT_R8_SNORM						, DXGI_FORMAT::DXGI_FORMAT_R8_SNORM          			},
		{ CustomStruct::CRenderFormat::FORMAT_R8_SINT 						, DXGI_FORMAT::DXGI_FORMAT_R8_SINT           			},
		{ CustomStruct::CRenderFormat::FORMAT_A8_UNORM						, DXGI_FORMAT::DXGI_FORMAT_A8_UNORM          			},
		{ CustomStruct::CRenderFormat::FORMAT_R1_UNORM						, DXGI_FORMAT::DXGI_FORMAT_R1_UNORM          			},
		{ CustomStruct::CRenderFormat::FORMAT_R9G9B9E5_SHAREDEXP			, DXGI_FORMAT::DXGI_FORMAT_R9G9B9E5_SHAREDEXP			},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_B8G8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_R8G8_B8G8_UNORM				},
		{ CustomStruct::CRenderFormat::FORMAT_G8R8_G8B8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_G8R8_G8B8_UNORM				},
		{ CustomStruct::CRenderFormat::FORMAT_BC1_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS					},
		{ CustomStruct::CRenderFormat::FORMAT_BC1_UNORM						, DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC1_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB				},
		{ CustomStruct::CRenderFormat::FORMAT_BC2_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_BC2_TYPELESS					},
		{ CustomStruct::CRenderFormat::FORMAT_BC2_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC2_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB				},
		{ CustomStruct::CRenderFormat::FORMAT_BC3_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS					},
		{ CustomStruct::CRenderFormat::FORMAT_BC3_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC3_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB				},
		{ CustomStruct::CRenderFormat::FORMAT_BC4_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS					},
		{ CustomStruct::CRenderFormat::FORMAT_BC4_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC4_SNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC5_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS					},
		{ CustomStruct::CRenderFormat::FORMAT_BC5_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC5_SNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_B5G6R5_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_B5G6R5_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_B5G5R5A1_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B5G5R5A1_UNORM				},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8A8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM				},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8X8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM				},
		{ CustomStruct::CRenderFormat::FORMAT_R10G10B10_XR_BIAS_A2_UNORM	, DXGI_FORMAT::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM	},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8A8_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_TYPELESS  			},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8A8_UNORM_SRGB 			, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB			},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8X8_TYPELESS 			, DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_TYPELESS  			},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8X8_UNORM_SRGB 			, DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB			},
		{ CustomStruct::CRenderFormat::FORMAT_BC6H_TYPELESS 				, DXGI_FORMAT::DXGI_FORMAT_BC6H_TYPELESS      			},
		{ CustomStruct::CRenderFormat::FORMAT_BC6H_UF16						, DXGI_FORMAT::DXGI_FORMAT_BC6H_UF16          			},
		{ CustomStruct::CRenderFormat::FORMAT_BC6H_SF16						, DXGI_FORMAT::DXGI_FORMAT_BC6H_SF16          			},
		{ CustomStruct::CRenderFormat::FORMAT_BC7_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC7_TYPELESS       			},
		{ CustomStruct::CRenderFormat::FORMAT_BC7_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM          			},
		{ CustomStruct::CRenderFormat::FORMAT_BC7_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB     			},
		{ CustomStruct::CRenderFormat::FORMAT_AYUV 							, DXGI_FORMAT::DXGI_FORMAT_AYUV               			},
		{ CustomStruct::CRenderFormat::FORMAT_Y410 							, DXGI_FORMAT::DXGI_FORMAT_Y410               			},
		{ CustomStruct::CRenderFormat::FORMAT_Y416 							, DXGI_FORMAT::DXGI_FORMAT_Y416               			},
		{ CustomStruct::CRenderFormat::FORMAT_NV12 							, DXGI_FORMAT::DXGI_FORMAT_NV12               			},
		{ CustomStruct::CRenderFormat::FORMAT_P010 							, DXGI_FORMAT::DXGI_FORMAT_P010               			},
		{ CustomStruct::CRenderFormat::FORMAT_P016 							, DXGI_FORMAT::DXGI_FORMAT_P016               			},
		{ CustomStruct::CRenderFormat::FORMAT_420_OPAQUE 					, DXGI_FORMAT::DXGI_FORMAT_420_OPAQUE         			},
		{ CustomStruct::CRenderFormat::FORMAT_YUY2 							, DXGI_FORMAT::DXGI_FORMAT_YUY2               			},
		{ CustomStruct::CRenderFormat::FORMAT_Y210 							, DXGI_FORMAT::DXGI_FORMAT_Y210               			},
		{ CustomStruct::CRenderFormat::FORMAT_Y216 							, DXGI_FORMAT::DXGI_FORMAT_Y216               			},
		{ CustomStruct::CRenderFormat::FORMAT_NV11 							, DXGI_FORMAT::DXGI_FORMAT_NV11               			},
		{ CustomStruct::CRenderFormat::FORMAT_AI44 							, DXGI_FORMAT::DXGI_FORMAT_AI44               			},
		{ CustomStruct::CRenderFormat::FORMAT_IA44 							, DXGI_FORMAT::DXGI_FORMAT_IA44               			},
		{ CustomStruct::CRenderFormat::FORMAT_P8							, DXGI_FORMAT::DXGI_FORMAT_P8                 			},
		{ CustomStruct::CRenderFormat::FORMAT_A8P8 							, DXGI_FORMAT::DXGI_FORMAT_A8P8               			},
		{ CustomStruct::CRenderFormat::FORMAT_B4G4R4A4_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B4G4R4A4_UNORM     			},
		{ CustomStruct::CRenderFormat::FORMAT_P208							, DXGI_FORMAT::DXGI_FORMAT_P208							},
		{ CustomStruct::CRenderFormat::FORMAT_V208							, DXGI_FORMAT::DXGI_FORMAT_V208							},
		{ CustomStruct::CRenderFormat::FORMAT_V408							, DXGI_FORMAT::DXGI_FORMAT_V408							},
		{ CustomStruct::CRenderFormat::FORMAT_FORCE_UINT					, DXGI_FORMAT::DXGI_FORMAT_FORCE_UINT }};

		output = formatMap[input];
}
void CRenderDevice::TranslateComparisonFunction(D3D11_COMPARISON_FUNC& output, CustomStruct::CRenderComparisonFunction input)
{
	static std::map<CustomStruct::CRenderComparisonFunction, D3D11_COMPARISON_FUNC> comparisonFuncMap = {
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_NEVER, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_LESS, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_EQUAL, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_EQUAL },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_LESS_EQUAL, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_GREATER, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_GREATER },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_NOT_EQUAL, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NOT_EQUAL },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_GREATER_EQUAL, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_GREATER_EQUAL },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_ALWAYS, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS } };

	output = comparisonFuncMap[input];
}
void CRenderDevice::TranslateStencilOperation(D3D11_DEPTH_STENCILOP_DESC& output, const CustomStruct::CRenderStencilOp& input)
{
	static std::map<CustomStruct::CRenderStencilOperation, D3D11_STENCIL_OP> stencilOpMap = {
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_KEEP, D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_ZERO, D3D11_STENCIL_OP::D3D11_STENCIL_OP_ZERO },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_REPLACE, D3D11_STENCIL_OP::D3D11_STENCIL_OP_REPLACE },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_INCR_SAT, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INCR_SAT },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_DECR_SAT, D3D11_STENCIL_OP::D3D11_STENCIL_OP_DECR_SAT },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_INVERT, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INVERT },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_INCR, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INCR },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_DECR, D3D11_STENCIL_OP::D3D11_STENCIL_OP_DECR } };

	output.StencilFailOp = stencilOpMap[input.StencilFailOp];
	output.StencilDepthFailOp = stencilOpMap[input.StencilDepthFailOp];
	output.StencilPassOp = stencilOpMap[input.StencilPassOp];
	TranslateComparisonFunction(output.StencilFunc, input.StencilFunc);
}
void CRenderDevice::TranslateDepthStencilState(D3D11_DEPTH_STENCIL_DESC& output, const CustomStruct::CRenderDepthState& depthInput, const CustomStruct::CRenderStencilState* stencilInput)
{
	static std::map<CustomStruct::CRenderDepthWriteMask, D3D11_DEPTH_WRITE_MASK> depthWriteMap = {
		{ CustomStruct::CRenderDepthWriteMask::DEPTH_WRITE_MASK_ZERO, D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ZERO },
		{ CustomStruct::CRenderDepthWriteMask::DEPTH_WRITE_MASK_ALL, D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL } };
	static std::map<CustomStruct::CRenderDepthWriteMask, D3D11_DEPTH_WRITE_MASK> depthWriteMap = {
		{ CustomStruct::CRenderDepthWriteMask::DEPTH_WRITE_MASK_ZERO, D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ZERO },
		{ CustomStruct::CRenderDepthWriteMask::DEPTH_WRITE_MASK_ALL, D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL } };

	output.DepthEnable = depthInput.DepthEnable;
	output.DepthWriteMask = depthWriteMap[depthInput.DepthWriteMask];
	TranslateComparisonFunction(output.DepthFunc, depthInput.DepthFunc);


	output.StencilEnable = FALSE;
	if (stencilInput)
	{
		output.StencilEnable = TRUE;
		output.StencilReadMask = static_cast<UINT8>(stencilInput->StencilReadMask);
		output.StencilWriteMask = static_cast<UINT8>(stencilInput->StencilWriteMask);
		CRenderDevice::TranslateStencilOperation(output.FrontFace, stencilInput->FrontFace);
		CRenderDevice::TranslateStencilOperation(output.BackFace, stencilInput->BackFace);
	}
}
void CRenderDevice::TranslateBlendState(D3D11_RENDER_TARGET_BLEND_DESC& output, const CustomStruct::CRenderBlendState& input)
{
	static std::map<CustomStruct::CRenderBlendOption, D3D11_BLEND> blendMap = {
		{ CustomStruct::CRenderBlendOption::BLEND_ZERO, D3D11_BLEND::D3D11_BLEND_ZERO },
		{ CustomStruct::CRenderBlendOption::BLEND_ONE, D3D11_BLEND::D3D11_BLEND_ONE },
		{ CustomStruct::CRenderBlendOption::BLEND_SRC_COLOR, D3D11_BLEND::D3D11_BLEND_SRC_COLOR },
		{ CustomStruct::CRenderBlendOption::BLEND_INV_SRC_COLOR, D3D11_BLEND::D3D11_BLEND_INV_SRC_COLOR },
		{ CustomStruct::CRenderBlendOption::BLEND_SRC_ALPHA, D3D11_BLEND::D3D11_BLEND_SRC_ALPHA },
		{ CustomStruct::CRenderBlendOption::BLEND_INV_SRC_ALPHA, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA },
		{ CustomStruct::CRenderBlendOption::BLEND_DEST_ALPHA, D3D11_BLEND::D3D11_BLEND_DEST_ALPHA },
		{ CustomStruct::CRenderBlendOption::BLEND_INV_DEST_ALPHA, D3D11_BLEND::D3D11_BLEND_INV_DEST_ALPHA },
		{ CustomStruct::CRenderBlendOption::BLEND_DEST_COLOR, D3D11_BLEND::D3D11_BLEND_DEST_COLOR },
		{ CustomStruct::CRenderBlendOption::BLEND_INV_DEST_COLOR, D3D11_BLEND::D3D11_BLEND_INV_DEST_COLOR },
		{ CustomStruct::CRenderBlendOption::BLEND_SRC_ALPHA_SAT, D3D11_BLEND::D3D11_BLEND_SRC_ALPHA_SAT },
		{ CustomStruct::CRenderBlendOption::BLEND_BLEND_FACTOR, D3D11_BLEND::D3D11_BLEND_BLEND_FACTOR },
		{ CustomStruct::CRenderBlendOption::BLEND_INV_BLEND_FACTOR, D3D11_BLEND::D3D11_BLEND_INV_BLEND_FACTOR } };
	static std::map<CustomStruct::CRenderBlendOperation, D3D11_BLEND_OP> blendOpMap = {
		{ CustomStruct::CRenderBlendOperation::BLEND_OP_ADD, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD },
		{ CustomStruct::CRenderBlendOperation::BLEND_OP_SUBTRACT, D3D11_BLEND_OP::D3D11_BLEND_OP_SUBTRACT },
		{ CustomStruct::CRenderBlendOperation::BLEND_OP_REV_SUBTRACT, D3D11_BLEND_OP::D3D11_BLEND_OP_REV_SUBTRACT },
		{ CustomStruct::CRenderBlendOperation::BLEND_OP_MIN, D3D11_BLEND_OP::D3D11_BLEND_OP_MIN },
		{ CustomStruct::CRenderBlendOperation::BLEND_OP_MAX, D3D11_BLEND_OP::D3D11_BLEND_OP_MAX } };
	static std::map<CustomStruct::CRenderBlendWriteEnable, D3D11_COLOR_WRITE_ENABLE> writeEnableMap = {
		{ CustomStruct::CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_RED, D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_RED },
		{ CustomStruct::CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_GREEN, D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_GREEN },
		{ CustomStruct::CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_BLUE, D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_BLUE },
		{ CustomStruct::CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_ALPHA, D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALPHA },
		{ CustomStruct::CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_ALL, D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL } };

	output.BlendEnable = input.BlendEnable;
	output.SrcBlend = blendMap[input.SrcBlend];
	output.DestBlend = blendMap[input.DstBlend];
	output.BlendOp = blendOpMap[input.BlendOp];
	output.SrcBlendAlpha = blendMap[input.SrcBlendAlpha];
	output.DestBlendAlpha = blendMap[input.DstBlendAlpha];
	output.BlendOpAlpha = blendOpMap[input.BlendOpAlpha];
	output.RenderTargetWriteMask = writeEnableMap[input.RenderTargetWriteMask];
}
void CRenderDevice::TranslateRasterizerState(D3D11_CULL_MODE& outputCull, D3D11_FILL_MODE& outputFill, const CustomStruct::CRenderRasterizerState& input)
{
	static std::map<CustomStruct::CRenderCullMode, D3D11_CULL_MODE> cullMap = {
		{ CustomStruct::CRenderCullMode::CULL_NONE, D3D11_CULL_MODE::D3D11_CULL_NONE },
		{ CustomStruct::CRenderCullMode::CULL_FRONT, D3D11_CULL_MODE::D3D11_CULL_FRONT },
		{ CustomStruct::CRenderCullMode::CULL_BACK, D3D11_CULL_MODE::D3D11_CULL_BACK } };
	static std::map<CustomStruct::CRenderFillMode, D3D11_FILL_MODE> fillMap = {
		{ CustomStruct::CRenderFillMode::FILL_WIREFRAME, D3D11_FILL_MODE::D3D11_FILL_WIREFRAME },
		{ CustomStruct::CRenderFillMode::FILL_SOLID, D3D11_FILL_MODE::D3D11_FILL_SOLID } };

	outputCull = cullMap[input.CullMode];
	outputFill = fillMap[input.FillMode];
}
void CRenderDevice::TranslateSamplerState(D3D11_SAMPLER_DESC& output, const CustomStruct::CRenderSamplerState& input)
{
	static std::map<CustomStruct::CRenderFilter, D3D11_FILTER> filterMap = {
		{ CustomStruct::CRenderFilter::FILTER_POINT, D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_POINT },
		{ CustomStruct::CRenderFilter::FILTER_LINEAR, D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR },
		{ CustomStruct::CRenderFilter::FILTER_ANISOTROPIC, D3D11_FILTER::D3D11_FILTER_ANISOTROPIC } };
	static std::map<CustomStruct::CRenderTextureAddressMode, D3D11_TEXTURE_ADDRESS_MODE> textureAddressModeMap = {
		{ CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP },
		{ CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_MIRROR, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_MIRROR },
		{ CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP },
		{ CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER },
		{ CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_MIRROR_ONCE, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_MIRROR_ONCE } };

	output.Filter = filterMap[input.Filter];
	output.AddressU = textureAddressModeMap[input.AddressU];
	output.AddressV = textureAddressModeMap[input.AddressV];
	output.AddressW = textureAddressModeMap[input.AddressW];
	output.MipLODBias = input.MipLODBias;
	output.MaxAnisotropy = input.MaxAnisotropy;
	TranslateComparisonFunction(output.ComparisonFunc, input.ComparisonFunc);
	output.BorderColor[0] = input.BorderColor.r;
	output.BorderColor[1] = input.BorderColor.g;
	output.BorderColor[2] = input.BorderColor.b;
	output.BorderColor[3] = input.BorderColor.a;
	output.MinLOD = input.MinLOD;
	output.MaxLOD = input.MaxLOD;
}

//void CRenderDevice::ResetRenderTarget()
//{
//	m_DeferredBuffer->ClearRenderTarget(0.f, 0.f, 0.f, 1.f);
//}
//void CRenderDevice::BeginShadow()
//{
//	CRenderDevice::SetDepthState(CRenderDevice::DSSE_TESTENABLEWRITEENABLE);
//	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
//	m_DeferredBuffer->SetDepthStencilRenderTarget(CDeferredBuffer::DEPTHSTENCILBUFFER_LIGHT, m_RenderTargetView);
//}
//void CRenderDevice::EndShadow()
//{
//
//}
//void CRenderDevice::BeginDeferred()
//{
//	CRenderDevice::SetDepthState(CRenderDevice::DSSE_TESTENABLEWRITEENABLE);
//	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
//	m_DeferredBuffer->SetDeferredRenderTarget();
//}
//void CRenderDevice::EndDeferred()
//{
//
//}
//void CRenderDevice::BeginDeferredResolve()
//{
//	CRenderDevice::SetDepthState(CRenderDevice::DSSE_ALLDISABLE);
//	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
//	m_DeferredBuffer->SetExtraRenderTarget();
//	CRenderDevice::BindTexture(m_DeferredBuffer->GetRenderTargetShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_WORLDNORMAL), ENGINE_GBUFFER_WORLD_NORMAL_START_SLOT);
//	CRenderDevice::BindTexture(m_DeferredBuffer->GetRenderTargetShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_ALBEDO), ENGINE_GBUFFER_ALBEDO_START_SLOT);
//	CRenderDevice::BindTexture(m_DeferredBuffer->GetRenderTargetShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_PROPERTY), ENGINE_GBUFFER_PROPERTY_START_SLOT);
//	CRenderDevice::BindTexture(m_DeferredBuffer->GetRenderTargetShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_ID), ENGINE_GBUFFER_ID_START_SLOT);
//}
//void CRenderDevice::EndDeferredResolve()
//{
//
//}
//void CRenderDevice::BeginForward()
//{
//	CRenderDevice::SetDepthState(CRenderDevice::DSSE_TESTENABLEWRITEENABLE);
//	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
//	m_DeferredBuffer->SetRenderTarget(CDeferredBuffer::DEFERREDBUFFER_EXTRA, CDeferredBuffer::DEPTHSTENCILBUFFER_CAMERA);
//}
//void CRenderDevice::EndForward()
//{
//
//}
//void CRenderDevice::SetOutputRTV()
//{
//	FLOAT ClearColor[4] = { 0.f, 0.f, 0.f, 1.f };
//	m_ImmediateContext->ClearRenderTargetView(m_RenderTargetView, ClearColor);
//	m_ImmediateContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0u);
//	CRenderDevice::SetDepthState(CRenderDevice::DSSE_ALLDISABLE);
//	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
//	m_ImmediateContext->OMSetRenderTargets(1u, &m_RenderTargetView, m_DepthStencilView);
//	m_ImmediateContext->RSSetViewports(1u, &m_ViewPort);
//	CRenderDevice::BindTexture(m_DeferredBuffer->GetRenderTargetShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_EXTRA), ENGINE_SRV_CAMERA_COLOR);
//	CRenderDevice::BindTexture(m_DeferredBuffer->GetDepthStencilShaderResourceView(CDeferredBuffer::DEPTHSTENCILBUFFER_CAMERA), ENGINE_SRV_CAMERA_DEPTH);
//}
//void CRenderDevice::Present()
//{
//	m_SwapChain->Present(0u, 0u);//DXGI_PRESENT
//}
//void CRenderDevice::SetBlendState(BlendStateEnum bse)
//{
//	FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
//	m_ImmediateContext->OMSetBlendState(m_BlendState[bse], blendFactor, 0xffffffff);
//}
//void CRenderDevice::SetDepthState(DepthStencilStateEnum dsse)
//{
//	m_ImmediateContext->OMSetDepthStencilState(m_DepthStencilStateTestWrite[dsse], NULL);
//}
//void CRenderDevice::SetRasterizerState(RasterizerStateEnum rse)
//{
//	m_ImmediateContext->RSSetState(m_RasterizerStateCull[rse]);
//}
//void CRenderDevice::BindTexture(CTexture2D* ptrTexture, const UINT& startSlot)
//{
//	if (ptrTexture == NULL)
//		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_WHITE]->GetShaderResourceView().GetAddressOf());
//	else
//		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, ptrTexture->GetShaderResourceView().GetAddressOf());
//}
//void CRenderDevice::BindTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ptrSRV, const UINT& startSlot)
//{
//	if (ptrSRV == NULL)
//		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_WHITE]->GetShaderResourceView().GetAddressOf());
//	else
//		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, ptrSRV.GetAddressOf());
//}
//void CRenderDevice::BindTexture(ID3D11ShaderResourceView* ptrSRV, const UINT& startSlot)
//{
//	if (ptrSRV == NULL)
//		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_WHITE]->GetShaderResourceView().GetAddressOf());
//	else
//	{
//		ID3D11ShaderResourceView* srv[1] = { ptrSRV };
//		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, srv);
//	}
//}
//void CRenderDevice::BindComputeShaderResourceView(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ptrSRV, const UINT& startSlot)
//{
//	if (ptrSRV == NULL)
//		m_ImmediateContext->CSSetShaderResources(startSlot, 1u, m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_WHITE]->GetShaderResourceView().GetAddressOf());
//	else
//		m_ImmediateContext->CSSetShaderResources(startSlot, 1u, ptrSRV.GetAddressOf());
//}
//void CRenderDevice::BindComputeUnorderedAccessView(Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> ptrUAV, const UINT& startSlot)
//{
//	m_ImmediateContext->CSSetUnorderedAccessViews(startSlot, 1u, ptrUAV.GetAddressOf(), NULL);
//}
//void CRenderDevice::SetShadowMap(const UINT& Slot)
//{
//	ID3D11ShaderResourceView* srv[1] = { m_DeferredBuffer->GetDepthStencilShaderResourceView(CDeferredBuffer::DEPTHSTENCILBUFFER_LIGHT) };
//	m_ImmediateContext->PSSetShaderResources(Slot, 1u, srv);
//}
//void CRenderDevice::DrawIndexed(const UINT& indexCount, const UINT& startIndexLocation, const INT& baseVertexLocation)
//{
//	m_ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//	m_ImmediateContext->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
//}
//BOOL CRenderDevice::CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& ptrBuffer, const UINT& sizeData, D3D11_USAGE usage)
//{
//	D3D11_BUFFER_DESC hBufferDesc;
//	hBufferDesc.Usage = usage;
//	hBufferDesc.ByteWidth = sizeData;
//	hBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//	hBufferDesc.CPUAccessFlags = 0u;
//	hBufferDesc.MiscFlags = 0u;
//	hBufferDesc.StructureByteStride = sizeof(FLOAT);
//
//	HRESULT hr = m_Device->CreateBuffer(&hBufferDesc, NULL, ptrBuffer.ReleaseAndGetAddressOf());
//	if (FAILED(hr))
//		return FALSE;
//	return TRUE;
//}
//void CRenderDevice::UploadConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const void* ptrData)
//{
//	m_ImmediateContext->UpdateSubresource(ptrBuffer.Get(), 0u, NULL, ptrData, 0u, 0u);
//}
//void CRenderDevice::BindConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot)
//{
//	m_ImmediateContext->VSSetConstantBuffers(startSlot, 1u, ptrBuffer.GetAddressOf());
//	m_ImmediateContext->PSSetConstantBuffers(startSlot, 1u, ptrBuffer.GetAddressOf());
//}
//void CRenderDevice::BindComputeShaderConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot)
//{
//	m_ImmediateContext->CSSetConstantBuffers(startSlot, 1u, ptrBuffer.GetAddressOf());
//}
//void CRenderDevice::BindVertexBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot, const UINT& stride, const UINT& offset)
//{
//	m_ImmediateContext->IASetVertexBuffers(startSlot, 1u, ptrBuffer.GetAddressOf(), &stride, &offset);
//}
//void CRenderDevice::BindIndexBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, DXGI_FORMAT format, const UINT& offset)
//{
//	m_ImmediateContext->IASetIndexBuffer(ptrBuffer.Get(), format, offset);
//}
BOOL CRenderDevice::CreateRenderTexture2D(RenderTexture2DViewInfo& output, const CustomStruct::CRenderTextureDesc& textureDesc, const BOOL& randomWrite)
{
	D3D11_TEXTURE2D_DESC texture2DDesc;
	ZeroMemory(&texture2DDesc, sizeof(texture2DDesc));
	texture2DDesc.Width = width;
	texture2DDesc.Height = height;
	texture2DDesc.MipLevels = mipLevels;
	texture2DDesc.ArraySize = arraySize;
	texture2DDesc.Format = format;
	texture2DDesc.SampleDesc.Count = 1u;
	texture2DDesc.SampleDesc.Quality = 0u;
	texture2DDesc.Usage = usage;
	texture2DDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	texture2DDesc.CPUAccessFlags = 0u;	//D3D11_CPU_ACCESS_FLAG
	texture2DDesc.MiscFlags = 0u;		//D3D11_RESOURCE_MISC_FLAG
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateTexture2D(&texture2DDesc, NULL, textureInfo.Texture2D.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return FALSE;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	ZeroMemory(&renderTargetViewDesc, sizeof(renderTargetViewDesc));
	renderTargetViewDesc.Format = texture2DDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0u;
	hr = CRenderDevice::m_RenderDevice->m_Device->CreateRenderTargetView(textureInfo.Texture2D.Get(), &renderTargetViewDesc, textureInfo.RenderTargetView.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return FALSE;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = texture2DDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0u;
	shaderResourceViewDesc.Texture2D.MipLevels = mipLevels;
	hr = CRenderDevice::m_RenderDevice->m_Device->CreateShaderResourceView(textureInfo.Texture2D.Get(), &shaderResourceViewDesc, textureInfo.ShaderResourceView.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return FALSE;
	if (randomWrite)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc;
		ZeroMemory(&unorderedAccessViewDesc, sizeof(unorderedAccessViewDesc));
		unorderedAccessViewDesc.Format = texture2DDesc.Format;
		unorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		unorderedAccessViewDesc.Texture2D.MipSlice = 0u;
		hr = CRenderDevice::m_RenderDevice->m_Device->CreateUnorderedAccessView(textureInfo.Texture2D.Get(), &unorderedAccessViewDesc, textureInfo.UnorderedAccessView.ReleaseAndGetAddressOf());
		if (FAILED(hr))
			return FALSE;
	}
	return TRUE;
}
BOOL CRenderDevice::CreateTexture2D(Texture2DViewInfo& output, const CustomStruct::CRenderTextureDesc& textureDesc)
{
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = mipLevels;
	desc.ArraySize = arraySize;
	desc.Format = format;
	desc.SampleDesc.Count = 1u;
	desc.SampleDesc.Quality = 0u;
	desc.Usage = usage;
	desc.BindFlags = flag;
	desc.CPUAccessFlags = 0u;
	desc.MiscFlags = 0u;	//D3D11_RESOURCE_MISC_FLAG

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = ptrData;
	initData.SysMemPitch = memPitch;
	initData.SysMemSlicePitch = memSlicePitch;

	HRESULT hr = m_Device->CreateTexture2D(&desc, &initData, ptrTexture.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return FALSE;
	return TRUE;
}