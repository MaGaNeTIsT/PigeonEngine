#include "GPUProfiler.h"

#if _GPU_ENABLE_PROFILER

#include <RHI/IRRHIDevice.h>
#include <RHI/IRCommandList.h>
#include <Main/MainManager.h>

namespace PigeonEngine
{

	EGPUProfiler::EGPUProfiler()
	{
	}
	EGPUProfiler::~EGPUProfiler()
	{
	}
	void EGPUProfiler::Initialize()
	{
		// Heap is allocated lazily on first BeginFrame so we don't depend on
		// MainManager->GetRenderDevice() being live when this singleton's
		// Initialize fires (boot order: Profiler manager init can land before
		// the device is fully created).
		Device				= nullptr;
		Heap				= nullptr;
		FrameInFlight		= 0u;
		CurrentFrameSlot	= 0u;
		bAnyFrameStarted	= FALSE;
		for (UINT32 i = 0u; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			Frames[i].Passes.Empty();
			Frames[i].SliceBegin		= 0u;
			Frames[i].NextSlotInFrame	= 0u;
			Frames[i].bPendingResolve	= FALSE;
		}
		PassStack.Empty();
	}
	void EGPUProfiler::ShutDown()
	{
		if (Device && Heap)
		{
			Device->DestroyTimestampHeap(Heap);
		}
		Device	= nullptr;
		Heap	= nullptr;
	}

	BOOL8 EGPUProfiler::EnsureHeap(IRRHIDevice* InDevice)
	{
		if (Heap) { return TRUE; }
		if (!InDevice) { return FALSE; }

		Device			= InDevice;
		FrameInFlight	= InDevice->GetFrameInFlightCount();
		if (FrameInFlight == 0u) { FrameInFlight = 1u; }
		if (FrameInFlight > MAX_FRAMES_IN_FLIGHT) { FrameInFlight = MAX_FRAMES_IN_FLIGHT; }

		const UINT32 TotalSlots = FrameInFlight * SLOTS_PER_FRAME;
		if (!InDevice->CreateTimestampHeap(TotalSlots, &Heap))
		{
			Heap = nullptr;
			return FALSE;
		}
		for (UINT32 i = 0u; i < FrameInFlight; i++)
		{
			Frames[i].SliceBegin		= i * SLOTS_PER_FRAME;
			Frames[i].NextSlotInFrame	= 0u;
			Frames[i].bPendingResolve	= FALSE;
			Frames[i].Passes.Empty();
		}
		return TRUE;
	}

	void EGPUProfiler::BeginFrame(IRCommandList* InCmd)
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!EnsureHeap(RenderDevice)) { return; }

		// Advance the slice. First frame stays at slot 0; subsequent frames
		// rotate through FrameInFlight slots so each slice's prior occupant
		// is at least FrameInFlight frames old by the time we touch it.
		if (bAnyFrameStarted)
		{
			CurrentFrameSlot = (CurrentFrameSlot + 1u) % FrameInFlight;
		}
		else
		{
			CurrentFrameSlot = 0u;
			bAnyFrameStarted = TRUE;
		}

		// The fence that protected this slice retired during IRRHIDevice::
		// BeginFrame just before us, so the prior frame's readback (if any)
		// is safe to read now.
		TryResolveFrame(CurrentFrameSlot);

		EFrameRecord& F = Frames[CurrentFrameSlot];
		F.Passes.Empty();
		F.NextSlotInFrame	= 0u;
		F.bPendingResolve	= FALSE;
		PassStack.Empty();

		// Disjoint Begin (D3D11) / no-op (D3D12).
		Device->BeginTimestampFrame(Heap);
		(void)InCmd;
	}

	void EGPUProfiler::EndFrame(IRCommandList* InCmd)
	{
		if (!Device || !Heap) { return; }

		EFrameRecord& F = Frames[CurrentFrameSlot];
		if (F.NextSlotInFrame > 0u && InCmd)
		{
			// GPU-side copy: query heap -> readback buffer. D3D11 backend
			// records a no-op here; D3D12 emits ResolveQueryData so the
			// readback is populated by the time this frame's fence retires.
			InCmd->ResolveTimestampHeap(Heap, F.SliceBegin, F.NextSlotInFrame);
			F.bPendingResolve = TRUE;
		}

		// Disjoint End (D3D11) / no-op (D3D12).
		Device->EndTimestampFrame(Heap);

		// Any leftover Begin without End - record but warn (would have to be
		// a renderer-side bug; ResolveTimestamps would skip such pairs).
		PassStack.Empty();
	}

	void EGPUProfiler::OnPassBegin(IRCommandList* InCmd, const CHAR* InName)
	{
		if (!InCmd || !InName || !Heap) { return; }

		EFrameRecord& F = Frames[CurrentFrameSlot];
		if ((F.NextSlotInFrame + 1u) > SLOTS_PER_FRAME) { return; }   // slice exhausted

		const UINT32 Slot = F.SliceBegin + F.NextSlotInFrame;
		F.NextSlotInFrame += 1u;

		InCmd->WriteTimestamp(Heap, Slot);
		InCmd->BeginGPUEvent(EString(InName));

		EPassRecord Rec;
		Rec.Name      = EName(InName);
		Rec.SlotBegin = Slot;
		Rec.SlotEnd   = Slot;	// filled on End
		PassStack.Add(Rec);
	}

	void EGPUProfiler::OnPassEnd(IRCommandList* InCmd)
	{
		if (!InCmd || !Heap) { return; }
		const INT32 StackNum = PassStack.Num<INT32>();
		if (StackNum <= 0) { return; }	// stray End - silently drop

		EPassRecord Rec = PassStack[StackNum - 1];
		PassStack.RemoveAt(StackNum - 1);

		EFrameRecord& F = Frames[CurrentFrameSlot];
		if ((F.NextSlotInFrame + 1u) > SLOTS_PER_FRAME)
		{
			// Slice exhausted between Begin and End - close the GPU event
			// for symmetry but drop the timing record.
			InCmd->EndGPUEvent();
			return;
		}

		Rec.SlotEnd = F.SliceBegin + F.NextSlotInFrame;
		F.NextSlotInFrame += 1u;

		InCmd->WriteTimestamp(Heap, Rec.SlotEnd);
		InCmd->EndGPUEvent();

		F.Passes.Add(Rec);
	}

	void EGPUProfiler::TryResolveFrame(UINT32 InFrameSlot)
	{
		if (InFrameSlot >= FrameInFlight) { return; }
		EFrameRecord& F = Frames[InFrameSlot];
		if (!F.bPendingResolve) { return; }
		if (F.NextSlotInFrame == 0u) { F.bPendingResolve = FALSE; F.Passes.Empty(); return; }

		// Pull every tick in this slice in one call to minimise GetData /
		// Map traffic.
		UINT64 Ticks[SLOTS_PER_FRAME] = {};
		const BOOL8 Resolved = Device->ResolveTimestamps(Heap, F.SliceBegin, F.NextSlotInFrame, Ticks);
		const UINT64 Freq    = Device->GetGPUTicksPerSecond(Heap);

		if (Resolved && Freq > 0u)
		{
			const DOUBLE InvFreq = 1.0 / static_cast<DOUBLE>(Freq);
			const INT32 PassNum = F.Passes.Num<INT32>();
			for (INT32 i = 0; i < PassNum; i++)
			{
				const EPassRecord& Rec  = F.Passes[i];
				const UINT32 IdxBegin   = Rec.SlotBegin - F.SliceBegin;
				const UINT32 IdxEnd     = Rec.SlotEnd   - F.SliceBegin;
				if (IdxBegin >= F.NextSlotInFrame || IdxEnd >= F.NextSlotInFrame) { continue; }
				const UINT64 TickBegin  = Ticks[IdxBegin];
				const UINT64 TickEnd    = Ticks[IdxEnd];
				if (TickEnd <= TickBegin) { continue; }	// wrap / glitch
				const DOUBLE Seconds = static_cast<DOUBLE>(TickEnd - TickBegin) * InvFreq;
				EProfiler::GetManagerSingleton()->RecordGPUSample(Rec.Name, Seconds);
			}
		}

		F.Passes.Empty();
		F.NextSlotInFrame   = 0u;
		F.bPendingResolve   = FALSE;
	}

};

#else

namespace PigeonEngine
{
	// Profiler disabled: empty out-of-line bodies so the singleton object
	// still has linkage for any vestigial references.
	EGPUProfiler::EGPUProfiler() {}
	EGPUProfiler::~EGPUProfiler() {}
	void EGPUProfiler::Initialize()										{}
	void EGPUProfiler::ShutDown()										{}
	void EGPUProfiler::BeginFrame(IRCommandList*)						{}
	void EGPUProfiler::EndFrame(IRCommandList*)							{}
	void EGPUProfiler::OnPassBegin(IRCommandList*, const CHAR*)			{}
	void EGPUProfiler::OnPassEnd(IRCommandList*)						{}
	BOOL8 EGPUProfiler::EnsureHeap(IRRHIDevice*)						{ return FALSE; }
	void EGPUProfiler::TryResolveFrame(UINT32)							{}
};

#endif
