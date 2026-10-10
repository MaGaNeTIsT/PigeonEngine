#include "GameThreadCommandQueue.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<EGameThreadCommandQueue, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EGameThreadCommandQueue::EGameThreadCommandQueue()
		: bIsShutDown(TRUE)		// closed until the engine opens its lifetime
	{
	}
	EGameThreadCommandQueue::~EGameThreadCommandQueue()
	{
	}

	void EGameThreadCommandQueue::Initialize()
	{
		// A fresh lifetime starts empty and accepting.
		Commands.Clear();
		bIsShutDown.Store(FALSE);
	}
	void EGameThreadCommandQueue::ShutDown()
	{
		/*
		* Queued commands are dropped, not run: teardown happens after the objects they were meant
		* for have started going away, and a command that outlived its target is exactly the case this
		* class cannot check for.
		*/
		bIsShutDown.Store(TRUE);
		Commands.Clear();
	}
	void EGameThreadCommandQueue::Uninit()
	{
		ShutDown();
	}

	void EGameThreadCommandQueue::EnqueueCommand(EVoidFunctionType InCommand)
	{
		if ((!InCommand) || (bIsShutDown.Load() != FALSE))
		{
			return;
		}
		Commands.Enqueue(static_cast<EVoidFunctionType&&>(InCommand));
	}

	void EGameThreadCommandQueue::DoCommands()
	{
		// A command may enqueue another one - a load completion that starts the next load, a mutation
		// that reports something back - so the loop keeps draining until a pass finds nothing. The
		// bound is what keeps a command that enqueues itself forever from hanging the frame.
		constexpr UINT32 MaxPassNum = 8u;
		for (UINT32 PassIndex = 0u; PassIndex < MaxPassNum; PassIndex++)
		{
			const UINT32 CommandNum = Commands.DequeueAll([](EVoidFunctionType&& InCommand) -> void
			{
				InCommand();
			});
			if (CommandNum == 0u)
			{
				break;
			}
		}
	}

	UINT32 EGameThreadCommandQueue::GetPendingCommandNum() const
	{
		return static_cast<UINT32>(Commands.GetPendingCount());
	}

}	// namespace PigeonEngine
