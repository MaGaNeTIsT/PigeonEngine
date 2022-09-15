#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "./CStructCommon.h"

class CRenderDevice
{
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
			if (ShaderResourceView)
			{
				ShaderResourceView->Release();
				ShaderResourceView = nullptr;
			}
		};
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Texture2D;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
	};
public:
	static void		Initialize();
	static void		ShutDown();
public:
	static void		Init(HWND hWnd, const CustomType::Vector2Int& bufferSize, const UINT& bufferDepth = 24u, const UINT& frameNum = 60u, const BOOL& windowed = TRUE);
	static void		Uninit();
public:
	static BOOL		LoadVertexShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout, const std::vector<D3D11_INPUT_ELEMENT_DESC>* layout = NULL);
	static BOOL		LoadPixelShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader);
	static BOOL		LoadComputeShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11ComputeShader>& computeShader);
	static BOOL		CreateBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const CustomStruct::CRenderBufferDesc& bufferDesc, const CustomStruct::CRenderSubresourceData* subData = NULL);
	static void		UploadBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& dstResource, const void* srcData, UINT srcRowPitch = 0u, UINT srcDepthPitch = 0u, UINT dstSubresource = 0u, const D3D11_BOX* dstBox = NULL);
	static BOOL		CreateRenderTexture2D(RenderTexture2DViewInfo& output, const CustomStruct::CRenderTextureDesc& textureDesc);
	static BOOL		CreateTexture2D(Texture2DViewInfo& output, const CustomStruct::CRenderTextureDesc& textureDesc, const CustomStruct::CRenderSubresourceData* subData = NULL);
public:
	static void		Present(const UINT& syncInterval = 0u);
	static void		SetDepthStencilState(const Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const UINT& stencilRef = 0x0u);
	static void		SetBlendState(const Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const CustomStruct::CColor& blendFactor = CustomStruct::CColor(0.f, 0.f, 0.f, 0.f), const UINT& sampleMask = 0xffffffff);
	static void		SetRenderTarget(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& dsv);
	static void		SetRenderTargets(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>* rtv, const UINT& rtvNum, const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& dsv);
	static void		SetRasterizerState(const Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs);
	static void		SetViewport(const D3D11_VIEWPORT& viewport);
	static void		SetViewports(std::vector<D3D11_VIEWPORT> viewports);
public:
	static void		SetVSShader(const Microsoft::WRL::ComPtr<ID3D11VertexShader>& vs);
	static void		SetPSShader(const Microsoft::WRL::ComPtr<ID3D11PixelShader>& ps);
	static void		SetCSShader(const Microsoft::WRL::ComPtr<ID3D11ComputeShader>& cs);
public:
	static void		BindVSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot);
	static void		BindVSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT& startSlot, const UINT& ssNum);
	static void		BindPSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot);
	static void		BindPSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT& startSlot, const UINT& ssNum);
	static void		BindCSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot);
	static void		BindCSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT& startSlot, const UINT& ssNum);
	static void		BindVSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT& startSlot);
	static void		BindVSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT& startSlot, const UINT& bufferNum);
	static void		BindPSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT& startSlot);
	static void		BindPSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT& startSlot, const UINT& bufferNum);
	static void		BindCSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT& startSlot);
	static void		BindCSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT& startSlot, const UINT& bufferNum);
	static void		BindVSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT& startSlot);
	static void		BindVSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT& startSlot, const UINT& srvNum);
	static void		BindPSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT& startSlot);
	static void		BindPSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT& startSlot, const UINT& srvNum);
	static void		BindCSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT& startSlot);
	static void		BindCSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT& startSlot, const UINT& srvNum);
	static void		BindCSUnorderedAccessView(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const UINT& startSlot);
	static void		BindCSUnorderedAccessViews(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>* uav, const UINT& startSlot, const UINT& uavNum);
public:
	static void		ClearRenderTargetView(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, const CustomStruct::CColor& clearColor = CustomStruct::CColor(0.f, 0.f, 0.f, 0.f));
	static void		ClearDepthStencilView(const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& dsv, CustomStruct::CRenderClearDepthStencilFlag flag = CustomStruct::CRenderClearDepthStencilFlag::CLEAR_DEPTH_STENCIL, const FLOAT& depth = 1.f, const UINT& stencil = 0u);
	static void		ClearUnorderedAccessViewFloat(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const CustomStruct::CColor& clearValue = CustomStruct::CColor(0.f, 0.f, 0.f, 0.f));
	static void		ClearUnorderedAccessViewUint(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const CustomType::Vector4Int& clearValue = CustomType::Vector4Int(0, 0, 0, 0));
public:
	static void		SetInputLayoutAndVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout, const Microsoft::WRL::ComPtr<ID3D11Buffer>& vb, const UINT& stride = 0u, const UINT& offset = 0u);
	static void		SetInputLayout(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout);
	static void		SetVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& vb, const UINT& stride = 0u, const UINT& offset = 0u);
	static void		SetIndexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& ib, const UINT& offset = 0u, CustomStruct::CRenderFormat format = CustomStruct::CRenderFormat::FORMAT_R32_UINT);
	static void		SetPrimitiveTopology(CustomStruct::CRenderPrimitiveTopology topology = CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	static void		Draw(const UINT& vertexCount, const UINT& startVertexLocation = 0u);
	static void		DrawIndexed(const UINT& indexCount, const UINT& startIndexLocation = 0u, const INT& baseVertexLocation = 0);
public:
	static BOOL		CreateDepthStencilState(Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const CustomStruct::CRenderDepthState& depthState, const CustomStruct::CRenderStencilState* stencilState = NULL);
	static BOOL		CreateBlendState(Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const std::vector<CustomStruct::CRenderBlendState>& blendStates);
	static BOOL		CreateRasterizerState(Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs, const CustomStruct::CRenderRasterizerState& rasterizerState);
	static BOOL		CreateSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const CustomStruct::CRenderSamplerState& samplerState);
private:
	static void		TranslateBindFlag(UINT& output, CustomStruct::CRenderBindFlag input);
	static void		TranslateUsage(D3D11_USAGE& output, CustomStruct::CRenderUsage input);
	static void		TranslateCPUAccessFlag(UINT& output, CustomStruct::CRenderCPUAccessFlag input);
	static void		TranslateResourceMiscFlag(UINT& output, CustomStruct::CRenderResourceMiscFlag input);
	static void		TranslateResourceFormat(DXGI_FORMAT& output, CustomStruct::CRenderFormat input);
	static void		TranslateComparisonFunction(D3D11_COMPARISON_FUNC& output, CustomStruct::CRenderComparisonFunction input);
	static void		TranslateStencilOperation(D3D11_DEPTH_STENCILOP_DESC& output, const CustomStruct::CRenderStencilOp& input);
	static void		TranslateDepthStencilState(D3D11_DEPTH_STENCIL_DESC& output, const CustomStruct::CRenderDepthState& depthInput, const CustomStruct::CRenderStencilState* stencilInput = NULL);
	static void		TranslateBlendState(D3D11_RENDER_TARGET_BLEND_DESC& output, const CustomStruct::CRenderBlendState& input);
	static void		TranslateRasterizerState(D3D11_CULL_MODE& outputCull, D3D11_FILL_MODE& outputFill, const CustomStruct::CRenderRasterizerState& input);
	static void		TranslateSamplerState(D3D11_SAMPLER_DESC& output, const CustomStruct::CRenderSamplerState& input);
	static void		TranslateClearDepthStencilFlag(UINT& output, CustomStruct::CRenderClearDepthStencilFlag input);
	static void		TranslatePrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY& output, CustomStruct::CRenderPrimitiveTopology input);
public:
	static void		SetFinalOutput();
public:
	static D3D11_VIEWPORT		GetViewport();
	static D3D_FEATURE_LEVEL	GetFeatureLevel();
private:
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	m_RenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	m_DepthStencilView;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>			m_DepthTexture;
	D3D11_VIEWPORT									m_Viewport;
private:
	Microsoft::WRL::ComPtr<IDXGISwapChain>			m_SwapChain;
	D3D_FEATURE_LEVEL								m_FeatureLevel;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		m_ImmediateContext;
	Microsoft::WRL::ComPtr<ID3D11Device>			m_Device;
private:
	CRenderDevice() { ::ZeroMemory(this, sizeof(*this)); }
	CRenderDevice(const CRenderDevice&) { ::ZeroMemory(this, sizeof(*this)); }
	~CRenderDevice() {}
private:
	static CRenderDevice* m_RenderDevice;
private:
	friend class CimGUIManager;
	static Microsoft::WRL::ComPtr<ID3D11DeviceContext>	GetRenderDeviceContext();
	static Microsoft::WRL::ComPtr<ID3D11Device>			GetRenderDevice();
};