#include "FrameworkSanityCheck.h"

#if ENGINE_EDITOR_ONLY

#include "PigeonBase/Object/Actor.h"
#include "PigeonBase/Object/World/World.h"
#include "PigeonBase/Object/Component/ActorComponent.h"
#include "PigeonBase/Object/Component/SceneComponent.h"
#include "PigeonBase/Object/Component/Primitive/StaticMeshComponent.h"
#include "PigeonBase/Object/Controller/Controller.h"
#include "HID/Input/Input.h"
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{
	// An actor whose UserTick requests a deferred destruction from inside the frame
	// loop - of itself or of another actor. This is the path that was undefined
	// behavior with the immediate delete.
	class PSanityTickActor : public PActor
	{
	public:
		enum class EDestroyTargetType : UINT8
		{
			NONE,
			SELF,
			OTHER,
		};

		PSanityTickActor(EDestroyTargetType InDestroyTarget, PActor* InOtherActor)
			: DestroyTarget(InDestroyTarget)
			, OtherActor(InOtherActor)
		{
			SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(this, "SanityTickActor");
		}

		void UserTick(FLOAT deltaTime) override
		{
			(void)deltaTime;
			PWorld* World = GetWorld();
			if (!World || (DestroyTarget == EDestroyTargetType::NONE))
			{
				return;
			}
			if (DestroyTarget == EDestroyTargetType::SELF)
			{
				World->DestroyActor(this);
			}
			else
			{
				World->DestroyActor(OtherActor);
			}
			// One request per instance; the ticks after it are the "object stays pending
			// and must not be reached" phase of the scenario.
			DestroyTarget = EDestroyTargetType::NONE;
		}

	private:
		EDestroyTargetType DestroyTarget = EDestroyTargetType::NONE;
		PActor* OtherActor = nullptr;
	};

	static void ReportSanityResult(const EString& InCheckName, const BOOL32& InIsPassed)
	{
		// A failed scenario is reported, not breakpointed: the module must be able to
		// run every remaining scenario and print the full picture.
		if (InIsPassed != FALSE)
		{
			PE_LOG_LOG(EString(PE_TEXT("[FrameworkSanityCheck] PASS: ")) + InCheckName);
		}
		else
		{
			PE_LOG_ERROR(EString(PE_TEXT("[FrameworkSanityCheck] FAIL: ")) + InCheckName);
		}
	}

	// Spawns an actor holding a nested component chain (root -> child -> grandchild,
	// all in the actor's component list) plus one non-scene component. Components are
	// added and scene-registered the way real content is, so the destroy scenarios run
	// against fully-initialized objects.
	static PActor* SpawnNestedActor(PWorld* InWorld)
	{
		PActor* NewActor = new PSanityTickActor(PSanityTickActor::EDestroyTargetType::NONE, nullptr);
		POBJ_DEBUGNAME_SET(NewActor, "SanityNestedActor");
		InWorld->AddActor(NewActor);

		PSceneComponent* ChildComponent = new PSceneComponent();
		NewActor->AddComponent(ChildComponent);
		PSceneComponent* GrandChildComponent = new PSceneComponent();
		NewActor->AddComponent(GrandChildComponent);
		// Re-parent under the child after the add: AddComponent attaches to the actor
		// root, and attaching first would have the add re-parent it right back.
		GrandChildComponent->AttachToComponent(ChildComponent);

		PActorComponent* PlainComponent = new PActorComponent();
		NewActor->AddComponent(PlainComponent);
		return NewActor;
	}

	static BOOL32 CheckNestedDeferredDestroy(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		PActor* Actor = SpawnNestedActor(InWorld);
		const ObjectIdentityType ActorId = Actor->GetUniqueID();
		if (InWorld->GetActorByUniqueID(ActorId, TRUE) == nullptr)
		{
			return FALSE;
		}

		// Mark, then one full world tick while the actor is pending: detached from the
		// tree, nothing may reach it. Then flush and require the registry slot free.
		InWorld->DestroyActor(Actor);
		InWorld->Tick(1.f / 60.f);
		InWorld->FlushPendingDestroy();
		if (InWorld->GetActorByUniqueID(ActorId, TRUE) != nullptr)
		{
			return FALSE;
		}
#if ENGINE_DEBUG_MODE
		// The whole nested chain must have been released exactly once - the roots-only
		// ClearComponents walk plus the parents' ClearChildren, no double frees.
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return TRUE;
	}

	static BOOL32 CheckSingleComponentDestroy(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityComponentActor");
		InWorld->AddActor(Actor);
		PSceneComponent* ChildComponent = new PSceneComponent();
		Actor->AddComponent(ChildComponent);
		const ObjectIdentityType ChildId = ChildComponent->GetUniqueID();

		// Routes into the world's pending-destroy queue; the actor ticks once without
		// the child before and once after the flush.
		Actor->DestroyComponent(ChildComponent);
		InWorld->Tick(1.f / 60.f);
		InWorld->FlushPendingDestroy();
		if (Actor->GetComponentByUniqueID(ChildId, TRUE) != nullptr)
		{
			return FALSE;
		}
		InWorld->Tick(1.f / 60.f);

		// Cleanup through the same deferred path, and no leaks overall.
		InWorld->DestroyActor(Actor);
		InWorld->FlushPendingDestroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return TRUE;
	}

	static BOOL32 CheckDestroySelfDuringTick(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		PActor* Actor = new PSanityTickActor(PSanityTickActor::EDestroyTargetType::SELF, nullptr);
		POBJ_DEBUGNAME_SET(Actor, "SanitySelfDestroyActor");
		InWorld->AddActor(Actor);
		const ObjectIdentityType ActorId = Actor->GetUniqueID();

		// The first tick requests the destruction from inside UserTick; a second tick
		// runs while the request is pending; the flush must release it exactly once.
		InWorld->Tick(1.f / 60.f);
		InWorld->Tick(1.f / 60.f);
		InWorld->FlushPendingDestroy();
		if (InWorld->GetActorByUniqueID(ActorId, TRUE) != nullptr)
		{
			return FALSE;
		}
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return TRUE;
	}

	static BOOL32 CheckDestroyOtherDuringTick(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		PActor* Victim = new PActor();
		POBJ_DEBUGNAME_SET(Victim, "SanityVictimActor");
		InWorld->AddActor(Victim);
		const ObjectIdentityType VictimId = Victim->GetUniqueID();
		PActor* Requester = new PSanityTickActor(PSanityTickActor::EDestroyTargetType::OTHER, Victim);
		POBJ_DEBUGNAME_SET(Requester, "SanityOtherDestroyActor");
		InWorld->AddActor(Requester);

		// The requester's UserTick queues the victim; the requester itself must survive
		// and keep ticking.
		InWorld->Tick(1.f / 60.f);
		InWorld->FlushPendingDestroy();
		if (InWorld->GetActorByUniqueID(VictimId, TRUE) != nullptr)
		{
			return FALSE;
		}
		InWorld->Tick(1.f / 60.f);

		InWorld->DestroyActor(Requester);
		InWorld->FlushPendingDestroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return TRUE;
	}

	static BOOL32 CheckControllerInputUnsubscribe(PWorld* InWorld)
	{
		// Full controller lifecycle: Init subscribes to the global input delegates,
		// Uninit must take those subscriptions off before the memory goes away. Input
		// events are dispatched straight from the Win32 message pump, so this
		// broadcast-after-destroy is exactly what the pump would do between frames.
		//
		// No alive-count assertion here, unlike the other scenarios: the controller's
		// camera creates its RViewProxy (an EObjectBase) on this thread, but the queued
		// remove command is only drained by the render thread after the first kick -
		// which has not happened yet at EditorInit time. The count is legitimately
		// non-deterministic at this point in startup; the subscription lifetime is what
		// this scenario is about, and a crash on the broadcasts below is the failure
		// signal.
		PController* Controller = new PController();
		POBJ_DEBUGNAME_SET(Controller, "SanityController");
		InWorld->AddActor(Controller);
		InWorld->DestroyActor(Controller);
		InWorld->FlushPendingDestroy();
		EInput::KeyEvent.Broadcast(IKeyboard::Event::EType::Press, static_cast<BYTE>(0x57));
		EInput::MouseEvent.Broadcast(IMouse::Event::EType::Move);
		return TRUE;
	}

	static BOOL32 CheckAttachCycleRejected(PWorld* InWorld)
	{
		(void)InWorld;
		// Component chain root -> child -> grandchild, then attaching the root under its
		// own grandchild must be refused and leave the hierarchy untouched.
		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityCycleActor");
		PSceneComponent* ChildComponent = new PSceneComponent();
		Actor->AddComponent(ChildComponent);
		PSceneComponent* GrandChildComponent = new PSceneComponent();
		Actor->AddComponent(GrandChildComponent);
		GrandChildComponent->AttachToComponent(ChildComponent);

		PSceneComponent* RootComponent = Actor->GetRootComponent();
		RootComponent->AttachToComponent(GrandChildComponent);
		if (RootComponent->GetParentComponent() != nullptr)
		{
			Actor->Destroy();
			return FALSE;
		}

		// Actor chain Actor1 <- Actor2 <- Actor3, then attaching Actor1 below Actor3
		// must be refused the same way.
		PActor* Actor1 = new PActor();
		PActor* Actor2 = new PActor();
		PActor* Actor3 = new PActor();
		Actor2->AttachToActor(Actor1);
		Actor3->AttachToActor(Actor2);
		Actor1->AttachToActor(Actor3);
		if ((Actor1->GetAttachedParentActor() != nullptr) || (Actor3->GetAttachedParentActor() != Actor2))
		{
			Actor1->Destroy();
			return FALSE;
		}

		Actor1->Destroy();
		return TRUE;
	}

	static BOOL32 CheckUnaddedObjectDestroy(PWorld* InWorld)
	{
		(void)InWorld;
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		// A mesh component that never joined a world: its destroy path used to run
		// straight into an unguarded GetWorld()->GetRenderScene().
		PStaticMeshComponent* MeshComponent = new PStaticMeshComponent();
		MeshComponent->Destroy();
		// An actor that was never added anywhere: never initialized, so teardown must
		// skip Uninit and still release the default root component.
		PActor* Actor = new PActor();
		Actor->Destroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return TRUE;
	}

	void EFrameworkSanityCheck::RunAllChecks(PWorld* InWorld)
	{
		if (!InWorld)
		{
			return;
		}
		PE_LOG_LOG(EString(PE_TEXT("[FrameworkSanityCheck] Running framework lifecycle checks.")));
		UINT32 FailedCount = 0u;

		const BOOL32 bNestedDeferredDestroy = CheckNestedDeferredDestroy(InWorld);
		ReportSanityResult(EString(PE_TEXT("nested deferred destroy")), bNestedDeferredDestroy);
		if (!bNestedDeferredDestroy)
		{
			FailedCount += 1u;
		}
		const BOOL32 bSingleComponentDestroy = CheckSingleComponentDestroy(InWorld);
		ReportSanityResult(EString(PE_TEXT("single component destroy")), bSingleComponentDestroy);
		if (!bSingleComponentDestroy)
		{
			FailedCount += 1u;
		}
		const BOOL32 bDestroySelfDuringTick = CheckDestroySelfDuringTick(InWorld);
		ReportSanityResult(EString(PE_TEXT("destroy requested for self during tick")), bDestroySelfDuringTick);
		if (!bDestroySelfDuringTick)
		{
			FailedCount += 1u;
		}
		const BOOL32 bDestroyOtherDuringTick = CheckDestroyOtherDuringTick(InWorld);
		ReportSanityResult(EString(PE_TEXT("destroy requested for another actor during tick")), bDestroyOtherDuringTick);
		if (!bDestroyOtherDuringTick)
		{
			FailedCount += 1u;
		}
		const BOOL32 bControllerInputUnsubscribe = CheckControllerInputUnsubscribe(InWorld);
		ReportSanityResult(EString(PE_TEXT("controller input unsubscribe on destroy")), bControllerInputUnsubscribe);
		if (!bControllerInputUnsubscribe)
		{
			FailedCount += 1u;
		}
		const BOOL32 bAttachCycleRejected = CheckAttachCycleRejected(InWorld);
		ReportSanityResult(EString(PE_TEXT("attach cycle rejected")), bAttachCycleRejected);
		if (!bAttachCycleRejected)
		{
			FailedCount += 1u;
		}
		const BOOL32 bUnaddedObjectDestroy = CheckUnaddedObjectDestroy(InWorld);
		ReportSanityResult(EString(PE_TEXT("destroy of never-added objects")), bUnaddedObjectDestroy);
		if (!bUnaddedObjectDestroy)
		{
			FailedCount += 1u;
		}

		if (FailedCount == 0u)
		{
			PE_LOG_LOG(EString(PE_TEXT("[FrameworkSanityCheck] All checks passed.")));
		}
		else
		{
			PE_LOG_ERROR(EString(PE_TEXT("[FrameworkSanityCheck] Failed check count: ")) + EString::FromInt(FailedCount));
		}
	}
}

#endif
