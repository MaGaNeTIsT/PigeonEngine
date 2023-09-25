#pragma once

#include <CoreMinimal.h>
#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "xaudio2.lib")
#include <RenderConfig/RenderConfig.h>
#include <RenderCommon.h>
#include <RenderResource.h>
#include <EngineCommon.h>

namespace PigeonEngine
{
	class RDeviceD3D11 : public EManagerBase
	{
	public:
		void			SetInitializeData(HWND hWnd, const Vector2Int& bufferSize, UINT32 bufferDepth = 24u, UINT32 frameNum = 60u, BOOL32 windowed = TRUE);
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
		BOOL32	CreateVertexShaderResource(const void* inCSO, const ULONG& inSize, RVertexShaderResource& outShaderResource, const RInputLayoutDesc* inLayouts, const UINT32& inLayoutNum);
		BOOL32	CreatePixelShaderResource(const void* inCSO, const ULONG& inSize, RPixelShaderResource& outShaderResource);
		BOOL32	CreateComputeShaderResource(const void* inCSO, const ULONG& inSize, RComputeShaderResource& outShaderResource);
		BOOL32	CreateBuffer(RBufferResource& buffer, const RBufferDesc& bufferDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL32	CreateStructuredBuffer(RStructuredBuffer& output, const RStructuredBufferDesc& structuredBufferDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL32	CreateRenderTexture2D(RRenderTexture2D& output, const RTextureDesc& textureDesc);
		BOOL32	CreateRenderTexture3D(RRenderTexture3D& output, const RTextureDesc& textureDesc);
		BOOL32	CreateTexture2D(RTexture2DResource& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL32	CreateTextureCube(RTextureCubeResource& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL32	LoadVertexShader(const EString& name, RVertexShaderResource& outShaderResource, const RInputLayoutDesc* inLayouts, const UINT32& inLayoutNum);
		BOOL32	LoadPixelShader(const EString& name, RPixelShaderResource& outShaderResource);
		BOOL32	LoadComputeShader(const EString& name, RComputeShaderResource& outShaderResource);
	public:
		BOOL32	CreateBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const RBufferDesc& bufferDesc, const RSubresourceDataDesc* subData = nullptr);
		void	UploadBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& dstResource, const void* srcData, UINT32 srcRowPitch = 0u, UINT32 srcDepthPitch = 0u, UINT32 dstSubresource = 0u, const D3D11_BOX* dstBox = nullptr);
		void	UploadResource(const Microsoft::WRL::ComPtr<ID3D11Texture2D>& dstResource, const void* srcData, UINT32 srcRowPitch, UINT32 srcDepthPitch, UINT32 dstSubresource = 0u, const D3D11_BOX* dstBox = nullptr);
	public:
		void	Present(const UINT32& syncInterval = 0u);
		void	SetDefaultDepthStencilState();
		void	SetDepthStencilState(const Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const UINT32& stencilRef = 0x0u);
		void	SetDefaultBlendState();
		void	SetBlendState(const Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const Color4& blendFactor = Color4(0.f, 0.f, 0.f, 0.f), const UINT32& sampleMask = 0xffffffffu);
		void	SetNoRenderTarget();
		void	SetRenderTarget(const RRenderTexture2D& InRTV);
		void	SetRenderTarget(const RRenderTexture2D& InDSV);
		void	SetRenderTarget(const RRenderTexture2D& InRTV, const RRenderTexture2D& InDSV);
		void	SetRenderTargets(const RRenderTexture2D* InRTVs, const UINT32& InRTVNum);
		void	SetRenderTargets(const RRenderTexture2D* InRTVs, const UINT32& InRTVNum, const RRenderTexture2D& InDSV);
		void	SetRasterizerState(const Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs);
		void	SetViewport(const EViewport& viewport);
		void	SetViewports(const EViewport* viewports, const UINT32& viewportNum);
	public:
		void	SetNoVSShader();
		void	SetNoPSShader();
		void	SetNoCSShader();
		void	SetVSShader(const Microsoft::WRL::ComPtr<ID3D11VertexShader>& vs);
		void	SetPSShader(const Microsoft::WRL::ComPtr<ID3D11PixelShader>& ps);
		void	SetCSShader(const Microsoft::WRL::ComPtr<ID3D11ComputeShader>& cs);
	public:
		void	BindVSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT32& startSlot);
		void	BindVSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT32& startSlot, const UINT32& ssNum);
		void	BindPSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT32& startSlot);
		void	BindPSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT32& startSlot, const UINT32& ssNum);
		void	BindCSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT32& startSlot);
		void	BindCSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT32& startSlot, const UINT32& ssNum);
		void	BindVSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT32& startSlot);
		void	BindVSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT32& startSlot, const UINT32& bufferNum);
		void	BindPSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT32& startSlot);
		void	BindPSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT32& startSlot, const UINT32& bufferNum);
		void	BindCSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT32& startSlot);
		void	BindCSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT32& startSlot, const UINT32& bufferNum);
		void	BindVSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT32& startSlot);
		void	BindVSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT32& startSlot, const UINT32& srvNum);
		void	BindPSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT32& startSlot);
		void	BindPSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT32& startSlot, const UINT32& srvNum);
		void	BindCSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT32& startSlot);
		void	BindCSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT32& startSlot, const UINT32& srvNum);
		void	BindNoCSUnorderedAccessView(const UINT32& startSlot);
		void	BindCSUnorderedAccessView(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const UINT32& startSlot);
		void	BindCSUnorderedAccessViews(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>* uav, const UINT32& startSlot, const UINT32& uavNum);
	public:
		void	CopyTexture2DResource(const Microsoft::WRL::ComPtr<ID3D11Texture2D>& src, const Microsoft::WRL::ComPtr<ID3D11Texture2D>& dst);
	public:
		void	ClearRenderTargetView(const RRenderTexture2D& InRTV, const Color4& InClearColor = Color4(0.f, 0.f, 0.f, 0.f));
		void	ClearDepthStencilView(const RRenderTexture2D& InDSV, const UINT32 InFlags = (RClearDepthStencilFlagType::CLEAR_DEPTH | RClearDepthStencilFlagType::CLEAR_STENCIL), const FLOAT InDepth = 1.f, const UINT32 InStencil = 0x0u);
		void	ClearUnorderedAccessViewFloat(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const Color4& clearValue = Color4(0.f, 0.f, 0.f, 0.f));
		void	ClearUnorderedAccessViewUint(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const Vector4Int& clearValue = Vector4Int(0, 0, 0, 0));
	public:
		void	SetInputLayoutAndVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout, const Microsoft::WRL::ComPtr<ID3D11Buffer>& vb, const UINT32& stride = 0u, const UINT32& offset = 0u);
		void	SetInputLayout(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout);
		void	SetNoInputLayout();
		void	SetVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& InBuffer, UINT32 InStride, UINT32 InOffset = 0u, UINT32 InStartSlot = 0u, UINT32 InBufferNum = 1u);
		void	SetNoVertexBuffer();
		void	SetIndexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& InBuffer, UINT32 InOffset = 0u, RFormatType InFormat = RFormatType::FORMAT_R32_UINT);
		void	SetNoIndexBuffer();
		void	SetPrimitiveTopology(RPrimitiveTopologyType topology = RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		void	Draw(const UINT32& vertexCount, const UINT32& startVertexLocation = 0u);
		void	DrawIndexed(const UINT32& indexCount, const UINT32& startIndexLocation = 0u, const INT32& baseVertexLocation = 0);
		void	DrawIndexedInstance(const UINT32& instanceCount, const UINT32& indexCountPerInstance, const UINT32& startInstanceLocation = 0u, const UINT32& startIndexLocation = 0u, const INT32& BaseVertexLocation = 0);
		void	Dispatch(const UINT32& x, const UINT32& y, const UINT32& z);
		void	DispatchIndirect(const Microsoft::WRL::ComPtr<ID3D11Buffer>& arg, const UINT32& alignedByteOffsetForArgs = 0u);
	public:
		BOOL32	CreateDepthStencilState(Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const RDepthState& depthState, const RStencilState* stencilState = nullptr);
		BOOL32	CreateBlendState(Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const RBlendState* blendStates, const UINT32& blendStateNum);
		BOOL32	CreateRasterizerState(Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs, const RRasterizerState& rasterizerState);
		BOOL32	CreateSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const RSamplerState& samplerState);
	public:
		BOOL32	CreateQuery(Microsoft::WRL::ComPtr<ID3D11Query>& q, const RQueryDesc& queryDesc);
		BOOL32	GetData(ID3D11Asynchronous* pAsync, void* output, const UINT32& size, RAsyncGetDataFlagType flag = RAsyncGetDataFlagType::ASYNC_GETDATA_DEFAULT);
		void	Begin(ID3D11Asynchronous* pAsync);
		void	End(ID3D11Asynchronous* pAsync);
		BOOL32	Map(const RStructuredBuffer& input, const UINT32& indexSubResource, RResourceMapType mapType, RResourceMapFlagType mapFlag, RMappedResource& output);
		void	Unmap(const RStructuredBuffer& input, const UINT32& indexSubResource);
	private:
		static void		TranslateBindFlag(UINT32& output, UINT8 input);
		static void		TranslateUsageFlag(D3D11_USAGE& output, RUsageFlagType input);
		static void		TranslateCPUAccessFlag(UINT32& output, RCPUAccessFlagType input);
		static void		TranslateResourceMiscFlag(UINT32& output, RResourceMiscFlagType input);
		static void		TranslateResourceFormat(DXGI_FORMAT& output, RFormatType input);
		static void		TranslateUAVFlag(UINT32& output, RUAVFlagType input);
		static void		TranslateComparisonFunction(D3D11_COMPARISON_FUNC& output, RComparisonFunctionType input);
		static void		TranslateStencilStateType(D3D11_DEPTH_STENCILOP_DESC& output, const RStencilStateType& input);
		static void		TranslateDepthStencilState(D3D11_DEPTH_STENCIL_DESC& output, const RDepthState& depthInput, const RStencilState* stencilInput = nullptr);
		static void		TranslateBlendState(D3D11_RENDER_TARGET_BLEND_DESC& output, const RBlendState& input);
		static void		TranslateRasterizerState(D3D11_CULL_MODE& outputCull, D3D11_FILL_MODE& outputFill, const RRasterizerState& input);
		static void		TranslateSamplerState(D3D11_SAMPLER_DESC& output, const RSamplerState& input);
		static void		TranslateClearDepthStencilFlag(UINT32& output, const UINT32& input);
		static void		TranslatePrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY& output, RPrimitiveTopologyType input);
		static void		TranslateQueryDesc(D3D11_QUERY_DESC& output, const RQueryDesc& input);
		static void		TranslateGetDataFlag(UINT32& output, RAsyncGetDataFlagType input);
		static void		TranslateResourceMapType(D3D11_MAP& output, RResourceMapType input);
		static void		TranslateResourceMapFlag(UINT32& output, RResourceMapFlagType input);
		static void		TranslateInputLayoutDesc(D3D11_INPUT_ELEMENT_DESC& output, const RInputLayoutDesc& input);
	public:
		void	ClearFinalOutput();
		void	SetFinalOutput();
	public:
		D3D11_VIEWPORT		GetViewport() const { return m_Viewport; }
		D3D_FEATURE_LEVEL	GetFeatureLevel() const { return m_FeatureLevel; }
	private:
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>		GetRenderDeviceContext() const { return m_ImmediateContext; }
		Microsoft::WRL::ComPtr<ID3D11Device>			GetRenderDevice() const { return m_Device; }
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
		RDeviceD3D11() : m_Device(nullptr), m_ImmediateContext(nullptr), m_SwapChain(nullptr), m_DepthTexture(nullptr), m_DepthStencilView(nullptr), m_RenderTargetView(nullptr), m_FeatureLevel(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_1), m_Viewport(D3D11_VIEWPORT()) {}
		~RDeviceD3D11() {}
		RDeviceD3D11(const RDeviceD3D11&) = delete;
		RDeviceD3D11& operator=(const RDeviceD3D11&) = delete;
	public:
		static RDeviceD3D11* GetDeviceSingleton()
		{
			static RDeviceD3D11 _DeviceSingletonObject;
			return (&_DeviceSingletonObject);
		}
	private:
		friend class CImGUIManager;
	};
};