#include "../../../../../Entry/EngineMain.h"
#include "../Headers/CDeferredBuffer.h"
#include "../Headers/CRenderDevice.h"

CDeferredBuffer::CDeferredBuffer()
{
	for (INT i = 0; i < DEFERREDBUFFER_COUNT; ++i)
	{
		m_RenderTargetViewArray[i] = NULL;
		m_RenderTargetTextureArray[i] = NULL;
		m_RT_SRV[i] = NULL;
	}

	for (INT i = 0; i < DEPTHSTENCILBUFFER_COUNT; ++i)
	{
		m_DepthStencilView[i] = NULL;
		m_DepthStencilTexture[i] = NULL;
		m_DS_SRV[i] = NULL;
	}

	m_TextureWidth = 0;
	m_TextureHeight = 0;
	ZeroMemory(&m_ViewPort, sizeof(D3D11_VIEWPORT));
}
CDeferredBuffer::~CDeferredBuffer()
{
	for (INT i = 0; i < DEFERREDBUFFER_COUNT; ++i)
	{
		if (m_RenderTargetViewArray[i])
		{
			m_RenderTargetViewArray[i]->Release();
			m_RenderTargetViewArray[i] = NULL;
		}
		if (m_RenderTargetTextureArray[i])
		{
			m_RenderTargetTextureArray[i]->Release();
			m_RenderTargetTextureArray[i] = NULL;
		}
		if (m_RT_SRV[i])
		{
			m_RT_SRV[i]->Release();
			m_RT_SRV[i] = NULL;
		}
	}
	for (INT i = 0; i < DEPTHSTENCILBUFFER_COUNT; ++i)
	{
		if (m_DepthStencilView[i])
		{
			m_DepthStencilView[i]->Release();
			m_DepthStencilView[i] = NULL;
		}
		if (m_DepthStencilTexture[i])
		{
			m_DepthStencilTexture[i]->Release();
			m_DepthStencilTexture[i] = NULL;
		}
		if (m_DS_SRV[i])
		{
			m_DS_SRV[i]->Release();
			m_DS_SRV[i] = NULL;
		}
	}
}
BOOL CDeferredBuffer::Initialize(INT iTextureWidth, INT iTexureHeight)
{
	m_TextureWidth = iTextureWidth;
	m_TextureHeight = iTexureHeight;
	HRESULT hr;

	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = iTextureWidth;
	textureDesc.Height = iTexureHeight;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	for (INT i = 0; i < DEFERREDBUFFER_COUNT; ++i)
	{
		hr = CRenderDevice::GetDevice()->CreateTexture2D(&textureDesc, NULL, &m_RenderTargetTextureArray[i]);
		if (FAILED(hr))return false;
	}
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	ZeroMemory(&renderTargetViewDesc, sizeof(renderTargetViewDesc));
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;
	for (INT i = 0; i < DEFERREDBUFFER_COUNT; ++i)
	{
		hr = CRenderDevice::GetDevice()->CreateRenderTargetView(m_RenderTargetTextureArray[i], &renderTargetViewDesc, &m_RenderTargetViewArray[i]);
		if (FAILED(hr))return false;
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;
	for (INT i = 0; i < DEFERREDBUFFER_COUNT; ++i)
	{
		hr = CRenderDevice::GetDevice()->CreateShaderResourceView(m_RenderTargetTextureArray[i], &shaderResourceViewDesc, &m_RT_SRV[i]);
		if (FAILED(hr))return false;
	}
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.Width = iTextureWidth;
	depthStencilDesc.Height = iTexureHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;
	for (INT i = 0; i < DEPTHSTENCILBUFFER_COUNT; ++i)
	{
		hr = CRenderDevice::GetDevice()->CreateTexture2D(&depthStencilDesc, NULL, &m_DepthStencilTexture[i]);
		if (FAILED(hr))return false;
	}
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	for (INT i = 0; i < DEPTHSTENCILBUFFER_COUNT; ++i)
	{
		hr = CRenderDevice::GetDevice()->CreateDepthStencilView(m_DepthStencilTexture[i], &depthStencilViewDesc, &m_DepthStencilView[i]);
		if (FAILED(hr))return false;
	}
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = depthStencilDesc.MipLevels;
	for (INT i = 0; i < DEPTHSTENCILBUFFER_COUNT; ++i)
	{
		hr = CRenderDevice::GetDevice()->CreateShaderResourceView(m_DepthStencilTexture[i], &shaderResourceViewDesc, &m_DS_SRV[i]);
		if (FAILED(hr))return false;
	}
	m_ViewPort.Width = static_cast<FLOAT>(iTextureWidth);
	m_ViewPort.Height = static_cast<FLOAT>(iTexureHeight);
	m_ViewPort.MinDepth = 0.0f;
	m_ViewPort.MaxDepth = 1.0f;
	m_ViewPort.TopLeftX = 0.0f;
	m_ViewPort.TopLeftY = 0.0f;

	return true;
}
void CDeferredBuffer::ShutDown()
{
	for (INT i = 0; i < DEFERREDBUFFER_COUNT; ++i)
	{
		if (m_RenderTargetViewArray[i])
		{
			m_RenderTargetViewArray[i]->Release();
			m_RenderTargetViewArray[i] = NULL;
		}
		if (m_RenderTargetTextureArray[i])
		{
			m_RenderTargetTextureArray[i]->Release();
			m_RenderTargetTextureArray[i] = NULL;
		}
		if (m_RT_SRV[i])
		{
			m_RT_SRV[i]->Release();
			m_RT_SRV[i] = NULL;
		}
	}
	for (INT i = 0; i < DEPTHSTENCILBUFFER_COUNT; ++i)
	{
		if (m_DepthStencilView[i])
		{
			m_DepthStencilView[i]->Release();
			m_DepthStencilView[i] = NULL;
		}
		if (m_DepthStencilTexture[i])
		{
			m_DepthStencilTexture[i]->Release();
			m_DepthStencilTexture[i] = NULL;
		}
		if (m_DS_SRV[i])
		{
			m_DS_SRV[i]->Release();
			m_DS_SRV[i] = NULL;
		}
	}
}
void CDeferredBuffer::ClearRenderTarget(FLOAT R, FLOAT G, FLOAT B, FLOAT A)
{
	FLOAT color[4] = { R,G,B,A };
	for (INT i = 0; i < DEFERREDBUFFER_COUNT; ++i)
		CRenderDevice::GetDeviceContext()->ClearRenderTargetView(m_RenderTargetViewArray[i], color);
	for (INT i = 0; i < DEPTHSTENCILBUFFER_COUNT; ++i)
		CRenderDevice::GetDeviceContext()->ClearDepthStencilView(m_DepthStencilView[i], D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}
void CDeferredBuffer::SetDeferredRenderTarget()
{
	ID3D11RenderTargetView* rtv[4] = {
		m_RenderTargetViewArray[DEFERREDBUFFER_WORLDNORMAL],
		m_RenderTargetViewArray[DEFERREDBUFFER_ALBEDO],
		m_RenderTargetViewArray[DEFERREDBUFFER_PROPERTY],
		m_RenderTargetViewArray[DEFERREDBUFFER_ID] };
	CRenderDevice::GetDeviceContext()->OMSetRenderTargets(DEFERREDBUFFER_COUNT - 1, rtv, m_DepthStencilView[DEPTHSTENCILBUFFER_CAMERA]);
	CRenderDevice::GetDeviceContext()->RSSetViewports(1, &m_ViewPort);
}
void CDeferredBuffer::SetExtraRenderTarget()
{
	CRenderDevice::GetDeviceContext()->OMSetRenderTargets(1, &m_RenderTargetViewArray[DEFERREDBUFFER_EXTRA], m_DepthStencilView[DEPTHSTENCILBUFFER_EXTRA]);
	CRenderDevice::GetDeviceContext()->RSSetViewports(1, &m_ViewPort);
}
void CDeferredBuffer::SetRenderTarget(DeferredBufferEnum rtv, DepthStencilBufferEnum dsv)
{
	CRenderDevice::GetDeviceContext()->OMSetRenderTargets(1, &m_RenderTargetViewArray[rtv], m_DepthStencilView[dsv]);
	CRenderDevice::GetDeviceContext()->RSSetViewports(1, &m_ViewPort);
}
ID3D11ShaderResourceView* CDeferredBuffer::GetRenderTargetShaderResourceView(DeferredBufferEnum idx)
{
	return m_RT_SRV[idx];
}
void CDeferredBuffer::SetDepthStencilRenderTarget(DepthStencilBufferEnum idx, ID3D11RenderTargetView* rtv)
{
	CRenderDevice::GetDeviceContext()->OMSetRenderTargets(1, &rtv, m_DepthStencilView[idx]);
	CRenderDevice::GetDeviceContext()->RSSetViewports(1, &m_ViewPort);
}
ID3D11ShaderResourceView* CDeferredBuffer::GetDepthStencilShaderResourceView(DepthStencilBufferEnum idx)
{
	return m_DS_SRV[idx];
}