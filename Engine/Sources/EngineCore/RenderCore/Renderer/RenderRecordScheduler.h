#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <MultiThread/Atomic.h>
#include <MultiThread/Event.h>
#include <MultiThread/Thread.h>
#include <Renderer/RenderPass.h>

namespace PigeonEngine
{

	class IRRHIDevice;
	class IRCommandList;

	/*
	* One recorder's share of a stage: a half open item range of one pass instance.
	*
	* A task is the scheduling unit, not a draw. The pass interprets its items, so the same
	* task shape serves a graphics pass, a per primitive compute pass and a future ray tracing
	* pass without the scheduler knowing which one it is.
	*/
	struct FRecordTask
	{
		const FRenderPass*			Pass		= nullptr;
		const FPassViewInstance*	Instance	= nullptr;
		UINT32						ItemBegin	= 0u;
		UINT32						ItemEnd		= 0u;
	};

	/*
	* Runs one stage's tasks on the recorder threads.
	*
	* The calling thread is recorder 0 and records its share of the tasks itself; recorder
	* threads 1..N-1 are persistent EThread workers. Every recorder owns one command list
	* per stage; the lists are closed by the thread that recorded them and handed back to the
	* renderer for one batched submission.
	*
	* A stage also owns one sync list, recorded on the render thread with IMMEDIATE state
	* tracking before the worker lists. That list carries the stage's transitions and clears;
	* worker lists run with EXTERNAL state and never touch IRRHIResource::CurrentState.
	*
	* Threading primitives are the engine wrappers: EThread, EEvent, TAtomic. The task set is
	* a fixed array for the duration of one stage, so an atomic cursor is enough; TMpscQueue is
	* single consumer and does not fit a multi worker pull.
	*/
	class FRenderRecordScheduler final
	{
	public:
		FRenderRecordScheduler();
		~FRenderRecordScheduler();

		FRenderRecordScheduler(const FRenderRecordScheduler&) = delete;
		FRenderRecordScheduler& operator=(const FRenderRecordScheduler&) = delete;

	public:
		// InRecorderNum counts the calling thread. 1 means no worker threads and every task
		// runs inline, which is the D3D11 path.
		void	Initialize(IRRHIDevice* InDevice, UINT32 InRecorderNum);
		void	Shutdown();

		void	BeginFrame(FFrameRenderData& InFrameData);
		void	EndFrame();

		// Runs one stage. InRecordSync is called on the stage's sync list before any task
		// list is recorded; it applies the stage's transitions and clears.
		void	ExecuteStage(ERenderPassStageType InStage, const TArray<FRecordTask>& InTasks
			, const TFunction<void(IRCommandList*)>& InRecordSync);

		PE_NODISCARD const TArray<IRCommandList*>&	GetRecordedLists() const { return RecordedLists; }
		PE_NODISCARD UINT32							GetRecorderNum() const { return RecorderNum; }

	private:
		struct FRecorderState
		{
			UINT32	RecorderIndex	= 0u;
			EThread	Thread;
			EEvent	WakeEvent;
		};

	private:
		void	WorkerEntry(UINT32 InRecorderIndex);
		void	RecordOneTask(FPassRecordContext& InOutContext, const FRecordTask& InTask) const;

	private:
		IRRHIDevice*						Device			= nullptr;
		FFrameRenderData*					FrameData		= nullptr;
		UINT32								RecorderNum		= 1u;
		TArray<TUniquePtr<FRecorderState>>	WorkerStates;
		TAtomic<UINT32>						NextTaskIndex{ 0u };
		TAtomic<UINT32>						PendingWorkerNum{ 0u };
		EEvent								StageDoneEvent{ FALSE };
		TAtomic<BOOL8>						bStopRequested{ FALSE };
		const TArray<FRecordTask>*			CurrentTasks	= nullptr;
		IRCommandList*						StageLists[RHI_MAX_RECORD_THREAD_NUM] = {};
		TArray<IRCommandList*>				RecordedLists;

	};

}	// namespace PigeonEngine
