#pragma once

#include <CoreMinimal.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <RHI/IRCommandList.h>

namespace PigeonEngine
{

	class RDeviceD3D11;

	/*
	* IRCommandList implementation for the D3D11 back-end.
	*
	* D3D11 has only one immediate context that is not free-threaded;
	* deferred contexts exist but cannot be mixed with immediate-context
	* recording from other code paths (e.g. mesh / proxy classes that still
	* call RDeviceD3D11 directly). To keep the D3D11 path correct without
	* migrating every proxy in Phase 1, RCommandListD3D11 is an
	* "immediate-mode" wrapper: every call records straight onto the
	* device's shared immediate context. AcquireCommandList / Submit are
	* therefore essentially no-ops at the D3D11 level - they exist only to
	* satisfy the IRCommandList contract that Phase 3 (D3D12) and Phase 2
	* (multi-threaded recording) will rely on.
	*
	* Multi-threaded recording is delivered by the D3D12 back-end. The
	* D3D11 path stays single-threaded by design.
	*/
	class RCommandListD3D11 final : public IRCommandList
	{
	public:
		RCommandListD3D11(RDeviceD3D11* InOwnerDevice, const Microsoft::WRL::ComPtr<ID3D11Device>& InNativeDevice);
		virtual ~RCommandListD3D11();

		RCommandListD3D11(const RCommandListD3D11&) = delete;
		RCommandListD3D11& operator=(const RCommandListD3D11&) = delete;

		// IRCommandList lifecycle ---------------------------------------------
		virtual void Reset()override;
		virtual void Close()override;

		// IRCommandList render passes -----------------------------------------
		virtual void BeginRenderPass(const RRHIRenderPassDesc& InDesc)override;
		virtual void EndRenderPass()override;

		// IRCommandList barriers (releases a UAV write binding) ---------------
		virtual void ResourceBarrier(IRRHIResource* InResource, ERHIResourceState InBefore, ERHIResourceState InAfter)override;

		// IRCommandList pipeline / fixed-function ------------------------------
		virtual void SetPipelineState(IRRHIPipelineState* InPipelineState)override;
		virtual void SetPrimitiveTopology(RPrimitiveTopologyType InTopology)override;
		virtual void SetViewports(const EViewport* InViewports, UINT32 InCount)override;
		virtual void SetScissorRects(const ERect* InRects, UINT32 InCount)override;
		virtual void SetBlendFactor(const Color4& InBlendFactor)override;
		virtual void SetStencilRef(UINT32 InStencilRef)override;

		// IRCommandList input assembly ----------------------------------------
		virtual void SetVertexBuffers(UINT32 InStartSlot, UINT32 InCount, IRRHIBuffer* const* InBuffers, const UINT32* InStrides, const UINT32* InOffsets)override;
		virtual void SetIndexBuffer(IRRHIBuffer* InBuffer, ERHIIndexFormat InFormat, UINT32 InOffset)override;

		// IRCommandList graphics root params ----------------------------------
		virtual void SetGraphicsRootConstantBuffer(UINT32 InRootIndex, IRRHIBuffer* InBuffer, SIZE_T InOffset = 0)override;
		virtual void SetGraphicsRoot32BitConstants(UINT32 InRootIndex, const void* InData, UINT32 InNum32BitValues, UINT32 InOffsetIn32BitValues = 0)override;

		// IRCommandList compute root params -----------------------------------
		virtual void SetComputeRootConstantBuffer(UINT32 InRootIndex, IRRHIBuffer* InBuffer, SIZE_T InOffset = 0)override;
		virtual void SetComputeRoot32BitConstants(UINT32 InRootIndex, const void* InData, UINT32 InNum32BitValues, UINT32 InOffsetIn32BitValues = 0)override;

		// IRCommandList slot-based bindings -----------------------------------
		virtual void BeginResourceTable()override;
		virtual void SetShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot, IRRHITexture* InTexture)override;
		virtual void SetShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot, IRRHIBuffer* InBuffer)override;
		virtual void ClearShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot)override;
		virtual void SetUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot, IRRHITexture* InTexture)override;
		virtual void SetUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot, IRRHIBuffer* InBuffer)override;
		virtual void ClearUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot)override;
		virtual void SetSampler(ERHIShaderStage InStage, UINT32 InSlot, IRRHISampler* InSampler)override;

		// IRCommandList clears -------------------------------------------------
		virtual void ClearRenderTarget(IRRHITexture* InTarget, const Color4& InColor)override;
		virtual void ClearDepthStencil(IRRHITexture* InTarget, UINT8 InClearFlags, FLOAT InDepth, UINT8 InStencil)override;
		virtual void ClearUnorderedAccessFloat(IRRHITexture* InTarget, const Color4& InValue)override;
		virtual void ClearUnorderedAccessUint(IRRHITexture* InTarget, const Vector4Int& InValue)override;

		// IRCommandList copy ---------------------------------------------------
		virtual void CopyResource(IRRHIResource* InDest, IRRHIResource* InSource)override;
		virtual void CopyBufferRegion(IRRHIBuffer* InDest, SIZE_T InDestOffset, IRRHIBuffer* InSource, SIZE_T InSourceOffset, SIZE_T InSize)override;

		// IRCommandList draw / dispatch ---------------------------------------
		virtual void Draw(UINT32 InVertexCount, UINT32 InInstanceCount, UINT32 InStartVertex, UINT32 InStartInstance)override;
		virtual void DrawIndexed(UINT32 InIndexCount, UINT32 InInstanceCount, UINT32 InStartIndex, INT32 InBaseVertex, UINT32 InStartInstance)override;
		virtual void Dispatch(UINT32 InThreadGroupCountX, UINT32 InThreadGroupCountY, UINT32 InThreadGroupCountZ)override;
		virtual void DispatchIndirect(IRRHIBuffer* InArgBuffer, SIZE_T InArgOffset)override;

		// IRCommandList debug markers -----------------------------------------
		virtual void BeginDebugMarker(const EString& InName)override;
		virtual void EndDebugMarker()override;

		// IRCommandList GPU timestamp -----------------------------------------
		virtual void WriteTimestamp(RRHITimestampHeap* InHeap, UINT32 InSlot)override;
		virtual void ResolveTimestampHeap(RRHITimestampHeap* InHeap, UINT32 InBeginSlot, UINT32 InCount)override;

		// Native access (used by RDeviceD3D11::SubmitCommandList) -------------
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>		GetImmediateContext()const { return ImmediateContext; }

	private:
		RDeviceD3D11*										OwnerDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>			ImmediateContext;
		Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation>	Annotation;

		// Tracked state used to redrive OMSetBlendState / OMSetDepthStencilState
		// when the user calls SetBlendFactor / SetStencilRef without rebinding
		// the pipeline.
		ID3D11BlendState*									CurrentBlendState;
		ID3D11DepthStencilState*							CurrentDepthStencilState;
		Color4												CurrentBlendFactor;
		UINT32												CurrentSampleMask;
		UINT32												CurrentStencilRef;

		// The compute unordered access view each slot currently holds, as this command list bound
		// it. A resource bound for write may not be read while that binding stands - D3D11 drops the
		// read view and leaves the slot holding nothing - so the barrier that takes a resource out of
		// the unordered access state is where this binding is released (see ResourceBarrier). Unlike
		// the state above, the slots mirror the immediate context, so Reset leaves them alone.
		IRRHIResource*										BoundComputeUAVs[D3D11_PS_CS_UAV_REGISTER_COUNT];
	};

};
