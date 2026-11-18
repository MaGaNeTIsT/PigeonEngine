#pragma once

#include <CoreMinimal.h>


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
	private:
		BOOL32 bCanTick = FALSE;
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
