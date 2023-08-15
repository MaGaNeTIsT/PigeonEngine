#pragma once

#include <CoreMinimal.h>
#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "xaudio2.lib")
#include <RenderConfig/RenderConfig.h>
#include <RenderCommon.h>
#include <EngineCommon.h>

namespace PigeonEngine
{
	class RDeviceD3D11 : public EManagerBase
	{
	public:
		struct RVertexShaderResource
		{
			RVertexShaderResource() : Shader(nullptr), InputLayout(nullptr), RawLayouts(nullptr), LayoutNum(0u) {}
			void Release()
			{
				if (Shader)
				{
					Shader->Release();
					Shader = nullptr;
				}
				if (InputLayout)
				{
					InputLayout->Release();
					InputLayout = nullptr;
				}
				if (RawLayouts)
				{
					delete[]RawLayouts;
					RawLayouts = nullptr;
					LayoutNum = 0u;
				}
			};
			Microsoft::WRL::ComPtr<ID3D11VertexShader>	Shader;
			Microsoft::WRL::ComPtr<ID3D11InputLayout>	InputLayout;
			RInputLayoutDesc*							RawLayouts;
			UINT										LayoutNum;
		};
		struct RPixelShaderResource
		{
			RPixelShaderResource() : Shader(nullptr) {}
			void Release()
			{
				if (Shader)
				{
					Shader->Release();
					Shader = nullptr;
				}
			};
			Microsoft::WRL::ComPtr<ID3D11PixelShader> Shader;
		};
		struct RComputeShaderResource
		{
			RComputeShaderResource() : Shader(nullptr) {}
			void Release()
			{
				if (Shader)
				{
					Shader->Release();
					Shader = nullptr;
				}
			};
			Microsoft::WRL::ComPtr<ID3D11ComputeShader> Shader;
		};
		struct RStructuredBuffer
		{
			RStructuredBuffer() : AccessMapRead(FALSE), AccessMapWrite(FALSE), Buffer(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr) {}
			void Release()
			{
				AccessMapRead = FALSE;
				AccessMapWrite = FALSE;
				if (Buffer)
				{
					Buffer->Release();
					Buffer = nullptr;
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
			BOOL	AccessMapRead;
			BOOL	AccessMapWrite;
			Microsoft::WRL::ComPtr<ID3D11Buffer>				Buffer;
			Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		};
		struct RRenderTexture2D
		{
			RRenderTexture2D() : Buffer(nullptr), RenderTargetView(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr), DepthStencilView(nullptr) {}
			void Release()
			{
				if (Buffer)
				{
					Buffer->Release();
					Buffer = nullptr;
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
			Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
			Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		RenderTargetView;
			Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
			Microsoft::WRL::ComPtr<ID3D11DepthStencilView>		DepthStencilView;
		};
		struct RRenderTexture3D
		{
			RRenderTexture3D() : Buffer(nullptr), RenderTargetView(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr) {}
			void Release()
			{
				if (Buffer)
				{
					Buffer->Release();
					Buffer = nullptr;
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
			};
			Microsoft::WRL::ComPtr<ID3D11Texture3D>				Buffer;
			Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		RenderTargetView;
			Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		};
		struct RTexture2DResource
		{
			RTexture2DResource() : Buffer(nullptr), ShaderResourceView(nullptr) {}
			void Release()
			{
				if (Buffer)
				{
					Buffer->Release();
					Buffer = nullptr;
				}
				if (ShaderResourceView)
				{
					ShaderResourceView->Release();
					ShaderResourceView = nullptr;
				}
			};
			Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		};
		struct RTextureCubeResource
		{
			RTextureCubeResource() : Buffer(nullptr), ShaderResourceView(nullptr) {}
			void Release()
			{
				if (Buffer)
				{
					Buffer->Release();
					Buffer = nullptr;
				}
				if (ShaderResourceView)
				{
					ShaderResourceView->Release();
					ShaderResourceView = nullptr;
				}
			};
			Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		};
		struct RBufferResource
		{
			RBufferResource() : Buffer(nullptr) {}
			void Release()
			{
				if (Buffer)
				{
					Buffer->Release();
					Buffer = nullptr;
				}
			};
			Microsoft::WRL::ComPtr<ID3D11Buffer>				Buffer;
		};
	public:
		void			SetInitializeData(HWND hWnd, const Vector2Int& bufferSize, UINT bufferDepth = 24u, UINT frameNum = 60u, BOOL windowed = TRUE);
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
		BOOL	CreateVertexShaderResource(const void* inCSO, const ULONG& inSize, RVertexShaderResource& outShaderResource, const RInputLayoutDesc* inLayouts, const UINT& inLayoutNum);
		BOOL	CreatePixelShaderResource(const void* inCSO, const ULONG& inSize, RPixelShaderResource& outShaderResource);
		BOOL	CreateComputeShaderResource(const void* inCSO, const ULONG& inSize, RComputeShaderResource& outShaderResource);
		BOOL	CreateBuffer(RBufferResource& buffer, const RBufferDesc& bufferDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL	CreateStructuredBuffer(RStructuredBuffer& output, const RStructuredBufferDesc& structuredBufferDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL	CreateRenderTexture2D(RRenderTexture2D& output, const RTextureDesc& textureDesc);
		BOOL	CreateRenderTexture3D(RRenderTexture3D& output, const RTextureDesc& textureDesc);
		BOOL	CreateTexture2D(RTexture2DResource& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL	CreateTextureCube(RTextureCubeResource& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL	LoadVertexShader(const EString& name, RVertexShaderResource& outShaderResource, const RInputLayoutDesc* inLayouts, const UINT& inLayoutNum);
		BOOL	LoadPixelShader(const EString& name, RPixelShaderResource& outShaderResource);
		BOOL	LoadComputeShader(const EString& name, RComputeShaderResource& outShaderResource);
	public:
		BOOL	CreateBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const RBufferDesc& bufferDesc, const RSubresourceDataDesc* subData = nullptr);
		void	UploadBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& dstResource, const void* srcData, UINT srcRowPitch = 0u, UINT srcDepthPitch = 0u, UINT dstSubresource = 0u, const D3D11_BOX* dstBox = nullptr);
		void	UploadResource(const Microsoft::WRL::ComPtr<ID3D11Texture2D>& dstResource, const void* srcData, UINT srcRowPitch, UINT srcDepthPitch, UINT dstSubresource = 0u, const D3D11_BOX* dstBox = nullptr);
	public:
		void	Present(const UINT& syncInterval = 0u);
		void	SetDefaultDepthStencilState();
		void	SetDepthStencilState(const Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const UINT& stencilRef = 0x0u);
		void	SetDefaultBlendState();
		void	SetBlendState(const Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const Color4& blendFactor = Color4(0.f, 0.f, 0.f, 0.f), const UINT& sampleMask = 0xffffffffu);
		void	SetNoRenderTarget();
		void	SetRenderTarget(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv);
		void	SetRenderTarget(const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& dsv);
		void	SetRenderTarget(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& dsv);
		void	SetRenderTargets(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>* rtv, const UINT& rtvNum);
		void	SetRenderTargets(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>* rtv, const UINT& rtvNum, const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& dsv);
		void	SetRasterizerState(const Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs);
		void	SetViewport(const EViewport& viewport);
		void	SetViewports(const EViewport* viewports, const UINT& viewportNum);
	public:
		void	SetNoVSShader();
		void	SetNoPSShader();
		void	SetNoCSShader();
		void	SetVSShader(const Microsoft::WRL::ComPtr<ID3D11VertexShader>& vs);
		void	SetPSShader(const Microsoft::WRL::ComPtr<ID3D11PixelShader>& ps);
		void	SetCSShader(const Microsoft::WRL::ComPtr<ID3D11ComputeShader>& cs);
	public:
		void	BindVSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot);
		void	BindVSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT& startSlot, const UINT& ssNum);
		void	BindPSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot);
		void	BindPSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT& startSlot, const UINT& ssNum);
		void	BindCSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot);
		void	BindCSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT& startSlot, const UINT& ssNum);
		void	BindVSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT& startSlot);
		void	BindVSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT& startSlot, const UINT& bufferNum);
		void	BindPSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT& startSlot);
		void	BindPSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT& startSlot, const UINT& bufferNum);
		void	BindCSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT& startSlot);
		void	BindCSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT& startSlot, const UINT& bufferNum);
		void	BindVSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT& startSlot);
		void	BindVSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT& startSlot, const UINT& srvNum);
		void	BindPSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT& startSlot);
		void	BindPSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT& startSlot, const UINT& srvNum);
		void	BindCSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT& startSlot);
		void	BindCSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT& startSlot, const UINT& srvNum);
		void	BindNoCSUnorderedAccessView(const UINT& startSlot);
		void	BindCSUnorderedAccessView(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const UINT& startSlot);
		void	BindCSUnorderedAccessViews(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>* uav, const UINT& startSlot, const UINT& uavNum);
	public:
		void	CopyTexture2DResource(const Microsoft::WRL::ComPtr<ID3D11Texture2D>& src, const Microsoft::WRL::ComPtr<ID3D11Texture2D>& dst);
	public:
		void	ClearRenderTargetView(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, const Color4& clearColor = Color4(0.f, 0.f, 0.f, 0.f));
		void	ClearDepthStencilView(const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& dsv, UINT flags = (RClearDepthStencilFlagType::CLEAR_DEPTH | RClearDepthStencilFlagType::CLEAR_STENCIL), const FLOAT& depth = 1.f, const UINT& stencil = 0x0u);
		void	ClearUnorderedAccessViewFloat(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const Color4& clearValue = Color4(0.f, 0.f, 0.f, 0.f));
		void	ClearUnorderedAccessViewUint(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const Vector4Int& clearValue = Vector4Int(0, 0, 0, 0));
	public:
		void	SetInputLayoutAndVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout, const Microsoft::WRL::ComPtr<ID3D11Buffer>& vb, const UINT& stride = 0u, const UINT& offset = 0u);
		void	SetInputLayout(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout);
		void	SetNoInputLayout();
		void	SetVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& vb, const UINT& stride, const UINT& offset = 0u);
		void	SetNoVertexBuffer();
		void	SetIndexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& ib, const UINT& offset = 0u, RFormatType format = RFormatType::FORMAT_R32_UINT);
		void	SetNoIndexBuffer();
		void	SetPrimitiveTopology(RPrimitiveTopologyType topology = RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		void	Draw(const UINT& vertexCount, const UINT& startVertexLocation = 0u);
		void	DrawIndexed(const UINT& indexCount, const UINT& startIndexLocation = 0u, const INT& baseVertexLocation = 0);
		void	DrawIndexedInstance(const UINT& instanceCount, const UINT& indexCountPerInstance, const UINT& startInstanceLocation = 0u, const UINT& startIndexLocation = 0u, const INT& BaseVertexLocation = 0);
		void	Dispatch(const UINT& x, const UINT& y, const UINT& z);
		void	DispatchIndirect(const Microsoft::WRL::ComPtr<ID3D11Buffer>& arg, const UINT& alignedByteOffsetForArgs = 0u);
	public:
		BOOL	CreateDepthStencilState(Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const RDepthState& depthState, const RStencilState* stencilState = nullptr);
		BOOL	CreateBlendState(Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const RBlendState* blendStates, const UINT& blendStateNum);
		BOOL	CreateRasterizerState(Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs, const RRasterizerState& rasterizerState);
		BOOL	CreateSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const RSamplerState& samplerState);
	public:
		BOOL	CreateQuery(Microsoft::WRL::ComPtr<ID3D11Query>& q, const RQueryDesc& queryDesc);
		BOOL	GetData(ID3D11Asynchronous* pAsync, void* output, const UINT& size, RAsyncGetDataFlagType flag = RAsyncGetDataFlagType::ASYNC_GETDATA_DEFAULT);
		void	Begin(ID3D11Asynchronous* pAsync);
		void	End(ID3D11Asynchronous* pAsync);
		BOOL	Map(const RStructuredBuffer& input, const UINT& indexSubResource, RResourceMapType mapType, RResourceMapFlagType mapFlag, RMappedResource& output);
		void	Unmap(const RStructuredBuffer& input, const UINT& indexSubResource);
	private:
		static void		TranslateBindFlag(UINT& output, RBindFlagType input);
		static void		TranslateUsageFlag(D3D11_USAGE& output, RUsageFlagType input);
		static void		TranslateCPUAccessFlag(UINT& output, RCPUAccessFlagType input);
		static void		TranslateResourceMiscFlag(UINT& output, RResourceMiscFlagType input);
		static void		TranslateResourceFormat(DXGI_FORMAT& output, RFormatType input);
		static void		TranslateUAVFlag(UINT& output, RUAVFlagType input);
		static void		TranslateComparisonFunction(D3D11_COMPARISON_FUNC& output, RComparisonFunctionType input);
		static void		TranslateStencilStateType(D3D11_DEPTH_STENCILOP_DESC& output, const RStencilStateType& input);
		static void		TranslateDepthStencilState(D3D11_DEPTH_STENCIL_DESC& output, const RDepthState& depthInput, const RStencilState* stencilInput = nullptr);
		static void		TranslateBlendState(D3D11_RENDER_TARGET_BLEND_DESC& output, const RBlendState& input);
		static void		TranslateRasterizerState(D3D11_CULL_MODE& outputCull, D3D11_FILL_MODE& outputFill, const RRasterizerState& input);
		static void		TranslateSamplerState(D3D11_SAMPLER_DESC& output, const RSamplerState& input);
		static void		TranslateClearDepthStencilFlag(UINT& output, const UINT& input);
		static void		TranslatePrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY& output, RPrimitiveTopologyType input);
		static void		TranslateQueryDesc(D3D11_QUERY_DESC& output, const RQueryDesc& input);
		static void		TranslateGetDataFlag(UINT& output, RAsyncGetDataFlagType input);
		static void		TranslateResourceMapType(D3D11_MAP& output, RResourceMapType input);
		static void		TranslateResourceMapFlag(UINT& output, RResourceMapFlagType input);
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