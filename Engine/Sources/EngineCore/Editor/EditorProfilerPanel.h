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

	// ImGui panel over EProfiler and EGPUProfiler. Two tabs: the four CPU thread
	// brackets, and the GPU frame total followed by one row per pass.
	// EditorManager drives the per-frame Draw call.
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
	private:
		BOOL32						bWindowOpen		= TRUE;
		BOOL32						bPaused			= FALSE;
		BOOL32						bRowsValid		= FALSE;
		TArray<EProfilerTableRow>	CpuRows;
		TArray<EProfilerTableRow>	GpuRows;
	};

#endif
};
