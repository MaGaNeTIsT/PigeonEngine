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
	//   * One heap, sized FrameInFlight * SLOTS_PER_FRAME slots.
	//   * Each frame uses a fixed slice [D * SLOTS_PER_FRAME, (D+1) * ...)
	//     where D = current_frame % FrameInFlight.
	//   * On BeginFrame for frame D the slice's prior occupant (frame
	//     D - FrameInFlight) has retired through IRRHIDevice::BeginFrame's
	//     fence wait - safe to resolve and feed into EProfiler.
	//
	// Threading: every public entry point runs on the RenderThread. No
	// internal locking - EProfiler::RecordGPUSample itself is mutex-guarded.
	// -----------------------------------------------------------------------
	class EGPUProfiler : public EManagerBase
	{
	public:
		static constexpr UINT32 SLOTS_PER_FRAME       = 128u;
		static constexpr UINT32 MAX_FRAMES_IN_FLIGHT  = 4u;

	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;

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
		};

		BOOL8			EnsureHeap(IRRHIDevice* InDevice);
		void			TryResolveFrame(UINT32 InFrameSlot);

	private:
		IRRHIDevice*		Device				= nullptr;
		RRHITimestampHeap*	Heap				= nullptr;
		UINT32				FrameInFlight		= 0u;
		EFrameRecord		Frames[MAX_FRAMES_IN_FLIGHT];
		UINT32				CurrentFrameSlot	= 0u;
		BOOL8				bAnyFrameStarted	= FALSE;
		TArray<EPassRecord>	PassStack;

		CLASS_MANAGER_SINGLETON_BODY(EGPUProfiler)
	};

};

// ---------------------------------------------------------------------------
// Macro hooks. Use these at call sites (RenderThread, SceneRenderer,
// CommandList backends). When _GPU_ENABLE_PROFILER is off they expand to
// nothing - argument expressions are NOT evaluated.
// ---------------------------------------------------------------------------
#if _GPU_ENABLE_PROFILER

#define PE_GPU_PROFILER_BEGIN_FRAME(InCmd) \
	::PigeonEngine::EGPUProfiler::GetManagerSingleton()->BeginFrame((InCmd))
#define PE_GPU_PROFILER_END_FRAME(InCmd) \
	::PigeonEngine::EGPUProfiler::GetManagerSingleton()->EndFrame((InCmd))
#define PE_GPU_PROFILER_PASS_BEGIN(InCmd, InName) \
	::PigeonEngine::EGPUProfiler::GetManagerSingleton()->OnPassBegin((InCmd), (InName))
#define PE_GPU_PROFILER_PASS_END(InCmd) \
	::PigeonEngine::EGPUProfiler::GetManagerSingleton()->OnPassEnd((InCmd))

#else

#define PE_GPU_PROFILER_BEGIN_FRAME(InCmd)
#define PE_GPU_PROFILER_END_FRAME(InCmd)
#define PE_GPU_PROFILER_PASS_BEGIN(InCmd, InName)
#define PE_GPU_PROFILER_PASS_END(InCmd)

#endif
