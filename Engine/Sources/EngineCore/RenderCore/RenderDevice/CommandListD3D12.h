#pragma once

#include <CoreMinimal.h>
#include <d3d12.h>
#include <RHI/IRCommandList.h>

namespace PigeonEngine
{

class RDeviceD3D12;

/*
* IRCommandList implementation for the D3D12 back-end.
*
* Wraps a native ID3D12GraphicsCommandList. Acquired from
* RDeviceD3D12::AcquireCommandList and returned via SubmitCommandList.
* The device owns per-frame ID3D12CommandAllocators; Reset() uses the
* allocator for the current frame index.
*
* Descriptor binding model (Phase 3):
*   - CBVs are root descriptors (SetGraphicsRootConstantBufferView).
*   - SRV/UAV/Sampler use descriptor tables copied from the global
*     CBV_SRV_UAV heap. SetShaderResourceView copies one descriptor
*     into the resource set the draw currently binds.
*   - A resource set is the SRV/UAV table pair one draw reads. Descriptor
*     contents are not versioned, so every set gets its own descriptors out
*     of the frame's arena: two draws sharing a slot would both read the last
*     resource written into it (see EnsureResourceSet).
*   - In Phase 4 these slot-based helpers become virtualised bindless
*     indices and the shaders switch to SM 6.6 ResourceDescriptorHeap.
*/
class RCommandListD3D12 final : public IRCommandList
{
public:
	RCommandListD3D12(RDeviceD3D12* InOwnerDevice);
	virtual ~RCommandListD3D12();

	RCommandListD3D12(const RCommandListD3D12&) = delete;
	RCommandListD3D12& operator=(const RCommandListD3D12&) = delete;

	// Called by RDeviceD3D12 when reusing from pool
	void SetNativeList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> InList,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> InAllocator);

	// IRCommandList lifecycle
	virtual void Reset()override;
	virtual void Close()override;

	// IRCommandList render passes
	virtual void BeginRenderPass(const RRHIRenderPassDesc& InDesc)override;
	virtual void EndRenderPass()override;

	// IRCommandList barriers
	virtual void ResourceBarrier(IRRHIResource* InResource, ERHIResourceState InBefore, ERHIResourceState InAfter)override;

	// IRCommandList pipeline / fixed-function
	virtual void SetPipelineState(IRRHIPipelineState* InPipelineState)override;
	virtual void SetPrimitiveTopology(RPrimitiveTopologyType InTopology)override;
	virtual void SetViewports(const EViewport* InViewports, UINT32 InCount)override;
	virtual void SetScissorRects(const ERect* InRects, UINT32 InCount)override;
	virtual void SetBlendFactor(const Color4& InBlendFactor)override;
	virtual void SetStencilRef(UINT32 InStencilRef)override;

	// IRCommandList input assembly
	virtual void SetVertexBuffers(UINT32 InStartSlot, UINT32 InCount, IRRHIBuffer* const* InBuffers, const UINT32* InStrides, const UINT32* InOffsets)override;
	virtual void SetIndexBuffer(IRRHIBuffer* InBuffer, ERHIIndexFormat InFormat, UINT32 InOffset)override;

	// IRCommandList graphics root params
	virtual void SetGraphicsRootConstantBuffer(UINT32 InRootIndex, IRRHIBuffer* InBuffer, SIZE_T InOffset = 0)override;
	virtual void SetGraphicsRoot32BitConstants(UINT32 InRootIndex, const void* InData, UINT32 InNum32BitValues, UINT32 InOffsetIn32BitValues = 0)override;

	// IRCommandList compute root params
	virtual void SetComputeRootConstantBuffer(UINT32 InRootIndex, IRRHIBuffer* InBuffer, SIZE_T InOffset = 0)override;
	virtual void SetComputeRoot32BitConstants(UINT32 InRootIndex, const void* InData, UINT32 InNum32BitValues, UINT32 InOffsetIn32BitValues = 0)override;

	// IRCommandList slot-based bindings
	virtual void BeginResourceTable()override;
	virtual void SetShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot, IRRHITexture* InTexture)override;
	virtual void SetShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot, IRRHIBuffer* InBuffer)override;
	virtual void ClearShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot)override;
	virtual void SetUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot, IRRHITexture* InTexture)override;
	virtual void SetUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot, IRRHIBuffer* InBuffer)override;
	virtual void ClearUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot)override;
	virtual void SetSampler(ERHIShaderStage InStage, UINT32 InSlot, IRRHISampler* InSampler)override;

	// IRCommandList clears
	virtual void ClearRenderTarget(IRRHITexture* InTarget, const Color4& InColor)override;
	virtual void ClearDepthStencil(IRRHITexture* InTarget, UINT8 InClearFlags, FLOAT InDepth, UINT8 InStencil)override;
	virtual void ClearUnorderedAccessFloat(IRRHITexture* InTarget, const Color4& InValue)override;
	virtual void ClearUnorderedAccessUint(IRRHITexture* InTarget, const Vector4Int& InValue)override;

	// IRCommandList copy
	virtual void CopyResource(IRRHIResource* InDest, IRRHIResource* InSource)override;
	virtual void CopyBufferRegion(IRRHIBuffer* InDest, SIZE_T InDestOffset, IRRHIBuffer* InSource, SIZE_T InSourceOffset, SIZE_T InSize)override;

	// IRCommandList draw / dispatch
	virtual void Draw(UINT32 InVertexCount, UINT32 InInstanceCount, UINT32 InStartVertex, UINT32 InStartInstance)override;
	virtual void DrawIndexed(UINT32 InIndexCount, UINT32 InInstanceCount, UINT32 InStartIndex, INT32 InBaseVertex, UINT32 InStartInstance)override;
	virtual void Dispatch(UINT32 InThreadGroupCountX, UINT32 InThreadGroupCountY, UINT32 InThreadGroupCountZ)override;
	virtual void DispatchIndirect(IRRHIBuffer* InArgBuffer, SIZE_T InArgOffset)override;

	// IRCommandList debug markers
	virtual void BeginDebugMarker(const EString& InName)override;
	virtual void EndDebugMarker()override;

	// IRCommandList GPU timestamp (Phase P.d - stubbed in P.c)
	virtual void WriteTimestamp(RRHITimestampHeap* InHeap, UINT32 InSlot)override;
	virtual void ResolveTimestampHeap(RRHITimestampHeap* InHeap, UINT32 InBeginSlot, UINT32 InCount)override;

	// Native access
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetNativeCommandList()const { return CommandList; }

	// Bindless SRV mode. While FALSE - the only mode the engine produces today, since every
	// variant is compiled with PE_SHADER_BINDLESS = 0 - the slot-based SRV helper asserts its
	// slot against the API's input-resource slot count. Once a bound program is a bindless
	// variant its shaders index ResourceDescriptorHeap directly, the slot helpers are unused,
	// and the assertion is skipped.
	void SetBindlessSRVBindingsEnabled(BOOL8 InEnabled) { bBindlessSRVBindingsEnabled = (InEnabled != FALSE); }
	BOOL8 IsBindlessSRVBindingsEnabled() const { return bBindlessSRVBindingsEnabled; }

private:
	// What one shader stage's root signature binding currently is. D3D12 keeps the graphics
	// and compute root signatures side by side - binding one does not invalidate the other -
	// so each stage tracks its own signature and the layout that came with it.
	struct RBoundSignature
	{
		ID3D12RootSignature*	NativeSignature	= nullptr;
		RRHIPipelineLayoutInfo	LayoutInfo{};
	};

	// Binds a layout's descriptor tables on one stage and records where the slot bindings
	// have to write. Called whenever a root signature is (re)bound, because switching
	// signatures drops that stage's descriptor table bindings.
	void											BindLayoutTables(const RRHIPipelineLayoutInfo& InLayoutInfo, BOOL8 InIsCompute);

	// Root index bound of a stage's bound signature. Nothing bound means no valid root index:
	// the zero layout reports a parameter count of zero.
	UINT32											GetRootParameterBound(const RBoundSignature& InBound) const;

	// Makes sure the set a descriptor write targets can take it, and reports whether the
	// caller may write. A fresh set is opened when none is live, when the live set has already
	// been consumed by a draw, or when the live set was sized for a narrower table than the
	// bound layout declares. A set a draw has read is never written again: the GPU reads a
	// descriptor when the draw executes, not when it is recorded, so a later write into the
	// same set would change what that draw reads.
	BOOL32											EnsureResourceSet(ERHIShaderStage InStage, UINT32 InSlot, BOOL8 InIsUAV);
	// Takes a set out of the frame's arena, sized for the bound layout and the slot being
	// written, primes it with null views and points the bound signatures' tables at it.
	BOOL32											OpenResourceSet(const RRHIPipelineLayoutInfo& InLayout, UINT32 InSlot, BOOL8 InIsUAV);
	// Opens a set wide enough for a layout when the live one is not. Called when a program's
	// tables are bound, so a program reads its own set even for the registers this frame's
	// binds leave alone: its root table range is what the shader may touch.
	void											ReserveResourceSetWidth(const RRHIPipelineLayoutInfo& InLayout);
	// Declares the live set finished. Every draw calls it, which is what keeps a set from
	// being written after it has been read; a caller may also call it itself to ask for the
	// next write to land in a set of its own.
	void											SealResourceSet() noexcept;

	// Compares the formats of the targets the current pass bound against the formats the
	// pipeline state declares. D3D12 requires them to be equal - a mismatch is a device
	// removed at the first draw - and it is exactly the mistake a pass makes when a texture
	// stops being linear (or starts being sRGB): the pass still declares the old format.
	// Reported as an engine error naming both formats, instead of leaving a removed device
	// and a black frame behind.
	void											ValidateBoundTargetFormats(IRRHIPipelineState* InPipelineState) const;

	RDeviceD3D12*									OwnerDevice;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	CommandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>	CommandAllocator;
	// Per stage, see RBoundSignature. Nothing is bound when a recording starts: the first
	// SetPipelineState binds the signature its program carries (see SetNativeList).
	RBoundSignature									GraphicsSignature;
	RBoundSignature									ComputeSignature;
	// Descriptor offsets the slot bindings write into. The region is shared by both stages,
	// because a slot binding is a descriptor copy and the frame's region is the only one
	// there is; it follows the last bound layout.
	UINT32											CurrentSRVTableBase;
	UINT32											CurrentUAVTableBase;
	UINT32											CurrentSamplerTableBase;
	// The live resource set: where the next set comes from, how wide the live one is, and
	// whether a draw already read it. SetNativeList settles the arena from the device's per
	// frame offsets; everything else is settled as the frame records.
	UINT32											SetArenaNext;
	UINT32											SetArenaEnd;
	UINT32											LiveSetSRVWidth;
	UINT32											LiveSetUAVWidth;
	BOOL8											bResourceSetLive;
	BOOL8											bResourceSetSealed;
	BOOL8											bSetArenaWrapped;
	// TRUE once the region above is known, which SetNativeList settles from the device's
	// per frame offsets. A descriptor write only needs that; a root write needs a bound
	// signature on top, because a signature switch drops the roots (see HasBoundPipeline).
	BOOL8											bTableRegionKnown;
	D3D12_PRIMITIVE_TOPOLOGY						CurrentPrimitiveTopology;
	// Phase 2: tracks whether the most recent SetPipelineState landed a real
	// D3D12 PSO (vs a stub created by CreateGraphicsPipelineState in Phase 1
	// transition). DrawIndexed / Draw skip silently when FALSE so the proxy
	// migration can keep recording bindings without producing D3D12 errors.
	BOOL8											HasBoundPipeline;
	// See SetBindlessSRVBindingsEnabled: gates the slot-based SRV range assertion.
	BOOL8											bBindlessSRVBindingsEnabled;
	// The view formats of the targets the live pass bound, as the textures declare them (see
	// IRRHITexture::GetViewFormat). SetPipelineState compares them against the state's declared
	// formats; a texture that reports FORMAT_UNKNOWN is skipped, because a back-end that does not
	// track a view format cannot be checked rather than guessed at.
	RFormatType										BoundRenderTargetFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
	UINT32											BoundRenderTargetCount;
	RFormatType										BoundDepthStencilFormat;
};

}
