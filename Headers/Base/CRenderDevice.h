#pragma once

#include "../../Entry/MyMain.h"
#include "./CBaseType.h"
#include "./CStructCommon.h"

class CTexture2D;
class CDeferredBuffer;
class CScreenPolygon2D;

class CRenderDevice
{
public:
	enum EngineDefaultTexture2DEnum
	{
		ENGINE_DEFAULT_TEXTURE2D_WHITE,
		ENGINE_DEFAULT_TEXTURE2D_BLACK,
		ENGINE_DEFAULT_TEXTURE2D_RED,
		ENGINE_DEFAULT_TEXTURE2D_GREEN,
		ENGINE_DEFAULT_TEXTURE2D_BLUE,
		ENGINE_DEFAULT_TEXTURE2D_BUMP,
		ENGINE_DEFAULT_TEXTURE2D_PROPERTY,
		ENGINE_DEFAULT_TEXTURE2D_COUNT
	};
	enum SamplerStateEnum
	{
		SSE_CLAMP_POINT		= 0,
		SSE_CLAMP_LINEAR	= 1,
		SSE_WRAP_POINT		= 2,
		SSE_WRAP_LINEAR		= 3,
		SSE_COUNT
	};
	enum BlendStateEnum
	{
		BSE_BLENDOFF		= 0,
		BSE_BLENDALPHA		= 1,
		BSE_COUNT
	};
	enum RasterizerStateEnum
	{
		RSE_NONE			= 0,
		RSE_BACK			= 1,
		RSE_FRONT			= 2,
		RSE_COUNT
	};
	enum DepthStencilStateEnum
	{
		DSSE_ALLDISABLE					= 0,
		DSSE_TESTENABLEWRITEENABLE		= 1,
		DSSE_TESTENABLEWRITEDISABLE		= 2,
		DSSE_COUNT
	};
public:
	static void		Init();
	static void		Uninit();
	static void		ClrShadowDeferred();
	static void		BeginShadow();
	static void		BeginDeferred();
	static void		BeginGBuffer();
	static void		Begin();
	static void		End();
public:
	static void		SetBlendState(BlendStateEnum bse);
	static void		SetDepthState(DepthStencilStateEnum dsse);
	static void		SetRasterizerState(RasterizerStateEnum rse);
	static void		BindTexture(CTexture2D* ptrTexture, const UINT& startSlot);
	static void		BindTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ptrSRV, const UINT& startSlot);
	static void		BindTexture(ID3D11ShaderResourceView* ptrSRV, const UINT& startSlot);
	static void		SetShadowMap(UINT Slot = 0);
	static void		DrawIndexed(const UINT& indexCount, const UINT& startIndexLocation = 0u, const INT& baseVertexLocation = 0);
public:
	static BOOL		CreateD3DBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& ptrBuffer, const void* ptrData, const UINT& stride, const UINT& count, UINT flag, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);
	static BOOL		LoadVertexShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout, const std::vector<D3D11_INPUT_ELEMENT_DESC>* layout = NULL);
	static BOOL		LoadPixelShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader);
	static BOOL		CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& ptrBuffer, const UINT& sizeData, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);
	static void		UploadConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const void* ptrData);
	static void		BindConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot);
	static void		BindVertexBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot = 0u, const UINT& stride = sizeof(CustomStruct::CVertex3D), const UINT& offset = 0u);
	static void		BindIndexBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, DXGI_FORMAT format = DXGI_FORMAT_R32_UINT, const UINT& offset = 0u);
public:
	static BOOL		CreateTexture2D(Microsoft::WRL::ComPtr<ID3D11Texture2D>& ptrTexture, const UINT& width, const UINT& height, DXGI_FORMAT format, const void* ptrData, const UINT& memPitch, const UINT& memSlicePitch, UINT flag = D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE usage = D3D11_USAGE_DEFAULT, const UINT& mipLevels = 1u, const UINT& arraySize = 1u);
	static BOOL		CreateTexture2DShaderResourceView(Microsoft::WRL::ComPtr<ID3D11Texture2D> ptrTexture, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& ptrSRV, DXGI_FORMAT format, D3D_SRV_DIMENSION viewDimension, const UINT& mipLevels = 1u, const UINT& mostDetailedMip = 0u);
public:
	static CTexture2D*				GetEngineDefaultTexture2D(EngineDefaultTexture2DEnum idx) { return m_EngineDefaultTexture2D[idx]; }
public:
	static ID3D11SamplerState*		GetSamplerState(SamplerStateEnum idx) { return m_SamplerState[idx]; }
public:
	static ID3D11Device*			GetDevice(void) { return m_D3DDevice; }
	static ID3D11DeviceContext*		GetDeviceContext(void) { return m_ImmediateContext; }

	static CDeferredBuffer*			GetDeferredBuffer() { return m_DeferredBuffer; }
	static CScreenPolygon2D*		GetDeferredResolve() { return m_DeferredResolve; }
	static CScreenPolygon2D*		GetPostEffect() { return m_PostEffect; }
private:
	static CTexture2D*				m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_COUNT];
private:
	static CDeferredBuffer*			m_DeferredBuffer;
	static CScreenPolygon2D*		m_PostEffect;
	static CScreenPolygon2D*		m_DeferredResolve;
private:
	static D3D_FEATURE_LEVEL		m_FeatureLevel;
	static ID3D11Device*			m_D3DDevice;
	static ID3D11DeviceContext*		m_ImmediateContext;
	static IDXGISwapChain*			m_SwapChain;
	static ID3D11RenderTargetView*	m_RenderTargetView;
	static ID3D11DepthStencilView*	m_DepthStencilView;
	static D3D11_VIEWPORT			m_ViewPort;
private:
	static ID3D11SamplerState*		m_SamplerState[SSE_COUNT];
	static ID3D11BlendState*		m_BlendState[BSE_COUNT];
	static ID3D11RasterizerState*	m_RasterizerStateCull[RSE_COUNT];
	static ID3D11DepthStencilState* m_DepthStencilStateTestWrite[DSSE_COUNT];
};