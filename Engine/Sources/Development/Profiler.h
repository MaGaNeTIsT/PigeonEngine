#pragma once

#include <CoreMinimal.h>
#include <Config/EngineConfig.h>
#include <Base/Timer/Timer.h>
#include <Base/DataStructure/Text/Name.h>
#include <MultiThread/Mutex.h>

// ---------------------------------------------------------------------------
// Profiler enable switches
//
// Three layers of compile-time on/off:
//   _ENABLE_PROFILER        - master switch. Defined by CMake via the
//                             PIGEON_ENABLE_PROFILER option (defaults ON for
//                             Debug / DebugEditor / Develop / DevelopEditor;
//                             always forced 0 for Shipping). If no CMake
//                             define reaches the translation unit the
//                             fallback here keeps it OFF - never silently
//                             turn it on without the build system's say.
//   _CPU_ENABLE_PROFILER    - CPU-side scope timer + counter macros.
//   _GPU_ENABLE_PROFILER    - GPU timestamp queries + draw-event macros.
//
// When a layer is off the matching PE_PROFILE_* / PE_GPU_EVENT_* macros
// expand to literally nothing - the argument expression is not parsed for
// side-effects and emits zero code.
// ---------------------------------------------------------------------------
#ifndef _ENABLE_PROFILER
#define _ENABLE_PROFILER 0
#endif
#ifndef _CPU_ENABLE_PROFILER
#define _CPU_ENABLE_PROFILER (_ENABLE_PROFILER)
#endif
#ifndef _GPU_ENABLE_PROFILER
#define _GPU_ENABLE_PROFILER (_ENABLE_PROFILER)
#endif

#define PE_PROFILER_CONCAT_INNER(A, B)  A##B
#define PE_PROFILER_CONCAT(A, B)        PE_PROFILER_CONCAT_INNER(A, B)

namespace PigeonEngine
{

	class IRCommandList;

	// -----------------------------------------------------------------------
	// Trace names shared by the instrumentation sites and the Profiler panel.
	// Both ends spell the name through these constants, so a rename cannot
	// leave the panel looking for a trace nothing records.
	//
	// The thread scopes are whole-thread brackets rather than subsystem
	// timers: each frames one pass of its thread's main loop. Physics and the
	// asset loads currently run on the game thread, so their brackets nest
	// inside GameThread; when either moves to its own thread the bracket keeps
	// reporting the same way without a call-site change.
	// -----------------------------------------------------------------------

	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_GAME_THREAD		= PE_TEXT("GameThread");
	// The part of GameThread spent waiting for the previous render frame. A child by name, so
	// the panel nests it under GameThread without a second grouping declaration.
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_GAME_WAIT_RENDER	= PE_TEXT("GameThread.WaitRender");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_THREAD	= PE_TEXT("RenderThread");
	// Render-thread work that runs before the RenderThread bracket opens: the resource
	// commands the game thread queued. The game thread waits for it through RenderDoneEvent,
	// so without this row its cost shows up as GameThread wait and is attributed to nobody.
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_RESOURCE_COMMANDS	= PE_TEXT("RenderThread.ResourceCommands");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_PHYSICS_THREAD	= PE_TEXT("PhysicsThread");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_LOAD_THREAD		= PE_TEXT("LoadThread");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_GPU_FRAME		= PE_TEXT("GPU Frame");

	// Render-thread sub-brackets. The whole-thread row above says how long the frame took;
	// these say where it went. They are separate rows rather than nested children because the
	// profiler records each scope as an independent trace.
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_BEGIN_FRAME	= PE_TEXT("RenderThread.BeginFrame");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_INIT_FRAME	= PE_TEXT("RenderThread.InitFrame");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_RECORD		= PE_TEXT("RenderThread.Record");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_AFTER			= PE_TEXT("RenderThread.AfterRender");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_PRESENT		= PE_TEXT("RenderThread.Present");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_SETUP			= PE_TEXT("RenderThread.Setup");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_SHADOWS		= PE_TEXT("RenderThread.ShadowViews");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_BASE_PASS		= PE_TEXT("RenderThread.BasePass");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_FINAL_OUTPUT	= PE_TEXT("RenderThread.FinalOutput");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_IMGUI			= PE_TEXT("RenderThread.ImGui");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_SUBMIT		= PE_TEXT("RenderThread.Submit");
	// One recorder thread's share of a stage. All workers share the name, so the panel shows
	// the aggregate of the pool; the per-recorder split is visible in the task counters.
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_SCOPE_RENDER_RECORD_WORKER	= PE_TEXT("RenderThread.Record.Worker");

	// Per-frame counters. Counts rather than timings: the first question about a slow render
	// thread is how much work it was asked to record, and a shadow pass multiplies that number
	// by its view count.
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_COUNTER_BASE_DRAW_CALLS	= PE_TEXT("BaseDrawCalls");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_COUNTER_SHADOW_DRAW_CALLS	= PE_TEXT("ShadowDrawCalls");
	PE_CONSTEXPR_STATIC const CHAR*	PROFILER_COUNTER_SHADOW_VIEW_NUM	= PE_TEXT("ShadowViewNum");

	// -----------------------------------------------------------------------
	// Snapshot result types returned by the EProfiler query API
	// -----------------------------------------------------------------------

	struct EProfileScopeStat
	{
		DOUBLE  AverageSeconds  = 0.0;
		DOUBLE  LastSeconds     = 0.0;
		DOUBLE  MinSeconds      = 0.0;
		DOUBLE  MaxSeconds      = 0.0;
		UINT64  SampleCount     = 0u;
	};

	struct EProfileCounterStat
	{
		DOUBLE  AverageValue    = 0.0;
		DOUBLE  LastValue       = 0.0;
		DOUBLE  MinValue        = 0.0;
		DOUBLE  MaxValue        = 0.0;
		UINT64  SampleCount     = 0u;
	};

	// -----------------------------------------------------------------------
	// Internal ring buffer: rolling window of the last N samples per trace,
	// plus running total / min / max for O(1) Get* queries. N comes from
	// EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM.
	// -----------------------------------------------------------------------
	struct EProfilerRing
	{
		DOUBLE  Samples[EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM] = {};
		UINT32  Head            = 0u;     // next write slot in Samples
		UINT32  WindowCount     = 0u;     // valid samples in Samples (<= N)
		UINT64  TotalSamples    = 0u;     // lifetime counter
		DOUBLE  WindowSum       = 0.0;    // sum of Samples[0..WindowCount-1]
		DOUBLE  LastValue       = 0.0;
		DOUBLE  MinValue        = 0.0;
		DOUBLE  MaxValue        = 0.0;
	};

	// -----------------------------------------------------------------------
	// EProfiler - singleton manager for CPU scope timers, CPU counters and
	// (future) resolved GPU scope timings.
	//
	// Trace identity is EName: the PE_PROFILE_* macros declare a `static
	// EName` at each call site, so the engine-wide string pool is hit only
	// once per call site at first execution. All subsequent lookups are
	// SIZE_T index compares - std::map<EName, ...> ordering also reduces to
	// integer compare.
	//
	// Thread-safety: every public mutator and reader serialises through
	// StatsMutex. The snapshot APIs copy out under the lock so ImGui / log
	// dumps don't hold the mutex while iterating.
	// -----------------------------------------------------------------------
	class EProfiler : public EManagerBase
	{
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
		void			ClearTraces();

	public:
		// CPU
		void			RecordScopeSample(const EName& InName, DOUBLE InSeconds);
		void			RecordCounterSample(const EName& InName, DOUBLE InValue);

		PE_NODISCARD BOOL8 GetScopeStat(const EName& InName, EProfileScopeStat& OutStat)const;
		PE_NODISCARD BOOL8 GetCounterStat(const EName& InName, EProfileCounterStat& OutStat)const;

		void			SnapshotScopes(TArray<TPair<EName, EProfileScopeStat>>& OutSnapshot)const;
		void			SnapshotCounters(TArray<TPair<EName, EProfileCounterStat>>& OutSnapshot)const;

	public:
		// GPU (P.c onward will fill these; declared now so callers can target
		// a single query surface regardless of which backend resolves the
		// timestamps).
		void			RecordGPUSample(const EName& InName, DOUBLE InSeconds);
		PE_NODISCARD BOOL8 GetGPUStat(const EName& InName, EProfileScopeStat& OutStat)const;
		void			SnapshotGPUScopes(TArray<TPair<EName, EProfileScopeStat>>& OutSnapshot)const;

	private:
		void			RecordSampleInternal(TMap<EName, EProfilerRing>& InOutMap, const EName& InName, DOUBLE InValue);
		BOOL8			GetRingStatsInternal(const TMap<EName, EProfilerRing>& InMap, const EName& InName, EProfileScopeStat& OutStat)const;
		void			SnapshotRingsInternal(const TMap<EName, EProfilerRing>& InMap, TArray<TPair<EName, EProfileScopeStat>>& OutSnapshot)const;

	private:
		mutable EMutex				StatsMutex;
		TMap<EName, EProfilerRing>	ScopeRings;
		TMap<EName, EProfilerRing>	CounterRings;
		TMap<EName, EProfilerRing>	GPURings;

		CLASS_MANAGER_SINGLETON_BODY(EProfiler)

	};

	// -----------------------------------------------------------------------
	// CPU scope macros
	//
	// PE_PROFILE_SCOPE emits two declarations into the surrounding scope:
	//   (1) a function-local `static EName` initialised once on first hit
	//   (2) a stack EProfileScopeGuard that lives until the scope ends
	//
	// Because PE_PROFILE_SCOPE is NOT wrapped in `do { } while(0)`, putting
	// it inside a brace-less `if` / `for` body would only bind the static
	// declaration to that body and let the guard escape - always use it
	// inside a proper `{}` block.
	// -----------------------------------------------------------------------
#if _CPU_ENABLE_PROFILER

	class EProfileScopeGuard final
	{
	public:
		EProfileScopeGuard(const EName& InName)noexcept
			: Name(InName), StartTicks(EBaseTimer::GetClockTicks()) {}
		~EProfileScopeGuard()
		{
			const UINT64 NowTicks = EBaseTimer::GetClockTicks();
			const DOUBLE Elapsed  = EBaseTimer::TicksToSeconds(NowTicks - StartTicks);
			EProfiler::GetManagerSingleton()->RecordScopeSample(Name, Elapsed);
		}
	public:
		EProfileScopeGuard(const EProfileScopeGuard&) = delete;
		EProfileScopeGuard& operator=(const EProfileScopeGuard&) = delete;
	private:
		EName	Name;
		UINT64	StartTicks;
	};

	#define PE_PROFILE_SCOPE(InName) \
		static ::PigeonEngine::EName PE_PROFILER_CONCAT(_PEProfileTag_, __LINE__){(InName)}; \
		::PigeonEngine::EProfileScopeGuard PE_PROFILER_CONCAT(_PEProfileGuard_, __LINE__)(PE_PROFILER_CONCAT(_PEProfileTag_, __LINE__))

	#define PE_PROFILE_COUNTER(InName, InValue) \
		do { \
			static ::PigeonEngine::EName PE_PROFILER_CONCAT(_PEProfileCnt_, __LINE__){(InName)}; \
			::PigeonEngine::EProfiler::GetManagerSingleton()->RecordCounterSample( \
				PE_PROFILER_CONCAT(_PEProfileCnt_, __LINE__), static_cast<DOUBLE>(InValue)); \
		} while(0)

#else

	#define PE_PROFILE_SCOPE(InName)
	#define PE_PROFILE_COUNTER(InName, InValue)

#endif

	// -----------------------------------------------------------------------
	// GPU draw-grain event scope
	//
	// Wraps IRCommandList::BeginGPUEvent / EndGPUEvent so render code can
	// name a Draw / Dispatch block once and have it show up in PIX /
	// RenderDoc / Nsight as a contiguous scope. The implementation lives in
	// Profiler.cpp so this header doesn't drag IRCommandList.h into every
	// CPU-only translation unit that uses PE_PROFILE_SCOPE.
	// -----------------------------------------------------------------------
#if _GPU_ENABLE_PROFILER

	class EGPUEventScope final
	{
	public:
		// InEnabled lets a draw decide at run time whether its own marker is emitted. Pass level
		// markers always pass TRUE; a per draw marker is the one an editor can turn off, because
		// a shadow pass emits one per caster per view.
		//
		// The ctor / dtor are forced inline so the disabled case costs a pointer and a bool
		// compare at the call site: a draw that does not emit a marker must not pay two
		// out-of-line calls for it. The command list calls stay in Profiler.cpp, which is the
		// only place that includes IRCommandList.h.
		PE_FORCEINLINE EGPUEventScope(IRCommandList* InCmd, const EString& InName, const BOOL32 InEnabled = TRUE) noexcept
			: Cmd(InCmd), bEnabled(InEnabled)
		{
			if ((Cmd) && (bEnabled != FALSE))
			{
				BeginEvent(InName);
			}
		}
		PE_FORCEINLINE ~EGPUEventScope() noexcept
		{
			if ((Cmd) && (bEnabled != FALSE))
			{
				EndEvent();
			}
		}
	public:
		EGPUEventScope(const EGPUEventScope&) = delete;
		EGPUEventScope& operator=(const EGPUEventScope&) = delete;
	private:
		void	BeginEvent(const EString& InName) noexcept;
		void	EndEvent() noexcept;
	private:
		IRCommandList* Cmd;
		BOOL32 bEnabled;
	};

	#define PE_GPU_EVENT_SCOPE(InCmd, InName) \
		::PigeonEngine::EGPUEventScope PE_PROFILER_CONCAT(_PEGPUEvent_, __LINE__)((InCmd), (InName))
	#define PE_GPU_EVENT_SCOPE_IF(InCmd, InName, InEnabled) \
		::PigeonEngine::EGPUEventScope PE_PROFILER_CONCAT(_PEGPUEvent_, __LINE__)((InCmd), (InName), (InEnabled))

#else

	#define PE_GPU_EVENT_SCOPE(InCmd, InName)
	#define PE_GPU_EVENT_SCOPE_IF(InCmd, InName, InEnabled)

#endif

};
