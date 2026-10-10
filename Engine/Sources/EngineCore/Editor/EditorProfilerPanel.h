#pragma once

#include <Base/DataStructure/ObjectBase.h>
#include <Development/Profiler.h>

namespace PigeonEngine
{
#if _EDITOR_ONLY

	// ImGui panel that snapshots EProfiler's three rings (CPU scope, CPU
	// counter, GPU scope) and renders them as sortable tables. Lives next
	// to EEditorGlobalPanel; EditorManager drives the per-frame Draw call.
	//
	// When _CPU_ENABLE_PROFILER / _GPU_ENABLE_PROFILER are off the
	// corresponding Snapshot APIs return empty arrays, and the matching tab
	// shows a disabled-state placeholder instead of a table.
	class EEditorProfilerPanel : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EEditorProfilerPanel)
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
		void			DrawPanel();
	private:
		BOOL32	bWindowOpen			= TRUE;
	};

#endif
};
