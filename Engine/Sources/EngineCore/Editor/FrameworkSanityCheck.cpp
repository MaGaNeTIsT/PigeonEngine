#include "FrameworkSanityCheck.h"

#if ENGINE_EDITOR_ONLY

#include "PigeonBase/Object/Actor.h"
#include "PigeonBase/Object/World/World.h"
#include "PigeonBase/Object/Component/ActorComponent.h"
#include "PigeonBase/Object/Component/SceneComponent.h"
#include "PigeonBase/Object/Component/Primitive/StaticMeshComponent.h"
#include "PigeonBase/Object/Component/CameraAndLight/CameraComponent.h"
#include "PigeonBase/Object/Component/CameraAndLight/LightComponent.h"
#include "PigeonBase/Object/Component/CameraAndLight/DirectionalLightComponent.h"
#include "PigeonBase/Object/Component/CameraAndLight/PointLightComponent.h"
#include "PigeonBase/Object/Component/Primitive/SkyLightComponent.h"
#include "PigeonBase/Object/Component/Physics/PhysicsComponent.h"
#include "PigeonBase/Object/Component/Physics/TriggerComponent.h"
#include "PigeonBase/Object/Component/Navigation/NavMeshComponent.h"
#include "PigeonBase/Object/Component/Navigation/NavObstacleComponent.h"
#include "PigeonBase/Object/Component/Navigation/NavAgentComponent.h"
#include "PigeonBase/Object/Component/Water/WaterSurfaceComponent.h"
#include "PigeonBase/Object/Component/Water/WaterSimulationComponent.h"
#include "../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "PigeonBase/Object/Controller/Controller.h"
#include "HID/Input/Input.h"
#include "Development/Message/Message.h"
#include <Reflection/Property.h>
#include <Reflection/JsonArchive.h>
#include <Reflection/LevelSerializer.h>
#include <RapidJson/Headers/JsonObject.h>
#include <Editor/EditorLogManager.h>
#include <Editor/WorldEditorCommand.h>
#include "EditorHelper.h"

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

	// A world with no demo content and no controller, for exercising PWorld::SaveLevel /
	// LoadLevel without touching the editor's live world or its global physics settings.
	class PSanityLevelWorld : public PWorld
	{
	protected:
		virtual PActor* CreateRootActor() override
		{
			return new PActor();
		}
		virtual void CreateDefaultController() override
		{
		}
		virtual BOOL32 ShouldAffectGlobalPhysics() const override
		{
			return FALSE;
		}
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

	static BOOL32 IsVectorNearlyEqual(const Vector3& InA, const Vector3& InB, const FLOAT& InTolerance)
	{
		return ((EMath::Abs(InA.x - InB.x) <= InTolerance) &&
			(EMath::Abs(InA.y - InB.y) <= InTolerance) &&
			(EMath::Abs(InA.z - InB.z) <= InTolerance)) ? TRUE : FALSE;
	}

	// Whole actor graph through the level serializer: a parent actor with a component
	// chain plus an attached child actor is saved to a JSON document, destroyed, reloaded
	// and compared - hierarchy, component parentage, transform values and a reflected
	// primitive property. Finishes by destroying the loaded roots and checking that the
	// alive object count returns to the baseline.
	static BOOL32 CheckLevelSerializationRoundTrip(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		PActor* ParentActor = new PActor();
		POBJ_DEBUGNAME_SET(ParentActor, "SanityLevelParent");
		InWorld->AddActor(ParentActor);

		PSceneComponent* OffsetComponent = new PSceneComponent();
		ParentActor->AddComponent(OffsetComponent);
		OffsetComponent->SetComponentLocation(Vector3(10.f, 20.f, 30.f));
		OffsetComponent->SetMobility(EMobilityType::EMT_STATIC);
		PSceneComponent* ChildComponent = new PSceneComponent();
		ParentActor->AddComponent(ChildComponent);
		ChildComponent->AttachToComponent(OffsetComponent);
		ChildComponent->SetComponentLocation(Vector3(-1.f, 0.f, 2.f));
		PStaticMeshComponent* MeshComponent = new PStaticMeshComponent();
		ParentActor->AddComponent(MeshComponent);
		MeshComponent->SetCastShadow(FALSE);

		PActor* ChildActor = new PActor();
		POBJ_DEBUGNAME_SET(ChildActor, "SanityLevelChild");
		InWorld->AddActor(ChildActor);
		ChildActor->SetActorLocation(Vector3(0.f, 5.f, 0.f));
		ChildActor->AttachToActor(ParentActor);

		TArray<PActor*> SourceActors;
		SourceActors.Add(ParentActor);
		SourceActors.Add(ChildActor);
		CJsonObject Document;
		const BOOL32 bSaved = ELevelSerializer::SaveActors(SourceActors, Document);

		BOOL32 bDocumentOk = FALSE;
		if (bSaved)
		{
			EJsonArchiveReader VersionReader(Document);
			UINT32 SavedFormatVersion = 0u;
			VersionReader.SerializeUInt32(PE_TEXT("FormatVersion"), SavedFormatVersion);
			bDocumentOk = ((SavedFormatVersion == ELevelSerializer::LevelFormatVersion) && Document.HasField(PE_TEXT("Actors"))) ? TRUE : FALSE;
		}

		// Destroy only the root: the child actor is destroyed with its parent.
		InWorld->DestroyActor(ParentActor);
		InWorld->FlushPendingDestroy();

		TArray<PActor*> LoadedActors;
		const BOOL32 bLoaded = ELevelSerializer::LoadActors(*InWorld, Document, &LoadedActors);

		BOOL32 bHierarchyOk = FALSE;
		BOOL32 bComponentTreeOk = FALSE;
		BOOL32 bPropertiesOk = FALSE;
		PActor* LoadedParent = nullptr;
		if (bLoaded)
		{
			// Top-level actors are attached to the world root actor by AddActor, so a loaded
			// child is the actor whose parent is another actor of this same load.
			PActor* LoadedChild = nullptr;
			for (UINT32 Index = 0u, Num = LoadedActors.Num<UINT32>(); Index < Num; Index++)
			{
				PActor* Actor = LoadedActors[Index];
				if (!Actor)
				{
					continue;
				}
				BOOL32 bHasLoadedParent = FALSE;
				for (UINT32 OtherIndex = 0u; OtherIndex < Num; OtherIndex++)
				{
					if (LoadedActors[OtherIndex] == Actor->GetAttachedParentActor())
					{
						bHasLoadedParent = TRUE;
						break;
					}
				}
				if (bHasLoadedParent)
				{
					LoadedChild = Actor;
				}
				else
				{
					LoadedParent = Actor;
				}
			}
			if (LoadedParent && LoadedChild)
			{
				bHierarchyOk = ((LoadedChild->GetAttachedParentActor() == LoadedParent) &&
					IsVectorNearlyEqual(LoadedChild->GetActorLocation(), Vector3(0.f, 5.f, 0.f), 1.0e-3f)) ? TRUE : FALSE;

				PSceneComponent* LoadedOffset = nullptr;
				const TArray<PSceneComponent*> SceneComponents = LoadedParent->GetComponents<PSceneComponent>();
				for (UINT32 ComponentIndex = 0u, ComponentNum = SceneComponents.Num<UINT32>(); ComponentIndex < ComponentNum; ComponentIndex++)
				{
					if (IsVectorNearlyEqual(SceneComponents[ComponentIndex]->GetTransform().GetLocation_Local(), Vector3(10.f, 20.f, 30.f), 1.0e-3f))
					{
						LoadedOffset = SceneComponents[ComponentIndex];
						break;
					}
				}
				if (LoadedOffset)
				{
					BOOL32 bChildUnderOffset = FALSE;
					const TArray<PSceneComponent*> OffsetChildren = LoadedOffset->GetChildrenComponents();
					for (UINT32 ChildIndex = 0u, ChildNum = OffsetChildren.Num<UINT32>(); ChildIndex < ChildNum; ChildIndex++)
					{
						if (IsVectorNearlyEqual(OffsetChildren[ChildIndex]->GetTransform().GetLocation_Local(), Vector3(-1.f, 0.f, 2.f), 1.0e-3f))
						{
							bChildUnderOffset = TRUE;
							break;
						}
					}
					bComponentTreeOk = ((LoadedOffset->GetMobility() == EMobilityType::EMT_STATIC) && bChildUnderOffset) ? TRUE : FALSE;

					PStaticMeshComponent* LoadedMesh = LoadedParent->FindComponent<PStaticMeshComponent>();
					bPropertiesOk = ((LoadedMesh != nullptr) && (LoadedMesh->IsCastShadow() == FALSE)) ? TRUE : FALSE;
				}
			}
		}

		// Collect the top-level loaded actors before destroying anything: children are
		// destroyed with their parent, and their pointers dangle afterwards.
		TArray<PActor*> LoadedRoots;
		for (UINT32 Index = 0u, Num = LoadedActors.Num<UINT32>(); Index < Num; Index++)
		{
			PActor* Actor = LoadedActors[Index];
			if (!Actor)
			{
				continue;
			}
			BOOL32 bHasLoadedParent = FALSE;
			for (UINT32 OtherIndex = 0u; OtherIndex < Num; OtherIndex++)
			{
				if (LoadedActors[OtherIndex] == Actor->GetAttachedParentActor())
				{
					bHasLoadedParent = TRUE;
					break;
				}
			}
			if (!bHasLoadedParent)
			{
				LoadedRoots.Add(Actor);
			}
		}
		for (UINT32 Index = 0u, Num = LoadedRoots.Num<UINT32>(); Index < Num; Index++)
		{
			InWorld->DestroyActor(LoadedRoots[Index]);
		}
		InWorld->FlushPendingDestroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			PE_LOG_ERROR(EString(PE_TEXT("level serialization: alive count before ")) + EString::FromInt(static_cast<INT32>(AliveBefore)) + EString(PE_TEXT(", after ")) + EString::FromInt(static_cast<INT32>(EObjectBase::GetAliveObjectCount())));
			return FALSE;
		}
#endif
		const BOOL32 bAllOk = (bDocumentOk && bLoaded && bHierarchyOk && bComponentTreeOk && bPropertiesOk) ? TRUE : FALSE;
		if (!bAllOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("level serialization detail: document=")) + EString::FromInt(bDocumentOk) +
				EString(PE_TEXT(" loaded=")) + EString::FromInt(bLoaded) +
				EString(PE_TEXT(" hierarchy=")) + EString::FromInt(bHierarchyOk) +
				EString(PE_TEXT(" tree=")) + EString::FromInt(bComponentTreeOk) +
				EString(PE_TEXT(" properties=")) + EString::FromInt(bPropertiesOk));
		}
		return bAllOk;
	}

	// Exercises the property tables added for the core component types: every value is set
	// on the source actor, saved, loaded into a fresh actor and compared. Values are chosen
	// distinct from the constructor defaults so a missing property registration fails the
	// check instead of passing on the default.
	static BOOL32 CheckComponentPropertySerialization(PWorld* InWorld)
	{
		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityPropertyActor");
		InWorld->AddActor(Actor);

		PCameraComponent* Camera = new PCameraComponent();
		Camera->SetFov(75.f);
		Camera->SetNearDistance(0.5f);
		Camera->SetFarDistance(1234.f);
		Camera->SetExposureValue100(10.f);
		Camera->SetISO(200.f);
		Camera->SetExposureCompensation(0.5f);
		Camera->SetIsMainCamera(FALSE);
		Actor->AddComponent(Camera);

		PDirectionalLightComponent* Directional = new PDirectionalLightComponent();
		Directional->SetLightColor(Color3(1.f, 0.5f, 0.25f));
		Directional->SetShadowMapSize(TRUE, Vector2Int(1024, 1024));
		Directional->SetIlluminanceLux(10000.f);
		Directional->SetSourceAngleDegrees(0.53f);
		{
			const FLOAT CascadeLayers[3] = { 100.f, 300.f, 1000.f };
			const FLOAT CascadeBorders[2] = { 10.f, 50.f };
			Directional->SetCascadeShadowData(ECascadeShadowData(TRUE, CascadeLayers, CascadeBorders, 3u));
		}
		Actor->AddComponent(Directional);

		PPointLightComponent* Point = new PPointLightComponent();
		Point->SetLightColor(Color3(0.9f, 0.8f, 0.7f));
		Point->SetLuminousIntensityCandela(500.f);
		Point->SetAttenuationRadiusMeters(100.f);
		Point->SetSourceRadiusMeters(0.1f);
		Actor->AddComponent(Point);

		PSkyLightComponent* Sky = new PSkyLightComponent();
		Sky->SetLightAdjust(Color3(0.9f, 0.95f, 1.f));
		Sky->SetIntensity(5000.f);
		Sky->SetPrefilterResolutionExponent(7u);
		Actor->AddComponent(Sky);

		PPhysicsComponent* Physics = new PPhysicsComponent();
		Physics->MotionType = PhysicsUtility::EMotionType::Kinematic;
		Physics->Layer = FPhysicsObjectLayer(1);
		Physics->SetShapeForSerialization(new FBoxShape(Vector3(10.f, 20.f, 30.f), 0.05f));
		Actor->AddComponent(Physics);

		PTriggerComponent* Trigger = new PTriggerComponent();
		Trigger->SetBackend(ETriggerBackendType::SENSOR);
		Trigger->SensorMotionType = PhysicsUtility::EMotionType::Static;
		Trigger->SensorLayer = FPhysicsObjectLayer(2);
		Trigger->bDetectStaticBodies = TRUE;
		Trigger->bDetectCharacters = FALSE;
		Trigger->bIgnoreOwnerActor = FALSE;
		Trigger->bFireStayEvents = TRUE;
		Trigger->MaxOverlaps = 12u;
		Trigger->SetShape(new FSphereShape(42.f), TRUE);
		Actor->AddComponent(Trigger);

		PNavMeshComponent* NavMesh = new PNavMeshComponent();
		NavMesh->SetBuildOnBeginPlay(FALSE);
		NavMesh->SetBakeVolumeExtent(Vector3(100.f, 200.f, 300.f));
		{
			ENavMeshBuildSettings BuildSettings;
			BuildSettings.CellSize = 21.f;
			BuildSettings.TileSizeCells = 64u;
			BuildSettings.MaxCrowdAgents = 8u;
			NavMesh->SetBuildSettings(BuildSettings);
		}
		Actor->AddComponent(NavMesh);

		PNavObstacleComponent* Obstacle = new PNavObstacleComponent();
		Obstacle->SetShapeType(NAVIGATION_OBSTACLE_BOX);
		Obstacle->SetBoxHalfExtents(Vector3(40.f, 75.f, 40.f));
		Obstacle->FollowThreshold = 7.f;
		Obstacle->bFollowTransform = FALSE;
		Actor->AddComponent(Obstacle);

		PNavAgentComponent* Agent = new PNavAgentComponent();
		{
			ENavigationAgentSettings AgentSettings;
			AgentSettings.MoveSpeed = 450.f;
			AgentSettings.AcceptanceRadius = 15.f;
			AgentSettings.ObstacleAvoidanceType = 2u;
			Agent->SetSettings(AgentSettings);
			Agent->SetUseCrowd(TRUE);
		}
		Actor->AddComponent(Agent);

		TArray<PActor*> Actors;
		Actors.Add(Actor);
		CJsonObject Document;
		const BOOL32 bSaved = ELevelSerializer::SaveActors(Actors, Document);

		InWorld->DestroyActor(Actor);
		InWorld->FlushPendingDestroy();

		TArray<PActor*> CreatedActors;
		const BOOL32 bLoaded = ELevelSerializer::LoadActors(*InWorld, Document, &CreatedActors);

		BOOL32 bPropertiesOk = FALSE;
		if (bSaved && bLoaded && (CreatedActors.Num<UINT32>() == 1u) && CreatedActors[0])
		{
			PActor* Loaded = CreatedActors[0];
			PCameraComponent* LoadedCamera = Loaded->FindComponent<PCameraComponent>();
			PDirectionalLightComponent* LoadedDirectional = Loaded->FindComponent<PDirectionalLightComponent>();
			PPointLightComponent* LoadedPoint = Loaded->FindComponent<PPointLightComponent>();
			PSkyLightComponent* LoadedSky = Loaded->FindComponent<PSkyLightComponent>();
			PPhysicsComponent* LoadedPhysics = Loaded->FindComponent<PPhysicsComponent>();
			PTriggerComponent* LoadedTrigger = Loaded->FindComponent<PTriggerComponent>();
			PNavMeshComponent* LoadedNavMesh = Loaded->FindComponent<PNavMeshComponent>();
			PNavObstacleComponent* LoadedObstacle = Loaded->FindComponent<PNavObstacleComponent>();
			PNavAgentComponent* LoadedAgent = Loaded->FindComponent<PNavAgentComponent>();

			const BOOL32 bCameraOk = LoadedCamera &&
				(EMath::Abs(LoadedCamera->GetCameraViewInfo().FovAngleY - 75.f) <= 1.0e-3f) &&
				(EMath::Abs(LoadedCamera->GetCameraViewInfo().NearDist - 0.5f) <= 1.0e-4f) &&
				(EMath::Abs(LoadedCamera->GetCameraViewInfo().FarDist - 1234.f) <= 1.0e-3f) &&
				(EMath::Abs(LoadedCamera->GetExposureValue100() - 10.f) <= 1.0e-4f) &&
				(EMath::Abs(LoadedCamera->GetISO() - 200.f) <= 1.0e-3f) &&
				(EMath::Abs(LoadedCamera->GetExposureCompensation() - 0.5f) <= 1.0e-4f);

			BOOL32 bLightOk = LoadedDirectional && LoadedPoint && LoadedSky;
			if (bLightOk)
			{
				const ECascadeShadowData* Cascade = LoadedDirectional->GetCascadeShadowData();
				bLightOk = (LoadedDirectional->IsLightCastShadow() != FALSE) &&
					(LoadedDirectional->GetShadowMapSize().x == 1024) &&
					(LoadedDirectional->GetShadowMapSize().y == 1024) &&
					(EMath::Abs(LoadedDirectional->GetIlluminanceLux() - 10000.f) <= 1.0e-2f) &&
					(EMath::Abs(LoadedDirectional->GetSourceAngleDegrees() - 0.53f) <= 1.0e-4f) &&
					Cascade && (Cascade->Layers.Num<UINT32>() == 3u) && (Cascade->Borders.Num<UINT32>() == 2u) &&
					(EMath::Abs(LoadedPoint->GetLuminousIntensityCandela() - 500.f) <= 1.0e-2f) &&
					(EMath::Abs(LoadedPoint->GetAttenuationRadiusMeters() - 100.f) <= 1.0e-3f) &&
					(EMath::Abs(LoadedPoint->GetSourceRadiusMeters() - 0.1f) <= 1.0e-4f) &&
					(EMath::Abs(LoadedSky->GetIntensity() - 5000.f) <= 1.0e-2f) &&
					(LoadedSky->GetPrefilterResolutionExponent() == 7u);
			}

			BOOL32 bPhysicsOk = FALSE;
			if (LoadedPhysics)
			{
				const FBoxShape* Box = dynamic_cast<const FBoxShape*>(LoadedPhysics->GetShapeForSerialization());
				bPhysicsOk = Box &&
					(LoadedPhysics->MotionType == PhysicsUtility::EMotionType::Kinematic) &&
					(LoadedPhysics->Layer.GetValue() == 1u) &&
					IsVectorNearlyEqual(Box->HalfExtent, Vector3(10.f, 20.f, 30.f), 1.0e-4f) &&
					(EMath::Abs(Box->ConvexRadius - 0.05f) <= 1.0e-5f);
			}

			BOOL32 bTriggerOk = FALSE;
			if (LoadedTrigger)
			{
				const FSphereShape* Sphere = dynamic_cast<const FSphereShape*>(LoadedTrigger->GetShape());
				bTriggerOk = (LoadedTrigger->GetBackend() == ETriggerBackendType::SENSOR) &&
					(LoadedTrigger->SensorMotionType == PhysicsUtility::EMotionType::Static) &&
					(LoadedTrigger->SensorLayer.GetValue() == 2u) &&
					(LoadedTrigger->bDetectStaticBodies != FALSE) &&
					(LoadedTrigger->bDetectCharacters == FALSE) &&
					(LoadedTrigger->bFireStayEvents != FALSE) &&
					(LoadedTrigger->MaxOverlaps == 12u) &&
					Sphere && (EMath::Abs(Sphere->Raidus - 42.f) <= 1.0e-4f);
			}

			const BOOL32 bNavOk = LoadedNavMesh && LoadedObstacle && LoadedAgent &&
				(LoadedNavMesh->GetBuildOnBeginPlay() == FALSE) &&
				IsVectorNearlyEqual(LoadedNavMesh->GetBakeVolumeExtent(), Vector3(100.f, 200.f, 300.f), 1.0e-4f) &&
				(EMath::Abs(LoadedNavMesh->GetBuildSettings().CellSize - 21.f) <= 1.0e-4f) &&
				(LoadedNavMesh->GetBuildSettings().TileSizeCells == 64u) &&
				(LoadedNavMesh->GetBuildSettings().MaxCrowdAgents == 8u) &&
				(LoadedObstacle->GetShapeType() == NAVIGATION_OBSTACLE_BOX) &&
				IsVectorNearlyEqual(LoadedObstacle->GetBoxHalfExtents(), Vector3(40.f, 75.f, 40.f), 1.0e-4f) &&
				(EMath::Abs(LoadedObstacle->FollowThreshold - 7.f) <= 1.0e-5f) &&
				(LoadedObstacle->bFollowTransform == FALSE) &&
				(EMath::Abs(LoadedAgent->GetSettings().MoveSpeed - 450.f) <= 1.0e-4f) &&
				(EMath::Abs(LoadedAgent->GetSettings().AcceptanceRadius - 15.f) <= 1.0e-4f) &&
				(LoadedAgent->GetSettings().ObstacleAvoidanceType == 2u) &&
				(LoadedAgent->GetUseCrowd() != FALSE);

#if ENGINE_EDITOR_ONLY
			const BOOL32 bNameOk = (Loaded->GetDebugName() == EString(PE_TEXT("SanityPropertyActor"))) ? TRUE : FALSE;
#else
			const BOOL32 bNameOk = TRUE;
#endif
			bPropertiesOk = (bCameraOk && bLightOk && bPhysicsOk && bTriggerOk && bNavOk && bNameOk) ? TRUE : FALSE;
			if (!bPropertiesOk)
			{
				PE_LOG_ERROR(EString(PE_TEXT("component properties detail: camera=")) + EString::FromInt(bCameraOk) +
					EString(PE_TEXT(" light=")) + EString::FromInt(bLightOk) +
					EString(PE_TEXT(" physics=")) + EString::FromInt(bPhysicsOk) +
					EString(PE_TEXT(" trigger=")) + EString::FromInt(bTriggerOk) +
					EString(PE_TEXT(" nav=")) + EString::FromInt(bNavOk) +
					EString(PE_TEXT(" name=")) + EString::FromInt(bNameOk));
			}
		}

		for (UINT32 Index = 0u, Num = CreatedActors.Num<UINT32>(); Index < Num; Index++)
		{
			if (!CreatedActors[Index])
			{
				continue;
			}
			BOOL32 bHasLoadedParent = FALSE;
			for (UINT32 OtherIndex = 0u; OtherIndex < Num; OtherIndex++)
			{
				if (CreatedActors[OtherIndex] == CreatedActors[Index]->GetAttachedParentActor())
				{
					bHasLoadedParent = TRUE;
					break;
				}
			}
			if (!bHasLoadedParent)
			{
				InWorld->DestroyActor(CreatedActors[Index]);
			}
		}
		InWorld->FlushPendingDestroy();
		// Camera/light components create render proxies that the render thread deletes after
		// its next kick, so the alive-object count is not a valid leak check for this scene.
		// The actor is still destroyed through the normal path, which the remaining scenarios
		// and the registry checks cover.
		return bPropertiesOk;
	}

	// World header + level-file object references: the header round trips independently of
	// the actor list, and a water surface's simulation reference survives a disk round trip
	// through file-local ids instead of process-local unique ids.
	static BOOL32 CheckLevelHeaderAndReferences(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		CJsonObject WorldDocument;
		const BOOL32 bWorldSaved = ELevelSerializer::SaveWorld(*InWorld, EString(PE_TEXT("SanityWorld")), WorldDocument);
		FLevelHeader Header;
		const BOOL32 bHeaderOk = bWorldSaved && ELevelSerializer::ReadLevelHeader(WorldDocument, Header) &&
			(Header.LevelName == EString(PE_TEXT("SanityWorld"))) &&
			IsVectorNearlyEqual(Header.UpVector, InWorld->GetUpVector(), 1.0e-4f);

		PActor* SimulationActor = new PActor();
		POBJ_DEBUGNAME_SET(SimulationActor, "SanitySimulationActor");
		InWorld->AddActor(SimulationActor);
		PWaterSimulationComponent* Simulation = new PWaterSimulationComponent();
		SimulationActor->AddComponent(Simulation);

		PActor* SurfaceActor = new PActor();
		POBJ_DEBUGNAME_SET(SurfaceActor, "SanitySurfaceActor");
		InWorld->AddActor(SurfaceActor);
		PWaterSurfaceComponent* Surface = new PWaterSurfaceComponent();
		SurfaceActor->AddComponent(Surface);
		Surface->SetSimulation(Simulation);

		TArray<PActor*> Actors;
		Actors.Add(SimulationActor);
		Actors.Add(SurfaceActor);
		const EString Path = EString(EBaseSettings::ENGINE_LEVEL_PATH) + EString(PE_TEXT("SanityRefs.PLevel"));
		const BOOL32 bSaved = ELevelSerializer::SaveActorsToFile(Actors, Path);

		InWorld->DestroyActor(SimulationActor);
		InWorld->DestroyActor(SurfaceActor);
		InWorld->FlushPendingDestroy();

		TArray<PActor*> CreatedActors;
		const BOOL32 bLoaded = ELevelSerializer::LoadActorsFromFile(*InWorld, Path, &CreatedActors);
		EFileHelper::DeleteFile(Path);

		PWaterSurfaceComponent* LoadedSurface = nullptr;
		PWaterSimulationComponent* LoadedSimulation = nullptr;
		for (UINT32 Index = 0u, Num = CreatedActors.Num<UINT32>(); Index < Num; Index++)
		{
			if (!CreatedActors[Index])
			{
				continue;
			}
			if (PWaterSurfaceComponent* FoundSurface = CreatedActors[Index]->FindComponent<PWaterSurfaceComponent>())
			{
				LoadedSurface = FoundSurface;
			}
			if (PWaterSimulationComponent* FoundSimulation = CreatedActors[Index]->FindComponent<PWaterSimulationComponent>())
			{
				LoadedSimulation = FoundSimulation;
			}
		}
		const BOOL32 bReferenceOk = bLoaded && LoadedSurface && LoadedSimulation &&
			LoadedSurface->GetSimulationHandle().IsValid() &&
			(LoadedSurface->GetSimulation() == LoadedSimulation);

		for (UINT32 Index = 0u, Num = CreatedActors.Num<UINT32>(); Index < Num; Index++)
		{
			if (!CreatedActors[Index])
			{
				continue;
			}
			BOOL32 bHasLoadedParent = FALSE;
			for (UINT32 OtherIndex = 0u; OtherIndex < Num; OtherIndex++)
			{
				if (CreatedActors[OtherIndex] == CreatedActors[Index]->GetAttachedParentActor())
				{
					bHasLoadedParent = TRUE;
					break;
				}
			}
			if (!bHasLoadedParent)
			{
				InWorld->DestroyActor(CreatedActors[Index]);
			}
		}
		InWorld->FlushPendingDestroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return (bHeaderOk && bSaved && bReferenceOk) ? TRUE : FALSE;
	}

	// A format-1 document with an older component ClassVersion: the v1 reader path must
	// accept it and the registered migrator must run after the properties were read.
	static BOOL32 CheckLevelFormatV1Migration(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		static BOOL32 bMigrationRegistered = FALSE;
		if (!bMigrationRegistered)
		{
			SetClassMigration<PSceneComponent>(1u, [](PSceneComponent& Component, UINT32 InFromVersion) -> void
			{
				if (InFromVersion < 1u)
				{
					Component.SetMobility(EMobilityType::EMT_STATION);
				}
			});
			bMigrationRegistered = TRUE;
		}

		const EString DocumentText = PE_TEXT("{\"FormatVersion\":1,\"Actors\":{\"Count\":1,\"0\":{\"Class\":\"PActor\",\"ClassVersion\":1,\"ParentActorID\":0,\"Properties\":{},\"Components\":{\"Count\":1,\"0\":{\"Class\":\"PSceneComponent\",\"ClassVersion\":0,\"Root\":true,\"ParentComponentIndex\":0,\"Properties\":{\"Mobility\":\"EMT_STATIC\"}}}}}}");
		CJsonObject Document(DocumentText);
		if (!Document.IsValid())
		{
			return FALSE;
		}

		TArray<PActor*> CreatedActors;
		const BOOL32 bLoaded = ELevelSerializer::LoadActors(*InWorld, Document, &CreatedActors);
		BOOL32 bMigrated = FALSE;
		if (bLoaded && (CreatedActors.Num<UINT32>() == 1u) && CreatedActors[0] && CreatedActors[0]->GetRootComponent())
		{
			bMigrated = (CreatedActors[0]->GetRootComponent()->GetMobility() == EMobilityType::EMT_STATION) ? TRUE : FALSE;
		}

		for (UINT32 Index = 0u, Num = CreatedActors.Num<UINT32>(); Index < Num; Index++)
		{
			if (!CreatedActors[Index])
			{
				continue;
			}
			BOOL32 bHasLoadedParent = FALSE;
			for (UINT32 OtherIndex = 0u; OtherIndex < Num; OtherIndex++)
			{
				if (CreatedActors[OtherIndex] == CreatedActors[Index]->GetAttachedParentActor())
				{
					bHasLoadedParent = TRUE;
					break;
				}
			}
			if (!bHasLoadedParent)
			{
				InWorld->DestroyActor(CreatedActors[Index]);
			}
		}
		InWorld->FlushPendingDestroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return (bLoaded && bMigrated) ? TRUE : FALSE;
	}

	// PWorld::SaveLevel / LoadLevel end to end on a disposable world: header, actor file,
	// clear-and-reload and the name/transform round trip. This is the path the editor File
	// menu uses, so it is tested without disturbing the live editor world.
	static BOOL32 CheckLevelFileIO()
	{
		const EString Path = EString(EBaseSettings::ENGINE_LEVEL_PATH) + EString(PE_TEXT("SanityWorld.PLevel"));

		PSanityLevelWorld* SaveWorld = new PSanityLevelWorld();
		SaveWorld->Init();
		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityFileActor");
		SaveWorld->AddActor(Actor);
		PSceneComponent* SceneComponent = new PSceneComponent();
		Actor->AddComponent(SceneComponent);
		SceneComponent->SetComponentLocation(Vector3(11.f, 22.f, 33.f));
		{
			FWorldGravitySettings GravitySettings;
			GravitySettings.GravityDirection = Vector3(0.f, -1.f, 0.f);
			GravitySettings.GravityStrength = 500.f;
			GravitySettings.bUseUpVector = FALSE;
			SaveWorld->SetGravitySettings(GravitySettings);
		}
		const BOOL32 bSaved = SaveWorld->SaveLevel(Path);
		SaveWorld->Destroy();

		PSanityLevelWorld* LoadWorld = new PSanityLevelWorld();
		LoadWorld->Init();
		const BOOL32 bLoaded = LoadWorld->LoadLevel(Path);
		BOOL32 bContentOk = FALSE;
		const TArray<PActor*> Actors = LoadWorld->GetAllActors();
		if (bLoaded && (Actors.Num<UINT32>() == 1u) && Actors[0])
		{
			PActor* Loaded = Actors[0];
			BOOL32 bLocationOk = FALSE;
			const TArray<PSceneComponent*> SceneComponents = Loaded->GetComponents<PSceneComponent>();
			for (UINT32 Index = 0u, Num = SceneComponents.Num<UINT32>(); Index < Num; Index++)
			{
				if (IsVectorNearlyEqual(SceneComponents[Index]->GetTransform().GetLocation_Local(), Vector3(11.f, 22.f, 33.f), 1.0e-3f))
				{
					bLocationOk = TRUE;
					break;
				}
			}
#if ENGINE_EDITOR_ONLY
			const BOOL32 bNameOk = (Loaded->GetDebugName() == EString(PE_TEXT("SanityFileActor"))) ? TRUE : FALSE;
#else
			const BOOL32 bNameOk = TRUE;
#endif
			const BOOL32 bGravityOk = (EMath::Abs(LoadWorld->GetGravitySettings().GravityStrength - 500.f) <= 1.0e-3f) &&
				(LoadWorld->IsGravityUsingUpVector() == FALSE) ? TRUE : FALSE;
			bContentOk = (bLocationOk && bNameOk && bGravityOk) ? TRUE : FALSE;
		}
		LoadWorld->Destroy();
		EFileHelper::DeleteFile(Path);
		return (bSaved && bLoaded && bContentOk) ? TRUE : FALSE;
	}

	// Command stack end to end on a disposable world: spawn/undo/redo, add/delete component,
	// a property edit and the handle remap that lets an older property command address a
	// component recreated by a structural undo.
	static BOOL32 CheckWorldEditorCommands()
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		PSanityLevelWorld* World = new PSanityLevelWorld();
		World->Init();
		FWorldEditorCommandStack Stack;
		Stack.GetContext().World = World;

		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityCommandActor");
		World->AddActor(Actor);
		const BOOL32 bSpawnPushed = PushSpawnActorCommand(Stack, *World, Actor);

		PSceneComponent* Component = new PSceneComponent();
		POBJ_DEBUGNAME_SET(Component, "SanityCommandComponent");
		Actor->AddComponent(Component);
		const BOOL32 bAddPushed = PushAddComponentCommand(Stack, *World, Actor, Component);

		Component->SetMobility(EMobilityType::EMT_DYNAMIC);
		FPropertyValue BeforeMobility;
		BeforeMobility.Type = EPropertyValueType::ENUM;
		BeforeMobility.IntValue = static_cast<INT64>(EMobilityType::EMT_STATIC);
		FPropertyValue AfterMobility;
		AfterMobility.Type = EPropertyValueType::ENUM;
		AfterMobility.IntValue = static_cast<INT64>(EMobilityType::EMT_DYNAMIC);
		const BOOL32 bPropertyPushed = PushSetPropertyCommand(Stack, MakeObjectHandle(Component), TRUE,
			GetClassHashCode<PSceneComponent>(), EString(PE_TEXT("Mobility")), BeforeMobility, AfterMobility);

		const BOOL32 bDeletePushed = PushDeleteComponentCommand(Stack, *World, Component);
		// Find the added component by identity, not by type: the actor's default root is also
		// a PSceneComponent and is always first in the component list.
		auto FindCommandComponent = [](PActor* InActor) -> PSceneComponent*
		{
			const TArray<PSceneComponent*> SceneComponents = InActor ? InActor->GetComponents<PSceneComponent>() : TArray<PSceneComponent*>();
			for (UINT32 Index = 0u, Num = SceneComponents.Num<UINT32>(); Index < Num; Index++)
			{
				if ((SceneComponents[Index] != InActor->GetRootComponent()) &&
					(POBJ_DEBUGNAME_GET(SceneComponents[Index]) == EString(PE_TEXT("SanityCommandComponent"))))
				{
					return SceneComponents[Index];
				}
			}
			return nullptr;
		};
		BOOL32 bDeleteOk = (FindCommandComponent(Actor) == nullptr) ? TRUE : FALSE;

		// Undo the delete: the component comes back and older commands must find it.
		BOOL32 bUndoDeleteOk = Stack.Undo();
		PSceneComponent* Restored = FindCommandComponent(Actor);
		bUndoDeleteOk = (bUndoDeleteOk && Restored && (Restored->GetMobility() == EMobilityType::EMT_DYNAMIC)) ? TRUE : FALSE;

		const BOOL32 bUndoPropertyOk = Stack.Undo();
		const BOOL32 bUndoPropertyValue = (Restored && (Restored->GetMobility() == EMobilityType::EMT_STATIC)) ? TRUE : FALSE;
		const BOOL32 bRedoPropertyOk = Stack.Redo();
		const BOOL32 bRedoPropertyValue = (Restored && (Restored->GetMobility() == EMobilityType::EMT_DYNAMIC)) ? TRUE : FALSE;

		const BOOL32 bRedoDeleteOk = Stack.Redo();
		const BOOL32 bRedoDeleteValue = (FindCommandComponent(Actor) == nullptr) ? TRUE : FALSE;
		const BOOL32 bUndoDeleteAgainOk = Stack.Undo();
		const BOOL32 bUndoDeleteAgainValue = (FindCommandComponent(Actor) != nullptr) ? TRUE : FALSE;

		// The property edit was redone above, so it sits between the delete and the add in the
		// undo stack; undo it again before undoing the add itself.
		const BOOL32 bUndoPropertyAgainOk = Stack.Undo();
		const BOOL32 bUndoPropertyAgainValue = (FindCommandComponent(Actor) && (FindCommandComponent(Actor)->GetMobility() == EMobilityType::EMT_STATIC)) ? TRUE : FALSE;

		const BOOL32 bUndoAddOk = Stack.Undo();
		const BOOL32 bUndoAddValue = (FindCommandComponent(Actor) == nullptr) ? TRUE : FALSE;

		const BOOL32 bUndoSpawnOk = Stack.Undo();
		const BOOL32 bUndoSpawnValue = (World->GetAllActors().Num<UINT32>() == 0u) ? TRUE : FALSE;
		if (!bUndoSpawnValue)
		{
			PE_LOG_ERROR(EString(PE_TEXT("command stack: actors after undoing spawn: ")) + EString::FromInt(World->GetAllActors().Num<UINT32>()));
		}
		const BOOL32 bRedoSpawnOk = Stack.Redo();
		const BOOL32 bRedoSpawnValue = (World->GetAllActors().Num<UINT32>() == 1u) ? TRUE : FALSE;

		// Transform is the most common reflected edit; exercise its SetProperty path too.
		PActor* RecreatedActor = (World->GetAllActors().Num<UINT32>() > 0u) ? World->GetAllActors()[0] : nullptr;
		PSceneComponent* RecreatedRoot = RecreatedActor ? RecreatedActor->GetRootComponent() : nullptr;
		BOOL32 bTransformPushed = FALSE;
		BOOL32 bUndoTransformOk = FALSE;
		BOOL32 bUndoTransformValue = FALSE;
		BOOL32 bRedoTransformOk = FALSE;
		BOOL32 bRedoTransformValue = FALSE;
		if (RecreatedRoot)
		{
			FPropertyValue BeforeTransform;
			BeforeTransform.Type = EPropertyValueType::TRANSFORM;
			BeforeTransform.TransformValue = RecreatedRoot->GetTransform();
			RecreatedRoot->SetComponentLocation(Vector3(7.f, 8.f, 9.f));
			FPropertyValue AfterTransform;
			AfterTransform.Type = EPropertyValueType::TRANSFORM;
			AfterTransform.TransformValue = RecreatedRoot->GetTransform();
			bTransformPushed = PushSetPropertyCommand(Stack, MakeObjectHandle(RecreatedRoot), TRUE,
				GetClassHashCode<PSceneComponent>(), EString(PE_TEXT("Transform")), BeforeTransform, AfterTransform);
			bUndoTransformOk = Stack.Undo();
			bUndoTransformValue = IsVectorNearlyEqual(RecreatedRoot->GetComponentLocalLocation(), BeforeTransform.TransformValue.GetLocation_Local(), 1.0e-4f);
			bRedoTransformOk = Stack.Redo();
			bRedoTransformValue = IsVectorNearlyEqual(RecreatedRoot->GetComponentLocalLocation(), Vector3(7.f, 8.f, 9.f), 1.0e-4f);
		}

		// Delete the recreated actor through the same command and restore it.
		const BOOL32 bDeleteActorPushed = RecreatedActor ? PushDeleteActorCommand(Stack, *World, RecreatedActor) : FALSE;
		const BOOL32 bDeleteActorValue = (World->GetAllActors().Num<UINT32>() == 0u) ? TRUE : FALSE;
		const BOOL32 bUndoDeleteActorOk = Stack.Undo();
		const BOOL32 bUndoDeleteActorValue = (World->GetAllActors().Num<UINT32>() == 1u) ? TRUE : FALSE;

		Stack.Clear();
		World->Destroy();
		(void)bSpawnPushed;
		(void)bAddPushed;
		(void)bPropertyPushed;
		(void)bDeletePushed;
		(void)bDeleteOk;
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		const BOOL32 bOk = (bSpawnPushed && bAddPushed && bPropertyPushed && bDeletePushed && bUndoDeleteOk &&
			bUndoPropertyOk && bUndoPropertyValue && bRedoPropertyOk && bRedoPropertyValue &&
			bRedoDeleteOk && bRedoDeleteValue && bUndoDeleteAgainOk && bUndoDeleteAgainValue &&
			bUndoPropertyAgainOk && bUndoPropertyAgainValue &&
			bUndoAddOk && bUndoAddValue && bUndoSpawnOk && bUndoSpawnValue && bRedoSpawnOk && bRedoSpawnValue &&
			bTransformPushed && bUndoTransformOk && bUndoTransformValue && bRedoTransformOk && bRedoTransformValue &&
			bDeleteActorPushed && bDeleteActorValue && bUndoDeleteActorOk && bUndoDeleteActorValue) ? TRUE : FALSE;
		if (!bOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("command stack detail: pushed=")) + EString::FromInt(bSpawnPushed) + EString::FromInt(bAddPushed) + EString::FromInt(bPropertyPushed) + EString::FromInt(bDeletePushed) +
				EString(PE_TEXT(" undoDelete=")) + EString::FromInt(bUndoDeleteOk) +
				EString(PE_TEXT(" undoProp=")) + EString::FromInt(bUndoPropertyOk) + EString(PE_TEXT("/")) + EString::FromInt(bUndoPropertyValue) +
				EString(PE_TEXT(" redoProp=")) + EString::FromInt(bRedoPropertyOk) + EString(PE_TEXT("/")) + EString::FromInt(bRedoPropertyValue) +
				EString(PE_TEXT(" redoDelete=")) + EString::FromInt(bRedoDeleteOk) + EString(PE_TEXT("/")) + EString::FromInt(bRedoDeleteValue) +
				EString(PE_TEXT(" undoDelete2=")) + EString::FromInt(bUndoDeleteAgainOk) + EString(PE_TEXT("/")) + EString::FromInt(bUndoDeleteAgainValue) +
				EString(PE_TEXT(" undoProp2=")) + EString::FromInt(bUndoPropertyAgainOk) + EString(PE_TEXT("/")) + EString::FromInt(bUndoPropertyAgainValue) +
				EString(PE_TEXT(" undoAdd=")) + EString::FromInt(bUndoAddOk) + EString(PE_TEXT("/")) + EString::FromInt(bUndoAddValue) +
				EString(PE_TEXT(" undoSpawn=")) + EString::FromInt(bUndoSpawnOk) + EString(PE_TEXT("/")) + EString::FromInt(bUndoSpawnValue) +
				EString(PE_TEXT(" redoSpawn=")) + EString::FromInt(bRedoSpawnOk) + EString(PE_TEXT("/")) + EString::FromInt(bRedoSpawnValue) +
				EString(PE_TEXT(" transform=")) + EString::FromInt(bTransformPushed) + EString(PE_TEXT("/")) + EString::FromInt(bUndoTransformOk) + EString(PE_TEXT("/")) + EString::FromInt(bUndoTransformValue) + EString(PE_TEXT("/")) + EString::FromInt(bRedoTransformOk) + EString(PE_TEXT("/")) + EString::FromInt(bRedoTransformValue) +
				EString(PE_TEXT(" deleteActor=")) + EString::FromInt(bDeleteActorPushed) + EString(PE_TEXT("/")) + EString::FromInt(bDeleteActorValue) +
				EString(PE_TEXT(" undoDeleteActor=")) + EString::FromInt(bUndoDeleteActorOk) + EString(PE_TEXT("/")) + EString::FromInt(bUndoDeleteActorValue));
		}
		return bOk;
	}

	// Exercises the real camera projection, gizmo screen-space hit test and drag delta: a
	// world point on the X axis must project on screen, be hit as AXIS_X, and dragging to a
	// second point must produce the matching +X delta (the sign used to be inverted).
	static BOOL32 CheckGizmoProjectionAndDrag(PWorld* InWorld)
	{
		PController* Controller = InWorld ? InWorld->GetController() : nullptr;
		PCameraComponent* Camera = Controller ? Controller->GetCamera() : nullptr;
		if (!Camera)
		{
			return FALSE;
		}

		PActor* Actor = new PActor();
		InWorld->AddActor(Actor);
		PSceneComponent* Component = new PSceneComponent();
		Actor->AddComponent(Component);
		Component->SetComponentWorldLocation(Vector3::Zero());

		const Vector3 Origin = Component->GetComponentWorldLocation();
		const Vector3 PressWorld = Origin + Vector3(20.f, 0.f, 0.f);
		const Vector3 MoveWorld = Origin + Vector3(40.f, 0.f, 0.f);
		Vector3 PressScreen;
		Vector3 MoveScreen;
		const ERect ScreenRect(0.f, 0.f, EEngineSettings::ENGINE_SCREEN_WIDTH, EEngineSettings::ENGINE_SCREEN_HEIGHT);

		BOOL32 bHitOk = FALSE;
		BOOL32 bDragOk = FALSE;
		Vector3 DragOrigin = Origin;
		if (Camera->TransformWorldToScreen(PressWorld, PressScreen) &&
			Camera->TransformWorldToScreen(MoveWorld, MoveScreen))
		{
			const EGizmoAxis HitAxis = HitTestTranslationGizmo(Camera, Component, PressScreen.x, PressScreen.y, ScreenRect, DragOrigin);
			bHitOk = (HitAxis == EGizmoAxis::AXIS_X) && IsVectorNearlyEqual(DragOrigin, PressWorld, 0.75f);

			const Vector3 Delta = ComputeGizmoDragDelta(Camera, Component, EGizmoAxis::AXIS_X, DragOrigin, MoveScreen.x, MoveScreen.y, ScreenRect);
			bDragOk = IsVectorNearlyEqual(Delta, Vector3(20.f, 0.f, 0.f), 0.75f);
		}

		InWorld->DestroyActor(Actor);
		InWorld->FlushPendingDestroy();
		return (bHitOk && bDragOk) ? TRUE : FALSE;
	}

	// A plain C++ class used only by the container-property scenario: it exercises the
	// TArray/TMap serialization and the editor value metadata without polluting the OCA class
	// registry.
	struct FSanityContainerData
	{
		TArray<INT32> Numbers;
		TMap<EString, FLOAT> Weights;
	};

	static BOOL32 CheckContainerPropertySerialization()
	{
		static BOOL32 bPropertiesRegistered = FALSE;
		const size_t ClassHash = GetClassHashCode<FSanityContainerData>();
		if (!bPropertiesRegistered)
		{
			AddClassPropertyAccessors<FSanityContainerData, TArray<INT32>>(ClassHash, PE_TEXT("Numbers"),
				[](const FSanityContainerData* Data) -> TArray<INT32> { return Data->Numbers; },
				[](FSanityContainerData* Data, const TArray<INT32>& Value) -> void { Data->Numbers = Value; },
				static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
			AddClassPropertyAccessors<FSanityContainerData, TMap<EString, FLOAT>>(ClassHash, PE_TEXT("Weights"),
				[](const FSanityContainerData* Data) -> TMap<EString, FLOAT> { return Data->Weights; },
				[](FSanityContainerData* Data, const TMap<EString, FLOAT>& Value) -> void { Data->Weights = Value; },
				static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
			bPropertiesRegistered = TRUE;
		}

		FSanityContainerData Source;
		Source.Numbers.Add(1);
		Source.Numbers.Add(2);
		Source.Numbers.Add(3);
		Source.Weights.Add(EString(PE_TEXT("A")), 1.5f);
		Source.Weights.Add(EString(PE_TEXT("B")), 2.5f);

		// Editor metadata: the array element type and the map key/value types are available
		// without knowing the concrete C++ class.
		const FProperty* NumberProperty = nullptr;
		const FProperty* WeightProperty = nullptr;
		EClassPropertyRegistry::ForEachProperty(ClassHash, [&NumberProperty, &WeightProperty](const FProperty& Property) -> void
		{
			if (Property.Name == EString(PE_TEXT("Numbers")))
			{
				NumberProperty = &Property;
			}
			else if (Property.Name == EString(PE_TEXT("Weights")))
			{
				WeightProperty = &Property;
			}
		});

		BOOL32 bMetadataOk = FALSE;
		if (NumberProperty && WeightProperty && NumberProperty->GetValue && WeightProperty->GetValue)
		{
			FPropertyValue NumbersValue;
			NumberProperty->GetValue(&Source, NumbersValue);
			FPropertyValue WeightsValue;
			WeightProperty->GetValue(&Source, WeightsValue);
			bMetadataOk = (NumbersValue.Type == EPropertyValueType::ARRAY) &&
				(NumbersValue.ElementType == EPropertyValueType::INT32) &&
				(NumbersValue.ArrayValues.Num<UINT32>() == 3u) &&
				(WeightsValue.Type == EPropertyValueType::MAP) &&
				(WeightsValue.KeyType == EPropertyValueType::STRING) &&
				(WeightsValue.MapValueType == EPropertyValueType::FLOAT) &&
				(WeightsValue.MapEntries.Num<UINT32>() == 2u) ? TRUE : FALSE;

			FPropertyElement NewNumber;
			NewNumber.Type = EPropertyValueType::INT32;
			NewNumber.IntValue = 4;
			NumbersValue.ArrayValues.Add(NewNumber);
			NumberProperty->SetValue(&Source, NumbersValue);

			FPropertyMapEntry NewWeight;
			NewWeight.Key.Type = EPropertyValueType::STRING;
			NewWeight.Key.StringValue = EString(PE_TEXT("C"));
			NewWeight.Value.Type = EPropertyValueType::FLOAT;
			NewWeight.Value.DoubleValue = 3.5;
			WeightsValue.MapEntries.Add(NewWeight);
			WeightProperty->SetValue(&Source, WeightsValue);
		}

		CJsonObject Document;
		EJsonArchiveWriter Writer(Document);
		EClassPropertyRegistry::SerializeObject(Writer, ClassHash, &Source);

		FSanityContainerData Target;
		EJsonArchiveReader Reader(Document);
		EClassPropertyRegistry::SerializeObject(Reader, ClassHash, &Target);

		FLOAT AValue = 0.f;
		FLOAT BValue = 0.f;
		FLOAT CValue = 0.f;
		const BOOL32 bMapValues = Target.Weights.FindValue(EString(PE_TEXT("A")), AValue) &&
			Target.Weights.FindValue(EString(PE_TEXT("B")), BValue) &&
			Target.Weights.FindValue(EString(PE_TEXT("C")), CValue) ? TRUE : FALSE;
		const BOOL32 bValuesOk = (Target.Numbers.Num<UINT32>() == 4u) &&
			(Target.Numbers[0] == 1) && (Target.Numbers[1] == 2) && (Target.Numbers[2] == 3) && (Target.Numbers[3] == 4) &&
			bMapValues &&
			(EMath::Abs(AValue - 1.5f) <= 1.0e-4f) &&
			(EMath::Abs(BValue - 2.5f) <= 1.0e-4f) &&
			(EMath::Abs(CValue - 3.5f) <= 1.0e-4f) ? TRUE : FALSE;

		if (!(bMetadataOk && bValuesOk))
		{
			PE_LOG_ERROR(EString(PE_TEXT("container detail: metadata=")) + EString::FromInt(bMetadataOk) +
				EString(PE_TEXT(" values=")) + EString::FromInt(bValuesOk) +
				EString(PE_TEXT(" sourceNumbers=")) + EString::FromInt(Source.Numbers.Num<UINT32>()) +
				EString(PE_TEXT(" targetNumbers=")) + EString::FromInt(Target.Numbers.Num<UINT32>()) +
				EString(PE_TEXT(" targetWeights=")) + EString::FromInt(Target.Weights.Num<UINT32>()) +
				EString(PE_TEXT(" A=")) + EString::FromFloat(AValue) +
				EString(PE_TEXT(" B=")) + EString::FromFloat(BValue) +
				EString(PE_TEXT(" C=")) + EString::FromFloat(CValue));
		}
		return (bMetadataOk && bValuesOk) ? TRUE : FALSE;
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
		const BOOL32 bLevelSerialization = CheckLevelSerializationRoundTrip(InWorld);
		ReportSanityResult(EString(PE_TEXT("level serialization round trip")), bLevelSerialization);
		if (!bLevelSerialization)
		{
			FailedCount += 1u;
		}
		const BOOL32 bComponentProperties = CheckComponentPropertySerialization(InWorld);
		ReportSanityResult(EString(PE_TEXT("component property serialization")), bComponentProperties);
		if (!bComponentProperties)
		{
			FailedCount += 1u;
		}
		const BOOL32 bLevelHeaderAndReferences = CheckLevelHeaderAndReferences(InWorld);
		ReportSanityResult(EString(PE_TEXT("level header and object references")), bLevelHeaderAndReferences);
		if (!bLevelHeaderAndReferences)
		{
			FailedCount += 1u;
		}
		const BOOL32 bLevelV1Migration = CheckLevelFormatV1Migration(InWorld);
		ReportSanityResult(EString(PE_TEXT("level format v1 migration")), bLevelV1Migration);
		if (!bLevelV1Migration)
		{
			FailedCount += 1u;
		}
		const BOOL32 bLevelFileIO = CheckLevelFileIO();
		ReportSanityResult(EString(PE_TEXT("level file save and load")), bLevelFileIO);
		if (!bLevelFileIO)
		{
			FailedCount += 1u;
		}
		const BOOL32 bWorldEditorCommands = CheckWorldEditorCommands();
		ReportSanityResult(EString(PE_TEXT("world editor command stack")), bWorldEditorCommands);
		if (!bWorldEditorCommands)
		{
			FailedCount += 1u;
		}
		const BOOL32 bGizmoProjection = CheckGizmoProjectionAndDrag(InWorld);
		ReportSanityResult(EString(PE_TEXT("gizmo projection and drag")), bGizmoProjection);
		if (!bGizmoProjection)
		{
			FailedCount += 1u;
		}
		const BOOL32 bContainerProperties = CheckContainerPropertySerialization();
		ReportSanityResult(EString(PE_TEXT("container property serialization")), bContainerProperties);
		if (!bContainerProperties)
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
