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
#include <RHI/IRRHIDevice.h>
#include "CommandListD3D11.h"

namespace PigeonEngine
{
	// D3D11 timestamp heap (Phase P.c). The struct lives in the header so
	// both RDeviceD3D11 (creation / resolve / disjoint framing) and
	// RCommandListD3D11 (per-slot End() writes) can reach into the same
	// TimestampQueries[] array without an extra opaque-handle layer.
	//
	// Note: TimestampQueries is a raw ID3D11Query** owned by this struct
	// (manual AddRef/Release in the dtor). Could be a TArray<ComPtr<...>>
	// now that Array.h's placement-new is operator&-safe, but the direct
	// pointer array makes the queries' lifetime explicit and avoids one
	// indirection in the hot WriteTimestamp / ResolveTimestamps paths.
	struct RRHITimestampHeapD3D11 : public RRHITimestampHeap
	{
		ID3D11Query**									TimestampQueries	= nullptr;
		Microsoft::WRL::ComPtr<ID3D11Query>				DisjointQuery;
		BOOL8											DisjointOpen		= FALSE;
		BOOL8											DisjointLatched		= FALSE;	// last EndTimestampFrame produced a valid disjoint
		BOOL8											LastFrameValid		= FALSE;	// !Disjoint flag from last resolve
		UINT64											LastFrequency		= 0u;

		virtual ~RRHITimestampHeapD3D11()override
		{
			if (TimestampQueries)
			{
				for (UINT32 i = 0u; i < Capacity; i++)
				{
					if (TimestampQueries[i])
					{
						TimestampQueries[i]->Release();
						TimestampQueries[i] = nullptr;
					}
				}
				delete[] TimestampQueries;
				TimestampQueries = nullptr;
			}
		}
	};

	/*
	* D3D11 shader object. This is a back-end private type: it holds the native COM
	* objects and is handed out as IRRHIShader, so engine code never sees a
	* ID3D11*Shader. The input layout belongs to the vertex stage, because D3D11 needs
	* the shader bytecode that declared the signature to build it.
	*/
	class RD3D11Shader final : public IRD3D11Shader
	{
	public:
		RD3D11Shader() noexcept
			: Stage(ERHIShaderStage::RHI_SHADER_STAGE_COUNT)
		{
		}
		virtual ~RD3D11Shader() noexcept
		{
		}

		// IRRHIShader
		virtual ERHIShaderStage GetShaderStage() const noexcept override { return Stage; }

		// IRD3D11Shader
		virtual ID3D11VertexShader*			GetD3D11VS()const override { return VertexShader.Get(); }
		virtual ID3D11PixelShader*			GetD3D11PS()const override { return PixelShader.Get(); }
		virtual ID3D11ComputeShader*		GetD3D11CS()const override { return ComputeShader.Get(); }
		virtual ID3D11InputLayout*			GetD3D11InputLayout()const override { return InputLayout.Get(); }
		virtual const void*					GetByteCode()const override { return ByteCode.GetData(); }
		virtual UINT64						GetByteCodeSize()const override { return ByteCode.Num<UINT64>(); }

		ERHIShaderStage							Stage;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	VertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	PixelShader;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>	ComputeShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	InputLayout;
		// Bytecode the object was created from. Kept because D3D11 bakes an input layout against the
		// vertex shader's signature, so a pipeline state that declares its own vertex data layout has
		// to build one here rather than at shader creation time.
		TArray<UINT8>								ByteCode;
	};

	/*
	* D3D11 root signature. The back-end has no root signature concept at all: constant
	* buffers are bound straight to the API's register slots and there are no descriptor
	* tables, so this object exists only to give the RHI layer a concrete handle and the
	* layout identity the caches key on. Its RRHIPipelineLayoutInfo stays default
	* constructed, which RRHIPipelineLayoutInfo's contract reads as "no root signature
	* back-end": a root index is a register slot here.
	*/
	class RD3D11RootSignature final : public IRRHIRootSignature
	{
	public:
		virtual ~RD3D11RootSignature() noexcept {}

		// IRRHIRootSignature
		virtual UINT64	GetLayoutHash() const noexcept override { return LayoutHash; }
		virtual const RRHIPipelineLayoutInfo&	GetLayoutInfo() const noexcept override { return LayoutInfo; }

		UINT64					LayoutHash = 0ull;
		// Stays default constructed: this back-end binds straight to register slots and has
		// no root parameters, which is exactly what RRHIPipelineLayoutInfo's zero value means.
		RRHIPipelineLayoutInfo	LayoutInfo{};
	};

	class RDeviceD3D11 : public EManagerBase, public IRRHIDevice
	{
	public:
		void			SetInitializeData(HWND hWnd, const Vector2Int& bufferSize, UINT32 bufferDepth = 24u, UINT32 frameNum = 60u, BOOL32 windowed = TRUE);
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
		BOOL32	CreateShaderObject(const RRHIShaderDesc& InDesc, RD3D11Shader& OutShader);
		BOOL32	CreateBuffer(RBufferResource& buffer, const RBufferDesc& bufferDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL32	CreateStructuredBuffer(RStructuredBuffer& output, const RStructuredBufferDesc& structuredBufferDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL32	CreateRenderTexture2D(RRenderTexture2D& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL32	CreateTexture2D(RTexture2DResource& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData = nullptr);
		BOOL32	CreateTextureCube(RTextureCubeResource& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData = nullptr);
	public:
		BOOL32	CreateBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const RBufferDesc& bufferDesc, const RSubresourceDataDesc* subData = nullptr);
		void	UploadBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& dstResource, const void* srcData, UINT32 srcRowPitch = 0u, UINT32 srcDepthPitch = 0u, UINT32 dstSubresource = 0u, const D3D11_BOX* dstBox = nullptr);
		void	UploadResource(const Microsoft::WRL::ComPtr<ID3D11Texture2D>& dstResource, const void* srcData, UINT32 srcRowPitch, UINT32 srcDepthPitch, UINT32 dstSubresource = 0u, const D3D11_BOX* dstBox = nullptr);
	public:
		void	SetDefaultDepthStencilState();
		void	SetDepthStencilState(const Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const UINT32& stencilRef = 0x0u);
		void	SetDefaultBlendState();
		void	SetBlendState(const Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const Color4& blendFactor = Color4(0.f, 0.f, 0.f, 0.f), const UINT32& sampleMask = 0xffffffffu);
		void	SetNoRenderTarget();
		void	SetRenderTargetOnly(const RRenderTexture2D& InRTV);
		void	SetDepthStencilOnly(const RRenderTexture2D& InDSV);
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
		BOOL32	CreateBlendState(Microsoft::WRL::ComPtr<ID3D11BlendState>& OutBlendState, const RBlendDesc& InBlendDesc);
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

		// ========================================================
		// IRRHIDevice interface
		//
		// Most methods delegate to the legacy direct-device APIs
		// above. New state (BackBufferWrapper, CommandList pool,
		// bindless virtual-handle tables) lives in the second
		// private block below.
		// ========================================================
	public:
		// Lifecycle
		virtual BOOL8           Initialize(const RRHIDeviceInitDesc& InDesc) noexcept override;
		virtual void            Shutdown() noexcept override;
		virtual ERHIBackendType GetBackendType()const noexcept override { return ERHIBackendType::RHI_BACKEND_D3D11; }
		// D3D11 has no root constant space: a block the shader marks 'RootConstant' is bound
		// as a plain constant buffer on this back-end.
		virtual BOOL8           SupportsRootConstants()const noexcept override { return FALSE; }
		virtual void            BeginFrame() noexcept override;
		virtual void            EndFrame() noexcept override;
		virtual void            Present(UINT32 InSyncInterval) noexcept override;
		virtual BOOL8           ResizeSwapChain(UINT32 InWidth, UINT32 InHeight) noexcept override;
		virtual void            WaitForGPUIdle() noexcept override;

		// Resource creation
		virtual BOOL8 CreateBuffer(const RBufferDesc& InDesc, const RRHISubresourceData& InInitialData, IRRHIBuffer** OutBuffer) noexcept override;
		virtual BOOL8 CreateTexture(const RTextureDesc& InDesc, const Color4& InClearColor, const RRHISubresourceData& InInitialSubresources, IRRHITexture** OutTexture) noexcept override;
		virtual BOOL8 CreateShader(const RRHIShaderDesc& InDesc, IRRHIShader** OutShader) noexcept override;
		virtual BOOL8 CreateRootSignature(const RRHIRootSignatureDesc& InDesc, IRRHIRootSignature** OutSignature) noexcept override;
		virtual void  DestroyRootSignature(IRRHIRootSignature* InSignature) noexcept override;
		virtual BOOL8 CreateGraphicsPipelineState(const RRHIGraphicsPipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) noexcept override;
		virtual BOOL8 CreateComputePipelineState(const RRHIComputePipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) noexcept override;
		virtual BOOL8 CreateSampler(const RSamplerState& InDesc, IRRHISampler** OutSampler) noexcept override;
		virtual void  DestroyResource(IRRHIResource* InResource) noexcept override;

		// Bindless (D3D11 maintains virtual handle tables; the actual
		// descriptor heap exists only on D3D12.)
		virtual UINT32 RegisterBindlessSRV(IRRHITexture* InTexture) noexcept override;
		virtual UINT32 RegisterBindlessSRV(IRRHIBuffer* InBuffer) noexcept override;
		virtual UINT32 RegisterBindlessUAV(IRRHITexture* InTexture) noexcept override;
		virtual UINT32 RegisterBindlessUAV(IRRHIBuffer* InBuffer) noexcept override;
		virtual void   ReleaseBindlessIndex(UINT32 InIndex) noexcept override;
		virtual IRRHIResource* GetBindlessResource(UINT32 InIndex)const noexcept override;

		// Upload
		virtual void UploadBuffer(IRRHIBuffer* InDest, const UINT8* InData, UINT64 InSize, UINT64 InDestOffset = 0u) noexcept override;
		virtual void UploadTexture(IRRHITexture* InDest, const RRHISubresourceData* InSubresources, UINT32 InSubresourceCount, UINT32 InFirstSubresource = 0u) noexcept override;

		// Command lists
		virtual IRCommandList*  AcquireCommandList() override;
		virtual void            SubmitCommandList(IRCommandList* InCommandList) noexcept override;
		virtual void            SubmitCommandLists(IRCommandList* const* InCommandLists, UINT32 InCount) noexcept override;

		// Backbuffer access
		virtual IRRHITexture*   GetCurrentBackBuffer()const noexcept override;
		virtual UINT32          GetCurrentFrameIndex()const noexcept override { return CurrentFrameIndex; }
		virtual UINT32          GetFrameInFlightCount()const noexcept override { return FrameInFlightCount; }

		// GPU timestamp queries (Phase P.c)
		virtual BOOL8   CreateTimestampHeap(UINT32 InCapacity, RRHITimestampHeap** OutHeap) noexcept override;
		virtual void    DestroyTimestampHeap(RRHITimestampHeap* InHeap) noexcept override;
		virtual void    BeginTimestampFrame(RRHITimestampHeap* InHeap) noexcept override;
		virtual void    EndTimestampFrame(RRHITimestampHeap* InHeap) noexcept override;
		virtual BOOL8   ResolveTimestamps(RRHITimestampHeap* InHeap, UINT32 InBeginSlot, UINT32 InCount, UINT64* OutTicks) noexcept override;
		virtual UINT64  GetGPUTicksPerSecond(const RRHITimestampHeap* InHeap)const noexcept override;
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
		// New IRRHIDevice state (Pascal names, no m_ prefix per current style)
		RRenderTexture2D								BackBufferWrapper;
		TArray<TUniquePtr<RCommandListD3D11>>			CommandListPool;
		TArray<RCommandListD3D11*>						CommandListFreeList;
		TArray<IRRHIResource*>							BindlessSRVTable;
		TArray<IRRHIResource*>							BindlessUAVTable;
		TArray<UINT32>									BindlessSRVFreeIndices;
		TArray<UINT32>									BindlessUAVFreeIndices;
		// Root signatures. D3D11 has no native root signature, so the created objects only
		// carry the layout hash; they are pooled so an equal layout hands back the same
		// handle, which mirrors the sharing semantics the D3D12 back-end implements. There
		// is no device default: a pipeline state names the signature its program's reflection
		// generated, and a desc without one is refused.
		TArray<RD3D11RootSignature*>					RootSignaturePool;
		TMap<UINT64, RD3D11RootSignature*>				RootSignatureCache;
		UINT32											CurrentFrameIndex;
		UINT32											FrameInFlightCount;
	private:
		RDeviceD3D11()
			: m_Device(nullptr), m_ImmediateContext(nullptr), m_SwapChain(nullptr), m_DepthTexture(nullptr)
			, m_DepthStencilView(nullptr), m_RenderTargetView(nullptr)
			, m_FeatureLevel(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_1), m_Viewport(D3D11_VIEWPORT())
			, CurrentFrameIndex(0u), FrameInFlightCount(2u) {}
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
