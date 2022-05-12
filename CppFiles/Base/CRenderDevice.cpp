#include "../../Entry/MyMain.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Base/CTexture.h"
#include "../../Headers/Game/CDeferredBuffer.h"
#include "../../Headers/Object/CPolygon2D.h"

DeferredBuffer*				CRenderDevice::m_DeferredBuffer			= NULL;
CPolygon2D*					CRenderDevice::m_DeferredQuadBefore		= NULL;
CPolygon2D*					CRenderDevice::m_DeferredQuadAfter		= NULL;

D3D_FEATURE_LEVEL			CRenderDevice::m_FeatureLevel			= D3D_FEATURE_LEVEL_11_0;

ID3D11Device*				CRenderDevice::m_D3DDevice				= NULL;
ID3D11DeviceContext*		CRenderDevice::m_ImmediateContext		= NULL;
IDXGISwapChain*				CRenderDevice::m_SwapChain				= NULL;
ID3D11RenderTargetView*		CRenderDevice::m_RenderTargetView		= NULL;
ID3D11DepthStencilView*		CRenderDevice::m_DepthStencilView		= NULL;
D3D11_VIEWPORT				CRenderDevice::m_ViewPort;

ID3D11BlendState*			CRenderDevice::m_BlendState[BSE_COUNT];
ID3D11RasterizerState*		CRenderDevice::m_RasterizerStateCull[RSE_COUNT];
ID3D11DepthStencilState*	CRenderDevice::m_DepthStencilStateTestWrite[DSSE_COUNT];

void CRenderDevice::Init()
{
	HRESULT hr = S_OK;

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = SCREEN_WIDTH;
	sd.BufferDesc.Height = SCREEN_HEIGHT;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = GetWindow();
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

	m_ViewPort.Width = static_cast<FLOAT>(SCREEN_WIDTH);
	m_ViewPort.Height = static_cast<FLOAT>(SCREEN_HEIGHT);
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


	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.f;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* samplerState = NULL;
	m_D3DDevice->CreateSamplerState(&samplerDesc, &samplerState);

	m_ImmediateContext->PSSetSamplers(0, 1, &samplerState);

	m_DeferredBuffer = new DeferredBuffer();
	m_DeferredBuffer->Initialize(SCREEN_WIDTH, SCREEN_HEIGHT);

	m_DeferredQuadBefore = new QuadBefore();
	m_DeferredQuadBefore->Init();
	m_DeferredQuadAfter = new QuadAfter();
	m_DeferredQuadAfter->Init();
}

void CRenderDevice::Uninit()
{
	m_DeferredQuadBefore->Uninit();
	delete m_DeferredQuadBefore;
	m_DeferredQuadAfter->Uninit();
	delete m_DeferredQuadAfter;
	m_DeferredBuffer->ShutDown();
	delete m_DeferredBuffer;

	m_DepthStencilView->Release();

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
	if (m_SwapChain)
		m_SwapChain->Release();
	if (m_ImmediateContext)
		m_ImmediateContext->Release();
	if (m_D3DDevice)
		m_D3DDevice->Release();
}

void CRenderDevice::ClrShadowDeferred()
{
	m_DeferredBuffer->ClearRenderTarget(0.f, 0.f, 0.f, 1.f);
}

void CRenderDevice::BeginShadow()
{
	m_DeferredBuffer->SetDepthStencilRenderTarget(DeferredBuffer::DEPTHSTENCILBUFFER_LIGHT, m_RenderTargetView);
}

void CRenderDevice::BeginDeferred()
{
	m_DeferredBuffer->SetDeferredRenderTarget();
}

void CRenderDevice::BeginGBuffer()
{
	m_DeferredBuffer->SetGBufferRenderTarget();
}

void CRenderDevice::Begin()
{
	m_ImmediateContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);
	m_ImmediateContext->RSSetViewports(1, &m_ViewPort);

	FLOAT ClearColor[4] = { 0.f, 0.f, 0.f, 1.f };
	m_ImmediateContext->ClearRenderTargetView(m_RenderTargetView, ClearColor);
	m_ImmediateContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
}

void CRenderDevice::End()
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

void CRenderDevice::SetVertexBuffers( ID3D11Buffer* VertexBuffer, UINT stride )
{
	UINT offset = 0;
	ID3D11Buffer* vb[1] = { VertexBuffer };
	m_ImmediateContext->IASetVertexBuffers( 0, 1, vb, &stride, &offset );
}

void CRenderDevice::SetIndexBuffer(ID3D11Buffer* IndexBuffer)
{
	m_ImmediateContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
}

void CRenderDevice::SetTexture(CTexture* Texture, UINT Slot)
{
	ID3D11ShaderResourceView* srv[1] = { Texture->GetShaderResourceView() };
	m_ImmediateContext->PSSetShaderResources(Slot, 1, srv);
}

void CRenderDevice::SetTexture(ID3D11ShaderResourceView* SRV, UINT Slot)
{
	ID3D11ShaderResourceView* srv[1] = { SRV };
	m_ImmediateContext->PSSetShaderResources(Slot, 1, srv);
}

void CRenderDevice::SetShadowMap(UINT Slot)
{
	ID3D11ShaderResourceView* srv[1] = { m_DeferredBuffer->GetDepthStencilShaderResourceView(DeferredBuffer::DEPTHSTENCILBUFFER_LIGHT) };
	m_ImmediateContext->PSSetShaderResources(Slot, 1, srv);
}

void CRenderDevice::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	m_ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_ImmediateContext->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
}