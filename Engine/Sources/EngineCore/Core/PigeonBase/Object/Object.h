#pragma once

#include <CoreMinimal.h>
#include <Base/Delegate/Delegate.h>


namespace PigeonEngine
{
	class CJsonObject;
	class PWorld;

	// Destruction is a fixed, engine-wide sequence. Subclasses customize the two
	// protected hooks, never the orchestrator:
	//   Destroy() -> (was in scene?) RemovedFromScene() -> OnDestroy() ->
	//   (was initialized?) Uninit() -> OnPreDelete() -> delete this
	// RemovedFromScene runs first so UserEndPlay sees the whole object; Uninit runs
	// before OnPreDelete so a component can still reach its owner while uninitializing.
	enum class EObjectLifecycleStateType : UINT8
	{
		CREATED = 0,
		INITIALIZED,
		DESTROYING,
		DESTROYED,
	};

	// Ordered tick bands. Lexicographic (group, priority, registration order) is the
	// full order an actor's components tick in - build that into the actor's cached tick
	// list, so the order is identical across runs.
	enum class ETickGroupType : UINT8
	{
		PRE_PHYSICS = 0,
		PHYSICS,
		GAME_PLAY,
		POST_PHYSICS,
	};

	class PObject : public EObjectBase
	{
	public:
		virtual void	Init() = 0;
		virtual void	Uninit();
		virtual void	Tick(FLOAT deltaTime) = 0;
#if ENGINE_EDITOR_ONLY
		virtual void	EditorTick(FLOAT deltaTime) = 0;
#endif
		CLASS_VIRTUAL_NOCOPY_BODY(PObject)

	public:
		void SetIsTickable(const BOOL32& bTickable);
		BOOL32 IsTickable() const;
		BOOL32 IsInitialized() const;
		BOOL32 IsAddedToScene() const;
		// Deferred-destruction mark, set by PWorld::DestroyActor/DestroyComponent when
		// the object enters the pending-destroy queue. The object stays alive until the
		// queue flushes, but tick and lookup paths treat it as already gone.
		void MarkPendingDestroy();
		BOOL32 IsPendingDestroy() const;
		// Tick band and priority. Within an actor, components tick in (group, priority,
		// registration) order; both default to the gameplay band at priority 0.
		virtual void SetTickGroup(const ETickGroupType& InGroup);
		PE_NODISCARD ETickGroupType GetTickGroup() const;
		virtual void SetTickPriority(const INT32& InPriority);
		PE_NODISCARD INT32 GetTickPriority() const;
	private:
		BOOL32 bCanTick = FALSE;
		ETickGroupType TickGroup = ETickGroupType::GAME_PLAY;
		INT32 TickPriority = 0;
		// Scene membership is orthogonal to the destroy phase, so it stays a plain flag:
		// an object can be DESTROYING while still "in scene" until RemovedFromScene ran.
		BOOL32 bAddedToScene = FALSE;
		EObjectLifecycleStateType LifecycleState = EObjectLifecycleStateType::CREATED;
		BOOL32 bPendingDestroy = FALSE;
	public:
		// Non-virtual teardown orchestrator (see EObjectLifecycleStateType above).
		// Idempotent: entering Destroy on an object already DESTROYING or DESTROYED is a
		// no-op, which is what makes a component reached through two owners, or a child
		// hit again by a snapshot loop, safe. Call sites are unchanged; subclasses
		// override OnDestroy/OnPreDelete instead of Destroy.
		void Destroy();

	protected:
		// Tear down objects this one owns (child actors, child components). Runs after
		// RemovedFromScene and before Uninit.
		virtual void OnDestroy();
		// Detach from the owner's lists as the final step before the memory goes away.
		// Runs after Uninit, so a component uninitializing can still use its owner.
		virtual void OnPreDelete();

	public:
		// Engine-wide notification raised by the destroy orchestrator after OnPreDelete
		// and before the memory goes away: the object is still complete, so global
		// observers (the message bus purging that object's subscriptions, debug
		// trackers) can drop their references safely. Fires for every destroyed PObject,
		// so listeners must be cheap.
		static TDelegateBroadcast<void (PObject*)> OnObjectPreDelete;

	public:
		virtual TSharedPtr<CJsonObject> Serialize();


	public:
		virtual PWorld* GetWorld() const;
	protected:
		virtual void SetWorld(PWorld* NewWorld);
		void SetInitialized(const BOOL32& bInInitialized);
		void SetAddedToScene(const BOOL32& bInAddedToScene);
	private:
		PWorld* MyWorld = nullptr;

	protected:
		// for frame, 
		virtual void BeginAddedToScene(PWorld* World);
		virtual void RemovedFromScene();
	};

};
