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

	//D3D11_SAMPLER_DESC samplerDesc;
	//ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	//samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	//samplerDesc.BorderColor[0] = 0.f;
	//samplerDesc.BorderColor[1] = 0.f;
	//samplerDesc.BorderColor[2] = 0.f;
	//samplerDesc.BorderColor[3] = 0.f;
	//samplerDesc.MinLOD = 0.f;
	//samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	//samplerDesc.MipLODBias = 0.f;
	//samplerDesc.MaxAnisotropy = 1u;
	//samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	//samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	//samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	//m_Device->CreateSamplerState(&samplerDesc, &m_SamplerState[SSE_CLAMP_POINT]);
	//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	//m_Device->CreateSamplerState(&samplerDesc, &m_SamplerState[SSE_CLAMP_LINEAR]);
	//samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	//samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	//samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	//m_Device->CreateSamplerState(&samplerDesc, &m_SamplerState[SSE_WRAP_POINT]);
	//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	//m_Device->CreateSamplerState(&samplerDesc, &m_SamplerState[SSE_WRAP_LINEAR]);

	//D3D11_BLEND_DESC blendDesc;
	//ZeroMemory(&blendDesc, sizeof(blendDesc));
	//blendDesc.AlphaToCoverageEnable = FALSE;
	//blendDesc.IndependentBlendEnable = FALSE;
	//blendDesc.RenderTarget[0].BlendEnable = TRUE;
	//blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	//blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	//blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	//blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	//blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	//blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	//m_Device->CreateBlendState(&blendDesc, &m_BlendState[BSE_BLENDALPHA]);
	//blendDesc.RenderTarget[0].BlendEnable = FALSE;
	//m_Device->CreateBlendState(&blendDesc, &m_BlendState[BSE_BLENDOFF]);
	//
	//FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	//m_ImmediateContext->OMSetBlendState(m_BlendState[BSE_BLENDALPHA], blendFactor, 0xffffffff);

	//D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	//ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	//depthStencilDesc.DepthEnable = FALSE;
	//depthStencilDesc.StencilEnable = FALSE;
	//m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilStateTestWrite[DSSE_ALLDISABLE]);
	//depthStencilDesc.DepthEnable = TRUE;
	//depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	//depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	//m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilStateTestWrite[DSSE_TESTENABLEWRITEENABLE]);
	//depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	//m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilStateTestWrite[DSSE_TESTENABLEWRITEDISABLE]);
	//
	//m_ImmediateContext->OMSetDepthStencilState(m_DepthStencilStateTestWrite[DSSE_TESTENABLEWRITEENABLE], NULL);

	//ID3D11SamplerState* sceneSampler[] = {
	//	m_SamplerState[SSE_CLAMP_POINT],
	//	m_SamplerState[SSE_CLAMP_LINEAR],
	//	m_SamplerState[SSE_WRAP_POINT],
	//	m_SamplerState[SSE_WRAP_LINEAR] };
	//CRenderDevice::GetDeviceContext()->VSSetSamplers(0u, 4u, sceneSampler);
	//CRenderDevice::GetDeviceContext()->PSSetSamplers(0u, 4u, sceneSampler);
	//CRenderDevice::GetDeviceContext()->CSSetSamplers(0u, 4u, sceneSampler);

	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_WHITE]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_WHITE);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_BLACK]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_BLACK);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_GRAY]		= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_GRAY);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_RED]		= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_RED);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_GREEN]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_GREEN);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_BLUE]		= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_BLUE);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_BUMP]		= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_BUMP);
	//m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_PROPERTY]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_PROPERTY);

	//m_DeferredBuffer = new CDeferredBuffer();
	//m_DeferredBuffer->Initialize(ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT);

	//m_DeferredResolve = new CScreenPolygon2D(ENGINE_SHADER_SCREEN_POLYGON_2D_VS, ENGINE_SHADER_GBUFFER_RESOLVE_PS);
	//m_DeferredResolve->Init();
	//m_PostEffect = new CScreenPolygon2D(ENGINE_SHADER_SCREEN_POLYGON_2D_VS, ENGINE_SHADER_POST_EFFECT_EXPOSURE_PS);
	//m_PostEffect->Init();
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
BOOL CRenderDevice::CreateRasterizerState(Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs, D3D11_CULL_MODE cullMode, D3D11_FILL_MODE fillMode)
{
	D3D11_RASTERIZER_DESC rd;
	{
		::ZeroMemory(&rd, sizeof(rd));
		rd.FillMode = fillMode;
		rd.CullMode = cullMode;
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
		return FALSE;
	return TRUE;
}
BOOL CRenderDevice::CreateBlendState(Microsoft::WRL::ComPtr<ID3D11BlendState>& bs)
{
	D3D11_BLEND_DESC bd;
	{
		::ZeroMemory(&bd, sizeof(bd));
		bd.AlphaToCoverageEnable = FALSE;
		bd.IndependentBlendEnable = FALSE;
		bd.RenderTarget[0].BlendEnable = TRUE;
		bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		bd.RenderTarget[0].BlendEnable = FALSE;
	}
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateBlendState(&bd, bs.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return FALSE;
	return TRUE;
}
D3D11_VIEWPORT CRenderDevice::GetViewport()
{
	return (CRenderDevice::m_RenderDevice->m_Viewport);
}
void CRenderDevice::ResetRenderTarget()
{
	m_DeferredBuffer->ClearRenderTarget(0.f, 0.f, 0.f, 1.f);
}
void CRenderDevice::BeginShadow()
{
	CRenderDevice::SetDepthState(CRenderDevice::DSSE_TESTENABLEWRITEENABLE);
	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
	m_DeferredBuffer->SetDepthStencilRenderTarget(CDeferredBuffer::DEPTHSTENCILBUFFER_LIGHT, m_RenderTargetView);
}
void CRenderDevice::EndShadow()
{

}
void CRenderDevice::BeginDeferred()
{
	CRenderDevice::SetDepthState(CRenderDevice::DSSE_TESTENABLEWRITEENABLE);
	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
	m_DeferredBuffer->SetDeferredRenderTarget();
}
void CRenderDevice::EndDeferred()
{

}
void CRenderDevice::BeginDeferredResolve()
{
	CRenderDevice::SetDepthState(CRenderDevice::DSSE_ALLDISABLE);
	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
	m_DeferredBuffer->SetExtraRenderTarget();
	CRenderDevice::BindTexture(m_DeferredBuffer->GetRenderTargetShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_WORLDNORMAL), ENGINE_GBUFFER_WORLD_NORMAL_START_SLOT);
	CRenderDevice::BindTexture(m_DeferredBuffer->GetRenderTargetShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_ALBEDO), ENGINE_GBUFFER_ALBEDO_START_SLOT);
	CRenderDevice::BindTexture(m_DeferredBuffer->GetRenderTargetShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_PROPERTY), ENGINE_GBUFFER_PROPERTY_START_SLOT);
	CRenderDevice::BindTexture(m_DeferredBuffer->GetRenderTargetShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_ID), ENGINE_GBUFFER_ID_START_SLOT);
}
void CRenderDevice::EndDeferredResolve()
{

}
void CRenderDevice::BeginForward()
{
	CRenderDevice::SetDepthState(CRenderDevice::DSSE_TESTENABLEWRITEENABLE);
	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
	m_DeferredBuffer->SetRenderTarget(CDeferredBuffer::DEFERREDBUFFER_EXTRA, CDeferredBuffer::DEPTHSTENCILBUFFER_CAMERA);
}
void CRenderDevice::EndForward()
{

}
void CRenderDevice::SetOutputRTV()
{
	FLOAT ClearColor[4] = { 0.f, 0.f, 0.f, 1.f };
	m_ImmediateContext->ClearRenderTargetView(m_RenderTargetView, ClearColor);
	m_ImmediateContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0u);
	CRenderDevice::SetDepthState(CRenderDevice::DSSE_ALLDISABLE);
	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
	m_ImmediateContext->OMSetRenderTargets(1u, &m_RenderTargetView, m_DepthStencilView);
	m_ImmediateContext->RSSetViewports(1u, &m_ViewPort);
	CRenderDevice::BindTexture(m_DeferredBuffer->GetRenderTargetShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_EXTRA), ENGINE_SRV_CAMERA_COLOR);
	CRenderDevice::BindTexture(m_DeferredBuffer->GetDepthStencilShaderResourceView(CDeferredBuffer::DEPTHSTENCILBUFFER_CAMERA), ENGINE_SRV_CAMERA_DEPTH);
}
void CRenderDevice::Present()
{
	m_SwapChain->Present(1u, 0u);
}
void CRenderDevice::SetBlendState(BlendStateEnum bse)
{
	FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	m_ImmediateContext->OMSetBlendState(m_BlendState[bse], blendFactor, 0xffffffff);
}
void CRenderDevice::SetDepthState(DepthStencilStateEnum dsse)
{
	m_ImmediateContext->OMSetDepthStencilState(m_DepthStencilStateTestWrite[dsse], NULL);
}
void CRenderDevice::SetRasterizerState(RasterizerStateEnum rse)
{
	m_ImmediateContext->RSSetState(m_RasterizerStateCull[rse]);
}
void CRenderDevice::BindTexture(CTexture2D* ptrTexture, const UINT& startSlot)
{
	if (ptrTexture == NULL)
		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_WHITE]->GetShaderResourceView().GetAddressOf());
	else
		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, ptrTexture->GetShaderResourceView().GetAddressOf());
}
void CRenderDevice::BindTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ptrSRV, const UINT& startSlot)
{
	if (ptrSRV == NULL)
		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_WHITE]->GetShaderResourceView().GetAddressOf());
	else
		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, ptrSRV.GetAddressOf());
}
void CRenderDevice::BindTexture(ID3D11ShaderResourceView* ptrSRV, const UINT& startSlot)
{
	if (ptrSRV == NULL)
		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_WHITE]->GetShaderResourceView().GetAddressOf());
	else
	{
		ID3D11ShaderResourceView* srv[1] = { ptrSRV };
		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, srv);
	}
}
void CRenderDevice::BindComputeShaderResourceView(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ptrSRV, const UINT& startSlot)
{
	if (ptrSRV == NULL)
		m_ImmediateContext->CSSetShaderResources(startSlot, 1u, m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_WHITE]->GetShaderResourceView().GetAddressOf());
	else
		m_ImmediateContext->CSSetShaderResources(startSlot, 1u, ptrSRV.GetAddressOf());
}
void CRenderDevice::BindComputeUnorderedAccessView(Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> ptrUAV, const UINT& startSlot)
{
	m_ImmediateContext->CSSetUnorderedAccessViews(startSlot, 1u, ptrUAV.GetAddressOf(), NULL);
}
void CRenderDevice::SetShadowMap(const UINT& Slot)
{
	ID3D11ShaderResourceView* srv[1] = { m_DeferredBuffer->GetDepthStencilShaderResourceView(CDeferredBuffer::DEPTHSTENCILBUFFER_LIGHT) };
	m_ImmediateContext->PSSetShaderResources(Slot, 1u, srv);
}
void CRenderDevice::DrawIndexed(const UINT& indexCount, const UINT& startIndexLocation, const INT& baseVertexLocation)
{
	m_ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_ImmediateContext->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
}
BOOL CRenderDevice::CreateD3DBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& ptrBuffer, const void* ptrData, const UINT& stride, const UINT& count, UINT flag, D3D11_USAGE usage)
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = usage;
	bd.ByteWidth = stride * count;
	bd.BindFlags = flag;
	bd.CPUAccessFlags = 0u;

	D3D11_SUBRESOURCE_DATA sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.pSysMem = ptrData;

	HRESULT hr = m_Device->CreateBuffer(&bd, &sd, ptrBuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return FALSE;
	return TRUE;
}
BOOL CRenderDevice::LoadVertexShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout, const std::vector<D3D11_INPUT_ELEMENT_DESC>* layout)
{
	FILE* file;
	LONG fsize;
	BYTE* buffer;
	{
		fopen_s(&file, name.c_str(), "rb");
		if (file == NULL)
			return FALSE;
		fsize = _filelength(_fileno(file));
		buffer = new BYTE[fsize];
		fread_s(buffer, fsize, fsize, 1u, file);
		fclose(file);
	}

	{
		HRESULT hr = m_Device->CreateVertexShader(static_cast<void*>(buffer), fsize, NULL, vertexShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
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
		
		HRESULT hr = m_Device->CreateInputLayout(tempLayout.data(), numElements, static_cast<void*>(buffer), fsize, inputLayout.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
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
			return FALSE;
		fsize = _filelength(_fileno(file));
		buffer = new BYTE[fsize];
		fread_s(buffer, fsize, fsize, 1u, file);
		fclose(file);
	}

	{
		HRESULT hr = m_Device->CreatePixelShader(static_cast<void*>(buffer), fsize, NULL, pixelShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
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
			return FALSE;
		fsize = _filelength(_fileno(file));
		buffer = new BYTE[fsize];
		fread_s(buffer, fsize, fsize, 1u, file);
		fclose(file);
	}

	{
		HRESULT hr = m_Device->CreateComputeShader(static_cast<void*>(buffer), fsize, NULL, computeShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			return FALSE;
		}
	}

	delete[] buffer;
	return TRUE;
}
BOOL CRenderDevice::CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& ptrBuffer, const UINT& sizeData, D3D11_USAGE usage)
{
	D3D11_BUFFER_DESC hBufferDesc;
	hBufferDesc.Usage = usage;
	hBufferDesc.ByteWidth = sizeData;
	hBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hBufferDesc.CPUAccessFlags = 0u;
	hBufferDesc.MiscFlags = 0u;
	hBufferDesc.StructureByteStride = sizeof(FLOAT);

	HRESULT hr = m_Device->CreateBuffer(&hBufferDesc, NULL, ptrBuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return FALSE;
	return TRUE;
}
void CRenderDevice::UploadConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const void* ptrData)
{
	m_ImmediateContext->UpdateSubresource(ptrBuffer.Get(), 0u, NULL, ptrData, 0u, 0u);
}
void CRenderDevice::BindConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot)
{
	m_ImmediateContext->VSSetConstantBuffers(startSlot, 1u, ptrBuffer.GetAddressOf());
	m_ImmediateContext->PSSetConstantBuffers(startSlot, 1u, ptrBuffer.GetAddressOf());
}
void CRenderDevice::BindComputeShaderConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot)
{
	m_ImmediateContext->CSSetConstantBuffers(startSlot, 1u, ptrBuffer.GetAddressOf());
}
void CRenderDevice::BindVertexBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot, const UINT& stride, const UINT& offset)
{
	m_ImmediateContext->IASetVertexBuffers(startSlot, 1u, ptrBuffer.GetAddressOf(), &stride, &offset);
}
void CRenderDevice::BindIndexBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, DXGI_FORMAT format, const UINT& offset)
{
	m_ImmediateContext->IASetIndexBuffer(ptrBuffer.Get(), format, offset);
}
BOOL CRenderDevice::CreateRenderTexture2D(RenderTexture2DViewInfo& textureInfo, const UINT& width, const UINT& height, DXGI_FORMAT format, const BOOL& randomWirte, D3D11_USAGE usage, const UINT& mipLevels, const UINT& arraySize)
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
	HRESULT hr = m_Device->CreateTexture2D(&texture2DDesc, NULL, textureInfo.Texture2D.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return FALSE;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	ZeroMemory(&renderTargetViewDesc, sizeof(renderTargetViewDesc));
	renderTargetViewDesc.Format = texture2DDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0u;
	hr = m_Device->CreateRenderTargetView(textureInfo.Texture2D.Get(), &renderTargetViewDesc, textureInfo.RenderTargetView.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return FALSE;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = texture2DDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0u;
	shaderResourceViewDesc.Texture2D.MipLevels = mipLevels;
	hr = m_Device->CreateShaderResourceView(textureInfo.Texture2D.Get(), &shaderResourceViewDesc, textureInfo.ShaderResourceView.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return FALSE;
	if (randomWirte)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc;
		ZeroMemory(&unorderedAccessViewDesc, sizeof(unorderedAccessViewDesc));
		unorderedAccessViewDesc.Format = texture2DDesc.Format;
		unorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		unorderedAccessViewDesc.Texture2D.MipSlice = 0u;
		hr = m_Device->CreateUnorderedAccessView(textureInfo.Texture2D.Get(), &unorderedAccessViewDesc, textureInfo.UnorderedAccessView.ReleaseAndGetAddressOf());
		if (FAILED(hr))
			return FALSE;
	}
	return TRUE;
}
BOOL CRenderDevice::CreateTexture2D(Microsoft::WRL::ComPtr<ID3D11Texture2D>& ptrTexture, const UINT& width, const UINT& height, DXGI_FORMAT format, const void* ptrData, const UINT& memPitch, const UINT& memSlicePitch, UINT flag, D3D11_USAGE usage, const UINT& mipLevels, const UINT& arraySize)
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
BOOL CRenderDevice::CreateTexture2DShaderResourceView(Microsoft::WRL::ComPtr<ID3D11Texture2D> ptrTexture, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& ptrSRV, DXGI_FORMAT format, D3D_SRV_DIMENSION viewDimension, const UINT& mipLevels, const UINT& mostDetailedMip)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = viewDimension;
	srvDesc.Texture2D.MipLevels = mipLevels;
	srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;

	HRESULT hr = m_Device->CreateShaderResourceView(ptrTexture.Get(), &srvDesc, ptrSRV.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return FALSE;
	return TRUE;
}