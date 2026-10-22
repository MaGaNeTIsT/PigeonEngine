#include "GPUProfiler.h"

#if _GPU_ENABLE_PROFILER

#include <RHI/IRRHIDevice.h>
#include <RHI/IRCommandList.h>
#include <Main/MainManager.h>
#include <EngineCore/Editor/EditorLogManager.h>

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
		// The heap is allocated lazily on the first BeginFrame: boot order can
		// run this Initialize before MainManager's render device exists. What
		// this does guarantee is a clean slate, so a second Initialize after a
		// device restart cannot inherit a stale slot or a pending record.
		ResetFrameRecords();
		Device				= nullptr;
		Heap				= nullptr;
		HeapFrameNum		= 0u;
		D3D11ReadbackDelay	= 0u;
		CurrentFrameSlot	= 0u;
		bAnyFrameStarted	= FALSE;
		DroppedResolveNum	= 0u;
		PassStack.Empty();
	}
	void EGPUProfiler::ShutDown()
	{
		if (Device)
		{
			// Drain the GPU before the heap goes away. The device owns the query
			// objects and the readback resource, and a frame may still be
			// executing against them; destroying the heap under that frame is
			// exactly the kind of shutdown-time crash the debug layer reports.
			Device->WaitForGPUIdle();
			if (Heap)
			{
				Device->DestroyTimestampHeap(Heap);
			}
		}
		Device				= nullptr;
		Heap				= nullptr;
		HeapFrameNum		= 0u;
		D3D11ReadbackDelay	= 0u;
		CurrentFrameSlot	= 0u;
		bAnyFrameStarted	= FALSE;
		DroppedResolveNum	= 0u;
		ResetFrameRecords();
		PassStack.Empty();
	}

	void EGPUProfiler::ResetFrameRecords()
	{
		for (UINT32 FrameIndex = 0u; FrameIndex < MAX_FRAMES_IN_FLIGHT; FrameIndex++)
		{
			Frames[FrameIndex]				= EFrameRecord();
			Frames[FrameIndex].SliceBegin	= FrameIndex * SLOTS_PER_FRAME;
		}
	}

	BOOL8 EGPUProfiler::EnsureHeap(IRRHIDevice* InDevice)
	{
		if (Heap)
		{
			return TRUE;
		}
		if (!InDevice)
		{
			return FALSE;
		}

		Device = InDevice;
		const UINT32 DeviceFrameInFlight = EMath::Clamp<UINT32>(InDevice->GetFrameInFlightCount(), 1u, MAX_FRAMES_IN_FLIGHT);
		const BOOL8 bIsD3D11 = (InDevice->GetBackendType() == ERHIBackendType::RHI_BACKEND_D3D11);
		D3D11ReadbackDelay = 0u;
		HeapFrameNum = DeviceFrameInFlight;
		if (bIsD3D11 != FALSE)
		{
			// Fixed delay, clamped so the ring always keeps at least one spare
			// slice for a retry: the delayed frame, the current one, and one
			// more before the delayed slice would be reused.
			D3D11ReadbackDelay = EMath::Clamp<UINT32>(EEngineSettings::ENGINE_PROFILER_GPU_D3D11_READBACK_DELAY, 1u, MAX_FRAMES_IN_FLIGHT - 2u);
			HeapFrameNum = EMath::Max(DeviceFrameInFlight, D3D11ReadbackDelay + 2u);
		}

		const UINT32 TotalSlotNum = HeapFrameNum * SLOTS_PER_FRAME;
		if (!InDevice->CreateTimestampHeap(TotalSlotNum, HeapFrameNum, &Heap))
		{
			// Reported once. The profiler is optional, but a silent failure is
			// indistinguishable from "the GPU produced no data", and that is the
			// harder symptom to chase.
			static BOOL32 bReportedHeapFailure = FALSE;
			if (bReportedHeapFailure == FALSE)
			{
				bReportedHeapFailure = TRUE;
				PE_LOG_ERROR(EString(PE_TEXT("[GPUProfiler] Could not create the GPU timestamp heap; GPU timings are unavailable.")));
			}
			Heap	= nullptr;
			Device	= nullptr;
			return FALSE;
		}

		ResetFrameRecords();
		bAnyFrameStarted	= FALSE;
		CurrentFrameSlot	= 0u;
		DroppedResolveNum	= 0u;
		PassStack.Empty();
		return TRUE;
	}

	void EGPUProfiler::BeginFrame(IRCommandList* InCmd)
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!EnsureHeap(RenderDevice))
		{
			return;
		}

		if (Device->GetBackendType() == ERHIBackendType::RHI_BACKEND_D3D11)
		{
			// The profiler's own ring. The D3D11 heap holds delay + current +
			// retry slices, so this slot index is not the device frame index.
			CurrentFrameSlot = ((bAnyFrameStarted != FALSE) ? ((CurrentFrameSlot + 1u) % HeapFrameNum) : 0u);
			bAnyFrameStarted = TRUE;

			// Try every pending frame that is old enough, not just the one this
			// frame happens to point at: a readback that came back not-ready must
			// be retried, and the ring keeps one spare slice past the delay for
			// exactly that.
			for (UINT32 Age = D3D11ReadbackDelay; Age < HeapFrameNum; Age++)
			{
				const UINT32 PendingSlot = ((CurrentFrameSlot + HeapFrameNum - Age) % HeapFrameNum);
				if ((PendingSlot == CurrentFrameSlot) || (Frames[PendingSlot].bPendingResolve == FALSE))
				{
					continue;
				}
				TryResolveFrame(PendingSlot);
			}
		}
		else
		{
			// D3D12: the device's frame index names the slot whose fence
			// protected the prior occupant, and the slice count is the device's
			// frame-in-flight.
			CurrentFrameSlot = (RenderDevice->GetCurrentFrameIndex() % HeapFrameNum);
			bAnyFrameStarted = TRUE;
			TryResolveFrame(CurrentFrameSlot);
		}

		EFrameRecord& Frame = Frames[CurrentFrameSlot];

		// A pending record still sitting in this slot belonged to a frame that
		// was old enough to read but never came back ready. The slice is about
		// to be overwritten, so its samples are lost - count the frame once.
		if (Frame.bPendingResolve != FALSE)
		{
			DroppedResolveNum += 1u;
		}

		Frame				= EFrameRecord();
		Frame.SliceBegin	= CurrentFrameSlot * SLOTS_PER_FRAME;
		PassStack.Empty();

		// D3D11: open the disjoint before the first timestamp of the frame, so
		// the frame bracket and every pass timestamp fall inside it.
		// D3D12: no-op.
		Device->BeginTimestampFrame(Heap, CurrentFrameSlot);

		if (InCmd)
		{
			Frame.FrameBeginSlot	= Frame.SliceBegin + Frame.NextSlotInFrame;
			Frame.NextSlotInFrame	+= 1u;
			InCmd->WriteTimestamp(Heap, Frame.FrameBeginSlot);
			Frame.bHasFrameScope = TRUE;
		}
	}

	void EGPUProfiler::EndFrame(IRCommandList* InCmd)
	{
		if ((!Device) || (!Heap))
		{
			return;
		}

		EFrameRecord& Frame = Frames[CurrentFrameSlot];

		if ((Frame.bHasFrameScope != FALSE) && (InCmd) && (Frame.NextSlotInFrame < SLOTS_PER_FRAME))
		{
			Frame.FrameEndSlot		= Frame.SliceBegin + Frame.NextSlotInFrame;
			Frame.NextSlotInFrame	+= 1u;
			InCmd->WriteTimestamp(Heap, Frame.FrameEndSlot);
		}

		if ((Frame.NextSlotInFrame > 0u) && (InCmd))
		{
			// GPU-side copy: query heap -> readback buffer. D3D11 records a
			// no-op here; D3D12 emits ResolveQueryData so the readback is
			// populated by the time this frame's fence retires.
			InCmd->ResolveTimestampHeap(Heap, Frame.SliceBegin, Frame.NextSlotInFrame);
			Frame.bPendingResolve = TRUE;
		}

		// Disjoint End (D3D11) / no-op (D3D12).
		Device->EndTimestampFrame(Heap, CurrentFrameSlot);

		if (Frame.DroppedPassNum > 0u)
		{
			// Once per process: a renderer that overruns SLOTS_PER_FRAME does it
			// every frame, and a per-frame log would bury everything else.
			static BOOL32 bReportedSlotOverflow = FALSE;
			if (bReportedSlotOverflow == FALSE)
			{
				bReportedSlotOverflow = TRUE;
				PE_LOG_WARN(EString(PE_TEXT("[GPUProfiler] A frame overflowed its pass slot budget; some pass timings were dropped. Raise SLOTS_PER_FRAME or lower the pass count.")));
			}
		}

		PassStack.Empty();
	}

	void EGPUProfiler::OnPassBegin(IRCommandList* InCmd, const CHAR* InName)
	{
		if ((!InCmd) || (!InName) || (!Heap))
		{
			return;
		}
		if (!bPassRecordingEnabled)
		{
			return;
		}

		EFrameRecord& Frame = Frames[CurrentFrameSlot];
		if ((Frame.NextSlotInFrame + 1u) > SLOTS_PER_FRAME)
		{
			Frame.DroppedPassNum += 1u;
			// Still open the GPU event and push a placeholder, so the matching
			// OnPassEnd pops this pass rather than its parent.
			InCmd->BeginGPUEvent(EString(InName));
			EPassRecord DroppedRecord;
			DroppedRecord.Name		= EName(InName);
			DroppedRecord.SlotBegin	= INVALID_SLOT;
			DroppedRecord.SlotEnd	= INVALID_SLOT;
			PassStack.Add(DroppedRecord);
			return;
		}

		const UINT32 Slot = Frame.SliceBegin + Frame.NextSlotInFrame;
		Frame.NextSlotInFrame += 1u;

		InCmd->WriteTimestamp(Heap, Slot);
		InCmd->BeginGPUEvent(EString(InName));

		EPassRecord Record;
		Record.Name			= EName(InName);
		Record.SlotBegin	= Slot;
		Record.SlotEnd		= Slot;	// filled on End
		PassStack.Add(Record);
	}

	void EGPUProfiler::OnPassEnd(IRCommandList* InCmd)
	{
		if ((!InCmd) || (!Heap))
		{
			return;
		}
		if (!bPassRecordingEnabled)
		{
			return;
		}
		const INT32 StackNum = PassStack.Num<INT32>();
		if (StackNum <= 0)
		{
			return;	// stray End - silently drop
		}

		EPassRecord Record = PassStack[StackNum - 1];
		PassStack.RemoveAt(StackNum - 1);

		if (Record.SlotBegin == INVALID_SLOT)
		{
			// Overflowed at Begin; only the GPU event is still open.
			InCmd->EndGPUEvent();
			return;
		}

		EFrameRecord& Frame = Frames[CurrentFrameSlot];
		if ((Frame.NextSlotInFrame + 1u) > SLOTS_PER_FRAME)
		{
			// The slice ran out between Begin and End. Close the GPU event for
			// symmetry but drop the timing record.
			Frame.DroppedPassNum += 1u;
			InCmd->EndGPUEvent();
			return;
		}

		Record.SlotEnd = Frame.SliceBegin + Frame.NextSlotInFrame;
		Frame.NextSlotInFrame += 1u;

		InCmd->WriteTimestamp(Heap, Record.SlotEnd);
		InCmd->EndGPUEvent();

		Frame.Passes.Add(Record);
	}

	void EGPUProfiler::TryResolveFrame(UINT32 InFrameSlot)
	{
		if (InFrameSlot >= HeapFrameNum)
		{
			return;
		}
		EFrameRecord& Frame = Frames[InFrameSlot];
		if (Frame.bPendingResolve == FALSE)
		{
			return;
		}
		if (Frame.NextSlotInFrame == 0u)
		{
			Frame				= EFrameRecord();
			Frame.SliceBegin	= InFrameSlot * SLOTS_PER_FRAME;
			return;
		}

		// Pull every tick in this slice in one call to minimise GetData / Map
		// traffic.
		UINT64 Ticks[SLOTS_PER_FRAME] = {};
		const BOOL8 bResolved = Device->ResolveTimestamps(Heap, InFrameSlot, Frame.SliceBegin, Frame.NextSlotInFrame, Ticks);
		if (bResolved == FALSE)
		{
			// D3D11: the query is simply not ready yet. Keep the record so the
			// next BeginFrame retries before the slice is reused.
			// D3D12: a failed resolve is a real fault (its readback is protected
			// by the frame fence), so report it and drop rather than retry a
			// readback that will never become valid.
			if (Device->GetBackendType() != ERHIBackendType::RHI_BACKEND_D3D11)
			{
				static BOOL32 bReportedResolveFailure = FALSE;
				if (bReportedResolveFailure == FALSE)
				{
					bReportedResolveFailure = TRUE;
					PE_LOG_WARN(EString(PE_TEXT("[GPUProfiler] A D3D12 timestamp readback could not be resolved; GPU samples are being dropped.")));
				}
				DroppedResolveNum += 1u;
				Frame				= EFrameRecord();
				Frame.SliceBegin	= InFrameSlot * SLOTS_PER_FRAME;
			}
			return;
		}

		const UINT64 Frequency = Device->GetGPUTicksPerSecond(Heap);
		if (Frequency > 0u)
		{
			const DOUBLE InverseFrequency = 1.0 / static_cast<DOUBLE>(Frequency);
			for (INT32 PassIndex = 0; PassIndex < Frame.Passes.Num<INT32>(); PassIndex++)
			{
				const EPassRecord& Pass = Frame.Passes[PassIndex];
				const UINT32 IndexBegin = Pass.SlotBegin - Frame.SliceBegin;
				const UINT32 IndexEnd   = Pass.SlotEnd   - Frame.SliceBegin;
				if ((IndexBegin >= Frame.NextSlotInFrame) || (IndexEnd >= Frame.NextSlotInFrame))
				{
					continue;
				}
				const UINT64 TickBegin = Ticks[IndexBegin];
				const UINT64 TickEnd   = Ticks[IndexEnd];
				if (TickEnd <= TickBegin)
				{
					continue;	// wrap / glitch
				}
				EProfiler::GetManagerSingleton()->RecordGPUSample(Pass.Name, static_cast<DOUBLE>(TickEnd - TickBegin) * InverseFrequency);
			}

			if ((Frame.bHasFrameScope != FALSE)
				&& (Frame.FrameBeginSlot >= Frame.SliceBegin) && (Frame.FrameEndSlot >= Frame.SliceBegin))
			{
				const UINT32 IndexBegin = Frame.FrameBeginSlot - Frame.SliceBegin;
				const UINT32 IndexEnd   = Frame.FrameEndSlot - Frame.SliceBegin;
				if ((IndexBegin < Frame.NextSlotInFrame) && (IndexEnd < Frame.NextSlotInFrame) && (Ticks[IndexEnd] > Ticks[IndexBegin]))
				{
					EProfiler::GetManagerSingleton()->RecordGPUSample(
						EName(PROFILER_SCOPE_GPU_FRAME), static_cast<DOUBLE>(Ticks[IndexEnd] - Ticks[IndexBegin]) * InverseFrequency);
				}
			}
		}

		Frame				= EFrameRecord();
		Frame.SliceBegin	= InFrameSlot * SLOTS_PER_FRAME;
	}

};

#else

namespace PigeonEngine
{
	// Profiler disabled: empty out-of-line bodies so the singleton object
	// still has linkage for any vestigial references.
	EGPUProfiler::EGPUProfiler()
	{

	}
	EGPUProfiler::~EGPUProfiler()
	{

	}
	void EGPUProfiler::Initialize()
	{

	}
	void EGPUProfiler::ShutDown()
	{

	}
	void EGPUProfiler::BeginFrame(IRCommandList*)
	{

	}
	void EGPUProfiler::EndFrame(IRCommandList*)
	{

	}
	void EGPUProfiler::OnPassBegin(IRCommandList*, const CHAR*)
	{

	}
	void EGPUProfiler::OnPassEnd(IRCommandList*)
	{

	}
	BOOL8 EGPUProfiler::EnsureHeap(IRRHIDevice*)
	{
		return FALSE;
	}
	void EGPUProfiler::TryResolveFrame(UINT32)
	{

	}
	void EGPUProfiler::ResetFrameRecords()
	{

	}
};

#endif
