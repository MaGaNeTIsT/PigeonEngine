#include "Profiler.h"

#if _GPU_ENABLE_PROFILER
#include <RHI/IRCommandList.h>
#endif

namespace PigeonEngine
{

	EProfiler::EProfiler()
	{
	}
	EProfiler::~EProfiler()
	{
	}
	void EProfiler::Initialize()
	{
		ClearTraces();
	}
	void EProfiler::ShutDown()
	{
		ClearTraces();
	}
	void EProfiler::ClearTraces()
	{
		EScopedLock Lock(StatsMutex);
		ScopeRings.Empty();
		CounterRings.Empty();
		GPURings.Empty();
	}

	// ----------------------------------------------------------------------
	// File-local helpers: translate a closed EProfilerRing window into the
	// public stat structs. The Scope and Counter stats share the same memory
	// layout but use different field names, so they need two thin fillers.
	// ----------------------------------------------------------------------
	static void FillScopeStatFromRing(const EProfilerRing& InRing, EProfileScopeStat& OutStat)
	{
		const DOUBLE Avg = (InRing.WindowCount > 0u)
			? (InRing.WindowSum / static_cast<DOUBLE>(InRing.WindowCount))
			: 0.0;
		OutStat.AverageSeconds = Avg;
		OutStat.LastSeconds    = InRing.LastValue;
		OutStat.MinSeconds     = InRing.MinValue;
		OutStat.MaxSeconds     = InRing.MaxValue;
		OutStat.SampleCount    = InRing.TotalSamples;
	}
	static void FillCounterStatFromRing(const EProfilerRing& InRing, EProfileCounterStat& OutStat)
	{
		const DOUBLE Avg = (InRing.WindowCount > 0u)
			? (InRing.WindowSum / static_cast<DOUBLE>(InRing.WindowCount))
			: 0.0;
		OutStat.AverageValue = Avg;
		OutStat.LastValue    = InRing.LastValue;
		OutStat.MinValue     = InRing.MinValue;
		OutStat.MaxValue     = InRing.MaxValue;
		OutStat.SampleCount  = InRing.TotalSamples;
	}

	// ----------------------------------------------------------------------
	// Internal mutator: slides one sample into the named ring under the
	// mutex. Buffer capacity = EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM.
	// Eviction subtracts the outgoing sample from WindowSum so GetXxxStat
	// stays O(1) for the average; min/max recomputes over the window since
	// the window is tiny by default (4).
	// ----------------------------------------------------------------------
	void EProfiler::RecordSampleInternal(TMap<EName, EProfilerRing>& InOutMap, const EName& InName, DOUBLE InValue)
	{
		if (InName.IsNone()) { return; }
		EScopedLock Lock(StatsMutex);

		EProfilerRing* RingPtr = InOutMap.FindValueAsPtr(InName);
		if (!RingPtr)
		{
			InOutMap.Add(InName, EProfilerRing());
			RingPtr = InOutMap.FindValueAsPtr(InName);
		}
		if (!RingPtr) { return; }

		EProfilerRing& Ring = *RingPtr;
		const UINT32 Capacity = EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM;

		if (Ring.WindowCount >= Capacity)
		{
			Ring.WindowSum -= Ring.Samples[Ring.Head];
		}
		Ring.Samples[Ring.Head] = InValue;
		Ring.WindowSum += InValue;

		Ring.Head = (Ring.Head + 1u) % Capacity;
		if (Ring.WindowCount < Capacity)
		{
			Ring.WindowCount += 1u;
		}

		Ring.LastValue = InValue;
		Ring.TotalSamples += 1u;

		// Window min/max over the live samples only. Capacity is tiny (4 by
		// default), so the linear sweep is cheap; tracking per-eviction would
		// require a deque and isn't worth the complexity.
		Ring.MinValue = Ring.Samples[0];
		Ring.MaxValue = Ring.Samples[0];
		for (UINT32 i = 1u; i < Ring.WindowCount; i++)
		{
			const DOUBLE V = Ring.Samples[i];
			if (V < Ring.MinValue) { Ring.MinValue = V; }
			if (V > Ring.MaxValue) { Ring.MaxValue = V; }
		}
	}

	BOOL8 EProfiler::GetRingStatsInternal(const TMap<EName, EProfilerRing>& InMap, const EName& InName, EProfileScopeStat& OutStat)const
	{
		if (InName.IsNone()) { return FALSE; }
		EScopedLock Lock(StatsMutex);

		const EProfilerRing* RingPtr = InMap.FindValueAsPtr(InName);
		if (!RingPtr || RingPtr->WindowCount == 0u) { return FALSE; }
		FillScopeStatFromRing(*RingPtr, OutStat);
		return TRUE;
	}

	void EProfiler::SnapshotRingsInternal(const TMap<EName, EProfilerRing>& InMap, TArray<TPair<EName, EProfileScopeStat>>& OutSnapshot)const
	{
		EScopedLock Lock(StatsMutex);
		for (auto It = InMap.Begin(); It != InMap.End(); ++It)
		{
			EProfileScopeStat Stat;
			FillScopeStatFromRing(It->second, Stat);
			OutSnapshot.Add(TPair<EName, EProfileScopeStat>(It->first, Stat));
		}
	}

	// ----------------------------------------------------------------------
	// CPU public API
	// ----------------------------------------------------------------------
	void EProfiler::RecordScopeSample(const EName& InName, DOUBLE InSeconds)
	{
		RecordSampleInternal(ScopeRings, InName, InSeconds);
	}
	void EProfiler::RecordCounterSample(const EName& InName, DOUBLE InValue)
	{
		RecordSampleInternal(CounterRings, InName, InValue);
	}
	BOOL8 EProfiler::GetScopeStat(const EName& InName, EProfileScopeStat& OutStat)const
	{
		return GetRingStatsInternal(ScopeRings, InName, OutStat);
	}
	BOOL8 EProfiler::GetCounterStat(const EName& InName, EProfileCounterStat& OutStat)const
	{
		if (InName.IsNone()) { return FALSE; }
		EScopedLock Lock(StatsMutex);

		const EProfilerRing* RingPtr = CounterRings.FindValueAsPtr(InName);
		if (!RingPtr || RingPtr->WindowCount == 0u) { return FALSE; }
		FillCounterStatFromRing(*RingPtr, OutStat);
		return TRUE;
	}
	void EProfiler::SnapshotScopes(TArray<TPair<EName, EProfileScopeStat>>& OutSnapshot)const
	{
		SnapshotRingsInternal(ScopeRings, OutSnapshot);
	}
	void EProfiler::SnapshotCounters(TArray<TPair<EName, EProfileCounterStat>>& OutSnapshot)const
	{
		EScopedLock Lock(StatsMutex);
		for (auto It = CounterRings.Begin(); It != CounterRings.End(); ++It)
		{
			EProfileCounterStat Stat;
			FillCounterStatFromRing(It->second, Stat);
			OutSnapshot.Add(TPair<EName, EProfileCounterStat>(It->first, Stat));
		}
	}

	// ----------------------------------------------------------------------
	// GPU public API. Resolved timings flow in via RecordGPUSample once the
	// active RHI backend reads back its timestamp heap each frame (Phase
	// P.c / P.d). Query surface mirrors the CPU one so ImGui can render all
	// three lists with the same row template.
	// ----------------------------------------------------------------------
	void EProfiler::RecordGPUSample(const EName& InName, DOUBLE InSeconds)
	{
		RecordSampleInternal(GPURings, InName, InSeconds);
	}
	BOOL8 EProfiler::GetGPUStat(const EName& InName, EProfileScopeStat& OutStat)const
	{
		return GetRingStatsInternal(GPURings, InName, OutStat);
	}
	void EProfiler::SnapshotGPUScopes(TArray<TPair<EName, EProfileScopeStat>>& OutSnapshot)const
	{
		SnapshotRingsInternal(GPURings, OutSnapshot);
	}

#if _GPU_ENABLE_PROFILER

	// ----------------------------------------------------------------------
	// GPU event scope: opens / closes a debug marker bracket on the command
	// list. The implementation sits in this TU so Profiler.h can keep
	// IRCommandList.h out of CPU-only call sites.
	// ----------------------------------------------------------------------
	EGPUEventScope::EGPUEventScope(IRCommandList* InCmd, const EString& InName)noexcept
		: Cmd(InCmd)
	{
		if (Cmd) { Cmd->BeginGPUEvent(InName); }
	}
	EGPUEventScope::~EGPUEventScope()
	{
		if (Cmd) { Cmd->EndGPUEvent(); }
	}

#endif

};
