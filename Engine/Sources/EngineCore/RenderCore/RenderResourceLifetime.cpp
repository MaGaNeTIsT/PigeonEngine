#include "RenderResourceLifetime.h"

#include <Main/MainManager.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	FRenderResourceRef RRenderResourceRegistry::ReserveSlot()
	{
		EScopedLock ScopedLock(TableMutex);
		FRenderResourceRef Result;
		if (FreeSlots.Num<UINT32>() > 0u)
		{
			const UINT32 FreeIndex = FreeSlots.Num<UINT32>() - 1u;
			Result.Slot = FreeSlots[FreeIndex];
			FreeSlots.RemoveAt(FreeIndex);
		}
		else
		{
			Result.Slot = Slots.Num<UINT32>();
			Slots.AddDefaultGetRef();
		}
		FSlot& Slot = Slots[Result.Slot];
		Result.Generation = Slot.Generation;
		Slot.Reserved = TRUE;
		ReservedNum++;
		return Result;
	}

	BOOL32 RRenderResourceRegistry::Commit(const FRenderResourceRef InRef, RRenderResourceInterface* InResource)
	{
		EScopedLock ScopedLock(TableMutex);
		if ((InRef.Slot >= Slots.Num<UINT32>()) || (!InResource))
		{
			return FALSE;
		}
		FSlot& Slot = Slots[InRef.Slot];
		if ((!Slot.Reserved) || (Slot.Generation != InRef.Generation))
		{
			return FALSE;
		}
		Slot.Resource = InResource;
		Slot.Reserved = FALSE;
		if (ReservedNum > 0u)
		{
			ReservedNum--;
		}
		LiveResourceNum++;
		return TRUE;
	}

	BOOL32 RRenderResourceRegistry::IsReservationPending(const FRenderResourceRef InRef) const
	{
		EScopedLock ScopedLock(TableMutex);
		if (InRef.Slot >= Slots.Num<UINT32>())
		{
			return FALSE;
		}
		const FSlot& Slot = Slots[InRef.Slot];
		return ((Slot.Reserved) && (Slot.Generation == InRef.Generation)) ? TRUE : FALSE;
	}

	FRenderResourceRef RRenderResourceRegistry::Register(RRenderResourceInterface* InResource)
	{
		if (!InResource)
		{
			return FRenderResourceRef();
		}
		EScopedLock ScopedLock(TableMutex);
		FRenderResourceRef Result;
		if (FreeSlots.Num<UINT32>() > 0u)
		{
			const UINT32 FreeIndex = FreeSlots.Num<UINT32>() - 1u;
			Result.Slot = FreeSlots[FreeIndex];
			FreeSlots.RemoveAt(FreeIndex);
		}
		else
		{
			Result.Slot = Slots.Num<UINT32>();
			Slots.AddDefaultGetRef();
		}
		FSlot& Slot = Slots[Result.Slot];
		// The generation survives the slot going free, which is what makes a reference
		// taken before the free stop matching the resource that reuses the slot.
		Result.Generation = Slot.Generation;
		Slot.Resource = InResource;
		LiveResourceNum++;
		return Result;
	}

	void RRenderResourceRegistry::Unregister(const FRenderResourceRef InRef)
	{
		EScopedLock ScopedLock(TableMutex);
		if (InRef.Slot >= Slots.Num<UINT32>())
		{
			return;
		}
		FSlot& Slot = Slots[InRef.Slot];
		if (Slot.Generation != InRef.Generation)
		{
			return;
		}
		if ((!Slot.Resource) && (!Slot.Reserved))
		{
			return;
		}
		// A reserved-but-uncommitted slot is released the same way, so a reservation does
		// not leak when the owner goes away before the creation command ran.
		const BOOL32 bWasReserved = Slot.Reserved;
		Slot.Resource = nullptr;
		Slot.Reserved = FALSE;
		Slot.Generation++;
		if (LiveResourceNum > 0u)
		{
			LiveResourceNum--;
		}
		if (bWasReserved && (ReservedNum > 0u))
		{
			ReservedNum--;
		}
		FreeSlots.Add(InRef.Slot);
	}

	RRenderResourceInterface* RRenderResourceRegistry::Resolve(const FRenderResourceRef InRef) const
	{
		EScopedLock ScopedLock(TableMutex);
		if (InRef.Slot >= Slots.Num<UINT32>())
		{
			return nullptr;
		}
		const FSlot& Slot = Slots[InRef.Slot];
		if (Slot.Generation != InRef.Generation)
		{
			return nullptr;
		}
		return Slot.Resource;
	}

	FRenderResourceRef RRenderResourceRegistry::FindReference(const RRenderResourceInterface* InResource) const
	{
		EScopedLock ScopedLock(TableMutex);
		for (UINT32 SlotIndex = 0u, SlotNum = Slots.Num<UINT32>(); SlotIndex < SlotNum; SlotIndex++)
		{
			if (Slots[SlotIndex].Resource == InResource)
			{
				FRenderResourceRef Result;
				Result.Slot = SlotIndex;
				Result.Generation = Slots[SlotIndex].Generation;
				return Result;
			}
		}
		return FRenderResourceRef();
	}

	UINT32 RRenderResourceRegistry::DestroyAllResources()
	{
		// Release and delete through the interface: each concrete resource returns its own
		// GPU object, so the table never has to know about the device or the back-end. The
		// table is emptied under its lock and the releases run outside it, because they call
		// into the device and that must not happen while a producer waits on the lock to
		// reserve a slot.
		TArray<RRenderResourceInterface*> ReleasedResources;
		{
			EScopedLock ScopedLock(TableMutex);
			for (UINT32 SlotIndex = 0u, SlotNum = Slots.Num<UINT32>(); SlotIndex < SlotNum; SlotIndex++)
			{
				if (Slots[SlotIndex].Resource)
				{
					ReleasedResources.Add(Slots[SlotIndex].Resource);
				}
			}
			Slots.Empty();
			FreeSlots.Empty();
			LiveResourceNum = 0u;
			ReservedNum = 0u;
		}
		for (UINT32 ResourceIndex = 0u, ResourceNum = ReleasedResources.Num<UINT32>(); ResourceIndex < ResourceNum; ResourceIndex++)
		{
			RRenderResourceInterface* Resource = ReleasedResources[ResourceIndex];
			Resource->ReleaseRenderResource();
			delete Resource;
		}
		return ReleasedResources.Num<UINT32>();
	}

	RRenderResourceManager::RRenderResourceManager()
	{
	}
	RRenderResourceManager::~RRenderResourceManager()
	{
		// Nothing is reported here on purpose: this runs at static destruction, where the log
		// singleton may already be gone. ShutDown is the place that reports residue, and it
		// runs while the engine is still alive.
	}

	void RRenderResourceManager::Initialize()
	{
		DrainedFrameIndex.Store(0u);
	}

	void RRenderResourceManager::ShutDown()
	{
		/*
		* Final drain. Preconditions, both owned by the caller (EMainManager::ShutDown):
		*   - the render thread has been stopped, so this thread is the only one touching the
		*     queue and the registry from here on;
		*   - the device is still alive, because the commands below call into it.
		*
		* Order matters: the retired objects are released first because releasing one enqueues
		* its resource destroys, and a command must never run inside the walk that produced it -
		* DoCommands detaches its batch before running it, so a command that enqueues is served
		* by the next pass instead of invalidating the current one.
		*/
		constexpr UINT32 MaxPassNum = 8u;
		for (UINT32 PassIndex = 0u; PassIndex < MaxPassNum; PassIndex++)
		{
			FlushRetiredObjects(TRUE);
			if (Commands.GetQueueCount() == 0u)
			{
				break;
			}
			DoAllCommands();
		}

		// Backstop: whatever the table still holds belongs to an owner that was never torn
		// down. Its count is the leak report.
		const UINT32 DestroyedNum = Registry.DestroyAllResources();
		CheckShutDownResidue(DestroyedNum);
	}

	void RRenderResourceManager::Uninit()
	{
		// Nothing survives an engine lifetime boundary, so the repeatable path has to leave
		// exactly the state a fresh manager starts in. The residue check is repeated here
		// because an Uninit + Init cycle must not inherit anything from the previous run.
		ShutDown();
		CheckRetiredObjectResidue();
	}

	void RRenderResourceManager::EnqueueDestroy(RRenderResourceInterface* InResource)
	{
		if (!InResource)
		{
			return;
		}
		EnqueueDestroyRef(Registry.FindReference(InResource));
	}

	void RRenderResourceManager::EnqueueDestroyRef(const FRenderResourceRef InRef)
	{
		if (!InRef.IsValid())
		{
			return;
		}
		EnqueueCommand([this, InRef]() -> void
		{
			// Resolve, release and unregister in one command: the table belongs to whoever
			// drains the queue, so callers never touch it directly.
			RRenderResourceInterface* Resource = Registry.Resolve(InRef);
			if (Resource)
			{
				Resource->ReleaseRenderResource();
				delete Resource;
			}
			Registry.Unregister(InRef);
		});
	}

	void RRenderResourceManager::DoCommands()
	{
		/*
		* The one queue holds every create / destroy request, from any thread, whether or not a
		* frame boundary passed since it was recorded: a request has no frame to belong to, it
		* only has to run before the draw that reads its result. Draining at the top of the frame
		* keeps that promise, and draining here rather than at the end also means a resource
		* requested during the game thread's tick is already alive for this frame's scene
		* commands.
		*
		* The loop covers commands that enqueue on their own behalf. The bound is what keeps a
		* command that enqueues itself forever from hanging the frame.
		*/
		constexpr UINT32 MaxPassNum = 8u;
		for (UINT32 PassIndex = 0u; PassIndex < MaxPassNum; PassIndex++)
		{
			if (Commands.DoCommands() == 0u)
			{
				break;
			}
		}
		DrainedFrameIndex.FetchAdd(1u);
	}

	void RRenderResourceManager::DoAllCommands()
	{
		// Same work as DoCommands: the single queue has no slots left to distinguish, and the
		// callers that want the shutdown intent stated call this name.
		DoCommands();
	}

	void RRenderResourceManager::RetireObject(EObjectBase* InObject)
	{
		if (!InObject)
		{
			return;
		}
		// Retiring the same object twice would delete it twice: the second entry would free
		// memory the first one already returned, and the recycling allocator would have handed
		// it to something else by then. That is a caller bug rather than a state worth
		// representing, so the duplicate is dropped and reported. Object is the only field
		// compared on purpose - a parked entry is one the manager has not released yet.
		EScopedLock ScopedLock(RetiredObjectsMutex);
		for (UINT32 Index = 0u, Num = RetiredObjects.Num<UINT32>(); Index < Num; Index++)
		{
			if (RetiredObjects[Index].Object == InObject)
			{
#if _EDITOR_ONLY
				PE_LOG_ERROR(EString(PE_TEXT("[RRenderResourceManager] '")) + InObject->GetDebugName()
					+ EString(PE_TEXT("' was retired twice; the second request is dropped.")));
#endif
				return;
			}
		}
		FRetiredObject Entry;
		Entry.Object = InObject;
		// Stamped with the frame count reached when the object was parked. A command queued
		// before this point is still allowed to read it.
		Entry.RetireFrameIndex = DrainedFrameIndex.Load();
		RetiredObjects.Add(Entry);
#if _EDITOR_ONLY
		// The name is logged here, where the object is still known to be alive: the release
		// below logs only the frame stamp, because a damaged entry must not be dereferenced to
		// find out what it was.
		PE_LOG_LOG(EString(PE_TEXT("[RRenderResourceManager] Retired '")) + InObject->GetDebugName()
			+ EString(PE_TEXT("' at frame ")) + ToString(DrainedFrameIndex.Load()) + EString(PE_TEXT(".")));
#endif
	}

	void RRenderResourceManager::RetireRelease(EVoidFunctionType InRelease)
	{
		if (!InRelease)
		{
			return;
		}
		FRetiredObject Entry;
		Entry.Release = new EVoidFunctionType(InRelease);
		// Same stamp as RetireObject: a command queued before this point is still allowed to
		// read what is being released.
		Entry.RetireFrameIndex = DrainedFrameIndex.Load();
		EScopedLock ScopedLock(RetiredObjectsMutex);
		RetiredObjects.Add(Entry);
	}

	void RRenderResourceManager::FlushRetiredObjects(const BOOL32 InIsForceAll)
	{
		/*
		* Two phases on purpose. The entries that are due are taken out of the list under the
		* lock, and the releases run outside it: a release can park something of its own (a
		* shader instance owns buffers that are retired the same way), which would deadlock
		* against a non-recursive lock held across the call.
		*/
		TArray<FRetiredObject> DueObjects;
		{
			EScopedLock ScopedLock(RetiredObjectsMutex);
			// Reverse so RemoveAt does not shift the entries that are still waiting.
			for (INT32 Index = static_cast<INT32>(RetiredObjects.Num<UINT32>()) - 1; Index >= 0; Index--)
			{
				const FRetiredObject& Entry = RetiredObjects[static_cast<UINT32>(Index)];
				if ((!InIsForceAll) && (DrainedFrameIndex.Load() <= Entry.RetireFrameIndex))
				{
					continue;
				}
				DueObjects.Add(Entry);
				RetiredObjects.RemoveAt(static_cast<UINT32>(Index));
			}
		}
		for (UINT32 Index = 0u, Num = DueObjects.Num<UINT32>(); Index < Num; Index++)
		{
			const FRetiredObject& Entry = DueObjects[Index];
#if _EDITOR_ONLY
			// Only what a damaged entry can still be trusted for. The object's name was logged
			// when it was parked, which is also where the pair it prints here can be looked up.
			PE_LOG_LOG(EString(PE_TEXT("[RRenderResourceManager] Releasing a retired object parked at frame "))
				+ ToString(Entry.RetireFrameIndex) + EString(PE_TEXT(".")));
#endif
			if (Entry.Object)
			{
				// Deleted through EObjectBase, whose destructor is virtual, so the concrete asset
				// releases everything it owns on the way out.
				delete Entry.Object;
			}
			else if (Entry.Release)
			{
				(*Entry.Release)();
				delete Entry.Release;
			}
		}
	}

	void RRenderResourceManager::CheckEngineLifetime()
	{
#if _EDITOR_ONLY
		const BOOL8 bInLifetime = ((EMainManager::GetManagerSingleton()->IsEngineLifetimeActive() != 0) ? TRUE : FALSE);
		Check(bInLifetime, (ENGINE_RENDER_CORE_ERROR)
			, (PE_TEXT("[RRenderResourceManager] A render resource was created or retained outside the engine lifetime. A file scope object touching an asset before EMainManager::Initialize is the usual cause.")));
#endif	// _EDITOR_ONLY
	}

	void RRenderResourceManager::CheckShutDownResidue(const UINT32 InDestroyedNum)
	{
#if _EDITOR_ONLY
		EString LogText(PE_TEXT("[RRenderResourceManager] Shutdown released "));
		LogText = LogText + ToString(InDestroyedNum) + PE_TEXT(" resource(s) the registry still held.");
		PE_LOG_LOG(LogText);
		Check((InDestroyedNum == 0u), (ENGINE_RENDER_CORE_ERROR)
			, (PE_TEXT("[RRenderResourceManager] Shutdown found live render resources. An asset owner was not torn down: register its manager with EMainManager and release it in ShutDown.")));
#endif	// _EDITOR_ONLY
	}

	void RRenderResourceManager::CheckRetiredObjectResidue()
	{
#if _EDITOR_ONLY
		Check((GetRetiredObjectNum() == 0u), (ENGINE_RENDER_CORE_ERROR)
			, (PE_TEXT("[RRenderResourceManager] Retired objects were never released. RetireObject callers must go through a manager that EMainManager shuts down.")));
#endif	// _EDITOR_ONLY
	}

} // namespace PigeonEngine
