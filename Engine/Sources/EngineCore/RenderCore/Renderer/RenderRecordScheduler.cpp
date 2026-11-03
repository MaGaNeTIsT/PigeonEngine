#include "RenderRecordScheduler.h"

#include <Development/Profiler.h>
#include <RHI/IRCommandList.h>
#include <RHI/IRRHIDevice.h>

namespace PigeonEngine
{

	FRenderRecordScheduler::FRenderRecordScheduler()
	{
	}

	FRenderRecordScheduler::~FRenderRecordScheduler()
	{
		Shutdown();
	}

	void FRenderRecordScheduler::Initialize(IRRHIDevice* InDevice, const UINT32 InRecorderNum)
	{
		Shutdown();

		Device = InDevice;
		RecorderNum = InRecorderNum;
		if (RecorderNum < 1u)
		{
			RecorderNum = 1u;
		}
		if (RecorderNum > RHI_MAX_RECORD_THREAD_NUM)
		{
			RecorderNum = RHI_MAX_RECORD_THREAD_NUM;
		}
		if (Device)
		{
			const UINT32 DeviceRecorderNum = Device->GetMaxRecordThreadNum();
			if (RecorderNum > DeviceRecorderNum)
			{
				RecorderNum = ((DeviceRecorderNum > 0u) ? DeviceRecorderNum : 1u);
			}
		}

		bStopRequested.Store(FALSE);
		WorkerStates.SetNum(RecorderNum - 1u);
		for (UINT32 WorkerIndex = 0u, WorkerNum = WorkerStates.Num<UINT32>(); WorkerIndex < WorkerNum; WorkerIndex++)
		{
			WorkerStates[WorkerIndex] = EMemory::MakeUnique<FRecorderState>();
			WorkerStates[WorkerIndex]->RecorderIndex = WorkerIndex + 1u;
			FRecorderState* State = WorkerStates[WorkerIndex].get();
			State->Thread.Start([this, State]() -> void
			{
				WorkerEntry(State->RecorderIndex);
			});
		}
	}

	void FRenderRecordScheduler::Shutdown()
	{
		bStopRequested.Store(TRUE);
		for (UINT32 WorkerIndex = 0u, WorkerNum = WorkerStates.Num<UINT32>(); WorkerIndex < WorkerNum; WorkerIndex++)
		{
			if (WorkerStates[WorkerIndex])
			{
				WorkerStates[WorkerIndex]->WakeEvent.Signal();
			}
		}
		for (UINT32 WorkerIndex = 0u, WorkerNum = WorkerStates.Num<UINT32>(); WorkerIndex < WorkerNum; WorkerIndex++)
		{
			if (WorkerStates[WorkerIndex])
			{
				WorkerStates[WorkerIndex]->Thread.Join();
			}
		}
		WorkerStates.SetNum(0);
		bStopRequested.Store(FALSE);
		RecorderNum = 1u;
		Device = nullptr;
	}

	void FRenderRecordScheduler::BeginFrame(FFrameRenderData& InFrameData)
	{
		FrameData = &InFrameData;
		RecordedLists.SetNum(0);
	}

	void FRenderRecordScheduler::EndFrame()
	{
		RecordedLists.SetNum(0);
		FrameData = nullptr;
	}

	void FRenderRecordScheduler::ExecuteStage(const ERenderPassStageType InStage, const TArray<FRecordTask>& InTasks
		, const TFunction<void(IRCommandList*)>& InRecordSync)
	{
		(void)InStage;
		if ((!Device) || (!FrameData))
		{
			return;
		}

		// The sync list owns the stage's transitions and clears. It runs on the render thread
		// with IMMEDIATE state tracking, and it is submitted before every task list of the
		// stage, so a worker list can assume the state it was planned for.
		//
		// D3D12 needs a distinct list index per list because two lists may not share an
		// allocator before the batch is submitted. D3D11 has one immediate context and one
		// recorder, so every list there is the same wrapper and index 0 is the only valid one.
		const BOOL32 bSingleListDevice = ((Device->GetMaxRecordThreadNum() <= 1u) ? TRUE : FALSE);
		const UINT32 SyncListIndex = ((bSingleListDevice != FALSE)
			? 0u : (RHI_MAX_RECORD_THREAD_NUM + static_cast<UINT32>(InStage)));
		IRCommandList* SyncList = Device->AcquireCommandList(SyncListIndex);
		if (SyncList)
		{
			SyncList->SetRecordMode(ERHIRecordModeType::RHI_RECORD_MODE_IMMEDIATE);
			if (InRecordSync)
			{
				InRecordSync(SyncList);
			}
			SyncList->Close();
			RecordedLists.Add(SyncList);
		}

		for (UINT32 RecorderIndex = 0u; RecorderIndex < RecorderNum; RecorderIndex++)
		{
			StageLists[RecorderIndex] = Device->AcquireCommandList(RecorderIndex);
			if (StageLists[RecorderIndex])
			{
				StageLists[RecorderIndex]->SetRecordMode(ERHIRecordModeType::RHI_RECORD_MODE_EXTERNAL);
			}
		}

		CurrentTasks = &InTasks;
		NextTaskIndex.Store(0u);
		PendingWorkerNum.Store(WorkerStates.Num<UINT32>());
		StageDoneEvent.Reset();

		for (UINT32 WorkerIndex = 0u, WorkerNum = WorkerStates.Num<UINT32>(); WorkerIndex < WorkerNum; WorkerIndex++)
		{
			if (WorkerStates[WorkerIndex])
			{
				WorkerStates[WorkerIndex]->WakeEvent.Signal();
			}
		}

		FPassRecordContext MainContext;
		MainContext.CommandList	= StageLists[0];
		MainContext.RecorderIndex	= 0u;
		MainContext.FrameIndex		= FrameData->GetFrameIndex();
		MainContext.FrameData		= FrameData;
		if (MainContext.CommandList)
		{
			for (;;)
			{
				const UINT32 TaskIndex = NextTaskIndex.FetchAdd(1u);
				if (TaskIndex >= InTasks.Num<UINT32>())
				{
					break;
				}
				RecordOneTask(MainContext, InTasks[TaskIndex]);
			}
			MainContext.CommandList->Close();
		}

		if (WorkerStates.Num<UINT32>() > 0u)
		{
			StageDoneEvent.Wait();
		}

		for (UINT32 RecorderIndex = 0u; RecorderIndex < RecorderNum; RecorderIndex++)
		{
			if (StageLists[RecorderIndex])
			{
				RecordedLists.Add(StageLists[RecorderIndex]);
				StageLists[RecorderIndex] = nullptr;
			}
		}
		CurrentTasks = nullptr;
	}

	void FRenderRecordScheduler::WorkerEntry(const UINT32 InRecorderIndex)
	{
		FPassRecordContext Context;
		Context.RecorderIndex = InRecorderIndex;

		for (;;)
		{
			FRecorderState* State = WorkerStates[InRecorderIndex - 1u].get();
			if (!State)
			{
				break;
			}
			State->WakeEvent.WaitAndReset();
			if (bStopRequested.Load() != FALSE)
			{
				break;
			}
			if ((!FrameData) || (!CurrentTasks))
			{
				continue;
			}

			PE_PROFILE_SCOPE(PROFILER_SCOPE_RENDER_RECORD_WORKER);
			IRCommandList* List = StageLists[InRecorderIndex];
			Context.CommandList	= List;
			Context.FrameIndex	= FrameData->GetFrameIndex();
			Context.FrameData	= FrameData;
			Context.Instance	= nullptr;
			Context.StateCache	= FPrimitiveDrawStateCache();
			if (List)
			{
				Context.StateCache.CommandList = List;
				for (;;)
				{
					const UINT32 TaskIndex = NextTaskIndex.FetchAdd(1u);
					if (TaskIndex >= CurrentTasks->Num<UINT32>())
					{
						break;
					}
					RecordOneTask(Context, (*CurrentTasks)[TaskIndex]);
				}
				List->Close();
			}

			if (PendingWorkerNum.FetchSub(1u) == 1u)
			{
				StageDoneEvent.Signal();
			}
		}
	}

	void FRenderRecordScheduler::RecordOneTask(FPassRecordContext& InOutContext, const FRecordTask& InTask) const
	{
		if ((!InTask.Pass) || (!InTask.Instance) || (!InOutContext.CommandList))
		{
			return;
		}
		if (InOutContext.Instance != InTask.Instance)
		{
			InOutContext.Instance = InTask.Instance;
			InOutContext.StateCache = FPrimitiveDrawStateCache();
			InOutContext.StateCache.CommandList = InOutContext.CommandList;
		}
		InTask.Pass->BeginPass(InOutContext);
		InTask.Pass->RecordItems(InOutContext, InTask.Instance->Items, InTask.ItemBegin, InTask.ItemEnd);
		InTask.Pass->EndPass(InOutContext);
	}

}	// namespace PigeonEngine
