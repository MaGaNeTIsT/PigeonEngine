#include "EditorProfilerPanel.h"

#if _EDITOR_ONLY

#include <Base/RTTI/RTTIManager.h>
#include <imgui.h>
#include <Base/DataStructure/Text/Name.h>
#include <Development/GPUProfiler.h>

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
		bWindowOpen	= TRUE;
		bPaused		= FALSE;
		bRowsValid	= FALSE;
		CpuRows.Empty();
		GpuRows.Empty();
	}
	void EEditorProfilerPanel::ShutDown()
	{
		CpuRows.Empty();
		GpuRows.Empty();
		bRowsValid = FALSE;
	}

	// -----------------------------------------------------------------------
	// Table ordering
	//
	// ImGui only reports which column the user picked and in which direction;
	// sorting the rows is the caller's job. The row counts here are single
	// digits and tens, so an insertion sort keeps the code obvious and the
	// order stable for equal values.
	// -----------------------------------------------------------------------

#if _ENABLE_PROFILER

	static INT32 CompareProfilerText(const CHAR* InLeft, const CHAR* InRight)
	{
		if ((!InLeft) || (!InRight))
		{
			return 0;
		}
		while ((*InLeft != static_cast<CHAR>(0)) && (*InLeft == *InRight))
		{
			InLeft++;
			InRight++;
		}
		return (static_cast<INT32>(static_cast<UINT8>(*InLeft)) - static_cast<INT32>(static_cast<UINT8>(*InRight)));
	}

	static DOUBLE GetProfilerSortValue(const EProfilerTableRow& InRow, INT32 InColumn, BOOL8 bShareColumn)
	{
		switch (InColumn)
		{
		case 1:
			return InRow.Stat.AverageSeconds;
		case 2:
			return InRow.Stat.LastSeconds;
		case 3:
			return InRow.Stat.MinSeconds;
		case 4:
			return InRow.Stat.MaxSeconds;
		case 5:
			return ((bShareColumn != FALSE) ? InRow.ShareRatio : static_cast<DOUBLE>(InRow.Stat.SampleCount));
		default:
			return 0.0;
		}
	}

	static INT32 CompareProfilerRows(const EProfilerTableRow& InLeft, const EProfilerTableRow& InRight, INT32 InColumn, BOOL8 bShareColumn)
	{
		if (InColumn == 0)
		{
			return CompareProfilerText(InLeft.Name.GetData(), InRight.Name.GetData());
		}
		const DOUBLE LeftValue = GetProfilerSortValue(InLeft, InColumn, bShareColumn);
		const DOUBLE RightValue = GetProfilerSortValue(InRight, InColumn, bShareColumn);
		if (LeftValue < RightValue)
		{
			return -1;
		}
		if (LeftValue > RightValue)
		{
			return 1;
		}
		return 0;
	}

	static void SortProfilerRows(TArray<EProfilerTableRow>& InOutRows, BOOL8 bShareColumn, ImGuiTableSortSpecs* InSortSpecs)
	{
		if ((!InSortSpecs) || (InSortSpecs->SpecsCount == 0))
		{
			return;
		}
		const ImGuiTableColumnSortSpecs& SortSpec = InSortSpecs->Specs[0];
		const INT32 SortColumn = static_cast<INT32>(SortSpec.ColumnIndex);
		const BOOL8 bAscending = (SortSpec.SortDirection != ImGuiSortDirection_Descending);
		const INT32 RowNum = InOutRows.Num<INT32>();
		for (INT32 RowIndex = 1; RowIndex < RowNum; RowIndex++)
		{
			const EProfilerTableRow KeyRow = InOutRows[RowIndex];
			INT32 InsertIndex = RowIndex - 1;
			while (InsertIndex >= 0)
			{
				const INT32 Order = CompareProfilerRows(KeyRow, InOutRows[InsertIndex], SortColumn, bShareColumn);
				const BOOL8 bMoveEarlier = ((bAscending != FALSE) ? (Order < 0) : (Order > 0));
				if (bMoveEarlier == FALSE)
				{
					break;
				}
				InOutRows[InsertIndex + 1] = InOutRows[InsertIndex];
				InsertIndex -= 1;
			}
			InOutRows[InsertIndex + 1] = KeyRow;
		}
	}

	// -----------------------------------------------------------------------
	// Table drawing
	// -----------------------------------------------------------------------

	static void DrawProfilerRows(const TArray<EProfilerTableRow>& InRows, BOOL8 bShareColumn)
	{
		const INT32 RowNum = InRows.Num<INT32>();
		for (INT32 RowIndex = 0; RowIndex < RowNum; RowIndex++)
		{
			const EProfilerTableRow& Row = InRows[RowIndex];
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(Row.Name.GetData());
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%.3f", Row.Stat.AverageSeconds * 1000.0);
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.3f", Row.Stat.LastSeconds * 1000.0);
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%.3f", Row.Stat.MinSeconds * 1000.0);
			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%.3f", Row.Stat.MaxSeconds * 1000.0);
			ImGui::TableSetColumnIndex(5);
			if (bShareColumn == FALSE)
			{
				ImGui::Text("%llu", static_cast<unsigned long long>(Row.Stat.SampleCount));
			}
			else if (Row.bHasShare != FALSE)
			{
				ImGui::Text("%.1f%%", Row.ShareRatio * 100.0);
			}
			else
			{
				ImGui::TextUnformatted("-");
			}
		}
	}

	static void DrawProfilerTable(const CHAR* InTableId, TArray<EProfilerTableRow>& InOutRows, BOOL8 bShareColumn)
	{
		if (InOutRows.Num() == 0)
		{
			ImGui::TextDisabled("(no samples yet)");
			return;
		}

		const ImGuiTableFlags TableFlags = ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg
			| ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable(InTableId, 6, TableFlags))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
			if (bShareColumn == FALSE)
			{
				// No DefaultSort: the CPU table opens in thread order, which is
				// the order the rows were built in.
				ImGui::TableSetupColumn("Avg(ms)", ImGuiTableColumnFlags_WidthStretch, 1.2f);
			}
			else
			{
				// The GPU table opens on the most expensive pass first; the
				// default spec also gives the sort code something to apply
				// before the user touches a header.
				ImGui::TableSetupColumn("Avg(ms)", ImGuiTableColumnFlags_WidthStretch
					| ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 1.2f);
			}
			ImGui::TableSetupColumn("Last(ms)", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Min(ms)", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Max(ms)", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			if (bShareColumn == FALSE)
			{
				ImGui::TableSetupColumn("Samples", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			}
			else
			{
				ImGui::TableSetupColumn("Share", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			}
			ImGui::TableHeadersRow();

			SortProfilerRows(InOutRows, bShareColumn, ImGui::TableGetSortSpecs());
			DrawProfilerRows(InOutRows, bShareColumn);
			ImGui::EndTable();
		}
	}

	// -----------------------------------------------------------------------
	// Header line
	// -----------------------------------------------------------------------

	static BOOL32 FindProfilerRow(const TArray<EProfilerTableRow>& InRows, const CHAR* InName, EProfileScopeStat& OutStat)
	{
		const EName TargetName(InName);
		const INT32 RowNum = InRows.Num<INT32>();
		for (INT32 RowIndex = 0; RowIndex < RowNum; RowIndex++)
		{
			if (InRows[RowIndex].Name == TargetName)
			{
				OutStat = InRows[RowIndex].Stat;
				return TRUE;
			}
		}
		return FALSE;
	}

	static void DrawHeaderSegment(const CHAR* InLabel, BOOL32 bInHasStat, const EProfileScopeStat& InStat)
	{
		ImGui::SameLine();
		if ((bInHasStat != FALSE) && (InStat.SampleCount > 0u))
		{
			ImGui::Text("| %s %.2f ms", InLabel, InStat.AverageSeconds * 1000.0);
		}
		else
		{
			ImGui::Text("| %s --", InLabel);
		}
	}

#endif	// _ENABLE_PROFILER

	// -----------------------------------------------------------------------
	// Snapshots
	// -----------------------------------------------------------------------

	void EEditorProfilerPanel::RefreshRows()
	{
		CpuRows.Empty();
		GpuRows.Empty();

#if _CPU_ENABLE_PROFILER
		// Fixed order, independent of the rings' own ordering: the four thread
		// brackets always read in the same sequence.
		const CHAR* ThreadScopeNames[4] = { PROFILER_SCOPE_GAME_THREAD, PROFILER_SCOPE_RENDER_THREAD, PROFILER_SCOPE_PHYSICS_THREAD, PROFILER_SCOPE_LOAD_THREAD };
		for (UINT32 Index = 0u; Index < 4u; Index++)
		{
			EProfileScopeStat Stat;
			if (EProfiler::GetManagerSingleton()->GetScopeStat(EName(ThreadScopeNames[Index]), Stat) == FALSE)
			{
				continue;	// no sample yet - the table stays empty for this thread
			}
			EProfilerTableRow Row;
			Row.Name = EName(ThreadScopeNames[Index]);
			Row.Stat = Stat;
			CpuRows.Add(Row);
		}
#endif

#if _GPU_ENABLE_PROFILER
		EProfiler* Profiler = EProfiler::GetManagerSingleton();
		EProfileScopeStat FrameStat;
		const BOOL32 bHasFrameStat = Profiler->GetGPUStat(EName(PROFILER_SCOPE_GPU_FRAME), FrameStat);
		const DOUBLE FrameAverage = ((bHasFrameStat != FALSE) ? FrameStat.AverageSeconds : 0.0);
		if (bHasFrameStat != FALSE)
		{
			EProfilerTableRow FrameRow;
			FrameRow.Name = EName(PROFILER_SCOPE_GPU_FRAME);
			FrameRow.Stat = FrameStat;
			GpuRows.Add(FrameRow);
		}

		TArray<TPair<EName, EProfileScopeStat>> GpuSnapshot;
		Profiler->SnapshotGPUScopes(GpuSnapshot);
		const INT32 SnapshotNum = GpuSnapshot.Num<INT32>();
		for (INT32 Index = 0; Index < SnapshotNum; Index++)
		{
			const TPair<EName, EProfileScopeStat>& Pair = GpuSnapshot[Index];
			if (Pair.first == EName(PROFILER_SCOPE_GPU_FRAME))
			{
				continue;	// already the first row
			}
			EProfilerTableRow Row;
			Row.Name = Pair.first;
			Row.Stat = Pair.second;
			if (FrameAverage > 0.0)
			{
				Row.ShareRatio = Pair.second.AverageSeconds / FrameAverage;
				Row.bHasShare = TRUE;
			}
			GpuRows.Add(Row);
		}
#endif

		bRowsValid = TRUE;
	}

	// -----------------------------------------------------------------------
	// Draw
	// -----------------------------------------------------------------------

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
		if ((bPaused == FALSE) || (bRowsValid == FALSE))
		{
			RefreshRows();
		}

		// Header. FPS comes from the game-thread bracket, which spans the whole
		// frame including the render-thread hand-off, so it is the wall clock a
		// frame actually took.
		EProfileScopeStat GameStat;
		const BOOL32 bHasGame = FindProfilerRow(CpuRows, PROFILER_SCOPE_GAME_THREAD, GameStat);
		if ((bHasGame != FALSE) && (GameStat.AverageSeconds > 0.0))
		{
			ImGui::Text("FPS %.1f", 1.0 / GameStat.AverageSeconds);
		}
		else
		{
			ImGui::TextDisabled("FPS --");
		}

		EProfileScopeStat RenderStat;
		const BOOL32 bHasRender = FindProfilerRow(CpuRows, PROFILER_SCOPE_RENDER_THREAD, RenderStat);
		DrawHeaderSegment("Game", bHasGame, GameStat);
		DrawHeaderSegment("Render", bHasRender, RenderStat);

		EProfileScopeStat PhysicsStat;
		const BOOL32 bHasPhysics = FindProfilerRow(CpuRows, PROFILER_SCOPE_PHYSICS_THREAD, PhysicsStat);
		DrawHeaderSegment("Physics", bHasPhysics, PhysicsStat);

		EProfileScopeStat LoadStat;
		const BOOL32 bHasLoad = FindProfilerRow(CpuRows, PROFILER_SCOPE_LOAD_THREAD, LoadStat);
		DrawHeaderSegment("Load", bHasLoad, LoadStat);

		EProfileScopeStat GpuFrameStat;
		const BOOL32 bHasGpuFrame = FindProfilerRow(GpuRows, PROFILER_SCOPE_GPU_FRAME, GpuFrameStat);
		DrawHeaderSegment("GPU", bHasGpuFrame, GpuFrameStat);

		if (ImGui::Button("Clear All"))
		{
			EProfiler::GetManagerSingleton()->ClearTraces();
			CpuRows.Empty();
			GpuRows.Empty();
			bRowsValid = FALSE;
		}
		ImGui::SameLine();
		if (ImGui::Button((bPaused != FALSE) ? "Resume" : "Pause"))
		{
			bPaused = ((bPaused != FALSE) ? FALSE : TRUE);
			if (bPaused == FALSE)
			{
				bRowsValid = FALSE;	// rebuild on the frame the pause is released
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled("window %u samples", static_cast<unsigned int>(EEngineSettings::ENGINE_PROFILER_AVERAGE_NUM));

#if _GPU_ENABLE_PROFILER
		// A GPU sample that never arrived: the D3D11 delay ring ran out of
		// retries, or a D3D12 readback failed. Named here because a missing row
		// and a row that never had data look the same in the table.
		ImGui::SameLine();
		ImGui::TextDisabled("lost %u frames", static_cast<unsigned int>(EGPUProfiler::GetManagerSingleton()->GetDroppedResolveNum()));
#endif

		if (ImGui::BeginTabBar("ProfilerTabs"))
		{
			if (ImGui::BeginTabItem("CPU Threads"))
			{
#if _CPU_ENABLE_PROFILER
				DrawProfilerTable("##cpu_threads", CpuRows, FALSE);
#else
				ImGui::TextDisabled("CPU profiler disabled.");
#endif
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("GPU"))
			{
#if _GPU_ENABLE_PROFILER
				DrawProfilerTable("##gpu_passes", GpuRows, TRUE);
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
