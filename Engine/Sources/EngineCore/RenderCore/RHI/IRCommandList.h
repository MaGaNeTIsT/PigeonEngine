#pragma once

#include <CoreMinimal.h>
#include <RHI/RHITypes.h>
#include <RHI/IRRHIResource.h>

namespace PigeonEngine
{

    /*
    * Pure-virtual command list. Concrete back-ends (D3D12: native command
    * list; D3D11: deferred-context wrapper) implement these methods and
    * are submitted via IRRHIDevice::SubmitCommandList.
    *
    * Threading: a single IRCommandList is bound to one thread between
    * AcquireCommandList and SubmitCommandList. Different worker threads
    * acquire different lists. The device's SubmitCommandList is
    * thread-safe.
    *
    * Resource bindings:
    *   - CBVs are bound by root index (root descriptor or 32-bit constant).
    *   - SRV / UAV access is bindless: shaders index ResourceDescriptorHeap
    *     directly via UINT32 indices baked into a CBV. The command list
    *     therefore has no SetGraphicsRootShaderResourceView; instead the
    *     application updates a small material/parameter CBV and binds it
    *     through SetGraphicsRootConstantBuffer.
    */
    class IRCommandList
    {
    public:
        IRCommandList() = default;
        virtual ~IRCommandList() = default;

        // ---------- Lifecycle ----------
        // Reset / Close are called by the device; user code does not call them.
        virtual void Reset() = 0;
        virtual void Close() = 0;

        // ---------- Render passes ----------
        virtual void BeginRenderPass(const RRHIRenderPassDesc& InDesc) = 0;
        virtual void EndRenderPass() = 0;

        // ---------- Resource state transitions ----------
        // D3D12 inserts a barrier; D3D11 records a no-op.
        virtual void ResourceBarrier(IRRHIResource* InResource, ERHIResourceState InBefore, ERHIResourceState InAfter) = 0;

        // ---------- Pipeline / fixed-function ----------
        virtual void SetPipelineState(IRRHIPipelineState* InPipelineState) = 0;
        virtual void SetPrimitiveTopology(RPrimitiveTopologyType InTopology) = 0;
        virtual void SetViewports(const RRHIViewport* InViewports, UINT32 InCount) = 0;
        virtual void SetScissorRects(const RRHIRect* InRects, UINT32 InCount) = 0;
        virtual void SetBlendFactor(const Color4& InBlendFactor) = 0;
        virtual void SetStencilRef(UINT32 InStencilRef) = 0;

        // ---------- Input assembly ----------
        virtual void SetVertexBuffers(UINT32 InStartSlot, UINT32 InCount, IRRHIBuffer* const* InBuffers, const UINT32* InStrides, const UINT32* InOffsets) = 0;
        virtual void SetIndexBuffer(IRRHIBuffer* InBuffer, ERHIIndexFormat InFormat, UINT32 InOffset) = 0;

        // ---------- Root parameters (graphics) ----------
        virtual void SetGraphicsRootConstantBuffer(UINT32 InRootIndex, IRRHIBuffer* InBuffer, SIZE_T InOffset = 0) = 0;
        virtual void SetGraphicsRoot32BitConstants(UINT32 InRootIndex, const void* InData, UINT32 InNum32BitValues, UINT32 InOffsetIn32BitValues = 0) = 0;

        // ---------- Root parameters (compute) ----------
        virtual void SetComputeRootConstantBuffer(UINT32 InRootIndex, IRRHIBuffer* InBuffer, SIZE_T InOffset = 0) = 0;
        virtual void SetComputeRoot32BitConstants(UINT32 InRootIndex, const void* InData, UINT32 InNum32BitValues, UINT32 InOffsetIn32BitValues = 0) = 0;

        // ---------- Slot-based SRV / UAV / Sampler bindings ----------
        // Used during the D3D11 transition; D3D12 backend implements these by
        // routing through the bindless heap (Phase 3). Phase 4 migrates
        // shaders to declare their own indices and these calls go away.
        virtual void SetShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot, IRRHITexture* InTexture) = 0;
        virtual void SetShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot, IRRHIBuffer* InBuffer) = 0;
        virtual void ClearShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot) = 0;
        virtual void SetUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot, IRRHITexture* InTexture) = 0;
        virtual void SetUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot, IRRHIBuffer* InBuffer) = 0;
        virtual void ClearUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot) = 0;
        virtual void SetSampler(ERHIShaderStage InStage, UINT32 InSlot, IRRHISampler* InSampler) = 0;

        // ---------- Clears ----------
        virtual void ClearRenderTarget(IRRHITexture* InTarget, const Color4& InColor) = 0;
        virtual void ClearDepthStencil(IRRHITexture* InTarget, UINT8 InClearFlags, FLOAT InDepth, UINT8 InStencil) = 0;
        virtual void ClearUnorderedAccessFloat(IRRHITexture* InTarget, const Color4& InValue) = 0;
        virtual void ClearUnorderedAccessUint(IRRHITexture* InTarget, const Vector4Int& InValue) = 0;

        // ---------- Copy ----------
        virtual void CopyResource(IRRHIResource* InDest, IRRHIResource* InSource) = 0;
        virtual void CopyBufferRegion(IRRHIBuffer* InDest, SIZE_T InDestOffset, IRRHIBuffer* InSource, SIZE_T InSourceOffset, SIZE_T InSize) = 0;

        // ---------- Draw / dispatch ----------
        virtual void Draw(UINT32 InVertexCount, UINT32 InInstanceCount, UINT32 InStartVertex, UINT32 InStartInstance) = 0;
        virtual void DrawIndexed(UINT32 InIndexCount, UINT32 InInstanceCount, UINT32 InStartIndex, INT32 InBaseVertex, UINT32 InStartInstance) = 0;
        virtual void Dispatch(UINT32 InThreadGroupCountX, UINT32 InThreadGroupCountY, UINT32 InThreadGroupCountZ) = 0;
        virtual void DispatchIndirect(IRRHIBuffer* InArgBuffer, SIZE_T InArgOffset) = 0;

        // ---------- Debug markers (optional, may be no-op in shipping) ----------
        virtual void BeginDebugMarker(const EString& InName) = 0;
        virtual void EndDebugMarker() = 0;

    public:
        IRCommandList(const IRCommandList&) = delete;
        IRCommandList(IRCommandList&&) = delete;
        IRCommandList& operator=(const IRCommandList&) = delete;
        IRCommandList& operator=(IRCommandList&&) = delete;
    };

};
