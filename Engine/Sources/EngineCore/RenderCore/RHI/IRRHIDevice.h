#pragma once

#include <CoreMinimal.h>
#include <RHI/RHITypes.h>
#include <RHI/IRRHIResource.h>
#include <RHI/IRCommandList.h>
#include <RHI/RHIResources.h>
#include <RHI/IRRHIRootSignature.h>

namespace PigeonEngine
{

	// Opaque base for GPU timestamp scratch storage. Backends derive (D3D11
	// = ID3D11Query pool + disjoint, D3D12 = ID3D12QueryHeap + readback) and
	// engine code only ever holds the base pointer. Capacity is fixed at
	// creation time so writes are O(1) and bookkeeping is just a slot ring
	// maintained at a higher layer (Phase P.e wires the renderer).
	struct RRHITimestampHeap
	{
		RRHITimestampHeap()					= default;
		virtual ~RRHITimestampHeap()		= default;

		UINT32		Capacity		= 0u;
	};

	/*
	* Pure-virtual rendering device. Implemented by RDeviceD3D11 and
	* RDeviceD3D12; selected at startup via EEngineSettings::GraphicsAPI.
	*
	* Threading model (see plan Section3):
	*   - Initialize / Shutdown / ResizeSwapChain / Present run on the
	*     RenderThread (main thread for D3D11 backend during transition).
	*   - BeginFrame / EndFrame frame the render-side of the lockstep
	*     handshake; they are called once per frame from the RenderThread.
	*   - AcquireCommandList / SubmitCommandList are thread-safe and may
	*     be called from worker recording threads.
	*   - Resource creation (CreateBuffer / CreateTexture / ...) is
	*     thread-safe; back-ends serialize internally if needed.
	*
	* Bindless model:
	*   - SRV / UAV access is via UINT32 indices into a global descriptor
	*     heap. RegisterBindless* returns an index; the index lives until
	*     ReleaseBindlessIndex is called.
	*   - For the D3D11 back-end the index is a virtual handle the
	*     command list resolves back to ID3D11ShaderResourceView* before
	*     Draw, so the same shader source path can target both back-ends
	*     once each is rebuilt for its API.
	*/
	class IRRHIDevice
	{
	public:
		IRRHIDevice()				= default;
		virtual ~IRRHIDevice()		= default;

		// ---------- Lifecycle ----------
		virtual BOOL8		Initialize(const RRHIDeviceInitDesc& InDesc) noexcept			= 0;
		virtual void		Shutdown() noexcept												= 0;

		// Which back-end this device is. Engine paths that are still back-end specific gate
		// on this rather than on which concrete device class was constructed.
		virtual ERHIBackendType	GetBackendType() const noexcept								= 0;

		// Whether a shader block can be fed through the root signature's 32 bit constant
		// space. A back-end without that space binds the same block as a plain constant
		// buffer instead, so the binding side branches on this rather than on which
		// back-end is active.
		virtual BOOL8			SupportsRootConstants() const noexcept						= 0;

		// ---------- Frame fencing ----------
		// Wait for the GPU to be done with frame (Current - FRAME_IN_FLIGHT)
		// before recording new commands; reset per-frame command allocators.
		virtual void		BeginFrame() noexcept											= 0;

		// Submit any pending work, signal the frame fence, advance the
		// current frame index. Present is called separately so the caller
		// can choose VSync interval.
		virtual void		EndFrame() noexcept												= 0;

		virtual void		Present(UINT32 InSyncInterval) noexcept							= 0;
		virtual BOOL8		ResizeSwapChain(UINT32 InWidth, UINT32 InHeight) noexcept		= 0;

		// Block on GPU until everything submitted so far has retired.
		// Use sparingly (shutdown, resize, hot-reload).
		virtual void		WaitForGPUIdle() noexcept										= 0;

		// ---------- Resource creation ----------
		virtual BOOL8		CreateBuffer(const RBufferDesc& InDesc, const RRHISubresourceData& InInitialData, IRRHIBuffer** OutBuffer) noexcept																			= 0;
		virtual BOOL8		CreateTexture(const RTextureDesc& InDesc, const Color4& InClearColor, const RRHISubresourceData& InInitialSubresources, IRRHITexture** OutTexture) noexcept		= 0;
		virtual BOOL8		CreateShader(const RRHIShaderDesc& InDesc, IRRHIShader** OutShader) noexcept																										= 0;

		// Root signature. The back-end owns the object and returns the same handle
		// for an equal desc (identity is RRHIRootSignatureDesc::LayoutHash), so
		// repeated calls for one program variant do not create duplicates.
		// OutSignature is left untouched when the call fails.
		virtual BOOL8		CreateRootSignature(const RRHIRootSignatureDesc& InDesc, IRRHIRootSignature** OutSignature) noexcept																= 0;
		// Releases a signature created by CreateRootSignature. Callers must not hold
		// live PSOs created against it.
		virtual void		DestroyRootSignature(IRRHIRootSignature* InSignature) noexcept																									= 0;

		virtual BOOL8		CreateGraphicsPipelineState(const RRHIGraphicsPipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) noexcept																			= 0;
		virtual BOOL8		CreateComputePipelineState(const RRHIComputePipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) noexcept																			= 0;
		virtual BOOL8		CreateSampler(const RSamplerState& InDesc, IRRHISampler** OutSampler) noexcept																												= 0;

		// Destruction is queued through the device so back-ends can defer
		// release until the GPU is no longer using the resource.
		virtual void		DestroyResource(IRRHIResource* InResource) noexcept		= 0;

		// ---------- High-level resource factories ----------
		// These produce engine-side RRHIxxx wrappers that the asset / proxy
		// layer holds. They are non-virtual: each forwards to the virtual
		// low-level CreateBuffer / CreateTexture / CreateShader / CreateSampler
		// and populates the wrapper with the resulting IRRHIxxx* plus the
		// engine-level metadata. Engine code never touches back-end-specific
		// types - D3D11 vs D3D12 dispatch lives entirely behind the virtuals.
		//
		// Shaders have no factory here on purpose: a compiled stage is an
		// RShaderStageResource owned by its EShaderStageAsset, which already wraps
		// IRRHIShader and its reflection derived input layout.
		BOOL8				CreateVertexBufferResource(const TArray<UINT8, UINT64>& InVertexData, UINT32 InVertexCount, UINT32 InStride, RRHIVertexBuffer& OutResource) noexcept;
		BOOL8				CreateIndexBufferResource(const TArray<UINT8, UINT64>& InIndexData, UINT32 InIndexCount, ERHIIndexFormat InFormat, RRHIIndexBuffer& OutResource) noexcept;
		BOOL8				CreateConstantBufferResource(const TArray<UINT8, UINT64>& InInitialData, RRHIConstantBuffer& OutResource) noexcept;
		BOOL8				CreateStructuredBufferResource(const TArray<UINT8, UINT64>& InInitialData, UINT32 InElementCount, UINT32 InStride, UINT8 InBindFlags, RRHIStructuredBuffer& OutResource) noexcept;
		BOOL8				CreateTexture2DResource(const RTextureDesc& InDesc, const Color4& InClearColor, const RRHISubresourceData& InInitialSubresources, RRHITexture2D& OutResource) noexcept;
		BOOL8				CreateTextureCubeResource(const RTextureDesc& InDesc, const Color4& InClearColor, const RRHISubresourceData& InInitialSubresources, RRHITextureCube& OutResource) noexcept;
		BOOL8				CreateRenderTexture2DResource(const RTextureDesc& InDesc, const Color4& InClearColor, RRHIRenderTexture2D& OutResource) noexcept;
		BOOL8				CreateSamplerResource(const RSamplerState& InDesc, RRHISamplerHandle& OutResource) noexcept;

		// ---------- Bindless descriptor management ----------
		virtual UINT32		RegisterBindlessSRV(IRRHITexture* InTexture) noexcept		= 0;
		virtual UINT32		RegisterBindlessSRV(IRRHIBuffer* InBuffer) noexcept			= 0;
		virtual UINT32		RegisterBindlessUAV(IRRHITexture* InTexture) noexcept		= 0;
		virtual UINT32		RegisterBindlessUAV(IRRHIBuffer* InBuffer) noexcept			= 0;
		virtual void		ReleaseBindlessIndex(UINT32 InIndex) noexcept				= 0;

		// Reverse lookup - returns the IRRHIResource previously registered
		// under InIndex, or nullptr if the slot is free / out of range.
		// Engine code (e.g. EImGuiRenderer) needs this on the D3D11 path
		// because the legacy register-bound shaders take an IRRHITexture*
		// directly via SetShaderResourceView. On D3D12 the renderer feeds
		// the index into a CB instead, so this lookup is rarely called.
		virtual IRRHIResource*		GetBindlessResource(UINT32 InIndex) const noexcept		= 0;

		// ---------- Data upload (CPU -> GPU) ----------
		// Routes through a back-end-managed upload heap / staging buffer.
		virtual void				UploadBuffer(IRRHIBuffer* InDest, const UINT8* InData, UINT64 InSize, UINT64 InDestOffset = 0) noexcept													= 0;
		virtual void				UploadTexture(IRRHITexture* InDest, const RRHISubresourceData* InSubresources, UINT32 InSubresourceCount, UINT32 InFirstSubresource = 0u) noexcept		= 0;

		// ---------- Command lists ----------
		// Acquired lists are returned to the device's pool when submitted.
		virtual IRCommandList*		AcquireCommandList() = 0;
		virtual void				SubmitCommandList(IRCommandList* InCommandList) noexcept								= 0;
		virtual void				SubmitCommandLists(IRCommandList* const* InCommandLists, UINT32 InCount) noexcept		= 0;

		// ---------- Backbuffer access ----------
		virtual IRRHITexture*		GetCurrentBackBuffer() const noexcept		= 0;
		virtual UINT32				GetCurrentFrameIndex() const noexcept		= 0;
		virtual UINT32				GetFrameInFlightCount() const noexcept		= 0;

		// ---------- GPU timestamp queries (Phase P) ----------
		// The heap is a fixed-capacity buffer of timestamp slots plus the
		// backend-specific framing primitive (D3D11 disjoint query / D3D12
		// query heap + readback). Engine code allocates one heap up front,
		// calls BeginTimestampFrame / EndTimestampFrame per frame, writes
		// into slots via IRCommandList::WriteTimestamp, then once the GPU
		// has retired the work calls ResolveTimestamps to read raw ticks.
		// GetGPUTicksPerSecond reports the frequency last latched by the
		// most recent EndTimestampFrame (D3D11 may report a different
		// value per frame if the GPU was disjoint; D3D12 is constant).
		virtual BOOL8		CreateTimestampHeap(UINT32 InCapacity, RRHITimestampHeap** OutHeap) noexcept									= 0;
		virtual void		DestroyTimestampHeap(RRHITimestampHeap* InHeap) noexcept														= 0;
		virtual void		BeginTimestampFrame(RRHITimestampHeap* InHeap) noexcept															= 0;
		virtual void		EndTimestampFrame(RRHITimestampHeap* InHeap) noexcept															= 0;
		virtual BOOL8		ResolveTimestamps(RRHITimestampHeap* InHeap, UINT32 InBeginSlot, UINT32 InCount, UINT64* OutTicks) noexcept		= 0;
		virtual UINT64		GetGPUTicksPerSecond(const RRHITimestampHeap* InHeap) const noexcept											= 0;

	public:
		IRRHIDevice(const IRRHIDevice&) = delete;
		IRRHIDevice(IRRHIDevice&&) = delete;
		IRRHIDevice& operator=(const IRRHIDevice&) = delete;
		IRRHIDevice& operator=(IRRHIDevice&&) = delete;
	};

};
