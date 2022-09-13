#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "./CStructCommon.h"

class CTexture2D;
class CDeferredBuffer;
class CScreenPolygon2D;

class CRenderDevice
{
public:
	enum EngineDefaultTexture2DEnum
	{
		ENGINE_DEFAULT_TEXTURE2D_WHITE		= 0,
		ENGINE_DEFAULT_TEXTURE2D_BLACK		= 1,
		ENGINE_DEFAULT_TEXTURE2D_GRAY		= 2,
		ENGINE_DEFAULT_TEXTURE2D_RED		= 3,
		ENGINE_DEFAULT_TEXTURE2D_GREEN		= 4,
		ENGINE_DEFAULT_TEXTURE2D_BLUE		= 5,
		ENGINE_DEFAULT_TEXTURE2D_BUMP		= 6,
		ENGINE_DEFAULT_TEXTURE2D_PROPERTY	= 7,
		ENGINE_DEFAULT_TEXTURE2D_COUNT
	};
public:
	struct RenderTexture2DViewInfo
	{
		RenderTexture2DViewInfo() { ::ZeroMemory(this, sizeof(*this)); }
		void Release()
		{
			if (Texture2D)
			{
				Texture2D->Release();
				Texture2D = nullptr;
			}
			if (RenderTargetView)
			{
				RenderTargetView->Release();
				RenderTargetView = nullptr;
			}
			if (UnorderedAccessView)
			{
				UnorderedAccessView->Release();
				UnorderedAccessView = nullptr;
			}
			if (ShaderResourceView)
			{
				ShaderResourceView->Release();
				ShaderResourceView = nullptr;
			}
			if (DepthStencilView)
			{
				DepthStencilView->Release();
				DepthStencilView = nullptr;
			}
		};
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Texture2D;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		RenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>		DepthStencilView;
	};
	struct Texture2DViewInfo
	{
		Texture2DViewInfo() { ::ZeroMemory(this, sizeof(*this)); }
		void Release()
		{
			if (Texture2D)
			{
				Texture2D->Release();
				Texture2D = nullptr;
			}
			if (UnorderedAccessView)
			{
				UnorderedAccessView->Release();
				UnorderedAccessView = nullptr;
			}
			if (ShaderResourceView)
			{
				ShaderResourceView->Release();
				ShaderResourceView = nullptr;
			}
		};
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Texture2D;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
	};
public:
	static void				Initialize();
	static void				ShutDown();
public:
	void					Init(HWND hWnd, const CustomType::Vector2Int& bufferSize, const UINT& bufferDepth = 24u, const UINT& frameNum = 60u, const BOOL& windowed = TRUE);
	void					Uninit();
public:
	static BOOL				LoadVertexShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout, const std::vector<D3D11_INPUT_ELEMENT_DESC>* layout = NULL);
	static BOOL				LoadPixelShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader);
	static BOOL				LoadComputeShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11ComputeShader>& computeShader);
	static BOOL				CreateBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const CustomStruct::CRenderBufferDesc& bufferDesc, const CustomStruct::CRenderSubresourceData* subData = NULL);
	static void				UploadBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& dstResource, const void* srcData, UINT srcRowPitch = 0u, UINT srcDepthPitch = 0u, UINT dstSubresource = 0u, const D3D11_BOX* dstBox = NULL);
	static BOOL				CreateRenderTexture2D(RenderTexture2DViewInfo& output, const CustomStruct::CRenderTextureDesc& textureDesc, const BOOL& randomWrite = FALSE);
	static BOOL				CreateTexture2D(Texture2DViewInfo& output, const CustomStruct::CRenderTextureDesc& textureDesc);
public:
	static void				Present(const UINT& syncInterval = 0u);
	static void				SetDepthStencilState(const Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const UINT& stencilRef = 0x0u);
	static void				SetBlendState(const Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const CustomStruct::CColor& blendFactor = CustomStruct::CColor(0.f, 0.f, 0.f, 0.f), const UINT& sampleMask = 0xffffffff);
	static void				SetRasterizerState(const Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs);
	static void				BindVSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot);
	static void				BindPSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot);
	static void				BindCSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot);
public:
	static BOOL				CreateDepthStencilState(Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const CustomStruct::CRenderDepthState& depthState, const CustomStruct::CRenderStencilState* stencilState = NULL);
	static BOOL				CreateBlendState(Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const std::vector<CustomStruct::CRenderBlendState>& blendStates);
	static BOOL				CreateRasterizerState(Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs, const CustomStruct::CRenderRasterizerState& rasterizerState);
	static BOOL				CreateSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const CustomStruct::CRenderSamplerState& samplerState);
public:
	static D3D11_VIEWPORT	GetViewport();
private:
	static void				TranslateBindFlag(UINT& output, CustomStruct::CRenderBindFlag input);
	static void				TranslateUsage(D3D11_USAGE& output, CustomStruct::CRenderUsage input);
	static void				TranslateCPUAccessFlag(UINT& output, CustomStruct::CRenderCPUAccessFlag input);
	static void				TranslateResourceMiscFlag(UINT& output, CustomStruct::CRenderResourceMiscFlag input);
	static void				TranslateResourceFormat(DXGI_FORMAT& output, CustomStruct::CRenderFormat input);
	static void				TranslateComparisonFunction(D3D11_COMPARISON_FUNC& output, CustomStruct::CRenderComparisonFunction input);
	static void				TranslateStencilOperation(D3D11_DEPTH_STENCILOP_DESC& output, const CustomStruct::CRenderStencilOp& input);
	static void				TranslateDepthStencilState(D3D11_DEPTH_STENCIL_DESC& output, const CustomStruct::CRenderDepthState& depthInput, const CustomStruct::CRenderStencilState* stencilInput = NULL);
	static void				TranslateBlendState(D3D11_RENDER_TARGET_BLEND_DESC& output, const CustomStruct::CRenderBlendState& input);
	static void				TranslateRasterizerState(D3D11_CULL_MODE& outputCull, D3D11_FILL_MODE& outputFill, const CustomStruct::CRenderRasterizerState& input);
	static void				TranslateSamplerState(D3D11_SAMPLER_DESC& output, const CustomStruct::CRenderSamplerState& input);
//public:
	//static void		ResetRenderTarget();
	//static void		BeginShadow();
	//static void		EndShadow();
	//static void		BeginDeferred();
	//static void		EndDeferred();
	//static void		BeginDeferredResolve();
	//static void		EndDeferredResolve();
	//static void		BeginForward();
	//static void		EndForward();
	//static void		SetOutputRTV();
	//static void		Present();
//public:
	//static void		SetBlendState(BlendStateEnum bse);
	//static void		SetDepthState(DepthStencilStateEnum dsse);
	//static void		SetRasterizerState(RasterizerStateEnum rse);
	//static void		BindTexture(CTexture2D* ptrTexture, const UINT& startSlot);
	//static void		BindTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ptrSRV, const UINT& startSlot);
	//static void		BindTexture(ID3D11ShaderResourceView* ptrSRV, const UINT& startSlot);
	//static void		BindComputeShaderResourceView(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ptrSRV, const UINT& startSlot);
	//static void		BindComputeUnorderedAccessView(Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> ptrUAV, const UINT& startSlot);
	//static void		SetShadowMap(const UINT& Slot);
	//static void		DrawIndexed(const UINT& indexCount, const UINT& startIndexLocation = 0u, const INT& baseVertexLocation = 0);
//public:
	//static BOOL		CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& ptrBuffer, const UINT& sizeData, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);
	//static void		UploadConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const void* ptrData);
	//static void		BindConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot);
	//static void		BindComputeShaderConstantBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot);
	//static void		BindVertexBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, const UINT& startSlot = 0u, const UINT& stride = sizeof(CustomStruct::CVertex3D), const UINT& offset = 0u);
	//static void		BindIndexBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> ptrBuffer, DXGI_FORMAT format = DXGI_FORMAT_R32_UINT, const UINT& offset = 0u);
//public:
	//static BOOL		CreateRenderTexture2D(RenderTexture2DViewInfo& textureInfo, const UINT& width, const UINT& height, DXGI_FORMAT format, const BOOL& randomWirte = FALSE, D3D11_USAGE usage = D3D11_USAGE_DEFAULT, const UINT& mipLevels = 1u, const UINT& arraySize = 1u);
	//static BOOL		CreateTexture2D(Microsoft::WRL::ComPtr<ID3D11Texture2D>& ptrTexture, const UINT& width, const UINT& height, DXGI_FORMAT format, const void* ptrData, const UINT& memPitch, const UINT& memSlicePitch, UINT flag = D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE usage = D3D11_USAGE_DEFAULT, const UINT& mipLevels = 1u, const UINT& arraySize = 1u);
	//static BOOL		CreateTexture2DShaderResourceView(Microsoft::WRL::ComPtr<ID3D11Texture2D> ptrTexture, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& ptrSRV, DXGI_FORMAT format, D3D_SRV_DIMENSION viewDimension, const UINT& mipLevels = 1u, const UINT& mostDetailedMip = 0u);
//public:
	//static CTexture2D*				GetEngineDefaultTexture2D(EngineDefaultTexture2DEnum idx) { return m_EngineDefaultTexture2D[idx]; }
//public:
	//static ID3D11SamplerState*		GetSamplerState(SamplerStateEnum idx) { return m_SamplerState[idx]; }
//private:
	//static CTexture2D*				m_EngineDefaultTexture2D[ENGINE_DEFAULT_TEXTURE2D_COUNT];
//private:
	//static CDeferredBuffer*			m_DeferredBuffer;
	//static CScreenPolygon2D*		m_PostEffect;
	//static CScreenPolygon2D*		m_DeferredResolve;
//private:
	//static ID3D11SamplerState*		m_SamplerState[SSE_COUNT];
	//static ID3D11BlendState*		m_BlendState[BSE_COUNT];
	//static ID3D11RasterizerState*	m_RasterizerStateCull[RSE_COUNT];
	//static ID3D11DepthStencilState* m_DepthStencilStateTestWrite[DSSE_COUNT];
private:
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		m_RenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>		m_DepthStencilView;
	D3D11_VIEWPORT										m_Viewport;
private:
	Microsoft::WRL::ComPtr<IDXGISwapChain>				m_SwapChain;
	D3D_FEATURE_LEVEL									m_FeatureLevel;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>			m_ImmediateContext;
	Microsoft::WRL::ComPtr<ID3D11Device>				m_Device;
private:
	CRenderDevice() { ::ZeroMemory(this, sizeof(*this)); }
	CRenderDevice(const CRenderDevice&) { ::ZeroMemory(this, sizeof(*this)); }
	~CRenderDevice() {}
private:
	static CRenderDevice* m_RenderDevice;
};