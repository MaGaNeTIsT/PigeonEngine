#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Base/RegisterBase.h>
#include <Core/Base/DataStructure/ObjectBase.h>
#include <MultiThread/Atomic.h>
#include <MultiThread/MpscQueue.h>

namespace PigeonEngine
{

	/*
	* GameThreadCommandQueue - the channel from any thread back into the game thread.
	*
	* The mirror image of the render resource command queue, and an instance of the same MPSC
	* template: any thread records a command, and the game thread runs them all at one point of its
	* frame (EMainManager::Update, right after the frame's asset loads). It exists because a load can
	* finish on the load thread - or, once that is a dedicated thread, on a thread that is not the
	* game thread at all - while the consequence of a load is very often a game side mutation: set
	* this component's asset, mark its render state dirty, start the next load.
	*
	* Contract, and the second half is the load bearing one:
	*   - Any thread may enqueue at any time, including the game thread itself.
	*   - A command must NOT outlive what it touches. The queue carries no ownership and is not told
	*     what a command refers to, so a command that captured a raw pointer to a game object is a use
	*     after free waiting for that object to die first. Prefer the form that cannot go wrong: let
	*     the owner keep the pending work itself (PSceneComponent::AddDeferredApply) and use this
	*     queue for commands whose target outlives them - a manager, a singleton, a process wide
	*     cache - or whose target is not a game object at all.
	*/
	class EGameThreadCommandQueue final : public EManagerBase
	{
	public:
		virtual void	Initialize() override;
		virtual void	ShutDown() override;
		virtual void	Uninit() override;
	public:
		// Any thread.
		void	EnqueueCommand(EVoidFunctionType InCommand);
		/*
		* Game thread: runs every command that is queued right now, plus the ones a command enqueues
		* while it runs. A command that enqueues belongs to the next pass, so the batch being walked
		* cannot grow under itself.
		*/
		void	DoCommands();
		// Diagnostics: commands queued and not yet drained.
		PE_NODISCARD UINT32	GetPendingCommandNum() const;
	private:
		// Producers of every thread push here; only the game thread drains.
		TMpscQueue<EVoidFunctionType>	Commands;
		// TRUE outside the engine lifetime: commands are refused rather than queued for a drain that
		// will never come.
		TAtomic<BOOL32>					bIsShutDown;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EGameThreadCommandQueue)

	};

}	// namespace PigeonEngine
