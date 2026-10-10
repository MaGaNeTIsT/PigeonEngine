#include "EditorProfilerPanel.h"

#if _EDITOR_ONLY

#include <Base/RTTI/RTTIManager.h>
#include <imgui.h>
#include <Base/DataStructure/Text/Name.h>

namespace PigeonEngine
{
	static void RegisterEditorProfilerPanelClassTypes()
	{
		RegisterClassType<EEditorProfilerPanel, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterEditorProfilerPanelClassTypes);

	EEditorProfilerPanel::EEditorProfilerPanel()
	{
	}
	EEditorProfilerPanel::~EEditorProfilerPanel()
	{
	}
	void EEditorProfilerPanel::Initialize()
	{
		bWindowOpen = TRUE;
	}
	void EEditorProfilerPanel::ShutDown()
	{
	}

	// -----------------------------------------------------------------------
	// Renders one of the three trace tables. The two stat structs (Scope /
	// Counter) share layout but have different field semantics - the
	// `bIsTimeSeries` flag tells the formatter whether to print
	// milliseconds (scope timings) or raw values (counters).
	// -----------------------------------------------------------------------
	template<typename TStatType>
	static void DrawProfilerRow(const EName& InTraceName, const TStatType& InStat, BOOL8 bIsTimeSeries);

	template<>
	void DrawProfilerRow<EProfileScopeStat>(const EName& InTraceName, const EProfileScopeStat& InStat, BOOL8 /*bIsTimeSeries*/)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(InTraceName.GetData());
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%.3f", InStat.AverageSeconds * 1000.0);
		ImGui::TableSetColumnIndex(2);
		ImGui::Text("%.3f", InStat.LastSeconds    * 1000.0);
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("%.3f", InStat.MinSeconds     * 1000.0);
		ImGui::TableSetColumnIndex(4);
		ImGui::Text("%.3f", InStat.MaxSeconds     * 1000.0);
		ImGui::TableSetColumnIndex(5);
		ImGui::Text("%llu", static_cast<unsigned long long>(InStat.SampleCount));
	}
	template<>
	void DrawProfilerRow<EProfileCounterStat>(const EName& InTraceName, const EProfileCounterStat& InStat, BOOL8 /*bIsTimeSeries*/)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(InTraceName.GetData());
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%.3f", InStat.AverageValue);
		ImGui::TableSetColumnIndex(2);
		ImGui::Text("%.3f", InStat.LastValue);
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("%.3f", InStat.MinValue);
		ImGui::TableSetColumnIndex(4);
		ImGui::Text("%.3f", InStat.MaxValue);
		ImGui::TableSetColumnIndex(5);
		ImGui::Text("%llu", static_cast<unsigned long long>(InStat.SampleCount));
	}

	static void DrawScopeTable(const CHAR* InTableId, const TArray<TPair<EName, EProfileScopeStat>>& InSnapshot, const CHAR* InValueHeader)
	{
		if (InSnapshot.Num() == 0)
		{
			ImGui::TextDisabled("(no samples yet)");
			return;
		}
		const ImGuiTableFlags Flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuter
			| ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable(InTableId, 6, Flags))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
			ImGui::TableSetupColumn(InValueHeader, ImGuiTableColumnFlags_WidthStretch, 1.2f);
			ImGui::TableSetupColumn("Last",       ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Min",        ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Max",        ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Samples",    ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableHeadersRow();
			const INT32 RowNum = InSnapshot.Num<INT32>();
			for (INT32 i = 0; i < RowNum; i++)
			{
				const TPair<EName, EProfileScopeStat>& Row = InSnapshot[i];
				DrawProfilerRow<EProfileScopeStat>(Row.first, Row.second, TRUE);
			}
			ImGui::EndTable();
		}
	}

	static void DrawCounterTable(const CHAR* InTableId, const TArray<TPair<EName, EProfileCounterStat>>& InSnapshot)
	{
		if (InSnapshot.Num() == 0)
		{
			ImGui::TextDisabled("(no samples yet)");
			return;
		}
		const ImGuiTableFlags Flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuter
			| ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable(InTableId, 6, Flags))
		{
			ImGui::TableSetupColumn("Name",   ImGuiTableColumnFlags_WidthStretch, 3.0f);
			ImGui::TableSetupColumn("Avg",    ImGuiTableColumnFlags_WidthStretch, 1.2f);
			ImGui::TableSetupColumn("Last",   ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Min",    ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Max",    ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Samples",ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableHeadersRow();
			const INT32 RowNum = InSnapshot.Num<INT32>();
			for (INT32 i = 0; i < RowNum; i++)
			{
				const TPair<EName, EProfileCounterStat>& Row = InSnapshot[i];
				DrawProfilerRow<EProfileCounterStat>(Row.first, Row.second, FALSE);
			}
			ImGui::EndTable();
		}
	}

	void EEditorProfilerPanel::DrawPanel()
	{
		bool bOpen = (bWindowOpen != FALSE);
		if (!ImGui::Begin("Profiler", &bOpen))
		{
			bWindowOpen = bOpen ? TRUE : FALSE;
			ImGui::End();
			return;
		}
		bWindowOpen = bOpen ? TRUE : FALSE;

#if !_ENABLE_PROFILER
		ImGui::TextDisabled("Profiler disabled (_ENABLE_PROFILER=0).");
		ImGui::TextDisabled("Rebuild with PIGEON_ENABLE_PROFILER=ON to capture samples.");
		ImGui::End();
		return;
#else
		EProfiler* Profiler = EProfiler::GetManagerSingleton();

		if (ImGui::Button("Clear All"))
		{
			Profiler->ClearTraces();
		}

		if (ImGui::BeginTabBar("ProfilerTabs"))
		{
			if (ImGui::BeginTabItem("CPU Scope"))
			{
#if _CPU_ENABLE_PROFILER
				TArray<TPair<EName, EProfileScopeStat>> Snapshot;
				Profiler->SnapshotScopes(Snapshot);
				DrawScopeTable("##cpu_scope", Snapshot, "Avg(ms)");
#else
				ImGui::TextDisabled("CPU profiler disabled.");
#endif
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("CPU Counter"))
			{
#if _CPU_ENABLE_PROFILER
				TArray<TPair<EName, EProfileCounterStat>> Snapshot;
				Profiler->SnapshotCounters(Snapshot);
				DrawCounterTable("##cpu_counter", Snapshot);
#else
				ImGui::TextDisabled("CPU profiler disabled.");
#endif
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("GPU Scope"))
			{
#if _GPU_ENABLE_PROFILER
				TArray<TPair<EName, EProfileScopeStat>> Snapshot;
				Profiler->SnapshotGPUScopes(Snapshot);
				DrawScopeTable("##gpu_scope", Snapshot, "Avg(ms)");
#else
				ImGui::TextDisabled("GPU profiler disabled.");
#endif
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
#endif
	}
}

#endif
