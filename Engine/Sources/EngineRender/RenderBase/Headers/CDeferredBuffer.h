#pragma once

class CDeferredBuffer
{
public:
	enum DeferredBufferIndex
	{
		DEFERREDBUFFER_WORLDNORMAL		= 0,
		DEFERREDBUFFER_ALBEDO			= 1,
		DEFERREDBUFFER_PROPERTY			= 2,
		DEFERREDBUFFER_ID				= 3,
		DEFERREDBUFFER_EXTRA			= 4,
		DEFERREDBUFFER_COUNT
	};
	enum DepthStencilBufferEnum
	{
		DEPTHSTENCILBUFFER_CAMERA		= 0,
		DEPTHSTENCILBUFFER_LIGHT		= 1,
		DEPTHSTENCILBUFFER_EXTRA		= 2,
		DEPTHSTENCILBUFFER_COUNT
	};
public:
	BOOL						Initialize(INT iTextureWidth, INT iTexureHeight);
	void						ShutDown();
public:
	void						ClearRenderTarget(FLOAT R, FLOAT G, FLOAT B, FLOAT A);
	void						SetDeferredRenderTarget();
	void						SetExtraRenderTarget();
	void						SetRenderTarget(DeferredBufferEnum rtv, DepthStencilBufferEnum dsv);
	ID3D11ShaderResourceView*	GetRenderTargetShaderResourceView(DeferredBufferEnum idx);
	void						SetDepthStencilRenderTarget(DepthStencilBufferEnum idx, ID3D11RenderTargetView* rtv = NULL);
	ID3D11ShaderResourceView*	GetDepthStencilShaderResourceView(DepthStencilBufferEnum idx);
private:
	ID3D11RenderTargetView*		m_RenderTargetViewArray[DEFERREDBUFFER_COUNT];
	ID3D11Texture2D*			m_RenderTargetTextureArray[DEFERREDBUFFER_COUNT];
	ID3D11ShaderResourceView*	m_RT_SRV[DEFERREDBUFFER_COUNT];

	ID3D11DepthStencilView*		m_DepthStencilView[DEPTHSTENCILBUFFER_COUNT];
	ID3D11Texture2D*			m_DepthStencilTexture[DEPTHSTENCILBUFFER_COUNT];
	ID3D11ShaderResourceView*	m_DS_SRV[DEPTHSTENCILBUFFER_COUNT];
private:
	INT							m_TextureWidth;
	INT							m_TextureHeight;
	D3D11_VIEWPORT				m_ViewPort;
public:
	CDeferredBuffer();
	~CDeferredBuffer();
};