#pragma once

#include "./CStructCommon.h"

class CVertexBuffer;
class CIndexBuffer;
class CTexture;
class DeferredBuffer;
class CPolygon2D;

class CRenderDevice
{
public:
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
private:
	static DeferredBuffer*				m_DeferredBuffer;
	static CPolygon2D*					m_DeferredQuadBefore;
	static CPolygon2D*					m_DeferredQuadAfter;

	static D3D_FEATURE_LEVEL			m_FeatureLevel;

	static ID3D11Device*				m_D3DDevice;
	static ID3D11DeviceContext*			m_ImmediateContext;
	static IDXGISwapChain*				m_SwapChain;
	static ID3D11RenderTargetView*		m_RenderTargetView;
	static ID3D11DepthStencilView*		m_DepthStencilView;
	static D3D11_VIEWPORT				m_ViewPort;

	static ID3D11BlendState*			m_BlendState[BSE_COUNT];
	static ID3D11RasterizerState*		m_RasterizerStateCull[RSE_COUNT];
	static ID3D11DepthStencilState*		m_DepthStencilStateTestWrite[DSSE_COUNT];
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
	static void		SetVertexBuffers(ID3D11Buffer* VertexBuffer, UINT stride = sizeof(CustomStruct::CVertex3DData));
	static void		SetIndexBuffer(ID3D11Buffer* IndexBuffer);
	static void		SetTexture(CTexture* Texture, UINT Slot = 0);
	static void		SetTexture(ID3D11ShaderResourceView* SRV, UINT Slot = 0);
	static void		SetShadowMap(UINT Slot = 0);
	static void		DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

	static ID3D11Device*			GetDevice(void) { return m_D3DDevice; }
	static ID3D11DeviceContext*		GetDeviceContext(void) { return m_ImmediateContext; }

	static DeferredBuffer*			GetDeferredBuffer() { return m_DeferredBuffer; }
	static CPolygon2D*				GetDeferredQuadBefore() { return m_DeferredQuadBefore; }
	static CPolygon2D*				GetDeferredQuadAfter() { return m_DeferredQuadAfter; }
};