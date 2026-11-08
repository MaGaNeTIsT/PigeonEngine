#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Text/Name.h>
#include "Profiler.h"

namespace PigeonEngine
{

	class IRCommandList;
	class IRRHIDevice;
	struct RRHITimestampHeap;

	// -----------------------------------------------------------------------
	// EGPUProfiler - singleton manager that owns the per-frame timestamp
	// heap and translates pairs of GPU ticks into EProfiler::RecordGPUSample
	// calls.
	//
	// Frame layout:
	//   * One heap, sized HeapFrameNum * SLOTS_PER_FRAME slots.
	//   * Each frame owns a fixed slice [D * SLOTS_PER_FRAME, (D+1) * ...).
	//   * A slice is read back only after the GPU has retired the frame that
	//     wrote it. How that is known differs by backend:
	//       D3D12 - D is the device's frame index, and IRRHIDevice::BeginFrame
	//               just waited the fence that protected this slice.
	//       D3D11 - there is no per-frame fence to wait on without stalling,
	//               so the profiler reads a slice D3D11ReadbackDelay frames
	//               after it was written and retries while it is not ready.
	//   * The heap therefore has max(device frame-in-flight, delay + 2) slices
	//     on D3D11: the current frame, the delayed one, and one spare so a
	//     not-ready query can be retried before its slice is reused.
	//
	// Threading: every public entry point runs on the RenderThread. No
	// internal locking - EProfiler::RecordGPUSample itself is mutex-guarded.
	// -----------------------------------------------------------------------
	class EGPUProfiler : public EManagerBase
	{
	public:
		static constexpr UINT32 SLOTS_PER_FRAME       = 128u;
		static constexpr UINT32 MAX_FRAMES_IN_FLIGHT  = 8u;
		// Marks a pass record whose timestamps were dropped because the frame's
		// slice ran out of budget. A placeholder still reaches PassStack, so the
		// matching OnPassEnd pops this pass rather than its parent.
		static constexpr UINT32 INVALID_SLOT          = 0xFFFFFFFFu;

	public:
		virtual void	Initialize() override;
		virtual void	ShutDown() override;

	public:
		// Frame ticks driven by SceneRenderer / RenderThread.
		void			BeginFrame(IRCommandList* InCmd);
		void			EndFrame(IRCommandList* InCmd);

		// Pass-grain hooks called by IRCommandList::BeginRenderPass /
		// EndRenderPass when RRHIRenderPassDesc::PassName is non-null. Safe
		// to call with null arguments - silently no-op so the call sites
		// don't need defensive checks.
		void			OnPassBegin(IRCommandList* InCmd, const CHAR* InName);
		void			OnPassEnd(IRCommandList* InCmd);
		// FALSE while a renderer that is not the frame's main renderer is recording. Its passes
		// are not part of the frame's timing slice - that slice is opened and closed by the main
		// renderer - so they are dropped rather than charged to it.
		void			SetPassRecordingEnabled(const BOOL8 InEnabled) { bPassRecordingEnabled = InEnabled; }

		// Frames whose GPU samples were lost: a D3D11 slice reused while its
		// readback was still pending, or a D3D12 readback that failed outright.
		// The panel shows it, so a session that loses samples behind a slow GPU
		// is distinguishable from one that never recorded any.
		PE_NODISCARD UINT32 GetDroppedResolveNum() const noexcept { return DroppedResolveNum; }

	private:
		struct EPassRecord
		{
			EName	Name;
			UINT32	SlotBegin	= 0u;
			UINT32	SlotEnd		= 0u;
		};
		struct EFrameRecord
		{
			TArray<EPassRecord>	Passes;
			UINT32				SliceBegin			= 0u;
			UINT32				NextSlotInFrame		= 0u;
			BOOL8				bPendingResolve		= FALSE;
			// Frame level bracket: one timestamp before the first pass and one
			// after the last. Taken out of the same slice, so the frame total
			// and the pass totals come from the same clock and the panel can
			// show a pass's share of the frame.
			UINT32				FrameBeginSlot		= 0u;
			UINT32				FrameEndSlot		= 0u;
			BOOL8				bHasFrameScope		= FALSE;
			// Passes dropped because the slice ran out. Reported once per frame,
			// not once per pass.
			UINT32				DroppedPassNum		= 0u;
		};

		BOOL8			EnsureHeap(IRRHIDevice* InDevice);
		void			TryResolveFrame(UINT32 InFrameSlot);
		// Clears every frame record and re-seats each SliceBegin on its slot.
		void			ResetFrameRecords();

	private:
		IRRHIDevice*		Device				= nullptr;
		RRHITimestampHeap*	Heap				= nullptr;
		// Slices the heap is split into. D3D12 uses the device's
		// frame-in-flight; D3D11 uses max(frame-in-flight, delay + 2).
		UINT32				HeapFrameNum		= 0u;
		// D3D11 only: frames between a timestamp write and its readback attempt.
		UINT32				D3D11ReadbackDelay	= 0u;
		EFrameRecord		Frames[MAX_FRAMES_IN_FLIGHT];
		UINT32				CurrentFrameSlot	= 0u;
		BOOL8				bAnyFrameStarted	= FALSE;
		BOOL8				bPassRecordingEnabled = TRUE;
		TArray<EPassRecord>	PassStack;
		// Frames whose GPU samples were lost: D3D11 slices reused while still
		// pending, and D3D12 frames whose readback failed outright. Reported in
		// the panel, so a session that loses samples behind a slow GPU is
		// distinguishable from one that never recorded any.
		UINT32				DroppedResolveNum	= 0u;

		CLASS_MANAGER_SINGLETON_BODY(EGPUProfiler)
	};

};

// ---------------------------------------------------------------------------
// Macro hooks. Use these at call sites (RenderThread, SceneRenderer,
// CommandList backends). When ENGINE_ENABLE_GPU_PROFILER is off they expand to
// nothing - argument expressions are NOT evaluated.
// ---------------------------------------------------------------------------
#if ENGINE_ENABLE_GPU_PROFILER

#define PE_GPU_PROFILER_BEGIN_FRAME(__CommandList) \
	::PigeonEngine::EGPUProfiler::GetManagerSingleton()->BeginFrame((__CommandList))
#define PE_GPU_PROFILER_END_FRAME(__CommandList) \
	::PigeonEngine::EGPUProfiler::GetManagerSingleton()->EndFrame((__CommandList))
#define PE_GPU_PROFILER_PASS_BEGIN(__CommandList, __Name) \
	::PigeonEngine::EGPUProfiler::GetManagerSingleton()->OnPassBegin((__CommandList), (__Name))
#define PE_GPU_PROFILER_PASS_END(__CommandList) \
	::PigeonEngine::EGPUProfiler::GetManagerSingleton()->OnPassEnd((__CommandList))

#else

#define PE_GPU_PROFILER_BEGIN_FRAME(__CommandList)
#define PE_GPU_PROFILER_END_FRAME(__CommandList)
#define PE_GPU_PROFILER_PASS_BEGIN(__CommandList, __Name)
#define PE_GPU_PROFILER_PASS_END(__CommandList)

#endif
