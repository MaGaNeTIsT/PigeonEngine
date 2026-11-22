#pragma once

#include <Base/DataStructure/ObjectBase.h>
#include <Development/Profiler.h>

namespace PigeonEngine
{
#if ENGINE_EDITOR_ONLY

	class PWorld;
	class PActor;

	// One row of either profiler table. A plain value on purpose: a snapshot can
	// be cached while the panel is paused, so the tables never reach back into
	// the profiler on the frames they draw from a frozen copy.
	struct EProfilerTableRow
	{
		EName				Name;
		EProfileScopeStat	Stat;
		// Share of the GPU frame total. bHasShare is FALSE for the frame row
		// itself, and for every pass when no frame total was recorded.
		DOUBLE				ShareRatio	= 0.0;
		BOOL8				bHasShare	= FALSE;
		// Per-frame aggregate of a CPU scope. Stat.AverageSeconds is per call, and a scope that
		// runs once per character has as many calls as there are characters. bHasPerFrame is
		// FALSE for a concurrent scope: worker rows overlap in time, so summing their samples
		// would count the same wall clock several times.
		DOUBLE				CallsPerFrame	= 0.0;
		DOUBLE				PerFrameSeconds	= 0.0;
		BOOL8				bIsConcurrent	= FALSE;
		BOOL8				bHasPerFrame	= FALSE;
	};

	// One node of the CPU scope tree. The tree comes from the scope names themselves:
	// "RenderThread.Record" is a child of "RenderThread", and a deeper name such as
	// "RenderThread.Record.ShadowViews" nests one more level. That convention already
	// carries the grouping, so a trace does not have to declare a parent.
	struct EProfilerTreeNode
	{
		EProfilerTableRow	Row;
		// FALSE for a synthetic node: a name that only exists as a prefix of another scope.
		BOOL8				bHasRow		= FALSE;
		TArray<INT32>		Children;
	};

	// ImGui panel over EProfiler and EGPUProfiler. Three tabs: the CPU thread brackets and their
	// sub-brackets, the GPU frame total followed by one row per pass, and the per-frame counters
	// (draw calls in particular). EditorManager drives the per-frame Draw call.
	//
	// Pause freezes what the panel reads, not what the engine records: the rows
	// are rebuilt from a fresh snapshot when the pause is released. Clear All
	// drops the traces and the cached rows together, so a cleared panel cannot
	// show a table built from samples that no longer exist.
	class EEditorProfilerPanel : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EEditorProfilerPanel)
	public:
		virtual void	Initialize() override;
		virtual void	ShutDown() override;
		// Runs the benchmark state machine every frame, whether the window is visible.
		void			Tick();
		// Draws the contents only; the window registry owns Begin/End and the size.
		void			DrawContents();
	private:
		void			RefreshRows();
		void			BuildCpuTree();
		void			DrawPerformanceBenchmarkPanel();
		void			TickPerformanceBenchmark();
		void			StartPerformanceBenchmark();
		void			StopPerformanceBenchmark();
	private:
		enum class EBenchmarkStateType : UINT8
		{
			IDLE,
			BUILDING,
			WARMUP,
			RUNNING,
			DONE,
			FAILED,
		};
	private:
		BOOL32						bPaused			= FALSE;
		BOOL32						bRowsValid		= FALSE;
		TArray<EProfilerTableRow>	CpuRows;
		TArray<EProfilerTreeNode>	CpuTree;
		// Roots in thread order: the four thread brackets first, then any scope whose name
		// does not start with a thread bracket. Kept separately so collapsing a thread does
		// not change the order the other threads are drawn in.
		TArray<INT32>				CpuRootNodes;
		TArray<EProfilerTableRow>	GpuRows;
		TArray<TPair<EName, EProfileCounterStat>>	CounterRows;

		// Performance benchmark. A disposable world keeps the measurement out of the editor
		// world; actors are built in batches so the panel itself does not freeze.
		EBenchmarkStateType		BenchmarkState = EBenchmarkStateType::IDLE;
		BOOL32						bPerformanceBenchmarkAutoStart = FALSE;
		INT32						BenchmarkActorCount = 5000;
		INT32						BenchmarkFrameCount = 120;
		INT32						BenchmarkMovePercent = 100;
		PWorld*						BenchmarkWorld = nullptr;
		TArray<PActor*>				BenchmarkActors;
		UINT32						BenchmarkBuildCursor = 0u;
		UINT32						BenchmarkWarmupFrames = 10u;
		UINT32						BenchmarkRunFrames = 0u;
		DOUBLE						BenchmarkSumMilliseconds = 0.0;
		DOUBLE						BenchmarkMoveSumMilliseconds = 0.0;
		DOUBLE						BenchmarkTickSumMilliseconds = 0.0;
		DOUBLE						BenchmarkRenderTreeSumMilliseconds = 0.0;
		DOUBLE						BenchmarkGameplaySumMilliseconds = 0.0;
		DOUBLE						BenchmarkMinMilliseconds = 0.0;
		DOUBLE						BenchmarkMaxMilliseconds = 0.0;
		TArray<FLOAT>				BenchmarkSamplesMilliseconds;
		DOUBLE						BenchmarkLastAverageMilliseconds = 0.0;
		DOUBLE						BenchmarkLastMoveAverageMilliseconds = 0.0;
		DOUBLE						BenchmarkLastTickAverageMilliseconds = 0.0;
		DOUBLE						BenchmarkLastRenderTreeAverageMilliseconds = 0.0;
		DOUBLE						BenchmarkLastGameplayAverageMilliseconds = 0.0;
		DOUBLE						BenchmarkLastP95Milliseconds = 0.0;
		DOUBLE						BenchmarkLastMinMilliseconds = 0.0;
		DOUBLE						BenchmarkLastMaxMilliseconds = 0.0;
	};

#endif
};
