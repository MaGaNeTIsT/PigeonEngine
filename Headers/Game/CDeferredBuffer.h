#pragma once

class DeferredBuffer
{
public:
	enum DeferredBufferEnum
	{
		DEFERREDBUFFER_WORLDPOSITION	= 0,
		DEFERREDBUFFER_WORLDNORMAL		= 1,
		DEFERREDBUFFER_DIFFUSE			= 2,
		DEFERREDBUFFER_SPECULAR			= 3,
		DEFERREDBUFFER_GBUFFER			= 4,
		DEFERREDBUFFER_COUNT
	};
	enum DepthStencilBufferEnum
	{
		DEPTHSTENCILBUFFER_CAMERA	= 0,
		DEPTHSTENCILBUFFER_LIGHT	= 1,
		DEPTHSTENCILBUFFER_COUNT
	};
public:
	BOOL						Initialize(INT iTextureWidth, INT iTexureHeight);
	void						ShutDown();
public:
	void						ClearRenderTarget(FLOAT R, FLOAT G, FLOAT B, FLOAT A);
	void						SetDeferredRenderTarget();
	void						SetGBufferRenderTarget();
	ID3D11ShaderResourceView*	GetDeferredShaderResourceView(DeferredBufferEnum idx);
	void						SetDepthStencilRenderTarget(DepthStencilBufferEnum idx, ID3D11RenderTargetView* rtv = NULL);
	ID3D11ShaderResourceView*	GetDepthStencilShaderResourceView(DepthStencilBufferEnum idx);
private:
	ID3D11RenderTargetView*		mRenderTargetViewArray[DEFERREDBUFFER_COUNT];
	ID3D11Texture2D*			mRenderTargetTextureArray[DEFERREDBUFFER_COUNT];
	ID3D11ShaderResourceView*	mRT_SRV[DEFERREDBUFFER_COUNT];

	ID3D11DepthStencilView*		mDepthStencilView[DEPTHSTENCILBUFFER_COUNT];
	ID3D11Texture2D*			mDepthStencilTexture[DEPTHSTENCILBUFFER_COUNT];
	ID3D11ShaderResourceView*	mDS_SRV[DEPTHSTENCILBUFFER_COUNT];
private:
	INT							mTextureWidth;
	INT							mTextureHeight;
	D3D11_VIEWPORT				mViewPort;
public:
	DeferredBuffer();
	~DeferredBuffer();
};