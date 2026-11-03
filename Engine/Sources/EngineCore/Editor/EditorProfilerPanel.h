#pragma once

#include <Base/DataStructure/ObjectBase.h>
#include <Development/Profiler.h>

namespace PigeonEngine
{
#if _EDITOR_ONLY

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
		void			DrawPanel();
	private:
		void			RefreshRows();
		void			BuildCpuTree();
	private:
		BOOL32						bWindowOpen		= TRUE;
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
	};

#endif
};
