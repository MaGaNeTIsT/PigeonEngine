#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderCommon.h>
#include <Base/RegisterBase.h>
#include <Core/Base/DataStructure/ObjectBase.h>
#include <MultiThread/Atomic.h>
#include <MultiThread/MpscQueue.h>
#include <MultiThread/Mutex.h>

namespace PigeonEngine
{

	/*
	* Render thread resource lifetime (phase 2).
	*
	* The two tier model says a GPU object is created and destroyed on the render thread,
	* while whoever loaded the asset owns it on the CPU side. Three pieces make that work:
	*
	*   FRenderResourceRef   what the owner thread keeps. Opaque: a slot plus the generation
	*                        the slot had when it was handed out, so a reference to a
	*                        destroyed-and-reused slot stops resolving instead of aliasing
	*                        an unrelated resource.
	*   RRenderResourceRegistry
	*                        the render thread side table that turns a reference back into
	*                        a pointer. Every conversion happens on the render thread.
	*   RRenderResourceCommandQueue
	*                        how any thread asks for a create or a destroy. Calls are pushed
	*                        into a lock free multi producer queue and executed by the render
	*                        thread at the top of a frame, before the scene commands that
	*                        may refer to their result.
	*
	* Threading: the command queue may be written from any thread at any time, including the
	* render thread itself and while the render thread is draining - that is what the queue is
	* for. The registry belongs to the thread that drains (the render thread while the engine
	* runs, the game thread during final shutdown, never both), with ReserveSlot as the one
	* entry point another thread may call because a reservation has to be visible before the
	* object it stands for exists. The retired object list is written by whichever thread
	* releases an asset and drained by the game thread, so it carries its own lock.
	*/

	struct FRenderResourceRef
	{
	public:
		static constexpr UINT32 INVALID_SLOT = 0xffffffffu;
	public:
		PE_NODISCARD BOOL32	IsValid() const noexcept { return ((Slot != INVALID_SLOT) ? TRUE : FALSE); }
		void				Reset() noexcept
		{
			Slot = INVALID_SLOT;
			Generation = 0u;
		}
		PE_NODISCARD BOOL32	operator==(const FRenderResourceRef& Other) const noexcept
		{
			return (((Slot == Other.Slot) && (Generation == Other.Generation)) ? TRUE : FALSE);
		}
		PE_NODISCARD BOOL32	operator!=(const FRenderResourceRef& Other) const noexcept
		{
			return ((!((*this) == Other)) ? TRUE : FALSE);
		}
	public:
		UINT32	Slot		= INVALID_SLOT;
		UINT32	Generation	= 0u;
	};

	/*
	* Create / destroy requests waiting for the render thread. Any thread may enqueue; only the
	* thread that drains may run them.
	*
	* There is one queue rather than a front / back pair, because a request has no frame to
	* belong to: "build this GPU object" and "give this one back" only have to run before the
	* draw that uses them, which draining at the top of the frame already guarantees. Removing
	* the pair is also what makes the queue correct for a second producer - the pair only ever
	* made the game thread's write safe because the game thread was assumed to be alone.
	*/
	class RRenderResourceCommandQueue final
	{
	public:
		// Any thread.
		template<typename _TFunctionType>
		void	EnqueueCommand(_TFunctionType InFunction)
		{
			RawCommands.Enqueue(EVoidFunctionType(InFunction));
		}
		// Draining thread: runs every command that is queued right now and reports how many ran.
		// A command that enqueues another one is not part of this batch; the caller's drain loop
		// runs it on its next pass. The count is what that loop decides on, and it is exact
		// unlike GetQueueCount.
		UINT32	DoCommands()
		{
			return RawCommands.DequeueAll([](EVoidFunctionType&& InCommand) -> void
			{
				InCommand();
			});
		}
		// Draining thread: drops whatever is queued without running it.
		void	EmptyQueue()
		{
			RawCommands.Clear();
		}
		// Diagnostics: how many commands are waiting, as last seen by this thread.
		PE_NODISCARD UINT32	GetQueueCount() const { return static_cast<UINT32>(RawCommands.GetPendingCount()); }
	private:
		TMpscQueue<EVoidFunctionType>	RawCommands;

	public:
		RRenderResourceCommandQueue() {}
		~RRenderResourceCommandQueue() {}

		CLASS_REMOVE_COPY_BODY(RRenderResourceCommandQueue)

	};

	/*
	* Render tier resource table. A slot that is freed keeps its generation number and is
	* handed out again with the next one, so a stale FRenderResourceRef resolves to null
	* instead of to whatever took the slot.
	*
	* Threading: the whole table is guarded, and for a reason that is easy to miss - ReserveSlot
	* runs on the thread that owns the asset (it has to hand out the reference before the object
	* it stands for exists) while every other entry point runs on the draining thread, and
	* reserving a slot can GROW the slot array. A resize moves the entries, so an unguarded
	* Resolve on the render thread could read storage the game thread just freed. Every entry
	* point therefore takes the lock; each critical section is a handful of array operations.
	*
	* Two step registration is what lets an asset hold its reference from the moment it is
	* loaded, while the object itself is still built on the render thread:
	*   ReserveSlot  owner thread, takes the lock briefly, returns a reference whose slot has
	*                no object yet (Resolve returns null until it is committed)
	*   Commit       queue draining thread, publishes the object into the reserved slot
	*/
	class RRenderResourceRegistry final
	{
	public:
		// Owner thread: claims a slot for an object that does not exist yet.
		FRenderResourceRef			ReserveSlot();
		// Queue draining thread: publishes InResource into a reserved slot. FALSE when the
		// reference is stale (the slot was already handed back).
		BOOL32						Commit(const FRenderResourceRef InRef, RRenderResourceInterface* InResource);
		// Queue draining thread: TRUE while InRef still names an outstanding reservation, which
		// means the object it was handed out for has neither arrived nor been given back.
		//
		// A queued creation command uses it to skip an owner whose reservation is gone: reading
		// such an owner is a use after free. It is a guard rather than the release rule - an
		// owner that is dropped while its command is queued is released through the manager's
		// retirement, which is what keeps the memory alive for the command to read.
		PE_NODISCARD BOOL32			IsReservationPending(const FRenderResourceRef InRef) const;
		FRenderResourceRef			Register(RRenderResourceInterface* InResource);
		void						Unregister(const FRenderResourceRef InRef);
		RRenderResourceInterface*	Resolve(const FRenderResourceRef InRef) const;
		// Reference currently published for InResource, or an invalid one when the object
		// was never registered.
		PE_NODISCARD FRenderResourceRef	FindReference(const RRenderResourceInterface* InResource) const;
		PE_NODISCARD UINT32			GetLiveResourceNum() const
		{
			EScopedLock ScopedLock(TableMutex);
			return LiveResourceNum;
		}
		PE_NODISCARD UINT32			GetReservedNum() const
		{
			EScopedLock ScopedLock(TableMutex);
			return ReservedNum;
		}
		/*
		* Releases every resource the table still holds and drops the whole table. This is the
		* shutdown backstop: anything left here belongs to an owner that was never torn down,
		* so the count it returns is a leak report. Reserved slots whose object never arrived
		* are discarded without touching them - no object was ever built for them.
		*
		* Each resource's own ReleaseRenderResource() is what returns its GPU object; the
		* table never talks to the device. The releases run outside the table lock, because
		* they call into the device and may not be held against a producer's reservation.
		*/
		UINT32						DestroyAllResources();
	private:
		struct FSlot
		{
			RRenderResourceInterface*	Resource	= nullptr;
			UINT32						Generation	= 0u;
			// TRUE while the slot is reserved for an object that has not been published yet;
			// such a slot must not be handed out to a second reservation.
			BOOL32						Reserved	= FALSE;
		};
	private:
		TArray<FSlot>	Slots;
		TArray<UINT32>	FreeSlots;
		UINT32			LiveResourceNum = 0u;
		UINT32			ReservedNum = 0u;
		// Guards the whole table, including the counters. Every entry point takes it.
		mutable EMutex	TableMutex;

	public:
		RRenderResourceRegistry() {}
		~RRenderResourceRegistry() {}

		CLASS_REMOVE_COPY_BODY(RRenderResourceRegistry)

	};

	/*
	* Process wide owner of the registry and the command queue. Any thread writes the queue; the
	* render thread drains it and owns the registry while the engine runs. During final shutdown
	* the render thread is already stopped, so the game thread drains and owns the registry until
	* the device goes away.
	*/
	class RRenderResourceManager final : public EManagerBase
	{
	public:
		virtual void	Initialize() override;
		virtual void	ShutDown() override;
		virtual void	Uninit() override;
	public:
		// Records a create / destroy request for whoever drains the queue. Any thread may call
		// this, at any time.
		template<typename _TFunctionType>
		void	EnqueueCommand(_TFunctionType InFunction)
		{
			CheckEngineLifetime();
			Commands.EnqueueCommand(InFunction);
		}
		// Frees a render tier resource and hands its slot back. Takes ownership of InResource.
		void	EnqueueDestroy(RRenderResourceInterface* InResource);
		// Same, addressed by reference. This is what an asset uses: it drops its own copy of
		// the reference immediately and lets the draining thread resolve, release and unregister
		// in one command, so the table never has to be touched from the owner's thread.
		// A reference whose creation command is still queued is unregistered as well, which
		// hands the reservation back instead of leaking it.
		void	EnqueueDestroyRef(const FRenderResourceRef InRef);

		// Render thread: runs the queued create / destroy calls. Draining before the scene
		// commands is what makes a resource created this frame visible to the proxies that
		// were added in the same frame.
		void	DoCommands();

		/*
		* Shutdown drain: runs every queued command on the calling thread, including the ones
		* the teardown of a retired object adds while it runs. DoCommands already repeats until
		* the queue stays empty, so this is the same work with the shutdown intent stated.
		*
		* Callers must have stopped the render thread and must still own a live device: the
		* commands call into IRRHIDevice.
		*/
		void	DoAllCommands();
	public:
		/*
		* A retired object cannot be released the moment its owner drops it: a queued creation
		* command may still read the asset's cold data. RetireObject parks it instead, and
		* FlushRetiredObjects releases it once the queue has drained a frame that started after
		* the parking - the point where no queued command can still reach it.
		*
		* Every asset derives from EObjectBase, so one pointer type covers the whole tree and
		* the virtual destructor does the releasing.
		*
		* InIsForceAll is the shutdown form: no frame will ever come, so the parking reason no
		* longer applies and everything waiting is released at once.
		*/
		void	RetireObject(EObjectBase* InObject);
		/*
		* The same parking for something that is not an EObjectBase: the caller hands over how
		* to release it instead. A shader instance is what this exists for - its constant buffers
		* are built by a queued command that reads its parameter mirrors, so an instance dropped
		* right after construction must not be freed before that command ran.
		*/
		void	RetireRelease(EVoidFunctionType InRelease);
		void	FlushRetiredObjects(const BOOL32 InIsForceAll = FALSE);
		PE_NODISCARD UINT32	GetDrainedFrameIndex() const { return DrainedFrameIndex.Load(); }
		PE_NODISCARD UINT32	GetRetiredObjectNum() const
		{
			EScopedLock ScopedLock(RetiredObjectsMutex);
			return (RetiredObjects.Num<UINT32>());
		}
	public:
		PE_NODISCARD RRenderResourceRegistry&		GetRegistry() { return Registry; }
		PE_NODISCARD const RRenderResourceRegistry&	GetRegistry() const { return Registry; }
	public:
		/*
		* Assertion helper for the ownership rule: a render resource must not outlive
		* EMainManager, so every entry point that can create or retain one checks that the
		* engine is inside its lifetime. This does not change any behaviour - the release path
		* is the same either way - it only turns a design violation into a located assertion
		* instead of a crash. A file scope object touching an asset before Main starts is the
		* case it is meant to catch.
		*/
		static void	CheckEngineLifetime();
	private:
		// Editor only leak reports: a non zero count means an owner was never torn down.
		static void	CheckShutDownResidue(const UINT32 InDestroyedNum);
		void		CheckRetiredObjectResidue();
	private:
		struct FRetiredObject
		{
			EObjectBase*		Object				= nullptr;
			// Set instead of Object when what was parked is not an EObjectBase. Held by pointer
			// on purpose: the list is shifted with a raw memory move, so an entry has to stay
			// trivially copyable, which a std::function is not.
			EVoidFunctionType*	Release				= nullptr;
			UINT32				RetireFrameIndex	= 0u;
		};
	private:
		RRenderResourceRegistry		Registry;
		RRenderResourceCommandQueue	Commands;
		// Frames the render thread has finished draining, counted since start up. Written by the
		// draining thread, read by whoever retires an object to decide when it may be released.
		TAtomic<UINT32>				DrainedFrameIndex;
		// Written by any thread that releases an asset, drained by the game thread. The lock is
		// what lets a load on a future load thread retire an asset while the game thread flushes;
		// a flush takes the entries it is about to release out under the lock and runs them
		// outside it, because a release may retire something of its own.
		TArray<FRetiredObject>		RetiredObjects;
		// Mutable because the const accessors (GetRetiredObjectNum) read the same list the
		// retiring thread appends to.
		mutable EMutex				RetiredObjectsMutex;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(RRenderResourceManager)

	};

} // namespace PigeonEngine
