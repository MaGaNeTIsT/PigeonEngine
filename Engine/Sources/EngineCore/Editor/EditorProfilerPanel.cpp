#include "EditorProfilerPanel.h"

#if ENGINE_EDITOR_ONLY

#include <Base/RTTI/RTTIManager.h>
#include <imgui.h>
#include <cstdlib>
#include <Base/DataStructure/Text/Name.h>
#include <Base/Timer/Timer.h>
#include <Development/GPUProfiler.h>
#include <Editor/EditorLogManager.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/World/World.h>

namespace PigeonEngine
{
	namespace
	{
		// The benchmark must not touch the editor world or global physics. A bare actor root
		// and no controller is enough for the transform/tick path being measured.
		class FPerformanceBenchmarkWorld final : public PWorld
		{
		protected:
			virtual PActor* CreateRootActor() override
			{
				return new PActor();
			}
			virtual void CreateDefaultController() override
			{
			}
			virtual BOOL32 ShouldAffectGlobalPhysics() const override
			{
				return FALSE;
			}
		};

		BOOL32 ShouldAutoStartPerformanceBenchmark()
		{
#if defined(_MSC_VER)
			CHAR* Value = nullptr;
			size_t Length = 0u;
			if (_dupenv_s(&Value, &Length, "PIGEON_PERFORMANCE_BENCHMARK") != 0)
			{
				return FALSE;
			}
			const BOOL32 bEnabled = ((Value != nullptr) && (Value[0] != static_cast<CHAR>(0)) && (Value[0] != '0')) ? TRUE : FALSE;
			std::free(Value);
			return bEnabled;
#else
			const CHAR* Value = std::getenv("PIGEON_PERFORMANCE_BENCHMARK");
			return ((Value != nullptr) && (Value[0] != static_cast<CHAR>(0)) && (Value[0] != '0')) ? TRUE : FALSE;
#endif
		}

		UINT32 ReadBenchmarkEnvironmentValue(const CHAR* InName, const UINT32 InDefault)
		{
#if defined(_MSC_VER)
			CHAR* Value = nullptr;
			size_t Length = 0u;
			if (_dupenv_s(&Value, &Length, InName) != 0)
			{
				return InDefault;
			}
			UINT32 Result = InDefault;
			if ((Value != nullptr) && (Value[0] != static_cast<CHAR>(0)))
			{
				Result = static_cast<UINT32>(std::strtoul(Value, nullptr, 10));
			}
			std::free(Value);
			return Result;
#else
			const CHAR* Value = std::getenv(InName);
			return ((Value != nullptr) && (Value[0] != static_cast<CHAR>(0)))
				? static_cast<UINT32>(std::strtoul(Value, nullptr, 10)) : InDefault;
#endif
		}
	}
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
		StopPerformanceBenchmark();
		BenchmarkLastAverageMilliseconds = 0.0;
		BenchmarkLastMoveAverageMilliseconds = 0.0;
		BenchmarkLastTickAverageMilliseconds = 0.0;
		BenchmarkLastRenderTreeAverageMilliseconds = 0.0;
		BenchmarkLastGameplayAverageMilliseconds = 0.0;
		BenchmarkLastP95Milliseconds = 0.0;
		BenchmarkLastMinMilliseconds = 0.0;
		BenchmarkLastMaxMilliseconds = 0.0;
		BenchmarkSamplesMilliseconds.Empty();
		bWindowOpen	= TRUE;
		bPaused		= FALSE;
		bRowsValid	= FALSE;
		CpuRows.Empty();
		CpuTree.Empty();
		CpuRootNodes.Empty();
		GpuRows.Empty();
		// Developer/CI hook: PIGEON_PERFORMANCE_BENCHMARK=1 runs the default stress benchmark
		// through the normal editor frame loop, so its log is available without UI input.
		// Deferred to the first editor update: manager initialization is not complete yet.
		BenchmarkActorCount = static_cast<INT32>(ReadBenchmarkEnvironmentValue("PIGEON_BENCHMARK_ACTORS", static_cast<UINT32>(BenchmarkActorCount)));
		BenchmarkFrameCount = static_cast<INT32>(ReadBenchmarkEnvironmentValue("PIGEON_BENCHMARK_FRAMES", static_cast<UINT32>(BenchmarkFrameCount)));
		BenchmarkMovePercent = static_cast<INT32>(ReadBenchmarkEnvironmentValue("PIGEON_BENCHMARK_MOVE_PERCENT", static_cast<UINT32>(BenchmarkMovePercent)));
		bPerformanceBenchmarkAutoStart = ShouldAutoStartPerformanceBenchmark();
	}
	void EEditorProfilerPanel::ShutDown()
	{
		StopPerformanceBenchmark();
		CpuRows.Empty();
		CpuTree.Empty();
		CpuRootNodes.Empty();
		GpuRows.Empty();
		bRowsValid = FALSE;
	}

	void EEditorProfilerPanel::StartPerformanceBenchmark()
	{
		StopPerformanceBenchmark();

		BenchmarkActorCount = static_cast<INT32>(EMath::Clamp(BenchmarkActorCount, 100, 50000));
		BenchmarkFrameCount = static_cast<INT32>(EMath::Clamp(BenchmarkFrameCount, 10, 1200));
		BenchmarkMovePercent = static_cast<INT32>(EMath::Clamp(BenchmarkMovePercent, 0, 100));
		BenchmarkWorld = new FPerformanceBenchmarkWorld();
		BenchmarkWorld->Init();
		PE_LOG_LOG(EString(PE_TEXT("[PerformanceBenchmark] start actors=")) + EString::FromInt(BenchmarkActorCount)
			+ EString(PE_TEXT(" frames=")) + EString::FromInt(BenchmarkFrameCount));
		BenchmarkActors.Empty();
		BenchmarkBuildCursor = 0u;
		BenchmarkWarmupFrames = 10u;
		BenchmarkRunFrames = 0u;
		BenchmarkSumMilliseconds = 0.0;
		BenchmarkMoveSumMilliseconds = 0.0;
		BenchmarkTickSumMilliseconds = 0.0;
		BenchmarkRenderTreeSumMilliseconds = 0.0;
		BenchmarkGameplaySumMilliseconds = 0.0;
		BenchmarkMinMilliseconds = 0.0;
		BenchmarkMaxMilliseconds = 0.0;
		BenchmarkSamplesMilliseconds.Empty();
		BenchmarkState = EBenchmarkStateType::BUILDING;
	}

	void EEditorProfilerPanel::StopPerformanceBenchmark()
	{
		if (BenchmarkWorld)
		{
			// PObject::Destroy owns the final delete; only clear the pointer here.
			BenchmarkWorld->Destroy();
			BenchmarkWorld = nullptr;
		}
		BenchmarkActors.Empty();
		BenchmarkState = EBenchmarkStateType::IDLE;
		BenchmarkBuildCursor = 0u;
		BenchmarkRunFrames = 0u;
	}

	void EEditorProfilerPanel::TickPerformanceBenchmark()
	{
		if (bPerformanceBenchmarkAutoStart)
		{
			bPerformanceBenchmarkAutoStart = FALSE;
			StartPerformanceBenchmark();
		}
		if (!BenchmarkWorld)
		{
			return;
		}
		if (BenchmarkState == EBenchmarkStateType::BUILDING)
		{
			const UINT32 TargetNum = static_cast<UINT32>(BenchmarkActorCount);
			const UINT32 BuildEnd = EMath::Min(TargetNum, BenchmarkBuildCursor + 200u);
			for (; BenchmarkBuildCursor < BuildEnd; BenchmarkBuildCursor++)
			{
				PActor* Actor = new PActor();
				Actor->SetIsTickable(TRUE);
				BenchmarkWorld->AddActor(Actor);
				PSceneComponent* Root = Actor->GetRootComponent();
				Root->SetMobility(EMobilityType::EMT_DYNAMIC);
				Root->SetComponentLocation(Vector3(
					static_cast<FLOAT>(BenchmarkBuildCursor % 100u) * 2.0f,
					0.0f,
					static_cast<FLOAT>(BenchmarkBuildCursor / 100u) * 2.0f));
				PSceneComponent* Child = new PSceneComponent();
				Actor->AddComponent(Child);
				Child->SetMobility(EMobilityType::EMT_DYNAMIC);
				Child->SetComponentLocation(Vector3(0.0f, 0.0f, 1.0f));
				BenchmarkActors.Add(Actor);
			}
			if (BenchmarkBuildCursor >= TargetNum)
			{
				BenchmarkState = EBenchmarkStateType::WARMUP;
				PE_LOG_LOG(EString(PE_TEXT("[PerformanceBenchmark] built actors=")) + EString::FromInt(BenchmarkActorCount));
			}
			return;
		}
		if (BenchmarkState == EBenchmarkStateType::WARMUP)
		{
			BenchmarkWorld->Tick(1.0f / 60.0f);
			if (BenchmarkWarmupFrames > 0u)
			{
				BenchmarkWarmupFrames--;
			}
			if (BenchmarkWarmupFrames == 0u)
			{
				BenchmarkState = EBenchmarkStateType::RUNNING;
				PE_LOG_LOG(EString(PE_TEXT("[PerformanceBenchmark] warmup done")));
			}
			return;
		}
		if (BenchmarkState != EBenchmarkStateType::RUNNING)
		{
			return;
		}

		const UINT64 StartTicks = EBaseTimer::GetClockTicks();
		const FLOAT MotionPhase = static_cast<FLOAT>(BenchmarkRunFrames % 60u) * 0.01f;
		const UINT64 MoveStartTicks = EBaseTimer::GetClockTicks();
		if (BenchmarkMovePercent > 0)
		{
			const UINT32 MovePercent = static_cast<UINT32>(BenchmarkMovePercent);
			for (UINT32 Index = 0u, Num = BenchmarkActors.Num<UINT32>(); Index < Num; Index++)
			{
				if ((MovePercent < 100u) && ((Index % 100u) >= MovePercent))
				{
					continue;
				}
				PActor* Actor = BenchmarkActors[Index];
				if (Actor)
				{
					Actor->GetRootComponent()->SetComponentLocation(Vector3(
						static_cast<FLOAT>(Index % 100u) * 2.0f + MotionPhase,
						0.0f,
						static_cast<FLOAT>(Index / 100u) * 2.0f));
				}
			}
		}
		const DOUBLE MoveMilliseconds = EBaseTimer::TicksToSeconds(EBaseTimer::GetClockTicks() - MoveStartTicks) * 1000.0;
		const UINT64 TickStartTicks = EBaseTimer::GetClockTicks();
		BenchmarkWorld->TickRenderTree();
		const DOUBLE RenderTreeMilliseconds = EBaseTimer::TicksToSeconds(EBaseTimer::GetClockTicks() - TickStartTicks) * 1000.0;
		const UINT64 GameplayStartTicks = EBaseTimer::GetClockTicks();
		BenchmarkWorld->TickGameplay(1.0f / 60.0f);
		const DOUBLE GameplayMilliseconds = EBaseTimer::TicksToSeconds(EBaseTimer::GetClockTicks() - GameplayStartTicks) * 1000.0;
		const DOUBLE TickMilliseconds = RenderTreeMilliseconds + GameplayMilliseconds;
		const DOUBLE ElapsedMilliseconds = EBaseTimer::TicksToSeconds(EBaseTimer::GetClockTicks() - StartTicks) * 1000.0;
		BenchmarkSamplesMilliseconds.Add(static_cast<FLOAT>(ElapsedMilliseconds));
		BenchmarkSumMilliseconds += ElapsedMilliseconds;
		BenchmarkMoveSumMilliseconds += MoveMilliseconds;
		BenchmarkTickSumMilliseconds += TickMilliseconds;
		BenchmarkRenderTreeSumMilliseconds += RenderTreeMilliseconds;
		BenchmarkGameplaySumMilliseconds += GameplayMilliseconds;
		if ((BenchmarkRunFrames == 0u) || (ElapsedMilliseconds < BenchmarkMinMilliseconds))
		{
			BenchmarkMinMilliseconds = ElapsedMilliseconds;
		}
		if (ElapsedMilliseconds > BenchmarkMaxMilliseconds)
		{
			BenchmarkMaxMilliseconds = ElapsedMilliseconds;
		}
		BenchmarkRunFrames++;

		if (BenchmarkRunFrames >= static_cast<UINT32>(BenchmarkFrameCount))
		{
			TArray<FLOAT> SortedSamples(BenchmarkSamplesMilliseconds);
			SortedSamples.Sort([](const FLOAT& InLeft, const FLOAT& InRight) -> BOOL32
			{
				return (InLeft < InRight) ? TRUE : FALSE;
			});
			const UINT32 SampleNum = SortedSamples.Num<UINT32>();
			const UINT32 P95Index = (SampleNum > 0u) ? EMath::Min(SampleNum - 1u, static_cast<UINT32>(static_cast<FLOAT>(SampleNum) * 0.95f)) : 0u;
			BenchmarkLastAverageMilliseconds = BenchmarkSumMilliseconds / static_cast<DOUBLE>(BenchmarkRunFrames);
			BenchmarkLastMoveAverageMilliseconds = BenchmarkMoveSumMilliseconds / static_cast<DOUBLE>(BenchmarkRunFrames);
			BenchmarkLastTickAverageMilliseconds = BenchmarkTickSumMilliseconds / static_cast<DOUBLE>(BenchmarkRunFrames);
			BenchmarkLastRenderTreeAverageMilliseconds = BenchmarkRenderTreeSumMilliseconds / static_cast<DOUBLE>(BenchmarkRunFrames);
			BenchmarkLastGameplayAverageMilliseconds = BenchmarkGameplaySumMilliseconds / static_cast<DOUBLE>(BenchmarkRunFrames);
			BenchmarkLastMinMilliseconds = BenchmarkMinMilliseconds;
			BenchmarkLastMaxMilliseconds = BenchmarkMaxMilliseconds;
			BenchmarkLastP95Milliseconds = (SampleNum > 0u) ? static_cast<DOUBLE>(SortedSamples[P95Index]) : 0.0;
			PE_LOG_LOG(EString(PE_TEXT("[PerformanceBenchmark] actors=")) + EString::FromInt(BenchmarkActorCount)
				+ EString(PE_TEXT(" components=")) + EString::FromInt(static_cast<INT32>(BenchmarkWorld->GetComponentCount()))
				+ EString(PE_TEXT(" frames=")) + EString::FromInt(BenchmarkFrameCount)
				+ EString(PE_TEXT(" avg_ms=")) + EString::FromFloat(static_cast<FLOAT>(BenchmarkLastAverageMilliseconds))
				+ EString(PE_TEXT(" move_ms=")) + EString::FromFloat(static_cast<FLOAT>(BenchmarkLastMoveAverageMilliseconds))
				+ EString(PE_TEXT(" render_tree_ms=")) + EString::FromFloat(static_cast<FLOAT>(BenchmarkLastRenderTreeAverageMilliseconds))
				+ EString(PE_TEXT(" gameplay_ms=")) + EString::FromFloat(static_cast<FLOAT>(BenchmarkLastGameplayAverageMilliseconds))
				+ EString(PE_TEXT(" p95_ms=")) + EString::FromFloat(static_cast<FLOAT>(BenchmarkLastP95Milliseconds))
				+ EString(PE_TEXT(" min_ms=")) + EString::FromFloat(static_cast<FLOAT>(BenchmarkLastMinMilliseconds))
				+ EString(PE_TEXT(" max_ms=")) + EString::FromFloat(static_cast<FLOAT>(BenchmarkLastMaxMilliseconds)));
			// PObject::Destroy owns the final delete; only clear the pointer here.
			BenchmarkWorld->Destroy();
			BenchmarkWorld = nullptr;
			BenchmarkActors.Empty();
			BenchmarkState = EBenchmarkStateType::DONE;
		}
	}

	void EEditorProfilerPanel::DrawPerformanceBenchmarkPanel()
	{
		if (!ImGui::CollapsingHeader("Performance Benchmark"))
		{
			return;
		}
		ImGui::TextDisabled("Measures transform dirty + World::Tick in a disposable world (no render scene).");
		ImGui::SetNextItemWidth(160.0f);
		ImGui::InputInt("Actors", &BenchmarkActorCount);
		ImGui::SetNextItemWidth(160.0f);
		ImGui::InputInt("Frames", &BenchmarkFrameCount);
		ImGui::SetNextItemWidth(160.0f);
		ImGui::InputInt("Move %", &BenchmarkMovePercent);
		BenchmarkActorCount = static_cast<INT32>(EMath::Clamp(BenchmarkActorCount, 100, 50000));
		BenchmarkFrameCount = static_cast<INT32>(EMath::Clamp(BenchmarkFrameCount, 10, 1200));
		BenchmarkMovePercent = static_cast<INT32>(EMath::Clamp(BenchmarkMovePercent, 0, 100));

		const BOOL32 bRunning = ((BenchmarkState == EBenchmarkStateType::BUILDING) ||
			(BenchmarkState == EBenchmarkStateType::WARMUP) ||
			(BenchmarkState == EBenchmarkStateType::RUNNING)) ? TRUE : FALSE;
		if (bRunning)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Run Benchmark"))
		{
			StartPerformanceBenchmark();
		}
		if (bRunning)
		{
			ImGui::EndDisabled();
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop"))
		{
			StopPerformanceBenchmark();
		}

		switch (BenchmarkState)
		{
		case EBenchmarkStateType::IDLE:
			ImGui::TextDisabled("Idle.");
			break;
		case EBenchmarkStateType::BUILDING:
			ImGui::Text("Building %u / %d", static_cast<unsigned int>(BenchmarkBuildCursor), BenchmarkActorCount);
			break;
		case EBenchmarkStateType::WARMUP:
			ImGui::Text("Warming up (%u frames left).", static_cast<unsigned int>(BenchmarkWarmupFrames));
			break;
		case EBenchmarkStateType::RUNNING:
			ImGui::Text("Running %u / %d", static_cast<unsigned int>(BenchmarkRunFrames), BenchmarkFrameCount);
			break;
		case EBenchmarkStateType::DONE:
			ImGui::Text("Last run: avg %.3f ms (move %.3f + render tree %.3f + gameplay %.3f)  p95 %.3f ms  min %.3f ms  max %.3f ms",
				BenchmarkLastAverageMilliseconds, BenchmarkLastMoveAverageMilliseconds, BenchmarkLastRenderTreeAverageMilliseconds,
				BenchmarkLastGameplayAverageMilliseconds, BenchmarkLastP95Milliseconds, BenchmarkLastMinMilliseconds, BenchmarkLastMaxMilliseconds);
			break;
		case EBenchmarkStateType::FAILED:
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Benchmark failed.");
			break;
		default:
			break;
		}
	}

	// -----------------------------------------------------------------------
	// Table ordering
	//
	// ImGui only reports which column the user picked and in which direction;
	// sorting the rows is the caller's job. The row counts here are single
	// digits and tens, so an insertion sort keeps the code obvious and the
	// order stable for equal values.
	// -----------------------------------------------------------------------

#if ENGINE_ENABLE_PROFILER

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

	// A scope recorded once per worker is concurrent with its siblings. Summing its samples
	// into a frame total would count overlapping wall clock more than once, so the panel marks
	// it and leaves the per-frame cell empty.
	static BOOL8 IsConcurrentProfilerScope(const EName& InName)
	{
		static const CHAR WorkerSuffix[] = PE_TEXT(".Worker");
		const CHAR* NameData = InName.GetData();
		if (!NameData)
		{
			return FALSE;
		}
		UINT32 NameLength = 0u;
		while (NameData[NameLength] != static_cast<CHAR>(0))
		{
			NameLength++;
		}
		constexpr UINT32 SuffixLength = 7u;
		if (NameLength < SuffixLength)
		{
			return FALSE;
		}
		for (UINT32 Index = 0u; Index < SuffixLength; Index++)
		{
			if (NameData[NameLength - SuffixLength + Index] != WorkerSuffix[Index])
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	// Stat.SampleCount is the number of calls since the traces were cleared; dividing by the
	// GameThread call count turns it into calls per frame. The GameThread bracket is one call
	// per frame by construction, so it is the frame counter the panel already has.
	static void FillProfilerPerFrame(EProfilerTableRow& InOutRow, UINT64 InFrameSampleCount)
	{
		InOutRow.bIsConcurrent = IsConcurrentProfilerScope(InOutRow.Name);
		InOutRow.CallsPerFrame = 0.0;
		InOutRow.PerFrameSeconds = 0.0;
		InOutRow.bHasPerFrame = FALSE;
		if (InFrameSampleCount == 0u)
		{
			return;
		}
		InOutRow.CallsPerFrame = static_cast<DOUBLE>(InOutRow.Stat.SampleCount)
			/ static_cast<DOUBLE>(InFrameSampleCount);
		if (InOutRow.bIsConcurrent == FALSE)
		{
			InOutRow.PerFrameSeconds = InOutRow.Stat.AverageSeconds * InOutRow.CallsPerFrame;
			InOutRow.bHasPerFrame = TRUE;
		}
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
		case 6:
			return InRow.PerFrameSeconds;
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

	// The numeric columns of one row. Shared by the flat GPU table and the CPU tree so the two
	// cannot disagree about what a column means; a synthetic tree node has no sample of its own
	// and shows the placeholder rather than a zero that would read as a measurement.
	static void DrawProfilerRowColumns(const EProfilerTableRow& InRow, BOOL8 bHasRow, BOOL8 bShareColumn, BOOL8 bShowPerFrame)
	{
		ImGui::TableSetColumnIndex(1);
		if (bHasRow != FALSE)
		{
			ImGui::Text("%.3f", InRow.Stat.AverageSeconds * 1000.0);
		}
		else
		{
			ImGui::TextUnformatted("-");
		}
		ImGui::TableSetColumnIndex(2);
		if (bHasRow != FALSE)
		{
			ImGui::Text("%.3f", InRow.Stat.LastSeconds * 1000.0);
		}
		else
		{
			ImGui::TextUnformatted("-");
		}
		ImGui::TableSetColumnIndex(3);
		if (bHasRow != FALSE)
		{
			ImGui::Text("%.3f", InRow.Stat.MinSeconds * 1000.0);
		}
		else
		{
			ImGui::TextUnformatted("-");
		}
		ImGui::TableSetColumnIndex(4);
		if (bHasRow != FALSE)
		{
			ImGui::Text("%.3f", InRow.Stat.MaxSeconds * 1000.0);
		}
		else
		{
			ImGui::TextUnformatted("-");
		}
		ImGui::TableSetColumnIndex(5);
		if (bHasRow == FALSE)
		{
			ImGui::TextUnformatted("-");
		}
		else if (bShareColumn == FALSE)
		{
			ImGui::Text("%llu", static_cast<unsigned long long>(InRow.Stat.SampleCount));
		}
		else if (InRow.bHasShare != FALSE)
		{
			ImGui::Text("%.1f%%", InRow.ShareRatio * 100.0);
		}
		else
		{
			ImGui::TextUnformatted("-");
		}
		if (bShowPerFrame != FALSE)
		{
			ImGui::TableSetColumnIndex(6);
			if ((bHasRow == FALSE) || (InRow.CallsPerFrame <= 0.0))
			{
				ImGui::TextUnformatted("-");
			}
			else if (InRow.bIsConcurrent != FALSE)
			{
				ImGui::TextUnformatted("-");
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%.1f calls/frame; concurrent workers are not summed", InRow.CallsPerFrame);
				}
			}
			else if (InRow.bHasPerFrame != FALSE)
			{
				ImGui::Text("%.3f", InRow.PerFrameSeconds * 1000.0);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%.1f calls/frame", InRow.CallsPerFrame);
				}
			}
			else
			{
				ImGui::TextUnformatted("-");
			}
		}
	}

	static void DrawProfilerRows(const TArray<EProfilerTableRow>& InRows, BOOL8 bShareColumn, BOOL8 bShowPerFrame)
	{
		const INT32 RowNum = InRows.Num<INT32>();
		for (INT32 RowIndex = 0; RowIndex < RowNum; RowIndex++)
		{
			const EProfilerTableRow& Row = InRows[RowIndex];
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(Row.Name.GetData());
			DrawProfilerRowColumns(Row, TRUE, bShareColumn, bShowPerFrame);
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
			DrawProfilerRows(InOutRows, bShareColumn, FALSE);
			ImGui::EndTable();
		}
	}

	// -----------------------------------------------------------------------
	// CPU tree table
	//
	// Thread rows are the roots, so collapsing one hides exactly its own sub-brackets. The
	// hierarchy comes from the dotted scope names; a deeper name adds a level without a second
	// declaration, and a prefix that only exists as part of a longer name is a synthetic node.
	// -----------------------------------------------------------------------

	static void SortProfilerTreeChildren(TArray<EProfilerTreeNode>& InOutNodes, INT32 InNodeIndex
		, INT32 InColumn, BOOL8 bAscending, BOOL8 bShareColumn)
	{
		if ((InNodeIndex < 0) || (InNodeIndex >= InOutNodes.Num<INT32>()))
		{
			return;
		}
		TArray<INT32>& Children = InOutNodes[InNodeIndex].Children;
		const INT32 ChildNum = Children.Num<INT32>();
		for (INT32 ChildIndex = 1; ChildIndex < ChildNum; ChildIndex++)
		{
			const INT32 KeyIndex = Children[ChildIndex];
			INT32 InsertIndex = ChildIndex - 1;
			while (InsertIndex >= 0)
			{
				const INT32 Order = CompareProfilerRows(InOutNodes[KeyIndex].Row
					, InOutNodes[Children[InsertIndex]].Row, InColumn, bShareColumn);
				const BOOL8 bMoveEarlier = ((bAscending != FALSE) ? (Order < 0) : (Order > 0)) ? TRUE : FALSE;
				if (bMoveEarlier == FALSE)
				{
					break;
				}
				Children[InsertIndex + 1] = Children[InsertIndex];
				InsertIndex -= 1;
			}
			Children[InsertIndex + 1] = KeyIndex;
		}

		for (INT32 ChildIndex = 0; ChildIndex < ChildNum; ChildIndex++)
		{
			SortProfilerTreeChildren(InOutNodes, Children[ChildIndex], InColumn, bAscending, bShareColumn);
		}
	}

	static void DrawProfilerTreeRow(const TArray<EProfilerTreeNode>& InNodes, INT32 InNodeIndex, BOOL8 bShareColumn)
	{		if ((InNodeIndex < 0) || (InNodeIndex >= InNodes.Num<INT32>()))
		{
			return;
		}
		const EProfilerTreeNode& Node = InNodes[InNodeIndex];
		ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_DefaultOpen;
		const BOOL32 bHasChildren = ((Node.Children.Num<INT32>() > 0) ? TRUE : FALSE);
		if (bHasChildren == FALSE)
		{
			NodeFlags |= (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		}

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		const BOOL32 bOpen = ImGui::TreeNodeEx(Node.Row.Name.GetData(), NodeFlags);
		DrawProfilerRowColumns(Node.Row, Node.bHasRow, bShareColumn, TRUE);
		if ((bHasChildren != FALSE) && (bOpen != FALSE))
		{
			for (INT32 ChildIndex = 0; ChildIndex < Node.Children.Num<INT32>(); ChildIndex++)
			{
				DrawProfilerTreeRow(InNodes, Node.Children[ChildIndex], bShareColumn);
			}
			ImGui::TreePop();
		}
	}

	static void DrawProfilerTreeTable(const CHAR* InTableId, TArray<EProfilerTreeNode>& InOutNodes
		, const TArray<INT32>& InRootNodes, BOOL8 bShareColumn)
	{
		if (InRootNodes.Num<INT32>() == 0)
		{
			ImGui::TextDisabled("(no samples yet)");
			return;
		}

		const ImGuiTableFlags TableFlags = ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg
			| ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable(InTableId, 7, TableFlags))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
			ImGui::TableSetupColumn("Avg(ms)", ImGuiTableColumnFlags_WidthStretch
				| ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 1.2f);
			ImGui::TableSetupColumn("Last(ms)", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Min(ms)", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Max(ms)", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn((bShareColumn != FALSE) ? "Share" : "Samples", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Per-Frame(ms)", ImGuiTableColumnFlags_WidthStretch, 1.2f);
			ImGui::TableHeadersRow();

			// Thread order is fixed; only the children of each thread follow the header sort.
			// An untouched table opens on the most expensive sub-bracket first.
			INT32 SortColumn = 1;
			BOOL8 bAscending = FALSE;
			ImGuiTableSortSpecs* SortSpecs = ImGui::TableGetSortSpecs();
			if ((SortSpecs) && (SortSpecs->SpecsCount > 0u))
			{
				SortColumn = static_cast<INT32>(SortSpecs->Specs[0].ColumnIndex);
				bAscending = ((SortSpecs->Specs[0].SortDirection != ImGuiSortDirection_Descending) ? TRUE : FALSE);
			}
			for (INT32 RootIndex = 0; RootIndex < InRootNodes.Num<INT32>(); RootIndex++)
			{
				SortProfilerTreeChildren(InOutNodes, InRootNodes[RootIndex], SortColumn, bAscending, bShareColumn);
			}
			for (INT32 RootIndex = 0; RootIndex < InRootNodes.Num<INT32>(); RootIndex++)
			{
				DrawProfilerTreeRow(InOutNodes, InRootNodes[RootIndex], bShareColumn);
			}
			ImGui::EndTable();
		}
	}

	// -----------------------------------------------------------------------
	// Counters table
	// -----------------------------------------------------------------------

	static void DrawCounterTable(const CHAR* InTableId, const TArray<TPair<EName, EProfileCounterStat>>& InRows)
	{
		if (InRows.Num() == 0)
		{
			ImGui::TextDisabled("(no samples yet)");
			return;
		}
		const ImGuiTableFlags TableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg
			| ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable(InTableId, 5, TableFlags))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
			ImGui::TableSetupColumn("Avg", ImGuiTableColumnFlags_WidthStretch, 1.2f);
			ImGui::TableSetupColumn("Last", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Min", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableHeadersRow();
			for (INT32 RowIndex = 0; RowIndex < InRows.Num<INT32>(); RowIndex++)
			{
				const TPair<EName, EProfileCounterStat>& Pair = InRows[RowIndex];
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(Pair.first.GetData());
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.1f", Pair.second.AverageValue);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%.0f", Pair.second.LastValue);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%.0f", Pair.second.MinValue);
				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%.0f", Pair.second.MaxValue);
			}
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

#endif	// ENGINE_ENABLE_PROFILER

	// -----------------------------------------------------------------------
	// Snapshots
	// -----------------------------------------------------------------------

	void EEditorProfilerPanel::RefreshRows()
	{
		CpuRows.Empty();
		GpuRows.Empty();
		CounterRows.Empty();

#if ENGINE_ENABLE_CPU_PROFILER
		// The GameThread bracket runs exactly once per frame, so its sample count is the frame
		// count every other CPU scope is divided by.
		EProfileScopeStat GameThreadScopeStat;
		const BOOL8 bHasGameThreadStat = EProfiler::GetManagerSingleton()->GetScopeStat(EName(PROFILER_SCOPE_GAME_THREAD), GameThreadScopeStat);
		const UINT64 FrameSampleCount = ((bHasGameThreadStat != FALSE) ? GameThreadScopeStat.SampleCount : 0u);

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
			FillProfilerPerFrame(Row, FrameSampleCount);
			CpuRows.Add(Row);
		}

		// Every other scope after the thread brackets: these are the sub-brackets a
		// thread row is made of, so a frame that is slow in one place can be read
		// without guessing from the total.
		TArray<TPair<EName, EProfileScopeStat>> CpuSnapshot;
		EProfiler::GetManagerSingleton()->SnapshotScopes(CpuSnapshot);
		for (UINT32 RowIndex = 0u, RowNum = CpuSnapshot.Num<UINT32>(); RowIndex < RowNum; RowIndex++)
		{
			const EName& ScopeName = CpuSnapshot[RowIndex].first;
			BOOL32 bIsThreadScope = FALSE;
			for (UINT32 ThreadIndex = 0u; ThreadIndex < 4u; ThreadIndex++)
			{
				if (ScopeName == EName(ThreadScopeNames[ThreadIndex]))
				{
					bIsThreadScope = TRUE;
					break;
				}
			}
			if (bIsThreadScope != FALSE)
			{
				continue;
			}
			EProfilerTableRow Row;
			Row.Name = ScopeName;
			Row.Stat = CpuSnapshot[RowIndex].second;
			FillProfilerPerFrame(Row, FrameSampleCount);
			CpuRows.Add(Row);
		}

		TArray<TPair<EName, EProfileCounterStat>> CounterSnapshot;
		EProfiler::GetManagerSingleton()->SnapshotCounters(CounterSnapshot);
		for (UINT32 RowIndex = 0u, RowNum = CounterSnapshot.Num<UINT32>(); RowIndex < RowNum; RowIndex++)
		{
			CounterRows.Add(CounterSnapshot[RowIndex]);
		}
#endif

#if ENGINE_ENABLE_GPU_PROFILER
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

		BuildCpuTree();
		bRowsValid = TRUE;
	}

	// -----------------------------------------------------------------------
	// CPU tree
	//
	// The scope names already carry their grouping: every render sub-bracket is named
	// "RenderThread.<Scope>", and a deeper bracket would be "RenderThread.<Scope>.<SubScope>".
	// Splitting the name on '.' is therefore enough to build the tree the panel shows, with no
	// second declaration at the instrumentation site. A prefix that exists only as part of a
	// longer name becomes a synthetic node so the hierarchy stays complete.
	// -----------------------------------------------------------------------

	void EEditorProfilerPanel::BuildCpuTree()
	{
		CpuTree.Empty();
		CpuRootNodes.Empty();

		const EString DotSeparator(PE_TEXT("."));
		for (INT32 RowIndex = 0; RowIndex < CpuRows.Num<INT32>(); RowIndex++)
		{
			const EProfilerTableRow& Row = CpuRows[RowIndex];
			const EString FullName = Row.Name.ToString();
			INT32 ParentIndex = -1;
			UINT32 SegmentStart = 0u;
			while (SegmentStart < FullName.Length())
			{
				const EString Remainder = FullName.Substring(SegmentStart, FullName.Length() - SegmentStart);
				const INT32 DotOffset = Remainder.Find(DotSeparator);
				const UINT32 SegmentEnd = ((DotOffset >= 0)
					? (SegmentStart + static_cast<UINT32>(DotOffset))
					: FullName.Length());
				const EString Prefix = FullName.Substring(0u, SegmentEnd);
				const EName PrefixName(Prefix);

				INT32 NodeIndex = -1;
				for (INT32 SearchIndex = 0; SearchIndex < CpuTree.Num<INT32>(); SearchIndex++)
				{
					if (CpuTree[SearchIndex].Row.Name == PrefixName)
					{
						NodeIndex = SearchIndex;
						break;
					}
				}
				if (NodeIndex < 0)
				{
					EProfilerTreeNode NewNode;
					NewNode.Row.Name = PrefixName;
					NodeIndex = static_cast<INT32>(CpuTree.Add(NewNode));
					if (ParentIndex >= 0)
					{
						CpuTree[ParentIndex].Children.Add(NodeIndex);
					}
					else
					{
						CpuRootNodes.Add(NodeIndex);
					}
				}

				if (SegmentEnd >= FullName.Length())
				{
					CpuTree[NodeIndex].Row = Row;
					CpuTree[NodeIndex].bHasRow = TRUE;
					break;
				}
				ParentIndex = NodeIndex;
				SegmentStart = SegmentEnd + 1u;
			}
		}
	}

	// -----------------------------------------------------------------------
	// Draw
	// -----------------------------------------------------------------------

	void EEditorProfilerPanel::DrawPanel()
	{
		TickPerformanceBenchmark();
		bool bOpen = (bWindowOpen != FALSE);
		if (!ImGui::Begin("Profiler", &bOpen))
		{
			bWindowOpen = bOpen ? TRUE : FALSE;
			ImGui::End();
			return;
		}
		bWindowOpen = bOpen ? TRUE : FALSE;

#if !ENGINE_ENABLE_PROFILER
		ImGui::TextDisabled("Profiler disabled (ENGINE_ENABLE_PROFILER=0).");
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
			CpuTree.Empty();
			CpuRootNodes.Empty();
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
		ImGui::SameLine();
		// Avg is per call; the new column multiplies it by calls/frame. Worker rows overlap in
		// time, so their per-frame cell is left empty rather than summing concurrent samples.
		ImGui::TextDisabled("Per-Frame = Avg x calls/frame; approx, workers excluded");

		DrawPerformanceBenchmarkPanel();

#if ENGINE_ENABLE_GPU_PROFILER
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
#if ENGINE_ENABLE_CPU_PROFILER
				DrawProfilerTreeTable("##cpu_threads", CpuTree, CpuRootNodes, FALSE);
#else
				ImGui::TextDisabled("CPU profiler disabled.");
#endif
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("GPU"))
			{
#if ENGINE_ENABLE_GPU_PROFILER
				DrawProfilerTable("##gpu_passes", GpuRows, TRUE);
#else
				ImGui::TextDisabled("GPU profiler disabled.");
#endif
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Counters"))
			{
#if ENGINE_ENABLE_CPU_PROFILER
				DrawCounterTable("##cpu_counters", CounterRows);
#else
				ImGui::TextDisabled("CPU profiler disabled.");
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
