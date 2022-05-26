#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Base/CTextureManager.h"
#include "../../Headers/Base/CTexture2D.h"
#include "../../Headers/Game/CDeferredBuffer.h"
#include "../../Headers/Object/CScreenPolygon2D.h"

CTexture2D*					CRenderDevice::m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_COUNT];
CDeferredBuffer*			CRenderDevice::m_DeferredBuffer			= NULL;
CScreenPolygon2D*			CRenderDevice::m_DeferredResolve		= NULL;
CScreenPolygon2D*			CRenderDevice::m_PostEffect				= NULL;

D3D_FEATURE_LEVEL			CRenderDevice::m_FeatureLevel			= D3D_FEATURE_LEVEL_11_0;

ID3D11Device*				CRenderDevice::m_D3DDevice				= NULL;
ID3D11DeviceContext*		CRenderDevice::m_ImmediateContext		= NULL;
IDXGISwapChain*				CRenderDevice::m_SwapChain				= NULL;
ID3D11RenderTargetView*		CRenderDevice::m_RenderTargetView		= NULL;
ID3D11DepthStencilView*		CRenderDevice::m_DepthStencilView		= NULL;
D3D11_VIEWPORT				CRenderDevice::m_ViewPort;

ID3D11SamplerState*			CRenderDevice::m_SamplerState[SSE_COUNT];
ID3D11BlendState*			CRenderDevice::m_BlendState[BSE_COUNT];
ID3D11RasterizerState*		CRenderDevice::m_RasterizerStateCull[RSE_COUNT];
ID3D11DepthStencilState*	CRenderDevice::m_DepthStencilStateTestWrite[DSSE_COUNT];

void CRenderDevice::Init()
{
	HRESULT hr = S_OK;

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = ENGINE_SCREEN_WIDTH;
	sd.BufferDesc.Height = ENGINE_SCREEN_HEIGHT;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = GetMainWindowHandle();
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		0,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&sd,
		&m_SwapChain,
		&m_D3DDevice,
		&m_FeatureLevel,
		&m_ImmediateContext);
	if (FAILED(hr))return;


	ID3D11Texture2D* pBackBuffer = NULL;
	hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))return;
	hr = m_D3DDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_RenderTargetView);
	if (FAILED(hr))return;
	pBackBuffer->Release();

	{
		ID3D11Texture2D* depthTexture = NULL;
		D3D11_TEXTURE2D_DESC td;
		ZeroMemory(&td, sizeof(td));
		td.Width = sd.BufferDesc.Width;
		td.Height = sd.BufferDesc.Height;
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		td.SampleDesc = sd.SampleDesc;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		td.CPUAccessFlags = 0;
		td.MiscFlags = 0;
		hr = m_D3DDevice->CreateTexture2D(&td, NULL, &depthTexture);
		if (FAILED(hr))return;


		D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
		ZeroMemory(&dsvd, sizeof(dsvd));
		dsvd.Format = td.Format;
		dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvd.Flags = 0;
		hr = m_D3DDevice->CreateDepthStencilView(depthTexture, &dsvd, &m_DepthStencilView);
		if (FAILED(hr))return;

		m_ImmediateContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);
	}

	m_ViewPort.Width = static_cast<FLOAT>(ENGINE_SCREEN_WIDTH);
	m_ViewPort.Height = static_cast<FLOAT>(ENGINE_SCREEN_HEIGHT);
	m_ViewPort.MinDepth = 0.f;
	m_ViewPort.MaxDepth = 1.f;
	m_ViewPort.TopLeftX = 0;
	m_ViewPort.TopLeftY = 0;
	m_ImmediateContext->RSSetViewports(1, &m_ViewPort);


	D3D11_RASTERIZER_DESC rd;
	ZeroMemory(&rd, sizeof(rd));
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_BACK;
	rd.DepthClipEnable = TRUE;
	rd.MultisampleEnable = FALSE;
	m_D3DDevice->CreateRasterizerState(&rd, &m_RasterizerStateCull[RSE_BACK]);
	rd.CullMode = D3D11_CULL_NONE;
	m_D3DDevice->CreateRasterizerState(&rd, &m_RasterizerStateCull[RSE_NONE]);
	rd.CullMode = D3D11_CULL_FRONT;
	m_D3DDevice->CreateRasterizerState(&rd, &m_RasterizerStateCull[RSE_FRONT]);

	m_ImmediateContext->RSSetState(m_RasterizerStateCull[RSE_BACK]);


	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 1;

	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	m_D3DDevice->CreateSamplerState(&samplerDesc, &m_SamplerState[SSE_CLAMP_POINT]);
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	m_D3DDevice->CreateSamplerState(&samplerDesc, &m_SamplerState[SSE_CLAMP_LINEAR]);

	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	m_D3DDevice->CreateSamplerState(&samplerDesc, &m_SamplerState[SSE_WRAP_POINT]);
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	m_D3DDevice->CreateSamplerState(&samplerDesc, &m_SamplerState[SSE_WRAP_LINEAR]);


	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };

	m_D3DDevice->CreateBlendState(&blendDesc, &m_BlendState[BSE_BLENDALPHA]);

	blendDesc.RenderTarget[0].BlendEnable = FALSE;

	m_D3DDevice->CreateBlendState(&blendDesc, &m_BlendState[BSE_BLENDOFF]);

	m_ImmediateContext->OMSetBlendState(m_BlendState[BSE_BLENDALPHA], blendFactor, 0xffffffff);


	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.StencilEnable = FALSE;
	m_D3DDevice->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilStateTestWrite[DSSE_ALLDISABLE]);

	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	m_D3DDevice->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilStateTestWrite[DSSE_TESTENABLEWRITEENABLE]);

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_D3DDevice->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilStateTestWrite[DSSE_TESTENABLEWRITEDISABLE]);

	m_ImmediateContext->OMSetDepthStencilState(m_DepthStencilStateTestWrite[DSSE_TESTENABLEWRITEENABLE], NULL);


	m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_WHITE]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_WHITE);
	m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_BLACK]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_BLACK);
	m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_RED]		= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_RED);
	m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_GREEN]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_GREEN);
	m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_BLUE]		= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_BLUE);
	m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_BUMP]		= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_BUMP);
	m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_PROPERTY]	= CTextureManager::LoadTexture2D(ENGINE_TEXTURE2D_DEFAULT_PROPERTY);


	m_DeferredBuffer = new CDeferredBuffer();
	m_DeferredBuffer->Initialize(ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT);

	m_DeferredResolve = new CScreenPolygon2D(ENGINE_SHADER_SCREEN_POLYGON_2D_VS, ENGINE_SHADER_GBUFFER_RESOLVE_PS);
	m_DeferredResolve->Init();
	m_PostEffect = new CScreenPolygon2D(ENGINE_SHADER_SCREEN_POLYGON_2D_VS, ENGINE_SHADER_POST_EFFECT_EXPOSURE_PS);
	m_PostEffect->Init();
}
void CRenderDevice::Uninit()
{
	m_DeferredResolve->Uninit();
	delete m_DeferredResolve;
	m_PostEffect->Uninit();
	delete m_PostEffect;
	m_DeferredBuffer->ShutDown();
	delete m_DeferredBuffer;

	for (INT i = 0; i < SSE_COUNT; ++i)
	{
		m_SamplerState[i]->Release();
	}
	for (INT i = 0; i < BSE_COUNT; ++i)
	{
		m_BlendState[i]->Release();
	}
	for (INT i = 0; i < RSE_COUNT; ++i)
	{
		m_RasterizerStateCull[i]->Release();
	}
	for (INT i = 0; i < DSSE_COUNT; ++i)
	{
		m_DepthStencilStateTestWrite[i]->Release();
	}

	if (m_ImmediateContext)
		m_ImmediateContext->ClearState();
	if (m_RenderTargetView)
		m_RenderTargetView->Release();
	if (m_DepthStencilView)
		m_DepthStencilView->Release();
	if (m_SwapChain)
		m_SwapChain->Release();
	if (m_ImmediateContext)
		m_ImmediateContext->Release();
	if (m_D3DDevice)
		m_D3DDevice->Release();
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
void CRenderDevice::BeginFinal()
{
	FLOAT ClearColor[4] = { 0.f, 0.f, 0.f, 1.f };
	m_ImmediateContext->ClearRenderTargetView(m_RenderTargetView, ClearColor);
	m_ImmediateContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	CRenderDevice::SetDepthState(CRenderDevice::DSSE_ALLDISABLE);
	CRenderDevice::SetBlendState(CRenderDevice::BSE_BLENDOFF);
	m_ImmediateContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);
	m_ImmediateContext->RSSetViewports(1, &m_ViewPort);
	CRenderDevice::BindTexture(m_DeferredBuffer->GetRenderTargetShaderResourceView(CDeferredBuffer::DEFERREDBUFFER_EXTRA), ENGINE_SRV_CAMERA_COLOR);
	CRenderDevice::BindTexture(m_DeferredBuffer->GetDepthStencilShaderResourceView(CDeferredBuffer::DEPTHSTENCILBUFFER_CAMERA), ENGINE_SRV_CAMERA_DEPTH);
}
void CRenderDevice::EndFinal()
{
	m_SwapChain->Present(1, 0);
}
void CRenderDevice::SetBlendState(BlendStateEnum bse)
{
	FLOAT blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
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
void CRenderDevice::SetShadowMap(UINT Slot)
{
	ID3D11ShaderResourceView* srv[1] = { m_DeferredBuffer->GetDepthStencilShaderResourceView(CDeferredBuffer::DEPTHSTENCILBUFFER_LIGHT) };
	m_ImmediateContext->PSSetShaderResources(Slot, 1, srv);
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
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.pSysMem = ptrData;

	HRESULT hr = m_D3DDevice->CreateBuffer(&bd, &sd, ptrBuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return false;
	return true;
}
BOOL CRenderDevice::LoadVertexShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout, const std::vector<D3D11_INPUT_ELEMENT_DESC>* layout)
{
	FILE* file;
	LONG fsize;
	BYTE* buffer;
	{
		fopen_s(&file, name.c_str(), "rb");
		if (file == NULL)
			return false;
		fsize = _filelength(_fileno(file));
		buffer = new BYTE[fsize];
		fread_s(buffer, fsize, fsize, 1, file);
		fclose(file);
	}

	{
		HRESULT hr = CRenderDevice::GetDevice()->CreateVertexShader(static_cast<void*>(buffer), fsize, NULL, vertexShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			return false;
		}
	}

	{
		UINT numElements = 5u;
		std::vector<D3D11_INPUT_ELEMENT_DESC> tempLayout;
		if (layout == NULL)
		{
			tempLayout.resize(5);
			tempLayout[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
			tempLayout[1] = { "NORMAL",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
			tempLayout[2] = { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
			tempLayout[3] = { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
			tempLayout[4] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
		}
		else
		{
			numElements = static_cast<UINT>(layout->size());
			tempLayout.resize(layout->size());
			for (UINT i = 0u; i < numElements; i++)
				tempLayout[i] = (*layout)[i];
		}
		
		HRESULT hr = CRenderDevice::GetDevice()->CreateInputLayout(tempLayout.data(), numElements, static_cast<void*>(buffer), fsize, inputLayout.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			return false;
		}
	}

	delete[]buffer;
	return true;
}
BOOL CRenderDevice::LoadPixelShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader)
{
	FILE* file;
	LONG fsize;
	BYTE* buffer;
	{
		fopen_s(&file, name.c_str(), "rb");
		if (file == NULL)
			return false;
		fsize = _filelength(_fileno(file));
		buffer = new BYTE[fsize];
		fread_s(buffer, fsize, fsize, 1, file);
		fclose(file);
	}

	{
		HRESULT hr = CRenderDevice::GetDevice()->CreatePixelShader(static_cast<void*>(buffer), fsize, NULL, pixelShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			return false;
		}
	}

	delete[] buffer;
	return true;
}
BOOL CRenderDevice::CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& ptrBuffer, const UINT& sizeData, D3D11_USAGE usage)
{
	D3D11_BUFFER_DESC hBufferDesc;
	hBufferDesc.Usage = usage;
	hBufferDesc.ByteWidth = sizeData;
	hBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hBufferDesc.CPUAccessFlags = 0;
	hBufferDesc.MiscFlags = 0;
	hBufferDesc.StructureByteStride = sizeof(FLOAT);

	HRESULT hr = m_D3DDevice->CreateBuffer(&hBufferDesc, NULL, ptrBuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return false;
	return true;
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
void CRenderDevice::BindVertexBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot, const UINT& stride, const UINT& offset)
{
	m_ImmediateContext->IASetVertexBuffers(startSlot, 1u, ptrBuffer.GetAddressOf(), &stride, &offset);
}
void CRenderDevice::BindIndexBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, DXGI_FORMAT format, const UINT& offset)
{
	m_ImmediateContext->IASetIndexBuffer(ptrBuffer.Get(), format, offset);
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
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;	//D3D11_RESOURCE_MISC_FLAG

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = ptrData;
	initData.SysMemPitch = memPitch;
	initData.SysMemSlicePitch = memSlicePitch;

	HRESULT hr = m_D3DDevice->CreateTexture2D(&desc, &initData, ptrTexture.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return false;
	return true;
}
BOOL CRenderDevice::CreateTexture2DShaderResourceView(Microsoft::WRL::ComPtr<ID3D11Texture2D> ptrTexture, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& ptrSRV, DXGI_FORMAT format, D3D_SRV_DIMENSION viewDimension, const UINT& mipLevels, const UINT& mostDetailedMip)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = viewDimension;
	srvDesc.Texture2D.MipLevels = mipLevels;
	srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;

	HRESULT hr = m_D3DDevice->CreateShaderResourceView(ptrTexture.Get(), &srvDesc, ptrSRV.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return false;
	return true;
}