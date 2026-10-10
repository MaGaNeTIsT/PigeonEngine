#include "CommandListD3D12.h"
#include "DeviceD3D12.h"
#include "D3D12Native.h"
#include <CoreMinimal.h>
#include <Editor/EditorLogManager.h>
#include <Development/GPUProfiler.h>

namespace PigeonEngine
{

// CD3DX12 replacements (d3dx12.h not available in this SDK)
static inline D3D12_CPU_DESCRIPTOR_HANDLE OffsetCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE Base, INT32 Offset, UINT32 DescriptorSize)
{
	D3D12_CPU_DESCRIPTOR_HANDLE Result = Base;
	Result.ptr += static_cast<SIZE_T>(Offset) * static_cast<SIZE_T>(DescriptorSize);
	return Result;
}
static inline D3D12_GPU_DESCRIPTOR_HANDLE OffsetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE Base, INT32 Offset, UINT32 DescriptorSize)
{
	D3D12_GPU_DESCRIPTOR_HANDLE Result = Base;
	Result.ptr += static_cast<UINT64>(Offset) * static_cast<UINT64>(DescriptorSize);
	return Result;
}

RCommandListD3D12::RCommandListD3D12(RDeviceD3D12* InOwnerDevice)
	: OwnerDevice(InOwnerDevice)
	, GraphicsSignature{}
	, ComputeSignature{}
	, CurrentSRVTableBase(0u)
	, CurrentUAVTableBase(0u)
	, CurrentSamplerTableBase(0u)
	, bTableRegionKnown(FALSE)
	, CurrentPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
	, HasBoundPipeline(FALSE)
	, bBindlessSRVBindingsEnabled(FALSE)
	, BoundRenderTargetFormats{}
	, BoundRenderTargetCount(0u)
	, BoundDepthStencilFormat(RFormatType::FORMAT_UNKNOWN)
{
}

RCommandListD3D12::~RCommandListD3D12()
{
}

void RCommandListD3D12::SetNativeList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> InList,
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> InAllocator)
{
	CommandList                = InList;
	CommandAllocator           = InAllocator;
	GraphicsSignature          = RBoundSignature{};
	ComputeSignature           = RBoundSignature{};
	CurrentSRVTableBase        = 0u;
	CurrentUAVTableBase        = 0u;
	CurrentSamplerTableBase    = 0u;
	SetArenaNext               = 0u;
	SetArenaEnd                = 0u;
	LiveSetSRVWidth            = 0u;
	LiveSetUAVWidth            = 0u;
	bResourceSetLive           = FALSE;
	bResourceSetSealed         = FALSE;
	bSetArenaWrapped           = FALSE;
	bTableRegionKnown          = FALSE;
	CurrentPrimitiveTopology   = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	HasBoundPipeline           = FALSE;
	// A fresh recording starts in slot-binding mode; see SetBindlessSRVBindingsEnabled.
	bBindlessSRVBindingsEnabled = FALSE;
	// Nothing is bound before the recording begins a pass, and the format comparison reads these
	// as "unknown" until one does.
	BoundRenderTargetCount     = 0u;
	BoundDepthStencilFormat    = RFormatType::FORMAT_UNKNOWN;
	for (UINT32 i = 0u; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		BoundRenderTargetFormats[i] = RFormatType::FORMAT_UNKNOWN;
	}

	// Bind the global shader-visible descriptor heaps so descriptor tables
	// and HLSL ResourceDescriptorHeap[idx] both resolve. Only valid when
	// the native list is open (newly created lists are open by default).
	if (CommandList && OwnerDevice)
	{
		ID3D12DescriptorHeap* Heaps[2] = {};
		UINT32 HeapCount = 0u;
		if (OwnerDevice->GetCBVSRVUAVHeap())
		{
			Heaps[HeapCount++] = OwnerDevice->GetCBVSRVUAVHeap();
		}
		if (OwnerDevice->GetSamplerHeap())
		{
			Heaps[HeapCount++] = OwnerDevice->GetSamplerHeap();
		}
		if (HeapCount > 0u)
		{
			CommandList->SetDescriptorHeaps(HeapCount, Heaps);
		}

		// No root signature is bound here. Every pipeline state carries the signature its own
		// program's reflection generated, so the first SetPipelineState of the recording binds
		// the right one and re-points the tables from that signature's layout. Nothing may be
		// written to a root before then: the bind entry points check the bound signature for
		// exactly that reason.
		//
		// The frame's descriptor arena, on the other hand, is a device constant and is known
		// before any signature is: a descriptor write only needs the arena, and becomes
		// visible once a pipeline state points its tables at the set holding it. The arena
		// follows the frame index, so two frames in flight never share descriptors.
		SetArenaNext               = OwnerDevice->GetSlotSetArenaBase();
		SetArenaEnd                = SetArenaNext + OwnerDevice->GetSlotSetArenaCapacity();
		CurrentSRVTableBase        = SetArenaNext;
		CurrentUAVTableBase        = SetArenaNext;
		CurrentSamplerTableBase    = OwnerDevice->GetSlotSamplerTableBase();
		bTableRegionKnown          = TRUE;
	}
}

void RCommandListD3D12::BindLayoutTables(const RRHIPipelineLayoutInfo& InLayoutInfo, BOOL8 InIsCompute)
{
	if (!CommandList || !OwnerDevice)
	{
		return;
	}

	// Every table of the layout is bound at the live resource set. Which descriptors that
	// set holds is the command list's answer, not a fixed frame offset: the set is what the
	// slot bindings of the current draw wrote into. A layout that has no table of some kind
	// leaves that root unbound.
	if (InLayoutInfo.SRVTableRoot != RHI_INVALID_ROOT_INDEX)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE SRVTableHandle = OffsetGPUHandle(
			OwnerDevice->GetCBVSRVUAVHeapGPUStart(),
			static_cast<INT32>(CurrentSRVTableBase),
			static_cast<INT32>(OwnerDevice->GetCBVSRVUAVDescriptorSize()));
		if (InIsCompute)
		{
			CommandList->SetComputeRootDescriptorTable(InLayoutInfo.SRVTableRoot, SRVTableHandle);
		}
		else
		{
			CommandList->SetGraphicsRootDescriptorTable(InLayoutInfo.SRVTableRoot, SRVTableHandle);
		}
	}
	if (InLayoutInfo.UAVTableRoot != RHI_INVALID_ROOT_INDEX)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE UAVTableHandle = OffsetGPUHandle(
			OwnerDevice->GetCBVSRVUAVHeapGPUStart(),
			static_cast<INT32>(CurrentUAVTableBase),
			static_cast<INT32>(OwnerDevice->GetCBVSRVUAVDescriptorSize()));
		if (InIsCompute)
		{
			CommandList->SetComputeRootDescriptorTable(InLayoutInfo.UAVTableRoot, UAVTableHandle);
		}
		else
		{
			CommandList->SetGraphicsRootDescriptorTable(InLayoutInfo.UAVTableRoot, UAVTableHandle);
		}
	}
	if ((InLayoutInfo.SamplerTableRoot != RHI_INVALID_ROOT_INDEX) && OwnerDevice->GetSamplerHeap())
	{
		// Samplers are a fixed region rather than part of the set: every program samples
		// through the renderer wide samplers, which are written once per frame at their own
		// registers, so no draw can disagree with another about them.
		D3D12_GPU_DESCRIPTOR_HANDLE SamplerTableHandle = OffsetGPUHandle(
			OwnerDevice->GetSamplerHeap()->GetGPUDescriptorHandleForHeapStart(),
			static_cast<INT32>(CurrentSamplerTableBase),
			static_cast<INT32>(OwnerDevice->GetSamplerDescriptorSize()));
		if (InIsCompute)
		{
			CommandList->SetComputeRootDescriptorTable(InLayoutInfo.SamplerTableRoot, SamplerTableHandle);
		}
		else
		{
			CommandList->SetGraphicsRootDescriptorTable(InLayoutInfo.SamplerTableRoot, SamplerTableHandle);
		}
	}
}

void RCommandListD3D12::SealResourceSet() noexcept
{
	// A draw has read the live set now, and a descriptor is read by the GPU when the draw
	// executes rather than when it is recorded: writing that set again would change what the
	// draw reads. Sealing it is what makes the next write open a set of its own.
	bResourceSetSealed = TRUE;
}

BOOL32 RCommandListD3D12::OpenResourceSet(const RRHIPipelineLayoutInfo& InLayout, const UINT32 InSlot, const BOOL8 InIsUAV)
{
	// One set is an SRV table followed by a UAV table, sized to cover what the bound
	// program's signature declares and never less than the slot being written: a write has
	// to land inside the range this set owns, or it would edit the next set's descriptors.
	const UINT32 SRVWidth = ((InIsUAV != FALSE) ? EMath::Max(InLayout.SRVTableSize, 1u)
		: EMath::Max(InLayout.SRVTableSize, static_cast<UINT32>(InSlot + 1u)));
	const UINT32 UAVWidth = ((InIsUAV != FALSE) ? EMath::Max(InLayout.UAVTableSize, static_cast<UINT32>(InSlot + 1u))
		: EMath::Max(InLayout.UAVTableSize, 1u));

	if ((SetArenaNext + SRVWidth + UAVWidth) > SetArenaEnd)
	{
		// The frame binds more resource sets than the arena holds. Wrapping hands the first
		// sets' descriptors out again, which makes the draws recorded with them read these
		// resources instead; the warning is what keeps that from happening silently.
		if (bSetArenaWrapped == FALSE)
		{
			bSetArenaWrapped = TRUE;
			PE_LOG_WARN(EString(PE_TEXT("[CommandListD3D12] The frame's descriptor set arena is exhausted; resource sets now share descriptors.")));
		}
		SetArenaNext = OwnerDevice->GetSlotSetArenaBase();
	}

	CurrentSRVTableBase = SetArenaNext;
	SetArenaNext += SRVWidth;
	CurrentUAVTableBase = SetArenaNext;
	SetArenaNext += UAVWidth;
	LiveSetSRVWidth = SRVWidth;
	LiveSetUAVWidth = UAVWidth;
	bResourceSetLive = TRUE;
	bResourceSetSealed = FALSE;

	// Every descriptor of the new set starts as a null view. A program's table range covers
	// registers it declares, culled ones included, and a register this frame never binds would
	// otherwise hold whatever the previous frame's set left there - the final blit's source in
	// the lighting pass's t0, for one, which reads as a resource that is also the draw's own
	// render target.
	for (UINT32 SlotIndex = 0u; SlotIndex < SRVWidth; SlotIndex++)
	{
		OwnerDevice->WriteNullShaderResourceView(CurrentSRVTableBase + SlotIndex);
	}
	for (UINT32 SlotIndex = 0u; SlotIndex < UAVWidth; SlotIndex++)
	{
		OwnerDevice->WriteNullUnorderedAccessView(CurrentUAVTableBase + SlotIndex);
	}

	// Point the bound signatures' tables at the new set. A root table binding is an ordered
	// command, so it only affects the draws recorded after it; the descriptors of the
	// previous set stay exactly what its own draws were recorded with. Both stages are
	// re-pointed because they share the arena: a set opened while recording one stage would
	// otherwise leave the other stage's roots on a set that is no longer the live one.
	if (GraphicsSignature.NativeSignature)
	{
		BindLayoutTables(GraphicsSignature.LayoutInfo, FALSE);
	}
	if (ComputeSignature.NativeSignature)
	{
		BindLayoutTables(ComputeSignature.LayoutInfo, TRUE);
	}
	return TRUE;
}

void RCommandListD3D12::ReserveResourceSetWidth(const RRHIPipelineLayoutInfo& InLayout)
{
	// A program reads the tables its own layout declares, whether or not this frame's binds
	// fill every register in them, so a set narrower than the layout would have that program
	// read the next set's descriptors. Sizing one here keeps that from depending on binding
	// discipline, and it only happens when the live set is too narrow: a run of programs with
	// the same table shape shares a set.
	if ((bTableRegionKnown == FALSE) || (!OwnerDevice))
	{
		return;
	}
	if ((bResourceSetLive != FALSE)
		&& (bResourceSetSealed == FALSE)
		&& (InLayout.SRVTableSize <= LiveSetSRVWidth)
		&& (InLayout.UAVTableSize <= LiveSetUAVWidth))
	{
		return;
	}
	OpenResourceSet(InLayout, 0u, FALSE);
}

BOOL32 RCommandListD3D12::EnsureResourceSet(ERHIShaderStage InStage, UINT32 InSlot, BOOL8 InIsUAV)
{
	if ((bTableRegionKnown == FALSE) || (!OwnerDevice))
	{
		return FALSE;
	}

	const BOOL8 bIsCompute = ((InStage == ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE) ? TRUE : FALSE);
	const RRHIPipelineLayoutInfo& Layout = (bIsCompute ? ComputeSignature.LayoutInfo : GraphicsSignature.LayoutInfo);

	// The live set takes the write when no draw has read it yet and it was sized for the bound
	// program's tables. Anything else opens a fresh set, so the descriptors a draw reads are
	// never written after the draw was recorded.
	//
	// The width test is against the bound program's tables, not only against the slot: a
	// program whose table is wider than the live set could read past the set's descriptors
	// into the next one's, even for a register this write does not touch.
	const BOOL8 bWideEnough = ((InIsUAV != FALSE)
		? (((Layout.UAVTableSize <= LiveSetUAVWidth) && (InSlot < LiveSetUAVWidth)) ? TRUE : FALSE)
		: (((Layout.SRVTableSize <= LiveSetSRVWidth) && (InSlot < LiveSetSRVWidth)) ? TRUE : FALSE));
	if ((bResourceSetLive != FALSE) && (bResourceSetSealed == FALSE) && (bWideEnough != FALSE))
	{
		return TRUE;
	}
	return OpenResourceSet(Layout, InSlot, InIsUAV);
}

UINT32 RCommandListD3D12::GetRootParameterBound(const RBoundSignature& InBound) const
{
	// The bound signature decides its own parameter count, zero included: a signature without
	// root parameters has no root index to write to, and with nothing bound there is no root
	// signature at all, so no root index is valid. A root write is only meaningful after the
	// pipeline state that carries the signature its roots live in.
	return InBound.LayoutInfo.ParameterCount;
}

// =========================================================================
// IRCommandList lifecycle
// =========================================================================

void RCommandListD3D12::Reset()
{
	// Device::BeginFrame calls SetNativeList(nullptr, ...) to drop pooled
	// command lists between frames. Reset() is the user-facing variant: it
	// releases the held ComPtrs without recording any GPU operation.
	CommandList.Reset();
	CommandAllocator.Reset();
	GraphicsSignature        = RBoundSignature{};
	ComputeSignature         = RBoundSignature{};
	CurrentSRVTableBase      = 0u;
	CurrentUAVTableBase      = 0u;
	CurrentSamplerTableBase  = 0u;
	SetArenaNext             = 0u;
	SetArenaEnd              = 0u;
	LiveSetSRVWidth          = 0u;
	LiveSetUAVWidth          = 0u;
	bResourceSetLive         = FALSE;
	bResourceSetSealed       = FALSE;
	bSetArenaWrapped         = FALSE;
	bTableRegionKnown        = FALSE;
	CurrentPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	HasBoundPipeline         = FALSE;
	BoundRenderTargetCount   = 0u;
	BoundDepthStencilFormat  = RFormatType::FORMAT_UNKNOWN;
	for (UINT32 i = 0u; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		BoundRenderTargetFormats[i] = RFormatType::FORMAT_UNKNOWN;
	}
}

void RCommandListD3D12::Close()
{
	if (CommandList)
	{
		CommandList->Close();
	}
}

// =========================================================================
// IRCommandList render passes
// =========================================================================

void RCommandListD3D12::BeginRenderPass(const RRHIRenderPassDesc& InDesc)
{
	if (!CommandList || !OwnerDevice)
	{
		return;
	}

	// GPU profiler: open event + first timestamp. No-op when PassName is
	// null or _GPU_ENABLE_PROFILER is off.
	PE_GPU_PROFILER_PASS_BEGIN(this, InDesc.PassName);

	// OMSetRenderTargets (and the barrier array below) only cover
	// D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT views; the RHI desc allows more, so a caller
	// asking for extra targets has to be told rather than silently served fewer.
	PE_CHECK((ENGINE_RENDER_CORE_ERROR),
		(PE_TEXT("D3D12: RenderTargetCount exceeds the simultaneous render-target limit.")),
		(InDesc.RenderTargetCount <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT));

	// Auto-insert transition barriers per Plan Section7.11. Each render target
	// goes to RENDER_TARGET; the depth-stencil goes to DEPTH_WRITE
	// (or DEPTH_READ when the binding is read-only). Resources already in
	// the requested state get skipped.
	D3D12_RESOURCE_BARRIER Barriers[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT + 1u] = {};
	UINT32 BarrierCount = 0u;

	// Gather RTV and DSV handles in the same pass so we record one
	// OMSetRenderTargets afterwards.
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandles[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
	UINT32 RTVCount = 0u;
	D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = {};
	BOOL8 bHasDSV = FALSE;

	// The formats of what this pass binds, recorded for the pipeline state comparison: a pass
	// that binds a target whose format its state does not declare is a device removed at the
	// first draw, and the only place that can see both is here and SetPipelineState.
	BoundRenderTargetCount  = 0u;
	BoundDepthStencilFormat = RFormatType::FORMAT_UNKNOWN;
	for (UINT32 i = 0u; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		BoundRenderTargetFormats[i] = RFormatType::FORMAT_UNKNOWN;
	}

	for (UINT32 i = 0u; i < InDesc.RenderTargetCount && i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		const RRHIRenderTargetBinding& RT = InDesc.RenderTargets[i];
		if (!RT.Target)
		{
			continue;
		}
		IRD3D12Texture* Tex = static_cast<IRD3D12Texture*>(RT.Target);
		const D3D12_CPU_DESCRIPTOR_HANDLE TargetView = Tex->GetD3D12RTVHandle();
		if (TargetView.ptr == 0u)
		{
			// A target whose view was never created would bind nothing - the slot would be
			// unbound and every draw of the pass would land nowhere - and handing the API a
			// null view is an error on its own. The pass is named so the missing view can be
			// traced back to whatever created the texture.
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D12: a render pass target carries no render target view."));
			continue;
		}
		RTVHandles[i] = TargetView;
		RTVCount = i + 1u;
		BoundRenderTargetFormats[i] = RT.Target->GetViewFormat(ERHIViewFormatType::RHI_VIEW_FORMAT_RTV);
		BoundRenderTargetCount = i + 1u;

		ERHIResourceState CurrentState = RT.Target->GetCurrentState();
		if (CurrentState != ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET)
		{
			D3D12_RESOURCE_STATES Before, After;
			RDeviceD3D12::TranslateResourceStates(Before, CurrentState);
			RDeviceD3D12::TranslateResourceStates(After, ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
			Barriers[BarrierCount].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			Barriers[BarrierCount].Transition.pResource   = Tex->GetD3D12Resource();
			Barriers[BarrierCount].Transition.StateBefore = Before;
			Barriers[BarrierCount].Transition.StateAfter  = After;
			Barriers[BarrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			BarrierCount++;
			RT.Target->SetCurrentState(ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
		}
	}

	// A pass that asked for targets and got none has nothing to write into: OMSetRenderTargets
	// with a count of zero unbinds every slot, and the draws of that pass would be recorded
	// against no render target at all. Saying so here names the pass instead of leaving a black
	// frame behind.
	PE_CHECK((ENGINE_RENDER_CORE_ERROR),
		(PE_TEXT("D3D12: a render pass has no target it can write into.")),
		((InDesc.RenderTargetCount == 0u) || (RTVCount > 0u)));

	if (InDesc.DepthStencil.Target)
	{
		IRD3D12Texture* Tex = static_cast<IRD3D12Texture*>(InDesc.DepthStencil.Target);
		DSVHandle = Tex->GetD3D12DSVHandle();
		// A depth target without a view would be passed to the API as a null handle: the pass
		// would then run with its depth test and depth write switched off, against a pipeline
		// state that declares both, which shows up as geometry drawing without occlusion. The
		// binding is dropped rather than handed over in that shape, and the missing view is
		// named so it can be traced back to whatever created the texture.
		bHasDSV = ((DSVHandle.ptr != 0u) ? TRUE : FALSE);
		PE_CHECK((ENGINE_RENDER_CORE_ERROR),
			(PE_TEXT("D3D12: a render pass depth target carries no depth stencil view.")),
			(bHasDSV != FALSE));
		if (bHasDSV != FALSE)
		{
			BoundDepthStencilFormat = InDesc.DepthStencil.Target->GetViewFormat(ERHIViewFormatType::RHI_VIEW_FORMAT_DSV);
		}

		ERHIResourceState TargetState = InDesc.DepthStencil.bIsReadOnly
			? ERHIResourceState::RHI_RESOURCE_STATE_DEPTH_READ
			: ERHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE;
		ERHIResourceState CurrentState = InDesc.DepthStencil.Target->GetCurrentState();
		if (CurrentState != TargetState)
		{
			D3D12_RESOURCE_STATES Before, After;
			RDeviceD3D12::TranslateResourceStates(Before, CurrentState);
			RDeviceD3D12::TranslateResourceStates(After, TargetState);
			Barriers[BarrierCount].Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			Barriers[BarrierCount].Transition.pResource   = Tex->GetD3D12Resource();
			Barriers[BarrierCount].Transition.StateBefore = Before;
			Barriers[BarrierCount].Transition.StateAfter  = After;
			Barriers[BarrierCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			BarrierCount++;
			InDesc.DepthStencil.Target->SetCurrentState(TargetState);
		}
	}

	if (BarrierCount > 0u)
	{
		CommandList->ResourceBarrier(BarrierCount, Barriers);
	}

	CommandList->OMSetRenderTargets(RTVCount, RTVHandles, FALSE, bHasDSV ? &DSVHandle : nullptr);

	// Execute clear-on-load
	for (UINT32 i = 0u; i < InDesc.RenderTargetCount && i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		const RRHIRenderTargetBinding& RT = InDesc.RenderTargets[i];
		if (RT.Target && RT.LoadOp == ERHILoadOp::RHI_LOAD_OP_CLEAR)
		{
			FLOAT ClearColor[4] = { RT.ClearColor.r, RT.ClearColor.g, RT.ClearColor.b, RT.ClearColor.a };
			CommandList->ClearRenderTargetView(RTVHandles[i], ClearColor, 0u, nullptr);
		}
	}

	if (bHasDSV && InDesc.DepthStencil.DepthLoadOp == ERHILoadOp::RHI_LOAD_OP_CLEAR)
	{
		D3D12_CLEAR_FLAGS ClearFlags = D3D12_CLEAR_FLAG_DEPTH;
		if (InDesc.DepthStencil.StencilLoadOp == ERHILoadOp::RHI_LOAD_OP_CLEAR)
		{
			ClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
		}
		CommandList->ClearDepthStencilView(DSVHandle, ClearFlags,
			InDesc.DepthStencil.ClearDepth, InDesc.DepthStencil.ClearStencil, 0u, nullptr);
	}
}

void RCommandListD3D12::EndRenderPass()
{
	// No-op on D3D12 (render passes are not natively scoped in basic D3D12).
	// Future enhancement: use ID3D12GraphicsCommandList4 render pass APIs.

	// GPU profiler: close event + second timestamp. No-op when this pass
	// wasn't auto-instrumented or _GPU_ENABLE_PROFILER is off.
	PE_GPU_PROFILER_PASS_END(this);
}

// =========================================================================
// IRCommandList barriers
// =========================================================================

void RCommandListD3D12::ResourceBarrier(IRRHIResource* InResource, ERHIResourceState InBefore, ERHIResourceState InAfter)
{
	if (!CommandList || !InResource)
	{
		return;
	}

	D3D12_RESOURCE_STATES StateBefore, StateAfter;
	RDeviceD3D12::TranslateResourceStates(StateBefore, InBefore);
	RDeviceD3D12::TranslateResourceStates(StateAfter, InAfter);

	ID3D12Resource* NativeResource = nullptr;
	ERHIResourceTypeTag TypeTag = InResource->GetResourceType();
	if (TypeTag == ERHIResourceTypeTag::RHI_RESOURCE_TYPE_TEXTURE)
	{
		NativeResource = static_cast<IRD3D12Texture*>(InResource)->GetD3D12Resource();
	}
	else if (TypeTag == ERHIResourceTypeTag::RHI_RESOURCE_TYPE_BUFFER)
	{
		NativeResource = static_cast<IRD3D12Buffer*>(InResource)->GetD3D12Resource();
	}
	else
	{
		return;
	}

	if (!NativeResource)
	{
		return;
	}

	D3D12_RESOURCE_BARRIER Barrier = {};
	Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	Barrier.Transition.pResource   = NativeResource;
	Barrier.Transition.StateBefore = StateBefore;
	Barrier.Transition.StateAfter  = StateAfter;
	Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	Barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	CommandList->ResourceBarrier(1u, &Barrier);
	InResource->SetCurrentState(InAfter);
}

// =========================================================================
// IRCommandList pipeline / fixed-function
// =========================================================================

void RCommandListD3D12::SetPipelineState(IRRHIPipelineState* InPipelineState)
{
	if (!CommandList || !InPipelineState)
	{
		return;
	}

	IRD3D12PipelineState* PSO = static_cast<IRD3D12PipelineState*>(InPipelineState);
	ID3D12PipelineState* NativePSO       = PSO->GetD3D12PipelineState();
	ID3D12RootSignature* NativeRootSig   = PSO->GetD3D12RootSignature();
	D3D12_PRIMITIVE_TOPOLOGY NativeTopo  = PSO->GetD3D12PrimitiveTopology();
	const BOOL8 bIsCompute               = PSO->IsCompute();

	// Before the state is bound: whether the targets the pass bound are the ones it declares.
	// A mismatch that reaches the API removes the device at the draw, so it is worth catching
	// where both halves are visible and can be named.
	if (bIsCompute == FALSE)
	{
		ValidateBoundTargetFormats(InPipelineState);
	}

	if (NativePSO)
	{
		CommandList->SetPipelineState(NativePSO);
		HasBoundPipeline = TRUE;
	}
	else
	{
		// Phase 1 stub PSO: no native PSO created (graphics PSO factory is
		// stubbed until Phase 4 reworks shader compilation). Mark pipeline
		// unbound so the next DrawIndexed / Draw can skip silently rather
		// than producing D3D12 errors.
		HasBoundPipeline = FALSE;
	}

	// The layout always follows the PSO, even when the signature is already bound: it is
	// what bounds this stage's root indices from here on.
	RBoundSignature& Bound = (bIsCompute ? ComputeSignature : GraphicsSignature);
	Bound.LayoutInfo = PSO->GetLayoutInfo();

	// Only rebind root signature + descriptor tables when the signature actually changes.
	// Switching root signatures clears that stage's root bindings, so the tables have to be
	// re-pointed from the new layout (never from a fixed root index). Binding one stage's
	// signature leaves the other stage's bindings alone, so graphics and compute are
	// tracked apart.
	if (NativeRootSig && NativeRootSig != Bound.NativeSignature)
	{
		Bound.NativeSignature = NativeRootSig;
		if (bIsCompute)
		{
			CommandList->SetComputeRootSignature(NativeRootSig);
		}
		else
		{
			CommandList->SetGraphicsRootSignature(NativeRootSig);
		}

		if (OwnerDevice)
		{
			BindLayoutTables(Bound.LayoutInfo, bIsCompute);
		}
	}

	// Then, and only then, make sure the live set is wide enough for this program's tables: a
	// set that has to be opened re-points the bound signatures' tables, and those roots are
	// the ones this PSO's signature declares. Opening it before the switch above would bind
	// the new layout's roots against the previous root signature, which declares fewer
	// parameters than the new program uses.
	ReserveResourceSetWidth(Bound.LayoutInfo);

	CurrentPrimitiveTopology = NativeTopo;
	CommandList->IASetPrimitiveTopology(NativeTopo);
}

void RCommandListD3D12::ValidateBoundTargetFormats(IRRHIPipelineState* InPipelineState) const
{
#if _EDITOR_ONLY
	if (!InPipelineState)
	{
		return;
	}
	const UINT32 DeclaredCount = InPipelineState->GetRenderTargetFormatNum();
	for (UINT32 i = 0u; i < DeclaredCount && i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		const RFormatType DeclaredFormat = InPipelineState->GetRenderTargetFormat(i);
		const RFormatType BoundFormat = (i < BoundRenderTargetCount) ? BoundRenderTargetFormats[i] : RFormatType::FORMAT_UNKNOWN;
		// A side that does not know its format (a back-end that does not track view formats, an
		// unwritten slot) cannot be compared, and is skipped rather than guessed at.
		if ((DeclaredFormat == RFormatType::FORMAT_UNKNOWN) || (BoundFormat == RFormatType::FORMAT_UNKNOWN)
			|| (DeclaredFormat == BoundFormat))
		{
			continue;
		}
		EString MismatchInfo(PE_TEXT("D3D12: render target "));
		MismatchInfo = MismatchInfo + EString::FromInt(i) + PE_TEXT(" was bound as ")
			+ EString(RFormatTraits::GetFormatName(BoundFormat)) + PE_TEXT(" while the pipeline state declares ")
			+ EString(RFormatTraits::GetFormatName(DeclaredFormat))
			+ PE_TEXT(". D3D12 removes the device on such a draw; the two have to agree.");
		PE_LOG_ERROR(MismatchInfo);
		PE_CHECK((ENGINE_RENDER_CORE_ERROR),
			(PE_TEXT("D3D12: a bound render target's format does not match the format the pipeline state declares.")),
			(FALSE));
	}

	const RFormatType DeclaredDepthFormat = InPipelineState->GetDepthStencilFormat();
	if ((DeclaredDepthFormat != RFormatType::FORMAT_UNKNOWN) && (BoundDepthStencilFormat != RFormatType::FORMAT_UNKNOWN)
		&& (DeclaredDepthFormat != BoundDepthStencilFormat))
	{
		EString MismatchInfo(PE_TEXT("D3D12: the depth target was bound as "));
		MismatchInfo = MismatchInfo + EString(RFormatTraits::GetFormatName(BoundDepthStencilFormat))
			+ PE_TEXT(" while the pipeline state declares ") + EString(RFormatTraits::GetFormatName(DeclaredDepthFormat))
			+ PE_TEXT(".");
		PE_LOG_ERROR(MismatchInfo);
		PE_CHECK((ENGINE_RENDER_CORE_ERROR),
			(PE_TEXT("D3D12: the bound depth target's format does not match the format the pipeline state declares.")),
			(FALSE));
	}
#else
	(void)(InPipelineState);
#endif
}

void RCommandListD3D12::SetPrimitiveTopology(RPrimitiveTopologyType InTopology)
{
	if (!CommandList)
	{
		return;
	}
	RDeviceD3D12::TranslatePrimitiveTopology(CurrentPrimitiveTopology, InTopology);
	CommandList->IASetPrimitiveTopology(CurrentPrimitiveTopology);
}

void RCommandListD3D12::SetViewports(const EViewport* InViewports, UINT32 InCount)
{
	if (!CommandList || !InViewports || InCount == 0u)
	{
		return;
	}
	// RSSetViewports accepts at most D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE
	// entries; the caller's count is clamped so the native call cannot read past Viewports.
	const UINT32 ViewportCount = (InCount > D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)
		? D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE : InCount;
	D3D12_VIEWPORT Viewports[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
	for (UINT32 i = 0u; i < ViewportCount; i++)
	{
		Viewports[i].TopLeftX = InViewports[i].TopLeftX;
		Viewports[i].TopLeftY = InViewports[i].TopLeftY;
		Viewports[i].Width    = InViewports[i].Width;
		Viewports[i].Height   = InViewports[i].Height;
		Viewports[i].MinDepth = InViewports[i].MinDepth;
		Viewports[i].MaxDepth = InViewports[i].MaxDepth;
	}
	CommandList->RSSetViewports(ViewportCount, Viewports);
}

void RCommandListD3D12::SetScissorRects(const ERect* InRects, UINT32 InCount)
{
	if (!CommandList || !InRects || InCount == 0u)
	{
		return;
	}
	// Same clamp as SetViewports: the scissor array holds
	// D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE rects.
	const UINT32 ScissorCount = (InCount > D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)
		? D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE : InCount;
	D3D12_RECT Rects[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
	for (UINT32 i = 0u; i < ScissorCount; i++)
	{
		Rects[i].left   = static_cast<LONG>(InRects[i].Left);
		Rects[i].top    = static_cast<LONG>(InRects[i].Top);
		Rects[i].right  = static_cast<LONG>(InRects[i].Right);
		Rects[i].bottom = static_cast<LONG>(InRects[i].Bottom);
	}
	CommandList->RSSetScissorRects(ScissorCount, Rects);
}

void RCommandListD3D12::SetBlendFactor(const Color4& InBlendFactor)
{
	if (!CommandList)
	{
		return;
	}
	FLOAT Factors[4] = { InBlendFactor.r, InBlendFactor.g, InBlendFactor.b, InBlendFactor.a };
	CommandList->OMSetBlendFactor(Factors);
}

void RCommandListD3D12::SetStencilRef(UINT32 InStencilRef)
{
	if (!CommandList)
	{
		return;
	}
	CommandList->OMSetStencilRef(InStencilRef);
}

// =========================================================================
// IRCommandList input assembly
// =========================================================================

void RCommandListD3D12::SetVertexBuffers(UINT32 InStartSlot, UINT32 InCount, IRRHIBuffer* const* InBuffers, const UINT32* InStrides, const UINT32* InOffsets)
{
	if (!CommandList || !InBuffers || InCount == 0u)
	{
		return;
	}

	// IASetVertexBuffers requires StartSlot + NumBuffers <=
	// D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, while the caller's count is unbounded.
	// Clamp both ends so the native call cannot read past Views.
	if (InStartSlot >= D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)
	{
		return;
	}
	const UINT32 MaxViewCount = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - InStartSlot;
	const UINT32 ViewCount = (InCount > MaxViewCount) ? MaxViewCount : InCount;
	D3D12_VERTEX_BUFFER_VIEW Views[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
	for (UINT32 i = 0u; i < ViewCount; i++)
	{
		IRD3D12Buffer* Buf = static_cast<IRD3D12Buffer*>(InBuffers[i]);
		if (!Buf)
		{
			continue;
		}
		Views[i].BufferLocation = Buf->GetD3D12GPUVirtualAddress();
		Views[i].SizeInBytes    = static_cast<UINT>(Buf->GetByteSize());
		Views[i].StrideInBytes  = InStrides ? InStrides[i] : 0u;
		if (InOffsets)
		{
			Views[i].BufferLocation += InOffsets[i];
		}
	}
	CommandList->IASetVertexBuffers(InStartSlot, ViewCount, Views);
}

void RCommandListD3D12::SetIndexBuffer(IRRHIBuffer* InBuffer, ERHIIndexFormat InFormat, UINT32 InOffset)
{
	if (!CommandList || !InBuffer)
	{
		return;
	}

	IRD3D12Buffer* Buf = static_cast<IRD3D12Buffer*>(InBuffer);
	D3D12_INDEX_BUFFER_VIEW View;
	View.BufferLocation = Buf->GetD3D12GPUVirtualAddress() + InOffset;
	View.SizeInBytes    = static_cast<UINT>(Buf->GetByteSize() - InOffset);
	View.Format         = (InFormat == ERHIIndexFormat::RHI_INDEX_FORMAT_UINT32)
		? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

	CommandList->IASetIndexBuffer(&View);
}

// =========================================================================
// IRCommandList graphics root params
// =========================================================================

void RCommandListD3D12::SetGraphicsRootConstantBuffer(UINT32 InRootIndex, IRRHIBuffer* InBuffer, SIZE_T InOffset)
{
	// A root write needs a bound signature, and the only thing that binds one is a pipeline
	// state: every state carries the signature its own program's reflection generated, and
	// switching it voids the roots the previous one had. A caller therefore binds its blocks
	// after its own SetPipelineState, which is also the order the root indices come in - they
	// are read from that program's binding plan, not written down by the caller.
	if (!CommandList || !InBuffer || !GraphicsSignature.NativeSignature)
	{
		return;
	}

	// API-level bound only: the index must name an existing root parameter of the bound
	// signature. Which roots carry CBVs is the shader system's contract, not the API's.
	const UINT32 RootParameterCount = GetRootParameterBound(GraphicsSignature);
	PE_CHECK((ENGINE_RENDER_CORE_ERROR),
		(PE_TEXT("D3D12: root index exceeds the bound root signature's parameter count.")),
		(InRootIndex < RootParameterCount));
	if (InRootIndex >= RootParameterCount)
	{
		return;
	}

	IRD3D12Buffer* Buf = static_cast<IRD3D12Buffer*>(InBuffer);
	D3D12_GPU_VIRTUAL_ADDRESS Address = Buf->GetD3D12GPUVirtualAddress() + InOffset;
	CommandList->SetGraphicsRootConstantBufferView(InRootIndex, Address);
}

void RCommandListD3D12::SetGraphicsRoot32BitConstants(UINT32 InRootIndex, const void* InData, UINT32 InNum32BitValues, UINT32 InOffsetIn32BitValues)
{
	if (!CommandList || !InData || InNum32BitValues == 0u || !HasBoundPipeline)
	{
		return;
	}
	// API-level bounds: the index must name an existing root parameter of the bound
	// signature, the write cannot carry more DWORDs than the whole root signature budget
	// allows, and it has to fit inside the bound layout's 32 bit constant block. Which
	// root is the push-constant root is the shader system's contract, not the API's.
	// The fit test subtracts instead of adding so the offset + count sum cannot overflow.
	const BOOL8 bRootIndexValid   = ((InRootIndex < GetRootParameterBound(GraphicsSignature)) ? TRUE : FALSE);
	const BOOL8 bSizeWithinBudget = ((InNum32BitValues <= D3D12_MAX_ROOT_COST) ? TRUE : FALSE);
	const BOOL8 bWithinConstants  = ((GraphicsSignature.LayoutInfo.Constants32BitCount == 0u)
		|| ((InOffsetIn32BitValues <= GraphicsSignature.LayoutInfo.Constants32BitCount)
			&& (InNum32BitValues <= (GraphicsSignature.LayoutInfo.Constants32BitCount - InOffsetIn32BitValues)))) ? TRUE : FALSE;
	PE_CHECK((ENGINE_RENDER_CORE_ERROR),
		(PE_TEXT("D3D12: root 32-bit constants exceed the API's root parameter limits.")),
		(bRootIndexValid && bSizeWithinBudget && bWithinConstants));
	if ((!bRootIndexValid) || (!bSizeWithinBudget) || (!bWithinConstants))
	{
		return;
	}
	CommandList->SetGraphicsRoot32BitConstants(InRootIndex, InNum32BitValues, InData, InOffsetIn32BitValues);
}

// =========================================================================
// IRCommandList compute root params
// =========================================================================

void RCommandListD3D12::SetComputeRootConstantBuffer(UINT32 InRootIndex, IRRHIBuffer* InBuffer, SIZE_T InOffset)
{
	// Gated on the compute signature actually being bound, unlike the graphics variant: no
	// compute root signature is pre-bound at the start of a recording, so a write before the
	// first compute SetPipelineState would have no signature to land on.
	if (!CommandList || !InBuffer || !HasBoundPipeline || !ComputeSignature.NativeSignature)
	{
		return;
	}

	// API-level bound only: the index must name an existing root parameter of the bound
	// signature. Which roots carry CBVs is the shader system's contract, not the API's.
	const UINT32 RootParameterCount = GetRootParameterBound(ComputeSignature);
	PE_CHECK((ENGINE_RENDER_CORE_ERROR),
		(PE_TEXT("D3D12: root index exceeds the bound root signature's parameter count.")),
		(InRootIndex < RootParameterCount));
	if (InRootIndex >= RootParameterCount)
	{
		return;
	}

	IRD3D12Buffer* Buf = static_cast<IRD3D12Buffer*>(InBuffer);
	D3D12_GPU_VIRTUAL_ADDRESS Address = Buf->GetD3D12GPUVirtualAddress() + InOffset;
	CommandList->SetComputeRootConstantBufferView(InRootIndex, Address);
}

void RCommandListD3D12::SetComputeRoot32BitConstants(UINT32 InRootIndex, const void* InData, UINT32 InNum32BitValues, UINT32 InOffsetIn32BitValues)
{
	if (!CommandList || !InData || InNum32BitValues == 0u || !HasBoundPipeline || !ComputeSignature.NativeSignature)
	{
		return;
	}
	// Same API bounds as the graphics variant above.
	const BOOL8 bRootIndexValid   = ((InRootIndex < GetRootParameterBound(ComputeSignature)) ? TRUE : FALSE);
	const BOOL8 bSizeWithinBudget = ((InNum32BitValues <= D3D12_MAX_ROOT_COST) ? TRUE : FALSE);
	const BOOL8 bWithinConstants  = ((ComputeSignature.LayoutInfo.Constants32BitCount == 0u)
		|| ((InOffsetIn32BitValues <= ComputeSignature.LayoutInfo.Constants32BitCount)
			&& (InNum32BitValues <= (ComputeSignature.LayoutInfo.Constants32BitCount - InOffsetIn32BitValues)))) ? TRUE : FALSE;
	PE_CHECK((ENGINE_RENDER_CORE_ERROR),
		(PE_TEXT("D3D12: root 32-bit constants exceed the API's root parameter limits.")),
		(bRootIndexValid && bSizeWithinBudget && bWithinConstants));
	if ((!bRootIndexValid) || (!bSizeWithinBudget) || (!bWithinConstants))
	{
		return;
	}
	CommandList->SetComputeRoot32BitConstants(InRootIndex, InNum32BitValues, InData, InOffsetIn32BitValues);
}

// =========================================================================
// IRCommandList slot-based bindings
//
// Descriptor-copy-based slot bindings: the resource's pre-built CPU descriptor is
// copied into the resource set the current draw reads, and the layout of the bound
// PSO says which root table points at that set. The GPU reads
// heap[CurrentSRVTableBase + InSlot] as SRV slot InSlot, heap[CurrentUAVTableBase +
// InSlot] as UAV slot InSlot, and likewise for samplers. Both the table roots and the
// set's offsets come from the command list's arena, never from a fixed root index.
// Phase 4: replaced by bindless index writes into material CBVs.
// =========================================================================

void RCommandListD3D12::BeginResourceTable()
{
	// The current set is declared finished, so the first write below opens the next one.
	// Nothing is allocated here: which set a write needs is decided when the write names
	// its slot, and the arena is what hands it out (see EnsureResourceSet).
	SealResourceSet();
}

void RCommandListD3D12::SetShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot, IRRHITexture* InTexture)
{
	if (!CommandList || !OwnerDevice || !InTexture || !bTableRegionKnown)
	{
		return;
	}

	// API-level bound: a D3D12 shader addresses at most
	// D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT input-resource slots in the classic
	// (non-bindless) binding model. The engine's tighter per-frame table layout is the
	// shader system's contract, enforced where the slots are authored.
	if (!bBindlessSRVBindingsEnabled)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR),
			(PE_TEXT("D3D12: SRV slot exceeds the API's input-resource slot count.")),
			(InSlot < D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT));
		if (InSlot >= D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)
		{
			return;
		}
	}

	IRD3D12Texture* Tex = static_cast<IRD3D12Texture*>(InTexture);
	D3D12_CPU_DESCRIPTOR_HANDLE Src = Tex->GetD3D12SRVHandle();
	if (Src.ptr == 0u)
	{
		return;
	}
	if (!EnsureResourceSet(InStage, InSlot, FALSE))
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Dst = OffsetCPUHandle(
		OwnerDevice->GetCBVSRVUAVHeapStart(),
		static_cast<INT32>(CurrentSRVTableBase + InSlot),
		static_cast<INT32>(OwnerDevice->GetCBVSRVUAVDescriptorSize()));

	OwnerDevice->GetD3D12Device()->CopyDescriptorsSimple(1u, Dst, Src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RCommandListD3D12::SetShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot, IRRHIBuffer* InBuffer)
{
	if (!CommandList || !OwnerDevice || !InBuffer || !bTableRegionKnown)
	{
		return;
	}

	// Same API bound as the texture overload above.
	if (!bBindlessSRVBindingsEnabled)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR),
			(PE_TEXT("D3D12: SRV slot exceeds the API's input-resource slot count.")),
			(InSlot < D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT));
		if (InSlot >= D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)
		{
			return;
		}
	}

	IRD3D12Buffer* Buf = static_cast<IRD3D12Buffer*>(InBuffer);
	D3D12_CPU_DESCRIPTOR_HANDLE Src = Buf->GetD3D12SRVHandle();
	if (Src.ptr == 0u)
	{
		return;
	}
	if (!EnsureResourceSet(InStage, InSlot, FALSE))
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Dst = OffsetCPUHandle(
		OwnerDevice->GetCBVSRVUAVHeapStart(),
		static_cast<INT32>(CurrentSRVTableBase + InSlot),
		static_cast<INT32>(OwnerDevice->GetCBVSRVUAVDescriptorSize()));

	OwnerDevice->GetD3D12Device()->CopyDescriptorsSimple(1u, Dst, Src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RCommandListD3D12::ClearShaderResourceView(ERHIShaderStage InStage, UINT32 InSlot)
{
	if (!CommandList || !OwnerDevice || !bTableRegionKnown)
	{
		return;
	}
	if (!bBindlessSRVBindingsEnabled)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR),
			(PE_TEXT("D3D12: SRV slot exceeds the API's input-resource slot count.")),
			(InSlot < D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT));
		if (InSlot >= D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)
		{
			return;
		}
	}
	if (!EnsureResourceSet(InStage, InSlot, FALSE))
	{
		return;
	}

	// A null descriptor rather than a skipped write: the slot has to hold a defined view, so
	// a shader that reads it samples zero instead of whatever a previously bound set left in
	// that descriptor. That is what lets a pass state "this program's texture has no resource
	// in this pass" instead of leaving the slot to chance.
	D3D12_SHADER_RESOURCE_VIEW_DESC NullDesc = {};
	NullDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
	NullDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
	NullDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	NullDesc.Texture2D.MipLevels     = 1u;

	D3D12_CPU_DESCRIPTOR_HANDLE Dst = OffsetCPUHandle(
		OwnerDevice->GetCBVSRVUAVHeapStart(),
		static_cast<INT32>(CurrentSRVTableBase + InSlot),
		static_cast<INT32>(OwnerDevice->GetCBVSRVUAVDescriptorSize()));

	OwnerDevice->GetD3D12Device()->CreateShaderResourceView(nullptr, &NullDesc, Dst);
}

void RCommandListD3D12::SetUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot, IRRHITexture* InTexture)
{
	if (!CommandList || !OwnerDevice || !InTexture || !bTableRegionKnown)
	{
		return;
	}

	// API-level bound: D3D12_PS_CS_UAV_REGISTER_COUNT UAVs per shader stage.
	PE_CHECK((ENGINE_RENDER_CORE_ERROR),
		(PE_TEXT("D3D12: UAV slot exceeds the API's UAV register count.")),
		(InSlot < D3D12_PS_CS_UAV_REGISTER_COUNT));
	if (InSlot >= D3D12_PS_CS_UAV_REGISTER_COUNT)
	{
		return;
	}

	IRD3D12Texture* Tex = static_cast<IRD3D12Texture*>(InTexture);
	D3D12_CPU_DESCRIPTOR_HANDLE Src = Tex->GetD3D12UAVHandle();
	if (Src.ptr == 0u)
	{
		return;
	}
	if (!EnsureResourceSet(InStage, InSlot, TRUE))
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Dst = OffsetCPUHandle(
		OwnerDevice->GetCBVSRVUAVHeapStart(),
		static_cast<INT32>(CurrentUAVTableBase + InSlot),
		static_cast<INT32>(OwnerDevice->GetCBVSRVUAVDescriptorSize()));

	OwnerDevice->GetD3D12Device()->CopyDescriptorsSimple(1u, Dst, Src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RCommandListD3D12::SetUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot, IRRHIBuffer* InBuffer)
{
	if (!CommandList || !OwnerDevice || !InBuffer || !bTableRegionKnown)
	{
		return;
	}

	// Same API bound as the texture overload above.
	PE_CHECK((ENGINE_RENDER_CORE_ERROR),
		(PE_TEXT("D3D12: UAV slot exceeds the API's UAV register count.")),
		(InSlot < D3D12_PS_CS_UAV_REGISTER_COUNT));
	if (InSlot >= D3D12_PS_CS_UAV_REGISTER_COUNT)
	{
		return;
	}

	IRD3D12Buffer* Buf = static_cast<IRD3D12Buffer*>(InBuffer);
	D3D12_CPU_DESCRIPTOR_HANDLE Src = Buf->GetD3D12UAVHandle();
	if (Src.ptr == 0u)
	{
		return;
	}
	if (!EnsureResourceSet(InStage, InSlot, TRUE))
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Dst = OffsetCPUHandle(
		OwnerDevice->GetCBVSRVUAVHeapStart(),
		static_cast<INT32>(CurrentUAVTableBase + InSlot),
		static_cast<INT32>(OwnerDevice->GetCBVSRVUAVDescriptorSize()));

	OwnerDevice->GetD3D12Device()->CopyDescriptorsSimple(1u, Dst, Src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RCommandListD3D12::ClearUnorderedAccessView(ERHIShaderStage InStage, UINT32 InSlot)
{
	if (!CommandList || !OwnerDevice || !bTableRegionKnown)
	{
		return;
	}
	PE_CHECK((ENGINE_RENDER_CORE_ERROR),
		(PE_TEXT("D3D12: UAV slot exceeds the API's UAV register count.")),
		(InSlot < D3D12_PS_CS_UAV_REGISTER_COUNT));
	if (InSlot >= D3D12_PS_CS_UAV_REGISTER_COUNT)
	{
		return;
	}
	if (!EnsureResourceSet(InStage, InSlot, TRUE))
	{
		return;
	}

	// Same reasoning as ClearShaderResourceView: the slot gets a defined null view.
	D3D12_UNORDERED_ACCESS_VIEW_DESC NullDesc = {};
	NullDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
	NullDesc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE2D;
	NullDesc.Texture2D.MipSlice    = 0u;

	D3D12_CPU_DESCRIPTOR_HANDLE Dst = OffsetCPUHandle(
		OwnerDevice->GetCBVSRVUAVHeapStart(),
		static_cast<INT32>(CurrentUAVTableBase + InSlot),
		static_cast<INT32>(OwnerDevice->GetCBVSRVUAVDescriptorSize()));

	OwnerDevice->GetD3D12Device()->CreateUnorderedAccessView(nullptr, nullptr, &NullDesc, Dst);
}

void RCommandListD3D12::SetSampler(ERHIShaderStage InStage, UINT32 InSlot, IRRHISampler* InSampler)
{
	if (!CommandList || !OwnerDevice || !InSampler || !bTableRegionKnown)
	{
		return;
	}

	// API-level bound: D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT samplers per shader stage.
	// (The sampler heap may hold many more; this is what a shader can address by register.)
	PE_CHECK((ENGINE_RENDER_CORE_ERROR),
		(PE_TEXT("D3D12: sampler slot exceeds the API's sampler slot count.")),
		(InSlot < D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT));
	if (InSlot >= D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT)
	{
		return;
	}

	IRD3D12Sampler* Sampler = static_cast<IRD3D12Sampler*>(InSampler);
	D3D12_CPU_DESCRIPTOR_HANDLE Src = Sampler->GetD3D12SamplerHandle();
	if (Src.ptr == 0u)
	{
		return;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Dst = OffsetCPUHandle(
		OwnerDevice->GetSamplerHeapStart(),
		static_cast<INT32>(CurrentSamplerTableBase + InSlot),
		static_cast<INT32>(OwnerDevice->GetSamplerDescriptorSize()));

	OwnerDevice->GetD3D12Device()->CopyDescriptorsSimple(1u, Dst, Src, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

// =========================================================================
// IRCommandList clears
// =========================================================================

void RCommandListD3D12::ClearRenderTarget(IRRHITexture* InTarget, const Color4& InColor)
{
	if (!CommandList || !InTarget)
	{
		return;
	}

	IRD3D12Texture* Tex = static_cast<IRD3D12Texture*>(InTarget);
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = Tex->GetD3D12RTVHandle();
	if (Handle.ptr == 0u)
	{
		return;
	}

	FLOAT Color[4] = { InColor.r, InColor.g, InColor.b, InColor.a };
	CommandList->ClearRenderTargetView(Handle, Color, 0u, nullptr);
}

void RCommandListD3D12::ClearDepthStencil(IRRHITexture* InTarget, UINT8 InClearFlags, FLOAT InDepth, UINT8 InStencil)
{
	if (!CommandList || !InTarget)
	{
		return;
	}

	IRD3D12Texture* Tex = static_cast<IRD3D12Texture*>(InTarget);
	D3D12_CPU_DESCRIPTOR_HANDLE Handle = Tex->GetD3D12DSVHandle();
	if (Handle.ptr == 0u)
	{
		return;
	}

	D3D12_CLEAR_FLAGS Flags = D3D12_CLEAR_FLAG_DEPTH;
	if (InClearFlags & 0x2u)  // Stencil flag (matching RClearDepthStencilFlagType pattern)
	{
		Flags |= D3D12_CLEAR_FLAG_STENCIL;
	}

	CommandList->ClearDepthStencilView(Handle, Flags, InDepth, InStencil, 0u, nullptr);
}

void RCommandListD3D12::ClearUnorderedAccessFloat(IRRHITexture* InTarget, const Color4& InValue)
{
	if (!CommandList || !InTarget || !OwnerDevice)
	{
		return;
	}

	IRD3D12Texture* Tex = static_cast<IRD3D12Texture*>(InTarget);
	D3D12_CPU_DESCRIPTOR_HANDLE UAVCPU = Tex->GetD3D12UAVHandle();
	if (UAVCPU.ptr == 0u)
	{
		return;
	}

	// ClearUnorderedAccessView requires a NON-shader-visible CPU handle plus a
	// matching shader-visible GPU handle. UAVCPU is now in the CBV/SRV/UAV
	// staging heap (CPU-only). The mirror in the shader-visible heap lives at
	// the same slot index, so we derive the GPU handle by reusing that offset.
	const SIZE_T DescSize = OwnerDevice->GetCBVSRVUAVDescriptorSize();
	const SIZE_T HeapByteOffset = UAVCPU.ptr - OwnerDevice->GetCBVSRVUAVStagingHeapStart().ptr;
	D3D12_GPU_DESCRIPTOR_HANDLE UAVGPU = OwnerDevice->GetCBVSRVUAVHeapGPUStart();
	UAVGPU.ptr += static_cast<UINT64>(HeapByteOffset);
	(void)DescSize;

	FLOAT Values[4] = { InValue.r, InValue.g, InValue.b, InValue.a };
	CommandList->ClearUnorderedAccessViewFloat(UAVGPU, UAVCPU, Tex->GetD3D12Resource(), Values, 0u, nullptr);
}

void RCommandListD3D12::ClearUnorderedAccessUint(IRRHITexture* InTarget, const Vector4Int& InValue)
{
	if (!CommandList || !InTarget || !OwnerDevice)
	{
		return;
	}

	IRD3D12Texture* Tex = static_cast<IRD3D12Texture*>(InTarget);
	D3D12_CPU_DESCRIPTOR_HANDLE UAVCPU = Tex->GetD3D12UAVHandle();
	if (UAVCPU.ptr == 0u)
	{
		return;
	}

	const SIZE_T HeapByteOffset = UAVCPU.ptr - OwnerDevice->GetCBVSRVUAVStagingHeapStart().ptr;
	D3D12_GPU_DESCRIPTOR_HANDLE UAVGPU = OwnerDevice->GetCBVSRVUAVHeapGPUStart();
	UAVGPU.ptr += static_cast<UINT64>(HeapByteOffset);

	UINT32 Values[4] = { static_cast<UINT32>(InValue.v[0]), static_cast<UINT32>(InValue.v[1]), static_cast<UINT32>(InValue.v[2]), static_cast<UINT32>(InValue.v[3]) };
	CommandList->ClearUnorderedAccessViewUint(UAVGPU, UAVCPU, Tex->GetD3D12Resource(), Values, 0u, nullptr);
}

// =========================================================================
// IRCommandList copy
// =========================================================================

void RCommandListD3D12::CopyResource(IRRHIResource* InDest, IRRHIResource* InSource)
{
	if (!CommandList || !InDest || !InSource)
	{
		return;
	}

	ID3D12Resource* Src = nullptr;
	ID3D12Resource* Dst = nullptr;

	if (InDest->GetResourceType() == ERHIResourceTypeTag::RHI_RESOURCE_TYPE_TEXTURE)
	{
		Dst = static_cast<IRD3D12Texture*>(InDest)->GetD3D12Resource();
	}
	else if (InDest->GetResourceType() == ERHIResourceTypeTag::RHI_RESOURCE_TYPE_BUFFER)
	{
		Dst = static_cast<IRD3D12Buffer*>(InDest)->GetD3D12Resource();
	}

	if (InSource->GetResourceType() == ERHIResourceTypeTag::RHI_RESOURCE_TYPE_TEXTURE)
	{
		Src = static_cast<IRD3D12Texture*>(InSource)->GetD3D12Resource();
	}
	else if (InSource->GetResourceType() == ERHIResourceTypeTag::RHI_RESOURCE_TYPE_BUFFER)
	{
		Src = static_cast<IRD3D12Buffer*>(InSource)->GetD3D12Resource();
	}

	if (Dst && Src)
	{
		CommandList->CopyResource(Dst, Src);
	}
}

void RCommandListD3D12::CopyBufferRegion(IRRHIBuffer* InDest, SIZE_T InDestOffset, IRRHIBuffer* InSource, SIZE_T InSourceOffset, SIZE_T InSize)
{
	if (!CommandList || !InDest || !InSource)
	{
		return;
	}

	ID3D12Resource* Src = static_cast<IRD3D12Buffer*>(InSource)->GetD3D12Resource();
	ID3D12Resource* Dst = static_cast<IRD3D12Buffer*>(InDest)->GetD3D12Resource();

	if (Dst && Src)
	{
		CommandList->CopyBufferRegion(Dst, InDestOffset, Src, InSourceOffset, InSize);
	}
}

// =========================================================================
// IRCommandList draw / dispatch
// =========================================================================

void RCommandListD3D12::Draw(UINT32 InVertexCount, UINT32 InInstanceCount, UINT32 InStartVertex, UINT32 InStartInstance)
{
	if (!CommandList || !HasBoundPipeline)
	{
		return;
	}
	CommandList->DrawInstanced(InVertexCount, InInstanceCount, InStartVertex, InStartInstance);
	// The draw reads the live set now: it may not be written again (see SealResourceSet).
	SealResourceSet();
}

void RCommandListD3D12::DrawIndexed(UINT32 InIndexCount, UINT32 InInstanceCount, UINT32 InStartIndex, INT32 InBaseVertex, UINT32 InStartInstance)
{
	if (!CommandList || !HasBoundPipeline)
	{
		return;
	}
	CommandList->DrawIndexedInstanced(InIndexCount, InInstanceCount, InStartIndex, InBaseVertex, InStartInstance);
	SealResourceSet();
}

void RCommandListD3D12::Dispatch(UINT32 InThreadGroupCountX, UINT32 InThreadGroupCountY, UINT32 InThreadGroupCountZ)
{
	if (!CommandList)
	{
		return;
	}
	CommandList->Dispatch(InThreadGroupCountX, InThreadGroupCountY, InThreadGroupCountZ);
	SealResourceSet();
}

void RCommandListD3D12::DispatchIndirect(IRRHIBuffer* InArgBuffer, SIZE_T InArgOffset)
{
	if (!CommandList || !InArgBuffer)
	{
		return;
	}

	IRD3D12Buffer* Buf = static_cast<IRD3D12Buffer*>(InArgBuffer);
	CommandList->ExecuteIndirect(nullptr, 1u, Buf->GetD3D12Resource(), InArgOffset, nullptr, 0u);
	SealResourceSet();
}

// =========================================================================
// IRCommandList debug markers
// =========================================================================

void RCommandListD3D12::BeginDebugMarker(const EString& InName)
{
#if _DEBUG_MODE
	if (CommandList)
	{
		WCHAR WideName[256] = {};
		const CHAR* NamePtr = *InName;
		MultiByteToWideChar(CP_UTF8, 0u, NamePtr, -1, WideName, 256);
		// PIX / RenderDoc: ID3D12GraphicsCommandList::BeginEvent's Size
		// parameter is the byte length of pData, not the character count.
		// Include the trailing null so capture tools see a properly
		// terminated wide string - passing the character count would only
		// reveal half the name (each WCHAR is 2 bytes).
		const UINT WideBytes = static_cast<UINT>((wcslen(WideName) + 1u) * sizeof(WCHAR));
		CommandList->BeginEvent(0u, WideName, WideBytes);
	}
#else
	(void)InName;
#endif
}

void RCommandListD3D12::EndDebugMarker()
{
#if _DEBUG_MODE
	if (CommandList)
	{
		CommandList->EndEvent();
	}
#endif
}

void RCommandListD3D12::WriteTimestamp(RRHITimestampHeap* InHeap, UINT32 InSlot)
{
	RRHITimestampHeapD3D12* Heap = static_cast<RRHITimestampHeapD3D12*>(InHeap);
	if (!Heap || !Heap->QueryHeap || !CommandList) { return; }
	if (InSlot >= Heap->Capacity) { return; }
	// EndQuery on a TIMESTAMP heap snapshots the current GPU pipeline tick
	// - there is no matching BeginQuery for the TIMESTAMP query type.
	CommandList->EndQuery(Heap->QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, InSlot);
}

void RCommandListD3D12::ResolveTimestampHeap(RRHITimestampHeap* InHeap, UINT32 InBeginSlot, UINT32 InCount)
{
	RRHITimestampHeapD3D12* Heap = static_cast<RRHITimestampHeapD3D12*>(InHeap);
	if (!Heap || !Heap->QueryHeap || !Heap->ReadbackBuffer || !CommandList) { return; }
	if (InCount == 0u) { return; }
	if ((InBeginSlot + InCount) > Heap->Capacity) { return; }
	// Destination offset matches the source slot offset (UINT64 stride) so
	// RDeviceD3D12::ResolveTimestamps can read [BeginSlot, BeginSlot+Count)
	// out of the readback at the same indices the caller wrote into.
	const UINT64 DstOffsetBytes = static_cast<UINT64>(InBeginSlot) * sizeof(UINT64);
	CommandList->ResolveQueryData(
		Heap->QueryHeap.Get(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		InBeginSlot,
		InCount,
		Heap->ReadbackBuffer.Get(),
		DstOffsetBytes);
}

}
