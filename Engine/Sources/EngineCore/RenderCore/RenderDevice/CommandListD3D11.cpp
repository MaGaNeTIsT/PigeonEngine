#include "CommandListD3D11.h"
#include "DeviceD3D11.h"
#include "PipelineStateD3D11.h"
#include "D3D11Native.h"
#include <RenderResource.h>

namespace PigeonEngine
{

	// ============================================================
	// Resource extractors
	//
	// Concrete D3D11 resource classes inherit through the IRD3D11xxx
	// mid-interfaces (D3D11Native.h), which expose virtual getters for
	// the native ID3D11* objects. The IRRHIxxx -> IRD3D11xxx cast is
	// safe via static_cast because the D3D11 back-end only ever sees
	// resources it created.
	// ============================================================

	static PE_FORCEINLINE IRD3D11Texture* AsD3D11(IRRHITexture* InTexture)
	{
		return static_cast<IRD3D11Texture*>(InTexture);
	}

	static PE_FORCEINLINE IRD3D11Buffer* AsD3D11(IRRHIBuffer* InBuffer)
	{
		return static_cast<IRD3D11Buffer*>(InBuffer);
	}

	static PE_FORCEINLINE IRD3D11Sampler* AsD3D11(IRRHISampler* InSampler)
	{
		return static_cast<IRD3D11Sampler*>(InSampler);
	}

	static PE_FORCEINLINE ID3D11ShaderResourceView* ExtractD3D11SRV(IRRHITexture* InTexture)
	{
		return InTexture ? AsD3D11(InTexture)->GetD3D11SRV() : nullptr;
	}

	static PE_FORCEINLINE ID3D11ShaderResourceView* ExtractD3D11SRV(IRRHIBuffer* InBuffer)
	{
		return InBuffer ? AsD3D11(InBuffer)->GetD3D11SRV() : nullptr;
	}

	static PE_FORCEINLINE ID3D11UnorderedAccessView* ExtractD3D11UAV(IRRHITexture* InTexture)
	{
		return InTexture ? AsD3D11(InTexture)->GetD3D11UAV() : nullptr;
	}

	static PE_FORCEINLINE ID3D11UnorderedAccessView* ExtractD3D11UAV(IRRHIBuffer* InBuffer)
	{
		return InBuffer ? AsD3D11(InBuffer)->GetD3D11UAV() : nullptr;
	}

	static PE_FORCEINLINE ID3D11RenderTargetView* ExtractD3D11RTV(IRRHITexture* InTexture)
	{
		return InTexture ? AsD3D11(InTexture)->GetD3D11RTV() : nullptr;
	}

	static PE_FORCEINLINE ID3D11DepthStencilView* ExtractD3D11DSV(IRRHITexture* InTexture)
	{
		return InTexture ? AsD3D11(InTexture)->GetD3D11DSV() : nullptr;
	}

	static PE_FORCEINLINE ID3D11Buffer* ExtractD3D11Buffer(IRRHIBuffer* InBuffer)
	{
		return InBuffer ? AsD3D11(InBuffer)->GetD3D11Buffer() : nullptr;
	}

	static PE_FORCEINLINE ID3D11SamplerState* ExtractD3D11Sampler(IRRHISampler* InSampler)
	{
		return InSampler ? AsD3D11(InSampler)->GetD3D11SamplerState() : nullptr;
	}

	static PE_FORCEINLINE ID3D11Resource* ExtractD3D11Resource(IRRHIResource* InResource)
	{
		if (!InResource)
		{
			return nullptr;
		}
		switch (InResource->GetResourceType())
		{
			case ERHIResourceTypeTag::RHI_RESOURCE_TYPE_TEXTURE:	return static_cast<IRD3D11Texture*>(static_cast<IRRHITexture*>(InResource))->GetD3D11Resource();
			case ERHIResourceTypeTag::RHI_RESOURCE_TYPE_BUFFER:		return static_cast<IRD3D11Buffer*>(static_cast<IRRHIBuffer*>(InResource))->GetD3D11Buffer();
			default:												return nullptr;
		}
	}

	// ============================================================
	// Topology / index format translation
	// ============================================================

	static D3D_PRIMITIVE_TOPOLOGY TranslateTopology(RPrimitiveTopologyType InTopology)
	{
		switch (InTopology)
		{
			case RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_POINTLIST:		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			case RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINELIST:		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINESTRIP:		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			case RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST:	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			default:														return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}

	static DXGI_FORMAT TranslateIndexFormat(ERHIIndexFormat InFormat)
	{
		switch (InFormat)
		{
			case ERHIIndexFormat::RHI_INDEX_FORMAT_UINT16:	return DXGI_FORMAT_R16_UINT;
			case ERHIIndexFormat::RHI_INDEX_FORMAT_UINT32:	return DXGI_FORMAT_R32_UINT;
			default:										return DXGI_FORMAT_R32_UINT;
		}
	}

	// ============================================================
	// RCommandListD3D11
	// ============================================================

	RCommandListD3D11::RCommandListD3D11(RDeviceD3D11* InOwnerDevice, const Microsoft::WRL::ComPtr<ID3D11Device>& InNativeDevice)
		: OwnerDevice(InOwnerDevice)
		, ImmediateContext(nullptr)
		, Annotation(nullptr)
		, CurrentBlendState(nullptr)
		, CurrentDepthStencilState(nullptr)
		, CurrentBlendFactor(0.f, 0.f, 0.f, 0.f)
		, CurrentSampleMask(0xffffffffu)
		, CurrentStencilRef(0u)
	{
		// D3D11 path uses the device's shared immediate context so legacy
		// proxy code that still calls RDeviceD3D11 directly stays in lockstep
		// with command-list recording (see header rationale).
		InNativeDevice->GetImmediateContext(ImmediateContext.ReleaseAndGetAddressOf());
		if (ImmediateContext)
		{
			ImmediateContext.As(&Annotation);
		}
	}

	RCommandListD3D11::~RCommandListD3D11()
	{
		Annotation = nullptr;
		ImmediateContext = nullptr;
	}

	void RCommandListD3D11::Reset()
	{
		// Immediate-mode wrapper: nothing to "reset" on the context itself
		// (it is shared device-wide). Clear our local pipeline-state cache so
		// SetBlendFactor / SetStencilRef calls before SetPipelineState do
		// not redrive stale state objects.
		CurrentBlendState = nullptr;
		CurrentDepthStencilState = nullptr;
		CurrentBlendFactor = Color4(0.f, 0.f, 0.f, 0.f);
		CurrentSampleMask = 0xffffffffu;
		CurrentStencilRef = 0u;
	}

	void RCommandListD3D11::Close()
	{
		// Immediate-mode wrapper: commands have already executed; nothing to
		// finalize. Phase 3 D3D12 backend will repurpose this hook to call
		// ID3D12GraphicsCommandList::Close().
	}

	// --------------------------------------------------------
	// Render passes
	// --------------------------------------------------------

	void RCommandListD3D11::BeginRenderPass(const RRHIRenderPassDesc& InDesc)
	{
		if (!ImmediateContext)
		{
			return;
		}

		// Collect RTVs.
		ID3D11RenderTargetView* RawRTVs[RHI_MAX_RENDER_TARGETS] = { nullptr };
		const UINT32 RenderTargetCount = (InDesc.RenderTargetCount < RHI_MAX_RENDER_TARGETS) ? InDesc.RenderTargetCount : RHI_MAX_RENDER_TARGETS;
		for (UINT32 i = 0u; i < RenderTargetCount; i++)
		{
			RawRTVs[i] = ExtractD3D11RTV(InDesc.RenderTargets[i].Target);
		}

		ID3D11DepthStencilView* RawDSV = ExtractD3D11DSV(InDesc.DepthStencil.Target);

		ImmediateContext->OMSetRenderTargets(RenderTargetCount, RawRTVs, RawDSV);

		// Apply clears for slots flagged with RHI_LOAD_OP_CLEAR.
		for (UINT32 i = 0u; i < RenderTargetCount; i++)
		{
			if (InDesc.RenderTargets[i].LoadOp == ERHILoadOp::RHI_LOAD_OP_CLEAR && RawRTVs[i])
			{
				ImmediateContext->ClearRenderTargetView(RawRTVs[i], &(InDesc.RenderTargets[i].ClearColor.x));
			}
		}
		if (RawDSV)
		{
			UINT32 ClearFlags = 0u;
			if (InDesc.DepthStencil.DepthLoadOp == ERHILoadOp::RHI_LOAD_OP_CLEAR)
			{
				ClearFlags |= D3D11_CLEAR_DEPTH;
			}
			if (InDesc.DepthStencil.StencilLoadOp == ERHILoadOp::RHI_LOAD_OP_CLEAR)
			{
				ClearFlags |= D3D11_CLEAR_STENCIL;
			}
			if (ClearFlags != 0u)
			{
				ImmediateContext->ClearDepthStencilView(RawDSV, ClearFlags, InDesc.DepthStencil.ClearDepth, InDesc.DepthStencil.ClearStencil);
			}
		}
	}

	void RCommandListD3D11::EndRenderPass()
	{
		// D3D11 has no explicit end-of-pass hook; deferred-mode store ops are
		// implicit. Phase 3 (D3D12) will use this to insert resource state
		// transitions back to whatever the next pass declared.
	}

	// --------------------------------------------------------
	// Barriers (no-op on D3D11)
	// --------------------------------------------------------

	void RCommandListD3D11::ResourceBarrier(IRRHIResource* /*InResource*/, ERHIResourceState /*InBefore*/, ERHIResourceState /*InAfter*/)
	{
	}

	// --------------------------------------------------------
	// Pipeline / fixed-function
	// --------------------------------------------------------

	void RCommandListD3D11::SetPipelineState(IRRHIPipelineState* InPipelineState)
	{
		if (!ImmediateContext || !InPipelineState)
		{
			return;
		}
		// D3D11 back-end always produces RPipelineStateD3D11; the cast is safe.
		auto* PipelineState = static_cast<RPipelineStateD3D11*>(InPipelineState);

		if (PipelineState->bIsCompute)
		{
			ImmediateContext->CSSetShader(PipelineState->ComputeShader.Get(), nullptr, 0u);
			return;
		}

		// Skip VS / PS / InputLayout when not specified so the pipeline state
		// can act as an "environment PSO" (raster + blend + depth only); the
		// caller - typically a mesh proxy - is then free to bind its own
		// shader/layout pair without being clobbered.
		if (PipelineState->VertexShader)
		{
			ImmediateContext->VSSetShader(PipelineState->VertexShader.Get(), nullptr, 0u);
		}
		if (PipelineState->PixelShader)
		{
			ImmediateContext->PSSetShader(PipelineState->PixelShader.Get(), nullptr, 0u);
		}
		if (PipelineState->InputLayout)
		{
			ImmediateContext->IASetInputLayout(PipelineState->InputLayout.Get());
		}
		ImmediateContext->IASetPrimitiveTopology(TranslateTopology(PipelineState->PrimitiveTopology));
		ImmediateContext->RSSetState(PipelineState->RasterizerState.Get());

		CurrentBlendState = PipelineState->BlendState.Get();
		CurrentDepthStencilState = PipelineState->DepthStencilState.Get();
		CurrentBlendFactor = PipelineState->DefaultBlendFactor;
		CurrentSampleMask = PipelineState->SampleMask;
		CurrentStencilRef = PipelineState->DefaultStencilRef;

		ImmediateContext->OMSetBlendState(CurrentBlendState, &(CurrentBlendFactor.x), CurrentSampleMask);
		ImmediateContext->OMSetDepthStencilState(CurrentDepthStencilState, CurrentStencilRef);
	}

	void RCommandListD3D11::SetPrimitiveTopology(RPrimitiveTopologyType InTopology)
	{
		if (ImmediateContext)
		{
			ImmediateContext->IASetPrimitiveTopology(TranslateTopology(InTopology));
		}
	}

	void RCommandListD3D11::SetViewports(const RRHIViewport* InViewports, UINT32 InCount)
	{
		if (!ImmediateContext || !InViewports || InCount == 0u)
		{
			return;
		}
		D3D11_VIEWPORT NativeViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		const UINT32 ClampedCount = (InCount > D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) ? D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE : InCount;
		for (UINT32 i = 0u; i < ClampedCount; i++)
		{
			NativeViewports[i].TopLeftX = InViewports[i].TopLeftX;
			NativeViewports[i].TopLeftY = InViewports[i].TopLeftY;
			NativeViewports[i].Width = InViewports[i].Width;
			NativeViewports[i].Height = InViewports[i].Height;
			NativeViewports[i].MinDepth = InViewports[i].MinDepth;
			NativeViewports[i].MaxDepth = InViewports[i].MaxDepth;
		}
		ImmediateContext->RSSetViewports(ClampedCount, NativeViewports);
	}

	void RCommandListD3D11::SetScissorRects(const RRHIRect* InRects, UINT32 InCount)
	{
		if (!ImmediateContext || !InRects || InCount == 0u)
		{
			return;
		}
		D3D11_RECT NativeRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		const UINT32 ClampedCount = (InCount > D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) ? D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE : InCount;
		for (UINT32 i = 0u; i < ClampedCount; i++)
		{
			NativeRects[i].left = InRects[i].Left;
			NativeRects[i].top = InRects[i].Top;
			NativeRects[i].right = InRects[i].Right;
			NativeRects[i].bottom = InRects[i].Bottom;
		}
		ImmediateContext->RSSetScissorRects(ClampedCount, NativeRects);
	}

	void RCommandListD3D11::SetBlendFactor(const Color4& InBlendFactor)
	{
		if (!ImmediateContext)
		{
			return;
		}
		CurrentBlendFactor = InBlendFactor;
		ImmediateContext->OMSetBlendState(CurrentBlendState, &(CurrentBlendFactor.x), CurrentSampleMask);
	}

	void RCommandListD3D11::SetStencilRef(UINT32 InStencilRef)
	{
		if (!ImmediateContext)
		{
			return;
		}
		CurrentStencilRef = InStencilRef;
		ImmediateContext->OMSetDepthStencilState(CurrentDepthStencilState, CurrentStencilRef);
	}

	// --------------------------------------------------------
	// Input assembly
	// --------------------------------------------------------

	void RCommandListD3D11::SetVertexBuffers(UINT32 InStartSlot, UINT32 InCount, IRRHIBuffer* const* InBuffers, const UINT32* InStrides, const UINT32* InOffsets)
	{
		if (!ImmediateContext || InCount == 0u)
		{
			return;
		}
		ID3D11Buffer* NativeBuffers[RHI_MAX_VERTEX_BUFFERS] = { nullptr };
		UINT			NativeStrides[RHI_MAX_VERTEX_BUFFERS] = { 0u };
		UINT			NativeOffsets[RHI_MAX_VERTEX_BUFFERS] = { 0u };
		const UINT32	ClampedCount = (InCount > RHI_MAX_VERTEX_BUFFERS) ? RHI_MAX_VERTEX_BUFFERS : InCount;
		for (UINT32 i = 0u; i < ClampedCount; i++)
		{
			NativeBuffers[i] = InBuffers ? ExtractD3D11Buffer(InBuffers[i]) : nullptr;
			NativeStrides[i] = InStrides ? InStrides[i] : 0u;
			NativeOffsets[i] = InOffsets ? InOffsets[i] : 0u;
		}
		ImmediateContext->IASetVertexBuffers(InStartSlot, ClampedCount, NativeBuffers, NativeStrides, NativeOffsets);
	}

	void RCommandListD3D11::SetIndexBuffer(IRRHIBuffer* InBuffer, ERHIIndexFormat InFormat, UINT32 InOffset)
	{
		if (!ImmediateContext)
		{
			return;
		}
		ImmediateContext->IASetIndexBuffer(ExtractD3D11Buffer(InBuffer), TranslateIndexFormat(InFormat), InOffset);
	}

	// --------------------------------------------------------
	// Root parameters
	//
	// D3D11 has no concept of root signatures or root indices. The
	// command list treats InRootIndex as a CB slot and broadcasts the
	// binding to VS + PS so existing code that only knows it bound
	// "to slot 0" still works. Compute root binds to CS.
	//
	// 32-bit constants paths are not implemented because no D3D11 call
	// path needs them today; SceneRenderer updates dynamic CBs the
	// classic Map / memcpy / Unmap way, then binds the resulting buffer.
	// --------------------------------------------------------

	void RCommandListD3D11::SetGraphicsRootConstantBuffer(UINT32 InRootIndex, IRRHIBuffer* InBuffer, SIZE_T /*InOffset*/)
	{
		if (!ImmediateContext)
		{
			return;
		}
		ID3D11Buffer* NativeBuffer = ExtractD3D11Buffer(InBuffer);
		ImmediateContext->VSSetConstantBuffers(InRootIndex, 1u, &NativeBuffer);
		ImmediateContext->PSSetConstantBuffers(InRootIndex, 1u, &NativeBuffer);
	}

	void RCommandListD3D11::SetGraphicsRoot32BitConstants(UINT32 /*InRootIndex*/, const void* /*InData*/, UINT32 /*InNum32BitValues*/, UINT32 /*InOffsetIn32BitValues*/)
	{
		PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D11 backend does not implement SetGraphicsRoot32BitConstants; use a CPU-mapped CB instead."));
	}

	void RCommandListD3D11::SetComputeRootConstantBuffer(UINT32 InRootIndex, IRRHIBuffer* InBuffer, SIZE_T /*InOffset*/)
	{
		if (!ImmediateContext)
		{
			return;
		}
		ID3D11Buffer* NativeBuffer = ExtractD3D11Buffer(InBuffer);
		ImmediateContext->CSSetConstantBuffers(InRootIndex, 1u, &NativeBuffer);
	}

	void RCommandListD3D11::SetComputeRoot32BitConstants(UINT32 /*InRootIndex*/, const void* /*InData*/, UINT32 /*InNum32BitValues*/, UINT32 /*InOffsetIn32BitValues*/)
	{
		PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D11 backend does not implement SetComputeRoot32BitConstants; use a CPU-mapped CB instead."));
	}

	// --------------------------------------------------------
	// Slot-based bindings
	// --------------------------------------------------------

	void RCommandListD3D11::SetShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot, IRRHITexture* InTexture)
	{
		if (!ImmediateContext)
		{
			return;
		}
		ID3D11ShaderResourceView* NativeSRV = ExtractD3D11SRV(InTexture);
		switch (InStage)
		{
			case ERHIShaderStage::RHI_SHADER_STAGE_VERTEX:	ImmediateContext->VSSetShaderResources(InSlot, 1u, &NativeSRV); break;
			case ERHIShaderStage::RHI_SHADER_STAGE_PIXEL:	ImmediateContext->PSSetShaderResources(InSlot, 1u, &NativeSRV); break;
			case ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE:	ImmediateContext->CSSetShaderResources(InSlot, 1u, &NativeSRV); break;
			default:										break;
		}
	}

	void RCommandListD3D11::SetShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot, IRRHIBuffer* InBuffer)
	{
		if (!ImmediateContext)
		{
			return;
		}
		ID3D11ShaderResourceView* NativeSRV = ExtractD3D11SRV(InBuffer);
		switch (InStage)
		{
			case ERHIShaderStage::RHI_SHADER_STAGE_VERTEX:	ImmediateContext->VSSetShaderResources(InSlot, 1u, &NativeSRV); break;
			case ERHIShaderStage::RHI_SHADER_STAGE_PIXEL:	ImmediateContext->PSSetShaderResources(InSlot, 1u, &NativeSRV); break;
			case ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE:	ImmediateContext->CSSetShaderResources(InSlot, 1u, &NativeSRV); break;
			default:										break;
		}
	}

	void RCommandListD3D11::ClearShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot)
	{
		if (!ImmediateContext)
		{
			return;
		}
		ID3D11ShaderResourceView* NullSRV = nullptr;
		switch (InStage)
		{
			case ERHIShaderStage::RHI_SHADER_STAGE_VERTEX:	ImmediateContext->VSSetShaderResources(InSlot, 1u, &NullSRV); break;
			case ERHIShaderStage::RHI_SHADER_STAGE_PIXEL:	ImmediateContext->PSSetShaderResources(InSlot, 1u, &NullSRV); break;
			case ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE:	ImmediateContext->CSSetShaderResources(InSlot, 1u, &NullSRV); break;
			default:										break;
		}
	}

	void RCommandListD3D11::SetUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot, IRRHITexture* InTexture)
	{
		if (!ImmediateContext || InStage != ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE)
		{
			return;
		}
		ID3D11UnorderedAccessView* NativeUAV = ExtractD3D11UAV(InTexture);
		UINT InitialCount = 0u;
		ImmediateContext->CSSetUnorderedAccessViews(InSlot, 1u, &NativeUAV, &InitialCount);
	}

	void RCommandListD3D11::SetUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot, IRRHIBuffer* InBuffer)
	{
		if (!ImmediateContext || InStage != ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE)
		{
			return;
		}
		ID3D11UnorderedAccessView* NativeUAV = ExtractD3D11UAV(InBuffer);
		UINT InitialCount = 0u;
		ImmediateContext->CSSetUnorderedAccessViews(InSlot, 1u, &NativeUAV, &InitialCount);
	}

	void RCommandListD3D11::ClearUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot)
	{
		if (!ImmediateContext || InStage != ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE)
		{
			return;
		}
		ID3D11UnorderedAccessView* NullUAV = nullptr;
		UINT InitialCount = 0u;
		ImmediateContext->CSSetUnorderedAccessViews(InSlot, 1u, &NullUAV, &InitialCount);
	}

	void RCommandListD3D11::SetSampler(ERHIShaderStage InStage, UINT32 InSlot, IRRHISampler* InSampler)
	{
		if (!ImmediateContext)
		{
			return;
		}
		ID3D11SamplerState* NativeSampler = ExtractD3D11Sampler(InSampler);
		switch (InStage)
		{
			case ERHIShaderStage::RHI_SHADER_STAGE_VERTEX:	ImmediateContext->VSSetSamplers(InSlot, 1u, &NativeSampler); break;
			case ERHIShaderStage::RHI_SHADER_STAGE_PIXEL:	ImmediateContext->PSSetSamplers(InSlot, 1u, &NativeSampler); break;
			case ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE:	ImmediateContext->CSSetSamplers(InSlot, 1u, &NativeSampler); break;
			default:										break;
		}
	}

	// --------------------------------------------------------
	// Clears
	// --------------------------------------------------------

	void RCommandListD3D11::ClearRenderTarget(IRRHITexture* InTarget, const Color4& InColor)
	{
		if (!ImmediateContext)
		{
			return;
		}
		if (ID3D11RenderTargetView* RawRTV = ExtractD3D11RTV(InTarget))
		{
			ImmediateContext->ClearRenderTargetView(RawRTV, &InColor.x);
		}
	}

	void RCommandListD3D11::ClearDepthStencil(IRRHITexture* InTarget, UINT8 InClearFlags, FLOAT InDepth, UINT8 InStencil)
	{
		if (!ImmediateContext)
		{
			return;
		}
		if (ID3D11DepthStencilView* RawDSV = ExtractD3D11DSV(InTarget))
		{
			UINT32 NativeFlags = 0u;
			if ((InClearFlags & RClearDepthStencilFlagType::CLEAR_DEPTH) != 0u)
			{
				NativeFlags |= D3D11_CLEAR_DEPTH;
			}
			if ((InClearFlags & RClearDepthStencilFlagType::CLEAR_STENCIL) != 0u)
			{
				NativeFlags |= D3D11_CLEAR_STENCIL;
			}
			if (NativeFlags == 0u)
			{
				NativeFlags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
			}
			ImmediateContext->ClearDepthStencilView(RawDSV, NativeFlags, InDepth, InStencil);
		}
	}

	void RCommandListD3D11::ClearUnorderedAccessFloat(IRRHITexture* InTarget, const Color4& InValue)
	{
		if (!ImmediateContext)
		{
			return;
		}
		if (ID3D11UnorderedAccessView* RawUAV = ExtractD3D11UAV(InTarget))
		{
			ImmediateContext->ClearUnorderedAccessViewFloat(RawUAV, &InValue.x);
		}
	}

	void RCommandListD3D11::ClearUnorderedAccessUint(IRRHITexture* InTarget, const Vector4Int& InValue)
	{
		if (!ImmediateContext)
		{
			return;
		}
		if (ID3D11UnorderedAccessView* RawUAV = ExtractD3D11UAV(InTarget))
		{
			const UINT NativeValues[4] = { static_cast<UINT>(InValue.x), static_cast<UINT>(InValue.y), static_cast<UINT>(InValue.z), static_cast<UINT>(InValue.w) };
			ImmediateContext->ClearUnorderedAccessViewUint(RawUAV, NativeValues);
		}
	}

	// --------------------------------------------------------
	// Copy
	// --------------------------------------------------------

	void RCommandListD3D11::CopyResource(IRRHIResource* InDest, IRRHIResource* InSource)
	{
		if (!ImmediateContext || !InDest || !InSource)
		{
			return;
		}
		ID3D11Resource* RawDest = ExtractD3D11Resource(InDest);
		ID3D11Resource* RawSource = ExtractD3D11Resource(InSource);
		if (RawDest && RawSource)
		{
			ImmediateContext->CopyResource(RawDest, RawSource);
		}
	}

	void RCommandListD3D11::CopyBufferRegion(IRRHIBuffer* InDest, SIZE_T InDestOffset, IRRHIBuffer* InSource, SIZE_T InSourceOffset, SIZE_T InSize)
	{
		if (!ImmediateContext)
		{
			return;
		}
		ID3D11Buffer* RawDest = ExtractD3D11Buffer(InDest);
		ID3D11Buffer* RawSource = ExtractD3D11Buffer(InSource);
		if (!RawDest || !RawSource)
		{
			return;
		}
		D3D11_BOX SourceBox;
		SourceBox.left = static_cast<UINT>(InSourceOffset);
		SourceBox.top = 0u;
		SourceBox.front = 0u;
		SourceBox.right = static_cast<UINT>(InSourceOffset + InSize);
		SourceBox.bottom = 1u;
		SourceBox.back = 1u;
		ImmediateContext->CopySubresourceRegion(RawDest, 0u, static_cast<UINT>(InDestOffset), 0u, 0u, RawSource, 0u, &SourceBox);
	}

	// --------------------------------------------------------
	// Draw / dispatch
	// --------------------------------------------------------

	void RCommandListD3D11::Draw(UINT32 InVertexCount, UINT32 InInstanceCount, UINT32 InStartVertex, UINT32 InStartInstance)
	{
		if (!ImmediateContext)
		{
			return;
		}
		if (InInstanceCount <= 1u && InStartInstance == 0u)
		{
			ImmediateContext->Draw(InVertexCount, InStartVertex);
		}
		else
		{
			ImmediateContext->DrawInstanced(InVertexCount, InInstanceCount, InStartVertex, InStartInstance);
		}
	}

	void RCommandListD3D11::DrawIndexed(UINT32 InIndexCount, UINT32 InInstanceCount, UINT32 InStartIndex, INT32 InBaseVertex, UINT32 InStartInstance)
	{
		if (!ImmediateContext)
		{
			return;
		}
		if (InInstanceCount <= 1u && InStartInstance == 0u)
		{
			ImmediateContext->DrawIndexed(InIndexCount, InStartIndex, InBaseVertex);
		}
		else
		{
			ImmediateContext->DrawIndexedInstanced(InIndexCount, InInstanceCount, InStartIndex, InBaseVertex, InStartInstance);
		}
	}

	void RCommandListD3D11::Dispatch(UINT32 InThreadGroupCountX, UINT32 InThreadGroupCountY, UINT32 InThreadGroupCountZ)
	{
		if (ImmediateContext)
		{
			ImmediateContext->Dispatch(InThreadGroupCountX, InThreadGroupCountY, InThreadGroupCountZ);
		}
	}

	void RCommandListD3D11::DispatchIndirect(IRRHIBuffer* InArgBuffer, SIZE_T InArgOffset)
	{
		if (!ImmediateContext)
		{
			return;
		}
		if (ID3D11Buffer* NativeBuffer = ExtractD3D11Buffer(InArgBuffer))
		{
			ImmediateContext->DispatchIndirect(NativeBuffer, static_cast<UINT>(InArgOffset));
		}
	}

	// --------------------------------------------------------
	// Debug markers
	// --------------------------------------------------------

	void RCommandListD3D11::BeginDebugMarker(const EString& InName)
	{
		if (!Annotation)
		{
			return;
		}
		// EString stores UTF-8; convert to UTF-16 for the annotation API.
		const CHAR* AnsiName = (*InName);
		if (!AnsiName)
		{
			return;
		}
		WCHAR WideName[128] = { 0 };
		const INT32 Converted = ::MultiByteToWideChar(CP_UTF8, 0, AnsiName, -1, WideName, static_cast<INT32>(sizeof(WideName) / sizeof(WideName[0])));
		if (Converted <= 0)
		{
			return;
		}
		Annotation->BeginEvent(WideName);
	}

	void RCommandListD3D11::EndDebugMarker()
	{
		if (Annotation)
		{
			Annotation->EndEvent();
		}
	}

};
