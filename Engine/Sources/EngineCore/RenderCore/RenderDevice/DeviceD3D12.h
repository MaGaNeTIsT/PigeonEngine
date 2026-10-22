#pragma once

#include <CoreMinimal.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#include <RenderConfig/RenderConfig.h>
#include <RenderCommon.h>
#include <RHI/RFormatTraits.h>
#include <EngineCommon.h>
#include <RHI/IRRHIDevice.h>
#include "D3D12Native.h"
#include "CommandListD3D12.h"

namespace PigeonEngine
{

	/*
	* Internal D3D12 resource types.
	*
	* Each implements the corresponding IRD3D12xxx mid-interface so
	* RCommandListD3D12 can retrieve native handles through virtual calls.
	*/

	class RD3D12Buffer final : public IRD3D12Buffer
	{
	public:
		Microsoft::WRL::ComPtr<ID3D12Resource>		Resource;
		D3D12_GPU_VIRTUAL_ADDRESS					GPUVirtualAddress;
		D3D12_CPU_DESCRIPTOR_HANDLE					SRVHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE					UAVHandle;
		UINT64										ByteSize;
		UINT32										StructureByteStride;
		UINT8										BindFlags;
		UINT32										BindlessSRVIndex;
		UINT32										BindlessUAVIndex;

		RD3D12Buffer() noexcept
			: GPUVirtualAddress(0)
			, SRVHandle{ 0 }
			, UAVHandle{ 0 }
			, ByteSize{ 0ull }
			, StructureByteStride{ 0u }
			, BindFlags{ 0u }
			, BindlessSRVIndex{ RHI_INVALID_BINDLESS_INDEX }
			, BindlessUAVIndex{ RHI_INVALID_BINDLESS_INDEX }
		{
		}
		virtual ~RD3D12Buffer() noexcept {}

		// IRRHIBuffer
		virtual UINT64		GetByteSize() const noexcept override { return ByteSize; }
		virtual UINT32		GetStructureByteStride() const noexcept override { return StructureByteStride; }
		virtual UINT8		GetBindFlags() const noexcept override { return BindFlags; }
		virtual UINT32		GetBindlessSRVIndex() const noexcept override { return BindlessSRVIndex; }
		virtual UINT32		GetBindlessUAVIndex() const noexcept override { return BindlessUAVIndex; }

		// IRD3D12Buffer
		virtual ID3D12Resource*					GetD3D12Resource() const noexcept override { return Resource.Get(); }
		virtual D3D12_GPU_VIRTUAL_ADDRESS		GetD3D12GPUVirtualAddress() const noexcept override { return GPUVirtualAddress; }
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12SRVHandle() const noexcept override { return SRVHandle; }
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12UAVHandle() const noexcept override { return UAVHandle; }
	};

	class RD3D12Texture final : public IRD3D12Texture
	{
	public:
		Microsoft::WRL::ComPtr<ID3D12Resource>		Resource;
		D3D12_CPU_DESCRIPTOR_HANDLE					SRVHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE					RTVHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE					DSVHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE					UAVHandle;
		D3D12_RESOURCE_STATES						DefaultState;
		UINT32										Width;
		UINT32										Height;
		UINT32										Depth;
		UINT32										MipLevels;
		UINT32										ArraySize;
		RFormatType									Format;
		// The formats the resource and its views were created with. Format is the content
		// format the desc asked for; this is what the API objects actually carry, which is
		// what the render target / pipeline state comparison reads (GetViewFormat).
		RTextureFormatSet							ViewFormats;
		UINT8										BindFlags;
		UINT32										BindlessSRVIndex;
		UINT32										BindlessUAVIndex;

		RD3D12Texture()
			: SRVHandle{ 0 }
			, RTVHandle{ 0 }
			, DSVHandle{ 0 }
			, UAVHandle{ 0 }
			, DefaultState(D3D12_RESOURCE_STATE_COMMON)
			, Width(0u), Height(0u), Depth(0u)
			, MipLevels(1u), ArraySize(1u)
			, Format(RFormatType::FORMAT_UNKNOWN)
			, BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX)
			, BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX)
		{
		}
		virtual ~RD3D12Texture() {}

		// IRRHITexture
		virtual UINT32			GetWidth() const noexcept override { return Width; }
		virtual UINT32			GetHeight() const noexcept override { return Height; }
		virtual UINT32			GetDepth() const noexcept override { return Depth; }
		virtual UINT32			GetMipLevels() const noexcept override { return MipLevels; }
		virtual UINT32			GetArraySize() const noexcept override { return ArraySize; }
		virtual RFormatType		GetFormat() const noexcept override { return Format; }
		virtual UINT8			GetBindFlags() const noexcept override { return BindFlags; }
		virtual UINT32			GetBindlessSRVIndex() const noexcept override { return BindlessSRVIndex; }
		virtual UINT32			GetBindlessUAVIndex() const noexcept override { return BindlessUAVIndex; }
		virtual RFormatType		GetViewFormat(ERHIViewFormatType InView) const noexcept override { return ViewFormats.GetViewFormat(InView); }

		// IRD3D12Texture
		virtual ID3D12Resource*					GetD3D12Resource() const noexcept override { return Resource.Get(); }
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12SRVHandle() const noexcept override { return SRVHandle; }
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12RTVHandle() const noexcept override { return RTVHandle; }
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12DSVHandle() const noexcept override { return DSVHandle; }
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12UAVHandle() const noexcept override { return UAVHandle; }
		virtual D3D12_RESOURCE_STATES			GetD3D12DefaultState() const noexcept override { return DefaultState; }
	};

	class RD3D12Shader final : public IRD3D12Shader
	{
	public:
		TArray<UINT8, UINT64>		ByteCode;
		ERHIShaderStage				Stage;

		RD3D12Shader()
			: ByteCode{}
			, Stage{ ERHIShaderStage::RHI_SHADER_STAGE_COUNT }
		{
		}
		virtual ~RD3D12Shader()
		{
			ByteCode.Empty();
		}

		// IRRHIShader
		virtual ERHIShaderStage		GetShaderStage() const noexcept override { return Stage; }

		// IRD3D12Shader
		virtual const void*			GetByteCode() const noexcept override { return (ByteCode.GetData()); }
		virtual UINT64				GetByteCodeSize() const noexcept override { return ByteCode.Num<UINT64>(); }
	};

	class RD3D12Sampler final : public IRD3D12Sampler
	{
	public:
		D3D12_CPU_DESCRIPTOR_HANDLE		SamplerHandle;

		RD3D12Sampler() : SamplerHandle{} {}
		virtual ~RD3D12Sampler() {}

		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12SamplerHandle() const noexcept override { return SamplerHandle; }
	};

	/*
	* D3D12 root signature. The device owns these (see RDeviceD3D12's signature
	* cache / pool); a PSO created against one keeps its own reference to the native
	* object so its draws stay valid regardless of cache teardown order.
	*
	* LayoutInfo is the binding plan that was generated together with the signature:
	* which root carries which table / constant block. The command list reads it off
	* the bound PSO instead of assuming a fixed set of root indices.
	*/
	class RD3D12RootSignature final : public IRRHIRootSignature
	{
	public:
		Microsoft::WRL::ComPtr<ID3D12RootSignature>		Signature;
		UINT64											LayoutHash;
		RRHIPipelineLayoutInfo							LayoutInfo;

		RD3D12RootSignature() noexcept
			: Signature{}
			, LayoutHash{ 0ull }
			, LayoutInfo{}
		{
		}
		virtual ~RD3D12RootSignature() noexcept {}

		// IRRHIRootSignature
		virtual UINT64	GetLayoutHash() const noexcept override { return LayoutHash; }
		virtual const RRHIPipelineLayoutInfo&	GetLayoutInfo() const noexcept override { return LayoutInfo; }

		ID3D12RootSignature*	GetD3D12RootSignature() const noexcept { return Signature.Get(); }
	};

	class RD3D12PipelineState final : public IRD3D12PipelineState
	{
	public:
		Microsoft::WRL::ComPtr<ID3D12PipelineState>		PipelineState;
		// Native signature the PSO was created with. Held as a reference so a PSO
		// never outlives its signature even if the device's cache is torn down first.
		Microsoft::WRL::ComPtr<ID3D12RootSignature>		RootSignature;
		// Non-owning: points into RDeviceD3D12's signature pool, which the device
		// releases after the PSO cache, so it stays valid for the whole PSO lifetime.
		IRRHIRootSignature*								RHIRootSignature;
		// Binding plan of RootSignature, copied so binding needs no indirection.
		RRHIPipelineLayoutInfo							LayoutInfo;
		D3D12_PRIMITIVE_TOPOLOGY						PrimitiveTopology;
		// The formats the state was created with. D3D12 requires a bound render target's
		// format to equal the pipeline state's, so the command list compares these against
		// what the pass bound instead of leaving the mismatch to the debug layer.
		RFormatType										RenderTargetFormats[RHI_MAX_RENDER_TARGETS];
		UINT32											RenderTargetFormatNum;
		RFormatType										DepthStencilFormat;
		// Key used for PSO cache lookup; rebuilt from RRHIGraphicsPipelineDesc.
		// 0 means the PSO is not cache-keyed (compute PSOs are owned by their creator).
		UINT64											PipelineStateHash;
		// TRUE for PSOs handed out by CreateComputePipelineState. Mirrors
		// RPipelineStateD3D11::bIsCompute so IsCompute() reports the real kind.
		BOOL8											bIsCompute;
		// TRUE once the PSO was handed to DestroyResource. The cache entry is dropped at
		// that moment, so a second destroy of the same pointer (an owner releasing what the
		// cache also holds) is a no-op instead of a second queue entry and a second delete.
		BOOL8											bPendingDelete;

		RD3D12PipelineState()
			: PipelineState{}
			, RootSignature{}
			, RHIRootSignature(nullptr)
			, LayoutInfo{}
			, PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
			, RenderTargetFormats{}
			, RenderTargetFormatNum(0u)
			, DepthStencilFormat(RFormatType::FORMAT_UNKNOWN)
			, PipelineStateHash(0ull)
			, bIsCompute(FALSE)
			, bPendingDelete(FALSE)
		{
		}
		virtual ~RD3D12PipelineState() {}

		// IRRHIPipelineState
		virtual BOOL8							IsCompute() const noexcept override { return bIsCompute; }
		virtual IRRHIRootSignature*				GetRootSignature() const noexcept override { return RHIRootSignature; }
		virtual const RRHIPipelineLayoutInfo&	GetLayoutInfo() const noexcept override { return LayoutInfo; }
		virtual UINT32							GetRenderTargetFormatNum() const noexcept override { return RenderTargetFormatNum; }
		virtual RFormatType						GetRenderTargetFormat(UINT32 InIndex) const noexcept override
		{
			return (InIndex < RenderTargetFormatNum) ? RenderTargetFormats[InIndex] : RFormatType::FORMAT_UNKNOWN;
		}
		virtual RFormatType						GetDepthStencilFormat() const noexcept override { return DepthStencilFormat; }

		// IRD3D12PipelineState
		virtual ID3D12PipelineState*			GetD3D12PipelineState() const noexcept override { return PipelineState.Get(); }
		virtual ID3D12RootSignature*			GetD3D12RootSignature() const noexcept override { return RootSignature.Get(); }
		virtual D3D12_PRIMITIVE_TOPOLOGY		GetD3D12PrimitiveTopology() const noexcept override { return PrimitiveTopology; }
	};

	// D3D12 timestamp heap (Phase P.d). Lives in the header so RDeviceD3D12
	// (creation / readback resolve / frequency cache) and RCommandListD3D12
	// (per-slot EndQuery + ResolveQueryData emit) both see the layout.
	//
	// Flow:
	//   1) device->CreateTimestampHeap                   creates QueryHeap + readback
	//   2) cmd->WriteTimestamp(heap, slot)               EndQuery into QueryHeap
	//   3) cmd->ResolveTimestampHeap(heap, beg, count)   ResolveQueryData -> Readback
	//   4) (submit + fence-wait for the frame to retire)
	//   5) device->ResolveTimestamps                     Map readback, memcpy ticks out
	struct RRHITimestampHeapD3D12 : public RRHITimestampHeap
	{
		Microsoft::WRL::ComPtr<ID3D12QueryHeap>		QueryHeap;
		Microsoft::WRL::ComPtr<ID3D12Resource>		ReadbackBuffer;
		UINT64										TicksPerSecond		= 0u;		// constant per device, latched at creation
	};

	class RDeviceD3D12 : public EManagerBase, public IRRHIDevice
	{
	public:
		// EManagerBase
		virtual void				Initialize() override;
		virtual void				ShutDown() override;

		// IRRHIDevice - Lifecycle
		virtual BOOL8				Initialize(const RRHIDeviceInitDesc& InDesc) noexcept override;
		virtual void				Shutdown() noexcept override;
		virtual ERHIBackendType		GetBackendType() const noexcept override { return ERHIBackendType::RHI_BACKEND_D3D12; }
		virtual BOOL8				SupportsRootConstants() const noexcept override { return TRUE; }

		// IRRHIDevice - Frame fencing
		virtual void				BeginFrame() noexcept override;
		virtual void				EndFrame() noexcept override;
		virtual void				Present(UINT32 InSyncInterval) noexcept override;
		virtual BOOL8				ResizeSwapChain(UINT32 InWidth, UINT32 InHeight) noexcept override;
		virtual IRRHISwapChain*		CreateSwapChainForWindow(HWND InWindowHandle, UINT32 InWidth, UINT32 InHeight, RFormatType InFormat) noexcept override;
		virtual void				WaitForGPUIdle() noexcept override;

		// IRRHIDevice - Resource creation
		virtual BOOL8				CreateBuffer(const RBufferDesc& InDesc, const RRHISubresourceData& InInitialData, IRRHIBuffer** OutBuffer) noexcept override;
		virtual BOOL8				CreateTexture(const RTextureDesc& InDesc, const Color4& InClearColor, const RRHISubresourceData& InInitialSubresources, IRRHITexture** OutTexture) noexcept override;
		virtual BOOL8				CreateShader(const RRHIShaderDesc& InDesc, IRRHIShader** OutShader) noexcept override;
		virtual BOOL8				CreateRootSignature(const RRHIRootSignatureDesc& InDesc, IRRHIRootSignature** OutSignature) noexcept override;
		virtual void				DestroyRootSignature(IRRHIRootSignature* InSignature) noexcept override;
		virtual BOOL8				CreateGraphicsPipelineState(const RRHIGraphicsPipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) noexcept override;
		virtual BOOL8				CreateComputePipelineState(const RRHIComputePipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) noexcept override;
		virtual BOOL8				CreateSampler(const RSamplerState& InDesc, IRRHISampler** OutSampler) noexcept override;
		virtual void				DestroyResource(IRRHIResource* InResource) noexcept override;

		// IRRHIDevice - Bindless
		virtual UINT32				RegisterBindlessSRV(IRRHITexture* InTexture) noexcept override;
		virtual UINT32				RegisterBindlessSRV(IRRHIBuffer* InBuffer) noexcept override;
		virtual UINT32				RegisterBindlessUAV(IRRHITexture* InTexture) noexcept override;
		virtual UINT32				RegisterBindlessUAV(IRRHIBuffer* InBuffer) noexcept override;
		virtual void				ReleaseBindlessIndex(UINT32 InIndex) noexcept override;
		virtual IRRHIResource*		GetBindlessResource(UINT32 InIndex) const noexcept override;

		// IRRHIDevice - Upload
		virtual void				UploadBuffer(IRRHIBuffer* InDest, const UINT8* InData, UINT64 InSize, UINT64 InDestOffset = 0) noexcept override;
		virtual void				UploadTexture(IRRHITexture* InDest, const RRHISubresourceData* InSubresources, UINT32 InSubresourceCount, UINT32 InFirstSubresource = 0u) noexcept override;

		// IRRHIDevice - Command lists
		virtual IRCommandList*		AcquireCommandList() noexcept override;
		virtual void				SubmitCommandList(IRCommandList* InCommandList) noexcept override;
		virtual void				SubmitCommandLists(IRCommandList* const* InCommandLists, UINT32 InCount) noexcept override;

		// IRRHIDevice - Backbuffer access
		virtual IRRHITexture*		GetCurrentBackBuffer() const noexcept override;
		virtual UINT32				GetCurrentFrameIndex() const noexcept override { return CurrentFrameIndex; }
		virtual UINT32				GetFrameInFlightCount() const noexcept override { return FrameInFlightCount; }

		// GPU timestamp queries (Phase P.d - stubbed in P.c)
		virtual BOOL8				CreateTimestampHeap(UINT32 InCapacity, RRHITimestampHeap** OutHeap) noexcept override;
		virtual void				DestroyTimestampHeap(RRHITimestampHeap* InHeap) noexcept override;
		virtual void				BeginTimestampFrame(RRHITimestampHeap* InHeap) noexcept override;
		virtual void				EndTimestampFrame(RRHITimestampHeap* InHeap) noexcept override;
		virtual BOOL8				ResolveTimestamps(RRHITimestampHeap* InHeap, UINT32 InBeginSlot, UINT32 InCount, UINT64* OutTicks) noexcept override;
		virtual UINT64				GetGPUTicksPerSecond(const RRHITimestampHeap* InHeap) const noexcept override;

		// Native access for command list
		Microsoft::WRL::ComPtr<ID3D12Device>		GetD3D12Device() const { return Device; }
		D3D12_CPU_DESCRIPTOR_HANDLE					GetRTVHeapStart() const { return RTVHeap->GetCPUDescriptorHandleForHeapStart(); }
		D3D12_CPU_DESCRIPTOR_HANDLE					GetDSVHeapStart() const { return DSVHeap->GetCPUDescriptorHandleForHeapStart(); }
		D3D12_CPU_DESCRIPTOR_HANDLE					GetCBVSRVUAVHeapStart() const { return CBVSRVUAVHeap->GetCPUDescriptorHandleForHeapStart(); }
		D3D12_GPU_DESCRIPTOR_HANDLE					GetCBVSRVUAVHeapGPUStart() const { return CBVSRVUAVHeap->GetGPUDescriptorHandleForHeapStart(); }
		D3D12_CPU_DESCRIPTOR_HANDLE					GetSamplerHeapStart() const { return SamplerHeap->GetCPUDescriptorHandleForHeapStart(); }
		D3D12_CPU_DESCRIPTOR_HANDLE					GetCBVSRVUAVStagingHeapStart() const { return CBVSRVUAVStagingHeap->GetCPUDescriptorHandleForHeapStart(); }
		D3D12_CPU_DESCRIPTOR_HANDLE					GetSamplerStagingHeapStart() const { return SamplerStagingHeap->GetCPUDescriptorHandleForHeapStart(); }
		UINT32										GetRTVDescriptorSize() const { return RTVDescriptorSize; }
		UINT32										GetDSVDescriptorSize() const { return DSVDescriptorSize; }
		UINT32										GetCBVSRVUAVDescriptorSize() const { return CBVSRVUAVDescriptorSize; }
		UINT32										GetSamplerDescriptorSize() const { return SamplerDescriptorSize; }
		UINT32										GetBackBufferCount() const { return BackBufferCount; }
		ID3D12DescriptorHeap*						GetCBVSRVUAVHeap() const { return CBVSRVUAVHeap.Get(); }
		ID3D12DescriptorHeap*						GetSamplerHeap() const { return SamplerHeap.Get(); }
		ID3D12CommandAllocator*						GetCurrentCommandAllocator() const { return CommandAllocators[CurrentFrameIndex].Get(); }

		// Persistent descriptor allocators (non-resetting; resource lifetime).
		UINT32				AllocatePersistentRTV();
		UINT32				AllocatePersistentDSV();
		UINT32				AllocatePersistentSRVUAV();
		UINT32				AllocatePersistentSampler();
		UINT32				AllocateUploadMemory(UINT64 InSize, UINT64 InAlignment, void** OutCPUAddress, D3D12_GPU_VIRTUAL_ADDRESS* OutGPUAddress);

		// Per-frame descriptor regions. A resource set - the descriptors one draw binds -
		// is allocated inside the frame's set arena by the command list, because descriptor
		// contents are not versioned: two draws that share a slot and bind different
		// resources would both read the last one written. The sampler table is a fixed
		// region instead, because every program samples through the renderer wide samplers.
		UINT32				GetSlotSetArenaBase() const;
		UINT32				GetSlotSetArenaCapacity() const;
		UINT32				GetSlotSamplerTableBase() const;

		// Writes a null view into the CBV_SRV_UAV heap at the given descriptor index. The
		// command list starts every resource set from these, so a register a program declares
		// but its frame does not bind reads zero: an earlier frame or draw's descriptor may
		// name a resource that has since been released, and a shader that reads it would be
		// reading released memory.
		void				WriteNullShaderResourceView(UINT32 InHeapIndex);
		void				WriteNullUnorderedAccessView(UINT32 InHeapIndex);

		// Lazy-open the dedicated upload command list (separate from per-frame
		// allocator). Auto-closed and submitted on EndFrame.
		void				FlushPendingUploads();

		// Public translation helpers used by command list
		static void			TranslateResourceStates(D3D12_RESOURCE_STATES& OutStates, UINT16 InStates) noexcept;
		static void			TranslatePrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY& OutTopology, RPrimitiveTopologyType InTopology) noexcept;

	private:
		static void			TranslateResourceFormat(DXGI_FORMAT& OutFormat, RFormatType InFormat) noexcept;
		static void			TranslateInputElementDesc(D3D12_INPUT_ELEMENT_DESC& OutDesc, const RInputLayoutDesc& InDesc) noexcept;
		static void			TranslateCullMode(D3D12_CULL_MODE& OutCull, RCullModeType InCull) noexcept;
		static void			TranslateFillMode(D3D12_FILL_MODE& OutFill, RFillModeType InFill) noexcept;
		static void			TranslateRenderTargetBlendDesc(D3D12_BLEND_DESC& OutBlend, const RBlendDesc& InBlend) noexcept;
		static void			TranslateDepthStencilDesc(D3D12_DEPTH_STENCIL_DESC& OutDS, const RDepthState& InDepth, const RStencilState& InStencil) noexcept;
		static void			TranslateSamplerDesc(D3D12_SAMPLER_DESC& OutSampler, const RSamplerState& InSampler) noexcept;
		/*
		* The two halves of D3D12_CLEAR_VALUE, kept apart on purpose.
		*
		* The struct holds the colour and the depth/stencil values in one union, so exactly one of
		* these may be applied to a given resource. A single function taking both would write the
		* depth value over the red channel and the stencil over the green one, and the resource
		* would then be registered with an optimised clear value that no clear call can match -
		* which D3D12 reports as EXECUTION WARNING #820 and answers by taking the slow clear path.
		*/
		static void			TranslateRenderTargetClearValue(D3D12_CLEAR_VALUE& OutClear, RFormatType InFormat, const Color4& InColor) noexcept;
		static void			TranslateDepthStencilClearValue(D3D12_CLEAR_VALUE& OutClear, RFormatType InFormat, FLOAT InDepth, UINT8 InStencil) noexcept;
		static void			TranslateRootParameterVisibility(D3D12_SHADER_VISIBILITY& OutVisibility, ERHIShaderVisibility InVisibility) noexcept;
		// Derives the binding layout from a signature's parameter list. The parameter
		// order is the contract (constant buffer roots first, then the 32 bit constant
		// block, then the tables), so the layout is derived rather than handed in
		// separately and cannot disagree with the signature it describes.
		static void			BuildLayoutInfo(const RRHIRootSignatureDesc& InDesc, RRHIPipelineLayoutInfo& OutLayoutInfo) noexcept;
		// InLayoutHash is the layout of the signature the PSO is built against, which is the
		// one the desc names, so two descs that share a signature share a cache entry.
		static UINT64		HashPipelineStateDesc(const RRHIGraphicsPipelineDesc& InDesc, UINT64 InLayoutHash) noexcept;

		BOOL8				CreateDeviceAndQueue();
		BOOL8				CreateSwapChain(HWND InWindowHandle);
		BOOL8				CreateDescriptorHeaps();
		BOOL8				CreateFrameFence();
		BOOL8				CreateUploadHeap();
		void				CreateBackBufferViews();
		void				ReleaseBackBufferViews();
		void				WaitForFrameFence(UINT32 InFrameIndex);

	private:
		// Core D3D12
		Microsoft::WRL::ComPtr<ID3D12Device>			Device;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>		CommandQueue;

		// Swap chain
		Microsoft::WRL::ComPtr<IDXGISwapChain3>			SwapChain;
		UINT32											BackBufferCount;
		UINT32											BackBufferWidth;
		UINT32											BackBufferHeight;
		RFormatType										BackBufferFormat;
		RFormatType										DepthStencilFormat;
		BOOL8											bIsWindowed;

		// Back buffers (RD3D12Texture wrappers)
		TArray<RD3D12Texture*>							BackBuffers;

		// CBV_SRV_UAV heap sub-region layout (descriptor count offsets within heap):
		//   [0,    512)      Frame 0 set arena: the root tables of every set it binds
		//     +0   .. +64    reserved: the fixed slot table bases the arena replaced
		//     +64  .. +512   sets, one SRV/UAV table pair each, allocated by the command list
		//   [512, 1024)      Frame 1 set arena (same split)
		//   [1024, 16384)    Bindless SRV pool       (HLSL ResourceDescriptorHeap[idx])
		//   [16384, 32768)   Bindless UAV pool
		//   [32768, 65536)   Persistent resource-creation descriptors
		// Sampler heap layout:
		//   [0,  8)          Frame 0 sampler table (the renderer wide samplers, s0..s6)
		//   [16, 24)         Frame 1 sampler table
		//   [32, 256)        Persistent samplers
		static constexpr UINT32		CBVSRVUAV_HEAP_SIZE					= 65536u;
		static constexpr UINT32		SAMPLER_HEAP_SIZE					= 256u;
		static constexpr UINT32		RTV_HEAP_SIZE						= 64u;
		static constexpr UINT32		DSV_HEAP_SIZE						= 32u;
		static constexpr UINT32		SLOT_TABLE_STRIDE_PER_FRAME			= 512u;
		static constexpr UINT32		SLOT_SET_ARENA_OFFSET				= 64u;
		static constexpr UINT32		SLOT_SAMPLER_STRIDE_PER_FRAME		= 16u;
		static constexpr UINT32		BINDLESS_SRV_BASE_INDEX				= 1024u;
		static constexpr UINT32		BINDLESS_SRV_CAPACITY				= 15360u;
		static constexpr UINT32		BINDLESS_UAV_BASE_INDEX				= 16384u;
		static constexpr UINT32		BINDLESS_UAV_CAPACITY				= 16384u;
		static constexpr UINT32		PERSISTENT_DESC_BASE_INDEX			= 32768u;
		static constexpr UINT32		PERSISTENT_DESC_CAPACITY			= 32768u;
		static constexpr UINT32		PERSISTENT_SAMPLER_BASE_INDEX		= 32u;
		static constexpr UINT32		PERSISTENT_SAMPLER_CAPACITY			= 224u;

		// Frame resources (2 frames in flight)
		static constexpr UINT32		FRAME_IN_FLIGHT						= 2u;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		RTVHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		DSVHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		CBVSRVUAVHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		SamplerHeap;
		// Phase 4: CPU-only staging heaps. Persistent descriptors are written
		// here first (CPU-readable, hence valid as CopyDescriptorsSimple source)
		// then mirrored into the shader-visible heaps at the same slot index.
		// Shader-visible heaps remain SHADER_VISIBLE (CPU-write-only), so any
		// later descriptor copy must source from the staging heap, never from
		// the shader-visible heap itself.
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		CBVSRVUAVStagingHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		SamplerStagingHeap;

		UINT32												RTVDescriptorSize;
		UINT32												DSVDescriptorSize;
		UINT32												CBVSRVUAVDescriptorSize;
		UINT32												SamplerDescriptorSize;

		// Persistent (non-resetting) bump allocators
		UINT32												RTVDescriptorOffset;
		UINT32												DSVDescriptorOffset;
		UINT32												PersistentDescriptorOffset;
		UINT32												PersistentSamplerOffset;

		// Root signatures. Every signature the device hands out lives in the pool for
		// the device's whole lifetime and is reachable through the cache by layout hash,
		// so a repeated request for one layout returns the object that is already live.
		// There is no device default: a pipeline state names the signature its program's
		// reflection generated, and a desc without one is refused.
		TArray<RD3D12RootSignature*>						RootSignaturePool;
		TMap<UINT64, RD3D12RootSignature*>					RootSignatureCache;

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		CommandAllocators[FRAME_IN_FLIGHT];
		Microsoft::WRL::ComPtr<ID3D12Fence>					FrameFence;
		UINT64												FrameFenceValues[FRAME_IN_FLIGHT];
		HANDLE												FrameFenceEvent;
		UINT32												CurrentFrameIndex;
		UINT32												FrameInFlightCount;
		// Monotonically increasing fence counter. Each Submit / Flush bumps this
		// and signals it on CommandQueue. FrameFenceValues[i] records the last
		// signaled value while CurrentFrameIndex == i so BeginFrame can wait on
		// the prior occupancy of this slot before reusing its allocator.
		UINT64												GlobalFenceCounter;

		// Command list pool
		TArray<TUniquePtr<RCommandListD3D12>>				CommandListPool;
		TArray<RCommandListD3D12*>							CommandListFreeList;

		// Upload heap (ring buffer, reset per frame)
		Microsoft::WRL::ComPtr<ID3D12Resource>				UploadHeap;
		void*												UploadHeapBase;
		UINT64												UploadHeapSize;
		UINT64												UploadHeapOffset;

		// Dedicated upload command list + allocator (NOT reusing per-frame
		// allocator). Opened lazily by UploadBuffer / UploadTexture, flushed
		// on FlushPendingUploads / EndFrame.
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	UploadCommandList;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		UploadCommandAllocator;
		BOOL8												bUploadCommandListOpen;
		UINT64												LastUploadFenceValue;

		// Bindless virtual handle tables (full implementation in Phase 4)
		TArray<IRRHIResource*>								BindlessSRVTable;
		TArray<IRRHIResource*>								BindlessUAVTable;
		TArray<UINT32>										BindlessSRVFreeIndices;
		TArray<UINT32>										BindlessUAVFreeIndices;

		// PSO cache
		TMap<UINT64, RD3D12PipelineState*>					PipelineStateCache;

		// Pending deletions (N+2 frame deferred)
		struct FDeferredDeleteEntry
		{
			IRRHIResource*		Resource;
			UINT32				RetireFrameIndex;
		};
		TArray<FDeferredDeleteEntry>						DeferredDeleteQueue;

		// Debug layer
		BOOL8												bEnableDebugLayer;
		BOOL8												bEnableGPUValidation;

		// Init state
		BOOL8												bInitialized;

	public:
		RDeviceD3D12();
		~RDeviceD3D12();
		RDeviceD3D12(const RDeviceD3D12&) = delete;
		RDeviceD3D12& operator=(const RDeviceD3D12&) = delete;

		static RDeviceD3D12* GetDeviceSingleton()
		{
			static RDeviceD3D12 DeviceSingletonObject;
			return (&DeviceSingletonObject);
		}
	};

};	// PigeonEngine
