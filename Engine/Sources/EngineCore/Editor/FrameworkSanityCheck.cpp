#include "FrameworkSanityCheck.h"

#if ENGINE_EDITOR_ONLY

#include "PigeonBase/Object/Actor.h"
#include "PigeonBase/Object/World/World.h"
#include "PigeonBase/Object/Component/ActorComponent.h"
#include "PigeonBase/Object/Component/SceneComponent.h"
#include "PigeonBase/Object/Component/Primitive/StaticMeshComponent.h"
#include "PigeonBase/Object/Controller/Controller.h"
#include "HID/Input/Input.h"
#include "Development/Message/Message.h"
#include <Reflection/Property.h>
#include <Reflection/JsonArchive.h>
#include <RapidJson/Headers/JsonObject.h>
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

	// Actor that subscribes through the object-bound overload in Init and keeps only the
	// returned listener ID. Nothing unsubscribes on purpose: the bus is expected to purge
	// the subscription when the object dies, via PObject::OnObjectPreDelete.
	class PSanityMessageSubscriber : public PActor
	{
	public:
		PSanityMessageSubscriber(const EName& InMessageName)
			: SubscriptionMessageName(InMessageName)
		{
		}

		void Init() override
		{
			PActor::Init();
			MessageListener = EMessageManager::GetManagerSingleton()->Subscribe(
				SubscriptionMessageName,
				this,
				TFunction<void(const EMessageBase&)>([this](const EMessageBase&) -> void
				{
					ReceivedCount += 1u;
				}));
		}

	public:
		UINT32 ReceivedCount = 0u;
		FMessageListenerHandle MessageListener;

	private:
		EName SubscriptionMessageName;
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

	static BOOL32 AreMatricesNearlyEqual(const Matrix4x4& InA, const Matrix4x4& InB, const FLOAT& InTolerance)
	{
		for (UINT32 Row = 0u; Row < 4u; Row++)
		{
			for (UINT32 Column = 0u; Column < 4u; Column++)
			{
				if (EMath::Abs(InA(Row, Column) - InB(Row, Column)) > InTolerance)
				{
					return FALSE;
				}
			}
		}
		return TRUE;
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

	static BOOL32 CheckMessageSubscriptionLifetime(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		EMessageManager* MessageManager = EMessageManager::GetManagerSingleton();
		const EName TestMessageName(PE_TEXT("FrameworkSanityCheck.TestMessage"));

		PSanityMessageSubscriber* Subscriber = new PSanityMessageSubscriber(TestMessageName);
		POBJ_DEBUGNAME_SET(Subscriber, "SanityMessageSubscriber");
		InWorld->AddActor(Subscriber);
		if (MessageManager->GetListenerNum(TestMessageName) != 1u)
		{
			InWorld->DestroyActor(Subscriber);
			InWorld->FlushPendingDestroy();
			return FALSE;
		}

		// One delivery proves the subscription is live before the lifetime check.
		MessageManager->Send(TestMessageName);
		if (Subscriber->ReceivedCount != 1u)
		{
			InWorld->DestroyActor(Subscriber);
			InWorld->FlushPendingDestroy();
			return FALSE;
		}

		// The subscriber never unsubscribes: PObject::OnObjectPreDelete must purge its
		// listener, and a later broadcast must be a safe no-op. The pointer is not
		// touched after the flush.
		InWorld->DestroyActor(Subscriber);
		InWorld->FlushPendingDestroy();
		if (MessageManager->GetListenerNum(TestMessageName) != 0u)
		{
			return FALSE;
		}
		MessageManager->Send(TestMessageName);

#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return TRUE;
	}

	static BOOL32 CheckWorldTransformComposition(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		// A three-level chain with rotation and scale at every level. The cached world
		// matrix must equal the manual product Local3 * Local2 * Local1 * ActorRootWorld
		// (row-vector convention, child on the left) - this is the check that pins the
		// multiplication order the engine's math expects.
		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityTransformActor");
		Actor->SetActorLocation(Vector3(100.f, 20.f, -30.f));
		Actor->SetActorRotation(MakeQuaternion(Euler(10.f, 25.f, -5.f)));
		Actor->SetActorScale(Vector3(1.5f, 1.5f, 1.5f));
		InWorld->AddActor(Actor);

		PSceneComponent* Level1 = new PSceneComponent();
		Actor->AddComponent(Level1);
		Level1->SetComponentLocation(Vector3(10.f, 0.f, 5.f));
		Level1->SetComponentRotation(MakeQuaternion(Euler(0.f, 45.f, 0.f)));
		Level1->SetComponentScale(Vector3(2.f, 2.f, 2.f));

		PSceneComponent* Level2 = new PSceneComponent();
		Actor->AddComponent(Level2);
		Level2->AttachToComponent(Level1);
		Level2->SetComponentLocation(Vector3(0.f, 7.f, 0.f));
		Level2->SetComponentRotation(MakeQuaternion(Euler(30.f, 0.f, 15.f)));
		Level2->SetComponentScale(Vector3(0.5f, 0.5f, 0.5f));

		PSceneComponent* Level3 = new PSceneComponent();
		Actor->AddComponent(Level3);
		Level3->AttachToComponent(Level2);
		Level3->SetComponentLocation(Vector3(0.f, 0.f, 12.f));
		Level3->SetComponentRotation(MakeQuaternion(Euler(0.f, 0.f, 90.f)));

		const Matrix4x4 ExpectedMatrix =
			Level3->GetTransform().ToMatrix()
			* Level2->GetTransform().ToMatrix()
			* Level1->GetTransform().ToMatrix()
			* Actor->GetRootComponent()->GetCachedLocalToWorldMatrix();
		const BOOL32 bMatrixMatches = AreMatricesNearlyEqual(Level3->GetCachedLocalToWorldMatrix(), ExpectedMatrix, 1.0e-3f);

		const Vector3 CachedLocation = Level3->GetComponentWorldLocation();
		const Vector3 MatrixLocation(ExpectedMatrix._41, ExpectedMatrix._42, ExpectedMatrix._43);
		const BOOL32 bLocationMatches = (Vector3::Length(CachedLocation - MatrixLocation) <= 1.0e-3f);

		// Nested actor: the child actor's root is parented to the parent actor's root, and
		// the parent must enter the chain exactly once. The old transform code added the
		// parent actor's root through both the component chain and the actor chain.
		PActor* ParentActor = new PActor();
		POBJ_DEBUGNAME_SET(ParentActor, "SanityTransformParentActor");
		ParentActor->SetActorLocation(Vector3(-80.f, 40.f, 15.f));
		ParentActor->SetActorRotation(MakeQuaternion(Euler(-15.f, 40.f, 5.f)));
		ParentActor->SetActorScale(Vector3(1.2f, 1.2f, 1.2f));
		InWorld->AddActor(ParentActor);

		PActor* ChildActor = new PActor();
		POBJ_DEBUGNAME_SET(ChildActor, "SanityTransformChildActor");
		ChildActor->SetActorLocation(Vector3(0.f, 12.f, 0.f));
		InWorld->AddActor(ChildActor);
		ChildActor->AttachToActor(ParentActor);

		PSceneComponent* NestedComponent = new PSceneComponent();
		ChildActor->AddComponent(NestedComponent);
		NestedComponent->SetComponentLocation(Vector3(0.f, 0.f, 20.f));

		const Matrix4x4 ExpectedNested =
			NestedComponent->GetTransform().ToMatrix()
			* ChildActor->GetRootComponent()->GetTransform().ToMatrix()
			* ParentActor->GetRootComponent()->GetCachedLocalToWorldMatrix();
		const BOOL32 bNestedMatches = AreMatricesNearlyEqual(NestedComponent->GetCachedLocalToWorldMatrix(), ExpectedNested, 1.0e-3f);

		InWorld->DestroyActor(ParentActor);
		InWorld->DestroyActor(Actor);
		InWorld->FlushPendingDestroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return (bMatrixMatches && bLocationMatches && bNestedMatches) ? TRUE : FALSE;
	}

	static BOOL32 CheckAttachmentRules(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		PActor* HostActor = new PActor();
		POBJ_DEBUGNAME_SET(HostActor, "SanityAttachHostActor");
		HostActor->SetActorLocation(Vector3(-40.f, 10.f, 25.f));
		HostActor->SetActorRotation(MakeQuaternion(Euler(20.f, 35.f, -10.f)));
		InWorld->AddActor(HostActor);

		PSceneComponent* HostComponent = new PSceneComponent();
		HostActor->AddComponent(HostComponent);
		HostComponent->SetComponentRotation(MakeQuaternion(Euler(0.f, 60.f, 0.f)));
		HostComponent->SetComponentScale(Vector3(1.5f, 1.5f, 1.5f));

		PSceneComponent* FloatingComponent = new PSceneComponent();
		HostActor->AddComponent(FloatingComponent);
		FloatingComponent->SetComponentLocation(Vector3(5.f, 0.f, 8.f));
		FloatingComponent->SetComponentRotation(MakeQuaternion(Euler(0.f, 0.f, 30.f)));

		// KEEP_WORLD: reparenting under a rotated, scaled parent must not move the child
		// in world space.
		const Matrix4x4 WorldBefore = FloatingComponent->GetCachedLocalToWorldMatrix();
		FloatingComponent->AttachToComponent(HostComponent, EAttachmentRuleType::KEEP_WORLD);
		const BOOL32 bWorldKept = AreMatricesNearlyEqual(FloatingComponent->GetCachedLocalToWorldMatrix(), WorldBefore, 1.0e-3f);

		// KEEP_RELATIVE: the provided transform becomes the child's local transform.
		const ETransform RelativeTransform(
			Vector3(1.f, 2.f, 3.f),
			MakeQuaternion(Euler(0.f, 15.f, 0.f)),
			Vector3(2.f, 2.f, 2.f));
		FloatingComponent->AttachToComponent(HostComponent, EAttachmentRuleType::KEEP_RELATIVE, RelativeTransform);
		const BOOL32 bRelativeApplied = AreMatricesNearlyEqual(FloatingComponent->GetTransform().ToMatrix(), RelativeTransform.ToMatrix(), 1.0e-4f);

		InWorld->DestroyActor(HostActor);
		InWorld->FlushPendingDestroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return (bWorldKept && bRelativeApplied) ? TRUE : FALSE;
	}

	// Component that records its tick into a shared log, so the sanity check can assert
	// the (group, priority, registration) order the actor ticks in.
	class PSanityTickOrderComponent : public PActorComponent
	{
	public:
		PSanityTickOrderComponent(TArray<UINT32>* InTickLog, const UINT32 InTag)
			: TickLog(InTickLog), Tag(InTag)
		{
			SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(this, "SanityTickOrderComponent");
		}

		void Tick(FLOAT deltaTime) override
		{
			(void)deltaTime;
			if (TickLog)
			{
				TickLog->Add(Tag);
			}
		}

	private:
		TArray<UINT32>* TickLog = nullptr;
		UINT32 Tag = 0u;
	};

	static BOOL32 CheckComponentLookupAndTickOrder(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		TArray<UINT32> TickLog;
		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityLookupActor");
		// A component only ticks when its own actor is tickable; the tick order this
		// scenario asserts lives inside PActor::Tick.
		Actor->SetIsTickable(TRUE);
		InWorld->AddActor(Actor);

		// Registration order 1, 2, 3 with different bands and priorities. Expected tick
		// order: PRE_PHYSICS (2), gameplay priority -1 (3), gameplay priority 5 (1).
		PSanityTickOrderComponent* GamePlayLate = new PSanityTickOrderComponent(&TickLog, 1u);
		GamePlayLate->SetTickPriority(5);
		Actor->AddComponent(GamePlayLate);
		PSanityTickOrderComponent* PrePhysics = new PSanityTickOrderComponent(&TickLog, 2u);
		PrePhysics->SetTickGroup(ETickGroupType::PRE_PHYSICS);
		Actor->AddComponent(PrePhysics);
		PSanityTickOrderComponent* GamePlayEarly = new PSanityTickOrderComponent(&TickLog, 3u);
		GamePlayEarly->SetTickPriority(-1);
		Actor->AddComponent(GamePlayEarly);

		// Template lookups: first exact match, all exact matches in registration order,
		// and a base-class match through the same list.
		const BOOL32 bFindWorks = (Actor->FindComponent<PSanityTickOrderComponent>() == GamePlayLate) ? TRUE : FALSE;
		const BOOL32 bListWorks = (Actor->GetComponents<PSanityTickOrderComponent>().Num<UINT32>() == 3u) ? TRUE : FALSE;
		const BOOL32 bBaseLookupWorks = (Actor->FindComponent<PSceneComponent>() == Actor->GetRootComponent()) ? TRUE : FALSE;

		InWorld->Tick(1.f / 60.f);
		const BOOL32 bOrderMatches =
			((TickLog.Num<UINT32>() == 3u) && (TickLog[0] == 2u) && (TickLog[1] == 3u) && (TickLog[2] == 1u)) ? TRUE : FALSE;

		InWorld->DestroyActor(Actor);
		InWorld->FlushPendingDestroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return (bFindWorks && bListWorks && bBaseLookupWorks && bOrderMatches) ? TRUE : FALSE;
	}

	static BOOL32 CheckWorldLifecycleEvents(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		UINT32 AddedCount = 0u;
		UINT32 RemovedCount = 0u;
		UINT32 DestroyedCount = 0u;
		UINT32 ComponentAddedCount = 0u;
		UINT32 ComponentRemovedCount = 0u;

		const FDelegateHandle AddedHandle = InWorld->OnActorAdded.Add(
			TFunction<void(PActor*)>([&AddedCount](PActor*) -> void { AddedCount += 1u; }));
		const FDelegateHandle RemovedHandle = InWorld->OnActorRemoved.Add(
			TFunction<void(PActor*)>([&RemovedCount](PActor*) -> void { RemovedCount += 1u; }));
		const FDelegateHandle DestroyedHandle = InWorld->OnActorDestroyed.Add(
			TFunction<void(PActor*)>([&DestroyedCount](PActor*) -> void { DestroyedCount += 1u; }));
		const FDelegateHandle ComponentAddedHandle = InWorld->OnComponentAdded.Add(
			TFunction<void(PActorComponent*)>([&ComponentAddedCount](PActorComponent*) -> void { ComponentAddedCount += 1u; }));
		const FDelegateHandle ComponentRemovedHandle = InWorld->OnComponentRemoved.Add(
			TFunction<void(PActorComponent*)>([&ComponentRemovedCount](PActorComponent*) -> void { ComponentRemovedCount += 1u; }));

		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityEventActor");
		InWorld->AddActor(Actor);
		// One extra component after the add, so both registration paths are covered (the
		// root registers during the scene add, this one through AddComponent).
		PActorComponent* ExtraComponent = new PActorComponent();
		Actor->AddComponent(ExtraComponent);
		InWorld->DestroyActor(Actor);
		InWorld->FlushPendingDestroy();

		InWorld->OnActorAdded.Remove(AddedHandle);
		InWorld->OnActorRemoved.Remove(RemovedHandle);
		InWorld->OnActorDestroyed.Remove(DestroyedHandle);
		InWorld->OnComponentAdded.Remove(ComponentAddedHandle);
		InWorld->OnComponentRemoved.Remove(ComponentRemovedHandle);

#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		// Two components (root + extra) join and leave the scene; the actor is added once,
		// removed once and destroyed once.
		return ((AddedCount == 1u) && (RemovedCount == 1u) && (DestroyedCount == 1u) &&
			(ComponentAddedCount == 2u) && (ComponentRemovedCount == 2u)) ? TRUE : FALSE;
	}

	static BOOL32 CheckObjectHandleResolution(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityHandleActor");
		PSceneComponent* Component = new PSceneComponent();
		Actor->AddComponent(Component);
		InWorld->AddActor(Actor);

		const FObjectHandle ActorHandle = MakeObjectHandle(Actor);
		const FObjectHandle ComponentHandle = MakeObjectHandle(Component);
		const BOOL32 bResolvesBefore =
			((InWorld->ResolveActor(ActorHandle) == Actor) &&
				(InWorld->ResolveComponent<PSceneComponent>(ComponentHandle) == Component)) ? TRUE : FALSE;

		InWorld->DestroyActor(Actor);
		InWorld->FlushPendingDestroy();
		const BOOL32 bFailsAfter =
			((InWorld->ResolveActor(ActorHandle) == nullptr) &&
				(InWorld->ResolveComponent<PSceneComponent>(ComponentHandle) == nullptr)) ? TRUE : FALSE;

#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return (bResolvesBefore && bFailsAfter) ? TRUE : FALSE;
	}

	static BOOL32 CheckReflectionSerialization(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		// Stable-name factory: a registered concrete class can be created from the name a
		// level file would store.
		BOOL32 bFactoryWorks = FALSE;
		PObject* CreatedObject = CreateClassObjectByStableName<PObject>(PE_TEXT("PSceneComponent"));
		if (CreatedObject)
		{
			PSceneComponent* CreatedComponent = dynamic_cast<PSceneComponent*>(CreatedObject);
			bFactoryWorks = (CreatedComponent != nullptr) ? TRUE : FALSE;
			CreatedObject->Destroy();
		}
		const BOOL32 bUnknownNameRejected = (GetClassHashCodeByStableName(PE_TEXT("PNotAClass")) == static_cast<size_t>(-1)) ? TRUE : FALSE;

		// Property round trip through the JSON backend: write the reflected properties of
		// one component, read them into a fresh instance, compare.
		PActor* SourceActor = new PActor();
		POBJ_DEBUGNAME_SET(SourceActor, "SanityReflectionSource");
		PSceneComponent* SourceComponent = new PSceneComponent();
		SourceActor->AddComponent(SourceComponent);
		InWorld->AddActor(SourceActor);
		SourceComponent->SetComponentLocation(Vector3(1.f, 2.f, 3.f));
		SourceComponent->SetComponentRotation(MakeQuaternion(Euler(10.f, 20.f, 30.f)));
		SourceComponent->SetComponentScale(Vector3(2.f, 2.f, 2.f));
		SourceComponent->SetMobility(EMobilityType::EMT_STATIC);

		CJsonObject Document;
		EJsonArchiveWriter Writer(Document);
		EClassPropertyRegistry::SerializeObject(Writer, GetClassHashCode(SourceComponent), SourceComponent);

		PSceneComponent* TargetComponent = new PSceneComponent();
		EJsonArchiveReader Reader(Document);
		EClassPropertyRegistry::SerializeObject(Reader, GetClassHashCode(TargetComponent), TargetComponent);

		const BOOL32 bTransformMatches = AreMatricesNearlyEqual(TargetComponent->GetTransform().ToMatrix(), SourceComponent->GetTransform().ToMatrix(), 1.0e-4f);
		const BOOL32 bMobilityMatches = (TargetComponent->GetMobility() == SourceComponent->GetMobility()) ? TRUE : FALSE;

		TargetComponent->Destroy();
		InWorld->DestroyActor(SourceActor);
		InWorld->FlushPendingDestroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return (bFactoryWorks && bUnknownNameRejected && bTransformMatches && bMobilityMatches) ? TRUE : FALSE;
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
		const BOOL32 bMessageSubscriptionLifetime = CheckMessageSubscriptionLifetime(InWorld);
		ReportSanityResult(EString(PE_TEXT("message subscription lifetime")), bMessageSubscriptionLifetime);
		if (!bMessageSubscriptionLifetime)
		{
			FailedCount += 1u;
		}
		const BOOL32 bWorldTransformComposition = CheckWorldTransformComposition(InWorld);
		ReportSanityResult(EString(PE_TEXT("world transform composition")), bWorldTransformComposition);
		if (!bWorldTransformComposition)
		{
			FailedCount += 1u;
		}
		const BOOL32 bAttachmentRules = CheckAttachmentRules(InWorld);
		ReportSanityResult(EString(PE_TEXT("attachment rules")), bAttachmentRules);
		if (!bAttachmentRules)
		{
			FailedCount += 1u;
		}
		const BOOL32 bComponentLookupAndTickOrder = CheckComponentLookupAndTickOrder(InWorld);
		ReportSanityResult(EString(PE_TEXT("component lookup and tick order")), bComponentLookupAndTickOrder);
		if (!bComponentLookupAndTickOrder)
		{
			FailedCount += 1u;
		}
		const BOOL32 bWorldLifecycleEvents = CheckWorldLifecycleEvents(InWorld);
		ReportSanityResult(EString(PE_TEXT("world lifecycle events")), bWorldLifecycleEvents);
		if (!bWorldLifecycleEvents)
		{
			FailedCount += 1u;
		}
		const BOOL32 bObjectHandleResolution = CheckObjectHandleResolution(InWorld);
		ReportSanityResult(EString(PE_TEXT("object handle resolution")), bObjectHandleResolution);
		if (!bObjectHandleResolution)
		{
			FailedCount += 1u;
		}
		const BOOL32 bReflectionSerialization = CheckReflectionSerialization(InWorld);
		ReportSanityResult(EString(PE_TEXT("reflection property round trip")), bReflectionSerialization);
		if (!bReflectionSerialization)
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
