#include <Editor/Test/FrameworkSanityCheck.h>

#if ENGINE_EDITOR_ONLY

#include "PigeonBase/Object/Actor.h"
#include "PigeonBase/Object/Actor/LevelCharacter.h"
#include "PigeonBase/Object/Component/Character/MovementComponent.h"
#include "PigeonBase/Object/World/World.h"
#include "PigeonBase/Object/Component/ActorComponent.h"
#include "PigeonBase/Object/Component/SceneComponent.h"
#include "PigeonBase/Object/Component/Primitive/StaticMeshComponent.h"
#include "PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h"
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
#include <Navigation/NavigationGeometry.h>
#include <Navigation/RecastNavigationProvider.h>
#include "PigeonBase/Object/Component/Water/WaterSurfaceComponent.h"
#include "PigeonBase/Object/Component/Water/WaterSimulationComponent.h"
#include "../../../EngineThirdParty/JoltPhysics/Headers/Shapes.h"
#include "PigeonBase/Object/Controller/Controller.h"
#include "HID/Input/Input.h"
#include "Development/Message/Message.h"
#include <Reflection/Property.h>
#include <Reflection/JsonArchive.h>
#include <Editor/Customization/EditorCustomization.h>
#include <Editor/Manager/EditorWorldContext.h>
#include "PigeonBase/Object/World/WorldManager.h"
#include <Reflection/LevelSerializer.h>
#include <RapidJson/Headers/JsonObject.h>
#include <Development/EditorLogManager.h>
#include <Editor/Commands/WorldEditorCommand.h>
#include <Editor/Manager/EditorHelper.h>
#include <Editor/Panels/PropertyEditor.h>

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
	// the (priority, registration) order the scheduler runs entries in.
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

	// Actor/component pair for the scheduler domains. Every callback appends a distinct tag
	// to its own log so cross-actor order, domain isolation, and tickable toggles are visible.
	class PSanitySchedulerActor : public PActor
	{
	public:
		PSanitySchedulerActor(TArray<UINT32>* InUserLog, TArray<UINT32>* InEditorLog, const UINT32 InTag)
			: UserLog(InUserLog), EditorLog(InEditorLog), Tag(InTag)
		{
			SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(this, "SanitySchedulerActor");
		}

	protected:
		void UserTick(FLOAT deltaTime) override
		{
			(void)deltaTime;
			if (UserLog)
			{
				UserLog->Add(Tag);
			}
		}
#if ENGINE_EDITOR_ONLY
	public:
		void EditorTick(FLOAT deltaTime) override
		{
			(void)deltaTime;
			if (EditorLog)
			{
				EditorLog->Add(Tag);
			}
		}
#endif

	private:
		TArray<UINT32>* UserLog = nullptr;
		TArray<UINT32>* EditorLog = nullptr;
		UINT32 Tag = 0u;
	};

	class PSanitySchedulerComponent : public PActorComponent
	{
	public:
		PSanitySchedulerComponent(TArray<UINT32>* InTickLog, TArray<UINT32>* InFixedLog,
			TArray<UINT32>* InPostPhysicsLog, TArray<UINT32>* InEditorLog, const UINT32 InTag)
			: TickLog(InTickLog), FixedLog(InFixedLog), PostPhysicsLog(InPostPhysicsLog)
			, EditorLog(InEditorLog), Tag(InTag)
		{
			SetIsTickable(TRUE);
			POBJ_DEBUGNAME_SET(this, "SanitySchedulerComponent");
		}

		void Tick(FLOAT deltaTime) override
		{
			(void)deltaTime;
			if (TickLog)
			{
				TickLog->Add(Tag);
			}
		}

		void FixedTick(FLOAT deltaTime) override
		{
			(void)deltaTime;
			if (FixedLog)
			{
				FixedLog->Add(Tag);
			}
		}

	protected:
		void HandlePostPhysicsTick(FLOAT deltaTime) override
		{
			(void)deltaTime;
			if (PostPhysicsLog)
			{
				PostPhysicsLog->Add(Tag);
			}
		}

		void BeginAddedToScene(PWorld* World) override
		{
			PActorComponent::BeginAddedToScene(World);
			RegisterPostPhysicsTick();
		}

#if ENGINE_EDITOR_ONLY
	public:
		void EditorTick(FLOAT deltaTime) override
		{
			(void)deltaTime;
			if (EditorLog)
			{
				EditorLog->Add(Tag);
			}
		}
#endif

	private:
		TArray<UINT32>* TickLog = nullptr;
		TArray<UINT32>* FixedLog = nullptr;
		TArray<UINT32>* PostPhysicsLog = nullptr;
		TArray<UINT32>* EditorLog = nullptr;
		UINT32 Tag = 0u;
	};

	static BOOL32 CheckRenderTreeDirtySkip(PWorld* InWorld)
	{
		class PCountingSceneComponent : public PSceneComponent
		{
		public:
			PCountingSceneComponent(UINT32* InTickCount) : TickCount(InTickCount) {}
			void Wake()
			{
				MarkRenderStateAsDirty();
			}
		protected:
			virtual void TickRenderImpl() override
			{
				if (TickCount)
				{
					(*TickCount)++;
				}
				PSceneComponent::TickRenderImpl();
			}
		private:
			UINT32* TickCount = nullptr;
		};

		UINT32 TickCount = 0u;
		PActor* Actor = new PActor();
		Actor->SetIsTickable(FALSE);
		InWorld->AddActor(Actor);
		PCountingSceneComponent* Component = new PCountingSceneComponent(&TickCount);
		Actor->AddComponent(Component);

		InWorld->Tick(1.0f / 60.0f);
		const UINT32 FirstTickCount = TickCount;
		InWorld->Tick(1.0f / 60.0f);
		const BOOL32 bCleanSkipWorks = (TickCount == FirstTickCount) ? TRUE : FALSE;

		Component->Wake();
		InWorld->Tick(1.0f / 60.0f);
		const BOOL32 bDirtyWakeWorks = (TickCount == (FirstTickCount + 1u)) ? TRUE : FALSE;

		INT32 DeferredAttempts = 0;
		Component->AddDeferredApply([&DeferredAttempts]() -> BOOL32
		{
			DeferredAttempts++;
			return (DeferredAttempts >= 2) ? TRUE : FALSE;
		});
		InWorld->Tick(1.0f / 60.0f);
		InWorld->Tick(1.0f / 60.0f);
		const BOOL32 bDeferredWakeWorks = ((DeferredAttempts == 2) && (TickCount == (FirstTickCount + 3u))) ? TRUE : FALSE;
		const UINT32 DeferredTickCount = TickCount;
		InWorld->Tick(1.0f / 60.0f);
		const BOOL32 bDeferredCleanSkipWorks = (TickCount == DeferredTickCount) ? TRUE : FALSE;

		InWorld->DestroyActor(Actor);
		InWorld->FlushPendingDestroy();
		return (bCleanSkipWorks && bDirtyWakeWorks && bDeferredWakeWorks && bDeferredCleanSkipWorks) ? TRUE : FALSE;
	}

	// The Material picker must list authoring material sources, not compiled shader/stage
	// artifacts. Engine materials and project-folder materials are both valid identifiers.
	static BOOL32 CheckMaterialSourceCatalog()
	{
		TArray<EString> Names;
		EPropertyEditor::CollectAssetNames(EPropertyAssetType::MATERIAL, Names);
		BOOL32 bHasEngineMaterial = FALSE;
		BOOL32 bHasProjectMaterial = FALSE;
		BOOL32 bHasCompiledArtifact = FALSE;
		for (UINT32 Index = 0u, Num = Names.Num<UINT32>(); Index < Num; Index++)
		{
			const EString& Name = Names[Index];
			if (Name == EString(PE_TEXT("M_EngineFallback")))
			{
				bHasEngineMaterial = TRUE;
			}
			if (Name == EString(PE_TEXT("Projects/Demo/M_DemoUnlit")))
			{
				bHasProjectMaterial = TRUE;
			}
			if (Name.Left(3u) == EString(PE_TEXT("PS_")) || Name.Left(3u) == EString(PE_TEXT("VS_")) ||
				Name.Left(3u) == EString(PE_TEXT("CS_")))
			{
				bHasCompiledArtifact = TRUE;
			}
		}
		if ((!bHasEngineMaterial) || (!bHasProjectMaterial) || bHasCompiledArtifact)
		{
			EString CatalogDebug;
			for (UINT32 Index = 0u, Num = Names.Num<UINT32>(); Index < Num; Index++)
			{
				CatalogDebug += EString(PE_TEXT("[")) + Names[Index] + EString(PE_TEXT("]"));
			}
			PE_LOG_ERROR(EString(PE_TEXT("material source catalog detail: ")) + CatalogDebug);
		}
		return (bHasEngineMaterial && bHasProjectMaterial && (!bHasCompiledArtifact)) ? TRUE : FALSE;
	}

	static BOOL32 CheckComponentLookupAndTickOrder(PWorld* InWorld)
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		TArray<UINT32> TickLog;
		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityLookupActor");
		// Component ticks are independent of the actor's UserTick switch: the actor stays
		// non-tickable here, while its tickable components still run in cached order.
		InWorld->AddActor(Actor);

		// Registration order 1, 2, 3 with different priorities. Expected tick order:
		// priority -10 (2), priority -1 (3), priority 5 (1).
		PSanityTickOrderComponent* GamePlayLate = new PSanityTickOrderComponent(&TickLog, 1u);
		GamePlayLate->SetTickPriority(5);
		Actor->AddComponent(GamePlayLate);
		PSanityTickOrderComponent* EarlyComponent = new PSanityTickOrderComponent(&TickLog, 2u);
		EarlyComponent->SetTickPriority(-10);
		Actor->AddComponent(EarlyComponent);
		PSanityTickOrderComponent* GamePlayEarly = new PSanityTickOrderComponent(&TickLog, 3u);
		GamePlayEarly->SetTickPriority(-1);
		Actor->AddComponent(GamePlayEarly);

		// Template lookups: first exact match, all exact matches in registration order,
		// and a base-class match through the same list.
		const BOOL32 bFindWorks = (Actor->FindComponent<PSanityTickOrderComponent>() == GamePlayLate) ? TRUE : FALSE;
		const BOOL32 bListWorks = (Actor->GetComponents<PSanityTickOrderComponent>().Num<UINT32>() == 3u) ? TRUE : FALSE;
		const BOOL32 bBaseLookupWorks = (Actor->FindComponent<PSceneComponent>() == Actor->GetRootComponent()) ? TRUE : FALSE;

		InWorld->Tick(1.f / 60.f);
		const BOOL32 bActorNotTickable = (Actor->IsTickable() == FALSE) ? TRUE : FALSE;
		const BOOL32 bOrderMatches =
			((TickLog.Num<UINT32>() == 3u) && (TickLog[0] == 2u) && (TickLog[1] == 3u) && (TickLog[2] == 1u)) ? TRUE : FALSE;

		// Changing the component's tickable flag after it joined the actor must refresh the
		// cached tickable list immediately.
		TickLog.Empty();
		GamePlayEarly->SetIsTickable(FALSE);
		InWorld->Tick(1.f / 60.f);
		const BOOL32 bDisableTickableWorks =
			((TickLog.Num<UINT32>() == 2u) && (TickLog[0] == 2u) && (TickLog[1] == 1u)) ? TRUE : FALSE;
		TickLog.Empty();
		GamePlayEarly->SetIsTickable(TRUE);
		InWorld->Tick(1.f / 60.f);
		const BOOL32 bEnableTickableWorks =
			((TickLog.Num<UINT32>() == 3u) && (TickLog[0] == 2u) && (TickLog[1] == 3u) && (TickLog[2] == 1u)) ? TRUE : FALSE;

		InWorld->DestroyActor(Actor);
		InWorld->FlushPendingDestroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		return (bFindWorks && bListWorks && bBaseLookupWorks && bActorNotTickable && bOrderMatches &&
			bDisableTickableWorks && bEnableTickableWorks) ? TRUE : FALSE;
	}

	// Exercises the per-world scheduler directly: global priority order across actors,
	// actor UserTick placement, fixed/post-physics/editor domains, tickability toggles,
	// component destruction, and two-world isolation.
	static BOOL32 CheckWorldTickScheduler()
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		TArray<UINT32> UserLog;
		TArray<UINT32> FixedLog;
		TArray<UINT32> PostPhysicsLog;
		TArray<UINT32> EditorLog;
		TArray<UINT32> OtherUserLog;
		TArray<UINT32> OtherEditorLog;

		PSanityLevelWorld* WorldA = new PSanityLevelWorld();
		WorldA->Init();
		PSanityLevelWorld* WorldB = new PSanityLevelWorld();
		WorldB->Init();

		PSanitySchedulerActor* ActorA = new PSanitySchedulerActor(&UserLog, &EditorLog, 100u);
		POBJ_DEBUGNAME_SET(ActorA, "SanitySchedulerActorA");
		WorldA->AddActor(ActorA);
		PSanitySchedulerActor* ActorB = new PSanitySchedulerActor(&OtherUserLog, &OtherEditorLog, 200u);
		ActorB->SetIsTickable(FALSE);
		POBJ_DEBUGNAME_SET(ActorB, "SanitySchedulerActorB");
		WorldA->AddActor(ActorB);

		PSanitySchedulerComponent* Early = new PSanitySchedulerComponent(&UserLog, &FixedLog, &PostPhysicsLog, &EditorLog, 1u);
		Early->SetTickPriority(-10);
		ActorA->AddComponent(Early);
		PSanitySchedulerComponent* Default = new PSanitySchedulerComponent(&UserLog, &FixedLog, &PostPhysicsLog, &EditorLog, 2u);
		Default->SetTickPriority(0);
		ActorA->AddComponent(Default);
		PSanitySchedulerComponent* Middle = new PSanitySchedulerComponent(&UserLog, &FixedLog, &PostPhysicsLog, &EditorLog, 3u);
		Middle->SetTickPriority(-5);
		ActorB->AddComponent(Middle);

		PSanitySchedulerActor* OtherActor = new PSanitySchedulerActor(nullptr, nullptr, 0u);
		OtherActor->SetIsTickable(TRUE);
		POBJ_DEBUGNAME_SET(OtherActor, "SanitySchedulerOtherActor");
		WorldB->AddActor(OtherActor);
		PSanitySchedulerComponent* OtherComponent = new PSanitySchedulerComponent(&OtherUserLog, &FixedLog, &PostPhysicsLog, &EditorLog, 9u);
		OtherActor->AddComponent(OtherComponent);

		WorldA->Tick(1.0f / 60.0f);
		WorldA->FixedTick(1.0f / 60.0f);
		WorldA->PostPhysicsTick(1.0f / 60.0f);
		WorldA->EditorTick(1.0f / 60.0f);

		const BOOL32 bCrossActorOrderOk =
			(UserLog.Num<UINT32>() == 4u) &&
			(UserLog[0] == 1u) && (UserLog[1] == 3u) && (UserLog[2] == 2u) && (UserLog[3] == 100u) ? TRUE : FALSE;
		const BOOL32 bActorUserAfterComponents = (UserLog[3] == 100u) ? TRUE : FALSE;
		const BOOL32 bFixedOk =
			(FixedLog.Num<UINT32>() == 3u) &&
			(FixedLog[0] == 1u) && (FixedLog[1] == 3u) && (FixedLog[2] == 2u) ? TRUE : FALSE;
		const BOOL32 bPostPhysicsOk =
			(PostPhysicsLog.Num<UINT32>() == 3u) &&
			(PostPhysicsLog[0] == 1u) && (PostPhysicsLog[1] == 3u) && (PostPhysicsLog[2] == 2u) ? TRUE : FALSE;
		const BOOL32 bEditorOk =
			(EditorLog.Num<UINT32>() == 4u) &&
			(EditorLog[0] == 1u) && (EditorLog[1] == 3u) && (EditorLog[2] == 2u) && (EditorLog[3] == 100u) ? TRUE : FALSE;
		const BOOL32 bWorldIsolationOk = (OtherUserLog.Num<UINT32>() == 0u) ? TRUE : FALSE;

		WorldB->Tick(1.0f / 60.0f);
		const BOOL32 bOtherWorldTicks = (OtherUserLog.Find(9u) != -1) ? TRUE : FALSE;

		// Priority is an INT32 that can be changed after registration; the scheduler must
		// rebuild on the next tick without re-registering the entries.
		UserLog.Empty();
		Early->SetTickPriority(5);
		Default->SetTickPriority(-20);
		WorldA->Tick(1.0f / 60.0f);
		const BOOL32 bPriorityUpdateOk =
			(UserLog.Num<UINT32>() == 4u) &&
			(UserLog[0] == 2u) && (UserLog[1] == 3u) && (UserLog[2] == 100u) && (UserLog[3] == 1u) ? TRUE : FALSE;
		Early->SetTickPriority(-10);
		Default->SetTickPriority(0);

		UserLog.Empty();
		EditorLog.Empty();
		ActorA->SetIsTickable(FALSE);
		WorldA->Tick(1.0f / 60.0f);
		WorldA->EditorTick(1.0f / 60.0f);
		const BOOL32 bActorToggleOk =
			(UserLog.Num<UINT32>() == 3u) && (UserLog[0] == 1u) && (UserLog[1] == 3u) && (UserLog[2] == 2u) &&
			(EditorLog.Num<UINT32>() == 3u) ? TRUE : FALSE;
		ActorA->SetIsTickable(TRUE);

		WorldA->DestroyComponent(Middle);
		WorldA->FlushPendingDestroy();
		UserLog.Empty();
		WorldA->Tick(1.0f / 60.0f);
		const BOOL32 bComponentDestroyOk =
			(UserLog.Num<UINT32>() == 3u) &&
			(UserLog[0] == 1u) && (UserLog[1] == 2u) && (UserLog[2] == 100u) ? TRUE : FALSE;

		WorldA->Destroy();
		WorldB->Destroy();
#if ENGINE_DEBUG_MODE
		const BOOL32 bAliveOk = (EObjectBase::GetAliveObjectCount() == AliveBefore) ? TRUE : FALSE;
#else
		const BOOL32 bAliveOk = TRUE;
#endif
		const BOOL32 bOk = (bCrossActorOrderOk && bActorUserAfterComponents && bFixedOk && bPostPhysicsOk &&
			bEditorOk && bWorldIsolationOk && bOtherWorldTicks && bPriorityUpdateOk &&
			bActorToggleOk && bComponentDestroyOk && bAliveOk) ? TRUE : FALSE;
		if (!bOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("tick scheduler detail: order=")) + EString::FromInt(bCrossActorOrderOk) +
				EString(PE_TEXT(" userAfter=")) + EString::FromInt(bActorUserAfterComponents) +
				EString(PE_TEXT(" fixed=")) + EString::FromInt(bFixedOk) +
				EString(PE_TEXT(" post=")) + EString::FromInt(bPostPhysicsOk) +
				EString(PE_TEXT(" editor=")) + EString::FromInt(bEditorOk) +
				EString(PE_TEXT(" isolation=")) + EString::FromInt(bWorldIsolationOk) +
				EString(PE_TEXT(" other=")) + EString::FromInt(bOtherWorldTicks) +
				EString(PE_TEXT(" priority=")) + EString::FromInt(bPriorityUpdateOk) +
				EString(PE_TEXT(" toggle=")) + EString::FromInt(bActorToggleOk) +
				EString(PE_TEXT(" destroy=")) + EString::FromInt(bComponentDestroyOk) +
				EString(PE_TEXT(" alive=")) + EString::FromInt(bAliveOk));
		}
		return bOk;
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
		BOOL32 bComponentCountOk = FALSE;
		BOOL32 bRootNameOk = FALSE;
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
				// The loader adopts exactly the component set in the file. Construction-time
				// defaults, including the factory root, must not survive as extra components.
				bComponentCountOk = ((LoadedParent->GetAllComponents().Num<UINT32>() == 4u) &&
					(LoadedChild->GetAllComponents().Num<UINT32>() == 1u)) ? TRUE : FALSE;
				// A root's debug name carries the suffix once, no matter how often the actor
				// is loaded or saved.
				bRootNameOk = ((LoadedParent->GetRootComponent() != nullptr) &&
					(!POBJ_DEBUGNAME_GET(LoadedParent->GetRootComponent()).Contains(EString(PE_TEXT("(Root)(Root)"))))) ? TRUE : FALSE;

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
		const BOOL32 bAllOk = (bDocumentOk && bLoaded && bHierarchyOk && bComponentTreeOk && bPropertiesOk && bComponentCountOk && bRootNameOk) ? TRUE : FALSE;
		if (!bAllOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("level serialization detail: document=")) + EString::FromInt(bDocumentOk) +
				EString(PE_TEXT(" loaded=")) + EString::FromInt(bLoaded) +
				EString(PE_TEXT(" hierarchy=")) + EString::FromInt(bHierarchyOk) +
				EString(PE_TEXT(" tree=")) + EString::FromInt(bComponentTreeOk) +
				EString(PE_TEXT(" properties=")) + EString::FromInt(bPropertiesOk) +
				EString(PE_TEXT(" componentCount=")) + EString::FromInt(bComponentCountOk) +
				EString(PE_TEXT(" rootName=")) + EString::FromInt(bRootNameOk));
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

		PWaterSimulationComponent* Water = new PWaterSimulationComponent();
		{
			FWaterSimulationParameters Parameters = Water->GetSimulationParameters();
			Parameters.Gravity = 321.f;
			Parameters.OutlineAverage = 0.75f;
			Water->SetSimulationParameters(Parameters);
		}
		Actor->AddComponent(Water);

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
			PWaterSimulationComponent* LoadedWater = Loaded->FindComponent<PWaterSimulationComponent>();

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

			const BOOL32 bWaterOk = LoadedWater &&
				(EMath::Abs(LoadedWater->GetSimulationParameters().Gravity - 321.f) <= 1.0e-3f) &&
				(EMath::Abs(LoadedWater->GetSimulationParameters().OutlineAverage - 0.75f) <= 1.0e-5f);

#if ENGINE_EDITOR_ONLY
			const BOOL32 bNameOk = (Loaded->GetDebugName() == EString(PE_TEXT("SanityPropertyActor"))) ? TRUE : FALSE;
#else
			const BOOL32 bNameOk = TRUE;
#endif
			bPropertiesOk = (bCameraOk && bLightOk && bPhysicsOk && bTriggerOk && bNavOk && bWaterOk && bNameOk) ? TRUE : FALSE;
			if (!bPropertiesOk)
			{
				PE_LOG_ERROR(EString(PE_TEXT("component properties detail: camera=")) + EString::FromInt(bCameraOk) +
					EString(PE_TEXT(" light=")) + EString::FromInt(bLightOk) +
					EString(PE_TEXT(" physics=")) + EString::FromInt(bPhysicsOk) +
					EString(PE_TEXT(" trigger=")) + EString::FromInt(bTriggerOk) +
					EString(PE_TEXT(" nav=")) + EString::FromInt(bNavOk) +
					EString(PE_TEXT(" water=")) + EString::FromInt(bWaterOk) +
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
		// A non-serialized component before the simulation shifts the live component list but
		// not the file list; the object-reference map must use the same filtered order.
		SimulationActor->AddComponent(new PMovementComponent());
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

	// A combined acceptance round trip: several actor/component families, a parent-child
	// actor link and an object reference whose target is shifted by a non-serialized
	// component. The existing focused checks cover each property in isolation; this one
	// exercises the complete PWorld::SaveLevel/LoadLevel path the editor uses.
	static BOOL32 CheckLevelAcceptanceRoundTrip()
	{
		const EString Path = EString(EBaseSettings::ENGINE_LEVEL_PATH) + EString(PE_TEXT("SanityAcceptance.PLevel"));

		PSanityLevelWorld* SaveWorld = new PSanityLevelWorld();
		SaveWorld->Init();

		PActor* ContentActor = new PActor();
		POBJ_DEBUGNAME_SET(ContentActor, "SanityAcceptanceContent");
		SaveWorld->AddActor(ContentActor);
		PStaticMeshComponent* StaticMesh = new PStaticMeshComponent();
		FMaterialSlotReference MaterialReference;
		MaterialReference.MaterialName = EString(PE_TEXT("M_EngineFallback"));
		MaterialReference.ShaderName = EString(PE_TEXT("M_EngineFallback"));
		MaterialReference.ProgramName = EString(PE_TEXT("BasePass"));
		StaticMesh->SetMaterialSlotReference(0u, MaterialReference);
		ContentActor->AddComponent(StaticMesh);
		PSkeletalMeshComponent* SkeletalMesh = new PSkeletalMeshComponent();
		ContentActor->AddComponent(SkeletalMesh);
		PCameraComponent* Camera = new PCameraComponent();
		Camera->SetFov(83.f);
		ContentActor->AddComponent(Camera);
		PDirectionalLightComponent* Directional = new PDirectionalLightComponent();
		Directional->SetIlluminanceLux(12345.f);
		ContentActor->AddComponent(Directional);
		PPhysicsComponent* Physics = new PPhysicsComponent();
		Physics->SetShapeForSerialization(new FBoxShape(Vector3(11.f, 22.f, 33.f), 0.02f));
		ContentActor->AddComponent(Physics);
		PTriggerComponent* Trigger = new PTriggerComponent();
		Trigger->SetBackend(ETriggerBackendType::SENSOR);
		Trigger->SetShape(new FSphereShape(44.f), TRUE);
		ContentActor->AddComponent(Trigger);
		PNavMeshComponent* NavMesh = new PNavMeshComponent();
		{
			ENavMeshBuildSettings Settings = NavMesh->GetBuildSettings();
			Settings.CellSize = 31.f;
			Settings.MaxCrowdAgents = 7u;
			NavMesh->SetBuildSettings(Settings);
		}
		ContentActor->AddComponent(NavMesh);

		PActor* ChildActor = new PActor();
		POBJ_DEBUGNAME_SET(ChildActor, "SanityAcceptanceChild");
		SaveWorld->AddActor(ChildActor);
		PSceneComponent* ChildScene = new PSceneComponent();
		ChildScene->SetComponentLocation(Vector3(1.f, 2.f, 3.f));
		ChildActor->AddComponent(ChildScene);
		ChildActor->AttachToActor(ContentActor);

		PActor* SimulationActor = new PActor();
		POBJ_DEBUGNAME_SET(SimulationActor, "SanityAcceptanceSimulation");
		SaveWorld->AddActor(SimulationActor);
		SimulationActor->AddComponent(new PMovementComponent());
		PWaterSimulationComponent* Simulation = new PWaterSimulationComponent();
		Simulation->SetInitialDepth(4.5f);
		SimulationActor->AddComponent(Simulation);

		PActor* SurfaceActor = new PActor();
		POBJ_DEBUGNAME_SET(SurfaceActor, "SanityAcceptanceSurface");
		SaveWorld->AddActor(SurfaceActor);
		PWaterSurfaceComponent* Surface = new PWaterSurfaceComponent();
		SurfaceActor->AddComponent(Surface);
		Surface->SetSimulation(Simulation);

		const BOOL32 bSaved = SaveWorld->SaveLevel(Path);
		SaveWorld->Destroy();

		PSanityLevelWorld* LoadWorld = new PSanityLevelWorld();
		LoadWorld->Init();
		const BOOL32 bLoaded = LoadWorld->LoadLevel(Path);
		BOOL32 bContentOk = FALSE;
		if (bLoaded)
		{
			PActor* LoadedContent = nullptr;
			PActor* LoadedChild = nullptr;
			PActor* LoadedSimulationActor = nullptr;
			PActor* LoadedSurfaceActor = nullptr;
			const TArray<PActor*> Actors = LoadWorld->GetAllActors();
			for (UINT32 Index = 0u, Num = Actors.Num<UINT32>(); Index < Num; Index++)
			{
				if (!Actors[Index])
				{
					continue;
				}
				const EString& Name = Actors[Index]->GetDebugName();
				if (Name == EString(PE_TEXT("SanityAcceptanceContent")))
				{
					LoadedContent = Actors[Index];
				}
				else if (Name == EString(PE_TEXT("SanityAcceptanceChild")))
				{
					LoadedChild = Actors[Index];
				}
				else if (Name == EString(PE_TEXT("SanityAcceptanceSimulation")))
				{
					LoadedSimulationActor = Actors[Index];
				}
				else if (Name == EString(PE_TEXT("SanityAcceptanceSurface")))
				{
					LoadedSurfaceActor = Actors[Index];
				}
			}

			if (LoadedContent && LoadedChild && LoadedSimulationActor && LoadedSurfaceActor)
			{
				PCameraComponent* LoadedCamera = LoadedContent->FindComponent<PCameraComponent>();
				PDirectionalLightComponent* LoadedDirectional = LoadedContent->FindComponent<PDirectionalLightComponent>();
				PPhysicsComponent* LoadedPhysics = LoadedContent->FindComponent<PPhysicsComponent>();
				PTriggerComponent* LoadedTrigger = LoadedContent->FindComponent<PTriggerComponent>();
				PNavMeshComponent* LoadedNavMesh = LoadedContent->FindComponent<PNavMeshComponent>();
				PWaterSurfaceComponent* LoadedSurface = LoadedSurfaceActor->FindComponent<PWaterSurfaceComponent>();
				PWaterSimulationComponent* LoadedSimulation = LoadedSimulationActor->FindComponent<PWaterSimulationComponent>();
				PStaticMeshComponent* LoadedStaticMesh = LoadedContent->FindComponent<PStaticMeshComponent>();
				const FBoxShape* LoadedBox = LoadedPhysics ? dynamic_cast<const FBoxShape*>(LoadedPhysics->GetShapeForSerialization()) : nullptr;
				const FSphereShape* LoadedSphere = LoadedTrigger ? dynamic_cast<const FSphereShape*>(LoadedTrigger->GetShape()) : nullptr;
				bContentOk = (LoadedContent->FindComponent<PStaticMeshComponent>() != nullptr) &&
					(LoadedContent->FindComponent<PSkeletalMeshComponent>() != nullptr) &&
					LoadedStaticMesh &&
					(LoadedStaticMesh->GetMaterialSlotReference(0u).MaterialName == EString(PE_TEXT("M_EngineFallback"))) &&
					(LoadedStaticMesh->GetMaterialSlotReference(0u).ShaderName == EString(PE_TEXT("M_EngineFallback"))) &&
					(LoadedStaticMesh->GetMaterialSlotReference(0u).ProgramName == EString(PE_TEXT("BasePass"))) &&
					LoadedCamera && (EMath::Abs(LoadedCamera->GetCameraViewInfo().FovAngleY - 83.f) <= 1.0e-3f) &&
					LoadedDirectional && (EMath::Abs(LoadedDirectional->GetIlluminanceLux() - 12345.f) <= 1.0e-2f) &&
					LoadedBox && IsVectorNearlyEqual(LoadedBox->HalfExtent, Vector3(11.f, 22.f, 33.f), 1.0e-4f) &&
					LoadedSphere && (EMath::Abs(LoadedSphere->Raidus - 44.f) <= 1.0e-4f) &&
					LoadedNavMesh && (EMath::Abs(LoadedNavMesh->GetBuildSettings().CellSize - 31.f) <= 1.0e-4f) &&
					(LoadedNavMesh->GetBuildSettings().MaxCrowdAgents == 7u) &&
					(LoadedChild->GetAttachedParentActor() == LoadedContent) &&
					LoadedSimulation && (EMath::Abs(LoadedSimulation->GetInitialDepth() - 4.5f) <= 1.0e-4f) &&
					LoadedSurface && LoadedSurface->GetSimulationHandle().IsValid() &&
					(LoadedSurface->GetSimulation() == LoadedSimulation) ? TRUE : FALSE;
			}
		}
		LoadWorld->Destroy();
		EFileHelper::DeleteFile(Path);
		return (bSaved && bLoaded && bContentOk) ? TRUE : FALSE;
	}

	// Command stack end to end on a disposable world: spawn/undo/redo, add/delete component,
	// a property edit and the handle remap that lets an older property command address a
	// component recreated by a structural undo.
	static BOOL32 CheckWorldAttachmentReparent()
	{
#if ENGINE_DEBUG_MODE
		const UINT64 AliveBefore = EObjectBase::GetAliveObjectCount();
#endif
		PSanityLevelWorld* World = new PSanityLevelWorld();
		World->Init();
		FWorldEditorCommandStack Stack;
		Stack.GetContext().World = World;

		// Three top-level actors. The first added actor's parent is the world root, which
		// is exactly what the editor's detach drop zone targets.
		PActor* ActorA = new PActor();
		POBJ_DEBUGNAME_SET(ActorA, "SanityReparentA");
		ActorA->SetActorLocation(Vector3(-40.f, 10.f, 25.f));
		ActorA->SetActorRotation(MakeQuaternion(Euler(20.f, 35.f, -10.f)));
		World->AddActor(ActorA);
		PActor* WorldRoot = ActorA->GetAttachedParentActor();
		const BOOL32 bWorldRootValid = (WorldRoot != nullptr) ? TRUE : FALSE;

		PActor* ActorB = new PActor();
		POBJ_DEBUGNAME_SET(ActorB, "SanityReparentB");
		ActorB->SetActorLocation(Vector3(100.f, 50.f, 0.f));
		World->AddActor(ActorB);

		PActor* ActorC = new PActor();
		POBJ_DEBUGNAME_SET(ActorC, "SanityReparentC");
		ActorC->SetActorLocation(Vector3(0.f, -60.f, 40.f));
		World->AddActor(ActorC);

		const Vector3 BWorldBefore = ActorB->GetActorWorldLocation();
		const ETransform BLocalBefore = ActorB->GetActorTransform();

		// Actor reparent under a rotated host: KEEP_WORLD must hold the world transform.
		const BOOL32 bAttachPushed = PushReparentActorCommand(Stack, *World, ActorB, ActorA);
		const BOOL32 bAttachParent = (ActorB->GetAttachedParentActor() == ActorA) ? TRUE : FALSE;
		const BOOL32 bAttachWorldKept = IsVectorNearlyEqual(ActorB->GetActorWorldLocation(), BWorldBefore, 1.0e-3f);

		// Re-parenting onto the current parent is a no-op and must not enter the history.
		const UINT64 RevisionBeforeNoop = Stack.GetHistoryRevision();
		const BOOL32 bNoopRefused = (!PushReparentActorCommand(Stack, *World, ActorB, ActorA)) ? TRUE : FALSE;
		const BOOL32 bNoopRevision = (Stack.GetHistoryRevision() == RevisionBeforeNoop) ? TRUE : FALSE;

		// Attaching an ancestor below its own descendant is refused without a command.
		const BOOL32 bCycleRefused = (!PushReparentActorCommand(Stack, *World, ActorA, ActorB)) ? TRUE : FALSE;

		// Component reparent inside A: the leaf moves under the host, KEEP_WORLD holds it.
		PSceneComponent* HostComponent = new PSceneComponent();
		POBJ_DEBUGNAME_SET(HostComponent, "SanityReparentHost");
		ActorA->AddComponent(HostComponent);
		HostComponent->SetComponentLocation(Vector3(0.f, 0.f, 10.f));

		PSceneComponent* LeafComponent = new PSceneComponent();
		POBJ_DEBUGNAME_SET(LeafComponent, "SanityReparentLeaf");
		ActorA->AddComponent(LeafComponent);
		LeafComponent->SetComponentLocation(Vector3(30.f, 0.f, 0.f));

		const Vector3 LeafWorldBefore = LeafComponent->GetComponentWorldLocation();
		const ETransform LeafLocalBefore = LeafComponent->GetTransform();

		const BOOL32 bComponentPushed = PushReparentComponentCommand(Stack, *World, LeafComponent, HostComponent);
		const BOOL32 bComponentParent = (LeafComponent->GetParentComponent() == HostComponent) ? TRUE : FALSE;
		const BOOL32 bComponentWorldKept = IsVectorNearlyEqual(LeafComponent->GetComponentWorldLocation(), LeafWorldBefore, 1.0e-3f);

		// Refusals never reach the history: cross-actor links and structural roots.
		const BOOL32 bCrossActorRefused = (!PushReparentComponentCommand(Stack, *World, LeafComponent, ActorC->GetRootComponent())) ? TRUE : FALSE;
		const BOOL32 bRootRefused = (!PushReparentComponentCommand(Stack, *World, ActorA->GetRootComponent(), HostComponent)) ? TRUE : FALSE;

		// Undo the component reparent, then the actor one: links and locals return exactly.
		const BOOL32 bUndoComponent = Stack.Undo();
		const BOOL32 bUndoComponentParent = (LeafComponent->GetParentComponent() == ActorA->GetRootComponent()) ? TRUE : FALSE;
		const BOOL32 bUndoComponentLocal = AreMatricesNearlyEqual(LeafComponent->GetTransform().ToMatrix(), LeafLocalBefore.ToMatrix(), 1.0e-3f);
		const BOOL32 bUndoActor = Stack.Undo();
		const BOOL32 bUndoActorParent = (ActorB->GetAttachedParentActor() == WorldRoot) ? TRUE : FALSE;
		const BOOL32 bUndoActorLocal = AreMatricesNearlyEqual(ActorB->GetActorTransform().ToMatrix(), BLocalBefore.ToMatrix(), 1.0e-3f);

		// Redo both: the hierarchy and the world placement come back.
		const BOOL32 bRedoActor = Stack.Redo();
		const BOOL32 bRedoActorParent = (ActorB->GetAttachedParentActor() == ActorA) ? TRUE : FALSE;
		const BOOL32 bRedoActorWorld = IsVectorNearlyEqual(ActorB->GetActorWorldLocation(), BWorldBefore, 1.0e-3f);
		const BOOL32 bRedoComponent = Stack.Redo();
		const BOOL32 bRedoComponentParent = (LeafComponent->GetParentComponent() == HostComponent) ? TRUE : FALSE;

		// The reparented structure survives the level format: save all three actors,
		// reload them, and find B attached under the loaded A.
		TArray<PActor*> SavedActors;
		SavedActors.Add(ActorA);
		SavedActors.Add(ActorB);
		SavedActors.Add(ActorC);
		CJsonObject RoundTripDocument;
		const BOOL32 bRoundTripSaved = ELevelSerializer::SaveActors(SavedActors, RoundTripDocument);
		// Destroying the parent takes the attached subtree with it: the child is marked
		// for destroy, and a second destroy call on it is a no-op rather than a failure.
		World->DestroyActor(ActorA);
		const BOOL32 bCascadeMarked = ActorB->IsPendingDestroy();
		World->DestroyActor(ActorB);
		World->DestroyActor(ActorC);
		World->FlushPendingDestroy();
		TArray<PActor*> LoadedActors;
		const BOOL32 bRoundTripLoaded = (bRoundTripSaved && ELevelSerializer::LoadActors(*World, RoundTripDocument, &LoadedActors)) ? TRUE : FALSE;
		PActor* LoadedA = nullptr;
		PActor* LoadedB = nullptr;
		for (UINT32 Index = 0u, Num = LoadedActors.Num<UINT32>(); Index < Num; Index++)
		{
			if (!LoadedActors[Index])
			{
				continue;
			}
			const EString& LoadedName = POBJ_DEBUGNAME_GET(LoadedActors[Index]);
			if (LoadedName == EString(PE_TEXT("SanityReparentA")))
			{
				LoadedA = LoadedActors[Index];
			}
			else if (LoadedName == EString(PE_TEXT("SanityReparentB")))
			{
				LoadedB = LoadedActors[Index];
			}
		}
		const BOOL32 bRoundTripHierarchy = (LoadedA && LoadedB && (LoadedB->GetAttachedParentActor() == LoadedA)) ? TRUE : FALSE;

		// The loaded subtree follows the same cascade: destroying LoadedA covers LoadedB,
		// and the pending-destroy early return keeps repeated calls below a no-op.
		for (UINT32 Index = 0u, Num = LoadedActors.Num<UINT32>(); Index < Num; Index++)
		{
			if (LoadedActors[Index])
			{
				World->DestroyActor(LoadedActors[Index]);
			}
		}
		World->FlushPendingDestroy();
		Stack.Clear();
		World->Destroy();
#if ENGINE_DEBUG_MODE
		if (EObjectBase::GetAliveObjectCount() != AliveBefore)
		{
			return FALSE;
		}
#endif
		const BOOL32 bOk = (bWorldRootValid && bAttachPushed && bAttachParent && bAttachWorldKept &&
			bNoopRefused && bNoopRevision && bCycleRefused &&
			bComponentPushed && bComponentParent && bComponentWorldKept &&
			bCrossActorRefused && bRootRefused &&
			bUndoComponent && bUndoComponentParent && bUndoComponentLocal &&
			bUndoActor && bUndoActorParent && bUndoActorLocal &&
			bRedoActor && bRedoActorParent && bRedoActorWorld &&
			bRedoComponent && bRedoComponentParent &&
			bCascadeMarked &&
			bRoundTripSaved && bRoundTripLoaded && bRoundTripHierarchy) ? TRUE : FALSE;
		if (!bOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("attachment reparent detail: root=")) + EString::FromInt(bWorldRootValid) +
				EString(PE_TEXT(" attach=")) + EString::FromInt(bAttachPushed) + EString::FromInt(bAttachParent) + EString::FromInt(bAttachWorldKept) +
				EString(PE_TEXT(" noop=")) + EString::FromInt(bNoopRefused) + EString::FromInt(bNoopRevision) +
				EString(PE_TEXT(" cycle=")) + EString::FromInt(bCycleRefused) +
				EString(PE_TEXT(" component=")) + EString::FromInt(bComponentPushed) + EString::FromInt(bComponentParent) + EString::FromInt(bComponentWorldKept) +
				EString(PE_TEXT(" refused=")) + EString::FromInt(bCrossActorRefused) + EString::FromInt(bRootRefused) +
				EString(PE_TEXT(" undoComp=")) + EString::FromInt(bUndoComponent) + EString::FromInt(bUndoComponentParent) + EString::FromInt(bUndoComponentLocal) +
				EString(PE_TEXT(" undoActor=")) + EString::FromInt(bUndoActor) + EString::FromInt(bUndoActorParent) + EString::FromInt(bUndoActorLocal) +
				EString(PE_TEXT(" redoActor=")) + EString::FromInt(bRedoActor) + EString::FromInt(bRedoActorParent) + EString::FromInt(bRedoActorWorld) +
				EString(PE_TEXT(" redoComp=")) + EString::FromInt(bRedoComponent) + EString::FromInt(bRedoComponentParent) +
				EString(PE_TEXT(" cascade=")) + EString::FromInt(bCascadeMarked) +
				EString(PE_TEXT(" roundTrip=")) + EString::FromInt(bRoundTripSaved) + EString::FromInt(bRoundTripLoaded) + EString::FromInt(bRoundTripHierarchy));
		}
		return bOk;
	}

	static BOOL32 CheckEditorCustomizationRegistry()
	{
		// The builtin registrations come from EEditorManager::EditorInit, which has already
		// run by the time the sanity checks execute. A class counts every registration on
		// its inheritance chain, mirroring the old virtual chain where each override called
		// its base before drawing its own extras.
		const BOOL32 bActorEditorCounted = (EEditorCustomization::CountActorEditorsForClass(GetClassHashCode<PActor>()) >= 1u) ? TRUE : FALSE;

		// PNavMeshComponent's chain composes PSceneComponent + PNavigationComponent +
		// PNavMeshComponent; the exact three-block layout the old override chain produced.
		const BOOL32 bNavMeshChainCounted = (EEditorCustomization::CountComponentEditorsForClass(GetClassHashCode<PNavMeshComponent>()) == 3u) ? TRUE : FALSE;

		// A class with no builtin registration anywhere on its chain counts zero - its
		// details are the bare reflection grid - and picks up editors when one registers.
		const UINT32 NavAgentEditorsBefore = EEditorCustomization::CountComponentEditorsForClass(GetClassHashCode<PNavAgentComponent>());
		const BOOL32 bNavAgentClean = (NavAgentEditorsBefore == 0u) ? TRUE : FALSE;
		EEditorCustomization::RegisterComponentCustomEditor<PNavAgentComponent>(
			[](PActorComponent* /*InComponent*/, PWorld* /*InWorld*/) -> void {});
		const BOOL32 bNavAgentRegistered = (EEditorCustomization::CountComponentEditorsForClass(GetClassHashCode<PNavAgentComponent>()) == (NavAgentEditorsBefore + 1u)) ? TRUE : FALSE;

		const BOOL32 bOk = (bActorEditorCounted && bNavMeshChainCounted && bNavAgentClean && bNavAgentRegistered) ? TRUE : FALSE;
		if (!bOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("editor customization detail: actor=")) + EString::FromInt(bActorEditorCounted) +
				EString(PE_TEXT(" navMeshChain=")) + EString::FromInt(bNavMeshChainCounted) +
				EString(PE_TEXT(" navAgentClean=")) + EString::FromInt(bNavAgentClean) +
				EString(PE_TEXT(" navAgentRegistered=")) + EString::FromInt(bNavAgentRegistered));
		}
		return bOk;
	}

	static BOOL32 CheckEditorWorldContextSelection()
	{
		PSanityLevelWorld* World = new PSanityLevelWorld();
		World->Init();

		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityContextActor");
		World->AddActor(Actor);
		PSceneComponent* Child = new PSceneComponent();
		POBJ_DEBUGNAME_SET(Child, "SanityContextChild");
		Actor->AddComponent(Child);

		// Selection round trip; selecting the actor auto-selects the root's single child.
		EEditorWorldContext::SetSelectedActor(World, Actor);
		const BOOL32 bActorSelected = (EEditorWorldContext::GetSelectedActor(World) == Actor) ? TRUE : FALSE;
		const BOOL32 bChildAutoSelected = (EEditorWorldContext::GetSelectedComponent(World) == Child) ? TRUE : FALSE;

		// Handle authority: destroying the selected object makes the reads resolve to null
		// through the registry, with no runtime lifecycle hook involved.
		World->DestroyActor(Actor);
		World->FlushPendingDestroy();
		const BOOL32 bClearedByResolution = ((EEditorWorldContext::GetSelectedActor(World) == nullptr) && (EEditorWorldContext::GetSelectedComponent(World) == nullptr)) ? TRUE : FALSE;

		// The command stack the panels record into round trips per world.
		FWorldEditorCommandStack Stack;
		Stack.GetContext().World = World;
		EEditorWorldContext::SetEditorCommandStack(World, &Stack);
		const BOOL32 bStackRoundTrip = (EEditorWorldContext::GetEditorCommandStack(World) == &Stack) ? TRUE : FALSE;
		EEditorWorldContext::SetEditorCommandStack(World, nullptr);

		Stack.Clear();
		World->Destroy();

		const BOOL32 bOk = (bActorSelected && bChildAutoSelected && bClearedByResolution && bStackRoundTrip) ? TRUE : FALSE;
		if (!bOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("editor world context detail: actor=")) + EString::FromInt(bActorSelected) +
				EString(PE_TEXT(" childAuto=")) + EString::FromInt(bChildAutoSelected) +
				EString(PE_TEXT(" clearedByResolution=")) + EString::FromInt(bClearedByResolution) +
				EString(PE_TEXT(" stack=")) + EString::FromInt(bStackRoundTrip));
		}
		return bOk;
	}

	static BOOL32 CheckLevelFileStateTransitions()
	{
		// These run against the live world manager, so the scenario only exercises the
		// no-op contract: a rename/delete notification for some other file must not touch
		// the current level state, and cancelling a pending discard action is pure state.
		EWorldManager* Manager = EWorldManager::GetManagerSingleton();
		const EString PathBefore = Manager->GetCurrentLevelPath();

		Manager->NotifyLevelFileRenamed(EString(PE_TEXT("__sanity_missing__.PLevel")), EString(PE_TEXT("__sanity_other__.PLevel")));
		const BOOL32 bRenameNoop = (Manager->GetCurrentLevelPath() == PathBefore) ? TRUE : FALSE;

		Manager->NotifyLevelFileDeleted(EString(PE_TEXT("__sanity_missing__.PLevel")));
		const BOOL32 bDeleteNoop = (Manager->GetCurrentLevelPath() == PathBefore) ? TRUE : FALSE;

		Manager->CancelPendingDiscardAction();
		const BOOL32 bCancelClean = (Manager->GetCurrentLevelPath() == PathBefore) ? TRUE : FALSE;

		const BOOL32 bOk = (bRenameNoop && bDeleteNoop && bCancelClean) ? TRUE : FALSE;
		if (!bOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("level file state detail: renameNoop=")) + EString::FromInt(bRenameNoop) +
				EString(PE_TEXT(" deleteNoop=")) + EString::FromInt(bDeleteNoop) +
				EString(PE_TEXT(" cancelClean=")) + EString::FromInt(bCancelClean));
		}
		return bOk;
	}

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

	// Plain C++ structs used by the struct-property scenarios. The nested struct, the array
	// of structs and the map of structs each exercise a different value-tree shape without
	// registering a new OCA class.
	struct FSanityStructNested
	{
		INT32 Count = 0;
		EString Label;
	};

	struct FSanityStructData
	{
		INT32 Integer = 0;
		BOOL32 bFlag = FALSE;
		EString AssetName;
		EWaterGroundType GroundType = EWaterGroundType::WATER_GROUND_FLAT;
		FSanityStructNested Nested;
		TArray<FSanityStructNested> Items;
		TMap<EString, FSanityStructNested> NamedItems;
	};

	static void SerializeSanityStructNested(EArchive& InArchive, const CHAR* InName, FSanityStructNested& InOutValue)
	{
		InArchive.BeginObject(InName);
		InArchive.SerializeInt32(PE_TEXT("Count"), InOutValue.Count);
		InArchive.SerializeString(PE_TEXT("Label"), InOutValue.Label);
		InArchive.EndObject();
	}

	static void SerializeSanityStructData(EArchive& InArchive, const CHAR* InName, FSanityStructData& InOutValue)
	{
		InArchive.BeginObject(InName);
		InArchive.SerializeInt32(PE_TEXT("Integer"), InOutValue.Integer);
		InArchive.SerializeBool(PE_TEXT("Flag"), InOutValue.bFlag);
		InArchive.SerializeString(PE_TEXT("AssetName"), InOutValue.AssetName);
		INT32 GroundType = static_cast<INT32>(InOutValue.GroundType);
		SerializeEnumValue(InArchive, PE_TEXT("GroundType"), GroundType, GetClassHashCode<EWaterGroundType>());
		InOutValue.GroundType = static_cast<EWaterGroundType>(GroundType);
		SerializeSanityStructNested(InArchive, PE_TEXT("Nested"), InOutValue.Nested);

		UINT32 ItemCount = InOutValue.Items.Num<UINT32>();
		InArchive.BeginArray(PE_TEXT("Items"), ItemCount);
		if (!InArchive.IsSaving())
		{
			InOutValue.Items.Empty();
		}
		for (UINT32 Index = 0u; Index < ItemCount; Index++)
		{
			const EString IndexKey = EString::FromInt(Index);
			FSanityStructNested Item = InArchive.IsSaving() ? InOutValue.Items[Index] : FSanityStructNested();
			SerializeSanityStructNested(InArchive, *IndexKey, Item);
			if (!InArchive.IsSaving())
			{
				InOutValue.Items.Add(Item);
			}
		}
		InArchive.EndArray();

		UINT32 NamedItemCount = InOutValue.NamedItems.Num<UINT32>();
		InArchive.BeginArray(PE_TEXT("NamedItems"), NamedItemCount);
		if (!InArchive.IsSaving())
		{
			InOutValue.NamedItems.Empty();
		}
		for (UINT32 Index = 0u; Index < NamedItemCount; Index++)
		{
			const EString IndexKey = EString::FromInt(Index);
			InArchive.BeginObject(*IndexKey);
			EString Key;
			FSanityStructNested Value;
			if (InArchive.IsSaving())
			{
				typename TMap<EString, FSanityStructNested>::TConstIterator It = InOutValue.NamedItems.Begin();
				for (UINT32 Advance = 0u; Advance < Index; Advance++)
				{
					++It;
				}
				Key = It->first;
				Value = It->second;
			}
			InArchive.SerializeString(PE_TEXT("Key"), Key);
			SerializeSanityStructNested(InArchive, PE_TEXT("Value"), Value);
			if (!InArchive.IsSaving())
			{
				InOutValue.NamedItems.Add(Key, Value);
			}
			InArchive.EndObject();
		}
		InArchive.EndArray();
		InArchive.EndObject();
	}

	// Registers the test structs once. The class property keeps a hand-written serializer,
	// which is the same shape every migrated component property uses.
	static void RegisterSanityStructProperties()
	{
		static BOOL32 bRegistered = FALSE;
		if (bRegistered)
		{
			return;
		}
		bRegistered = TRUE;
		const UINT32 EditFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE);
		const size_t NestedHash = GetClassHashCode<FSanityStructNested>();
		const EString NestedName(PE_TEXT("FSanityStructNested"));
		AddStructProperty(NestedHash, NestedName, PE_TEXT("Count"), &FSanityStructNested::Count, EditFlags);
		AddStructProperty(NestedHash, NestedName, PE_TEXT("Label"), &FSanityStructNested::Label, EditFlags);

		const size_t DataHash = GetClassHashCode<FSanityStructData>();
		const EString DataName(PE_TEXT("FSanityStructData"));
		AddStructProperty(DataHash, DataName, PE_TEXT("Integer"), &FSanityStructData::Integer, EditFlags);
		AddStructBoolProperty(DataHash, DataName, PE_TEXT("Flag"), &FSanityStructData::bFlag, EditFlags);
		AddStructAssetProperty(DataHash, DataName, PE_TEXT("AssetName"), &FSanityStructData::AssetName,
			EPropertyAssetType::MATERIAL, EditFlags);
		AddStructEnumProperty(DataHash, DataName, PE_TEXT("GroundType"), &FSanityStructData::GroundType, EditFlags);
		AddStructProperty(DataHash, DataName, PE_TEXT("Nested"), &FSanityStructData::Nested, EditFlags);
		AddStructProperty(DataHash, DataName, PE_TEXT("Items"), &FSanityStructData::Items, EditFlags);
		AddStructProperty(DataHash, DataName, PE_TEXT("NamedItems"), &FSanityStructData::NamedItems, EditFlags);

		const size_t ClassHash = GetClassHashCode<FSanityStructData>();
		AddClassPropertyStructCustom<FSanityStructData, FSanityStructData>(ClassHash, PE_TEXT("Data"),
			[](const FSanityStructData* Data) -> FSanityStructData { return *Data; },
			[](FSanityStructData* Data, const FSanityStructData& Value) -> void { *Data = Value; },
			SerializeSanityStructData, EditFlags);
	}

	// A plain C++ class used only by the container-property scenario: it exercises the
	// TArray/TMap serialization and the editor value metadata without polluting the OCA class
	// registry.
	struct FSanityContainerData
	{
		TArray<INT32> Numbers;
		TMap<EString, FLOAT> Weights;
		TArray<EWaterGroundType> GroundTypes;
		TMap<EWaterGroundType, INT32> EnumKeys;
		TArray<TArray<INT32>> NestedNumbers;
		TMap<EString, TArray<FLOAT>> NestedWeights;
	};

	// Re-baking an asset under the same name must read the new file, not the object the manager
	// cached before the bake. The test uses a scratch asset because the real demo asset is
	// generated locally and must not be touched.
	static BOOL32 CheckNavMeshAssetCacheReload()
	{
		const EString AssetName(PE_TEXT("Sanity_NavMeshReload"));
		const EString AssetPath(EBaseSettings::ENGINE_NAV_MESH_PATH);
		const EString FullPathName = MakeAssetFullPathName(AssetPath, AssetName);
		EFileHelper::DeleteFile(FullPathName);

		TArray<UINT8> FirstBlob;
		FirstBlob.Add(1u);
		FirstBlob.Add(2u);
		FNavMeshAssetSettings FirstSettings;
		FirstSettings.ProviderName = EString(PE_TEXT("RecastNavMesh"));
		FirstSettings.PolygonCount = 1u;
		FirstSettings.TileCount = 1u;
		const ENavMeshAsset* FirstAsset = nullptr;
		EString Error;
		const BOOL32 bFirstSaved = ENavMeshAssetManager::GetManagerSingleton()->CreateNavMeshAsset(
			AssetName, FirstSettings, FirstBlob, AssetPath, FirstAsset, Error);

		EFileHelper::DeleteFile(FullPathName);
		TArray<UINT8> SecondBlob;
		SecondBlob.Add(3u);
		SecondBlob.Add(4u);
		SecondBlob.Add(5u);
		FNavMeshAssetSettings SecondSettings = FirstSettings;
		SecondSettings.PolygonCount = 2u;
		SecondSettings.TileCount = 3u;
		const ENavMeshAsset* SecondAsset = nullptr;
		const BOOL32 bSecondSaved = ENavMeshAssetManager::GetManagerSingleton()->CreateNavMeshAsset(
			AssetName, SecondSettings, SecondBlob, AssetPath, SecondAsset, Error);

		const ENavMeshData* SecondData = SecondAsset ? SecondAsset->GetStoragedResource() : nullptr;
		const BOOL32 bReloaded = (bFirstSaved && bSecondSaved && FirstAsset && SecondAsset &&
			(FirstAsset != SecondAsset) &&
			SecondData &&
			(SecondData->GetSettings().PolygonCount == 2u) &&
			(SecondData->GetSettings().TileCount == 3u) &&
			(SecondData->GetBlob().Num<UINT32>() == 3u) &&
			(SecondData->GetBlob()[0] == 3u) &&
			(SecondData->GetBlob()[2] == 5u)) ? TRUE : FALSE;

		EFileHelper::DeleteFile(FullPathName);
		if (!bReloaded)
		{
			PE_LOG_ERROR(EString(PE_TEXT("navmesh reload detail: firstSaved=")) + EString::FromInt(bFirstSaved) +
				EString(PE_TEXT(" secondSaved=")) + EString::FromInt(bSecondSaved) +
				EString(PE_TEXT(" first=")) + EString::FromInt(FirstAsset ? 1 : 0) +
				EString(PE_TEXT(" second=")) + EString::FromInt(SecondAsset ? 1 : 0) +
				EString(PE_TEXT(" error=")) + Error);
		}
		return bReloaded;
	}

	static BOOL32 CheckNavMeshSaveAutoAssign()
	{
		const EString AssetPath(EBaseSettings::ENGINE_NAV_MESH_PATH);

		// A component with no published bake takes no convention name and writes no file:
		// a fresh checkout loads the level and bakes, or the author names the asset.
		PSanityLevelWorld* World = new PSanityLevelWorld();
		World->Init();
		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityNavAutoActor");
		World->AddActor(Actor);
		PNavMeshComponent* NavComponent = new PNavMeshComponent();
		POBJ_DEBUGNAME_SET(NavComponent, "SanityNavAutoComponent");
		Actor->AddComponent(NavComponent);
		EString SkipError;
		const BOOL32 bSkippedWithoutBuild = NavComponent->AutoAssignNamedNavMeshAsset(EString(PE_TEXT("Sanity_Auto")), SkipError);
		const BOOL32 bNoFileWritten = !EFileHelper::IsFileExists(MakeAssetFullPathName(AssetPath, EString(PE_TEXT("Sanity_Auto_NavMesh"))));
		const BOOL32 bNameStillEmpty = (NavComponent->GetNavMeshAssetName().Length() == 0u);
		World->DestroyActor(Actor);
		World->FlushPendingDestroy();
		World->Destroy();

		// The overwrite flag on CreateNavMeshAsset: refused without it, allowed with it, and
		// the reload chain behind the overwrite still publishes the newest bytes.
		const EString AssetName(PE_TEXT("Sanity_NavMeshAutoAssign"));
		const EString FullPathName = MakeAssetFullPathName(AssetPath, AssetName);
		EFileHelper::DeleteFile(FullPathName);

		TArray<UINT8> FirstBlob;
		FirstBlob.Add(1u);
		FirstBlob.Add(2u);
		FNavMeshAssetSettings Settings;
		Settings.ProviderName = EString(PE_TEXT("RecastNavMesh"));
		const ENavMeshAsset* FirstAsset = nullptr;
		EString Error;
		const BOOL32 bFirstSaved = ENavMeshAssetManager::GetManagerSingleton()->CreateNavMeshAsset(
			AssetName, Settings, FirstBlob, AssetPath, FirstAsset, Error);

		TArray<UINT8> SecondBlob;
		SecondBlob.Add(9u);
		const ENavMeshAsset* RefusedAsset = nullptr;
		EString RefuseError;
		const BOOL32 bOverwriteRefused = (ENavMeshAssetManager::GetManagerSingleton()->CreateNavMeshAsset(
			AssetName, Settings, SecondBlob, AssetPath, RefusedAsset, RefuseError) == FALSE) ? TRUE : FALSE;

		const ENavMeshAsset* OverwrittenAsset = nullptr;
		EString OverwriteError;
		const BOOL32 bOverwritten = ENavMeshAssetManager::GetManagerSingleton()->CreateNavMeshAsset(
			AssetName, Settings, SecondBlob, AssetPath, OverwrittenAsset, OverwriteError, TRUE);
		const ENavMeshData* OverwrittenData = OverwrittenAsset ? OverwrittenAsset->GetStoragedResource() : nullptr;
		const BOOL32 bOverwriteContent = (OverwrittenData && (OverwrittenData->GetBlob().Num<UINT32>() == 1u) &&
			(OverwrittenData->GetBlob()[0] == 9u)) ? TRUE : FALSE;

		EFileHelper::DeleteFile(FullPathName);

		const BOOL32 bOk = (bSkippedWithoutBuild && bNoFileWritten && bNameStillEmpty &&
			bFirstSaved && bOverwriteRefused && bOverwritten && bOverwriteContent) ? TRUE : FALSE;
		if (!bOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("navmesh auto-assign detail: skipped=")) + EString::FromInt(bSkippedWithoutBuild) +
				EString(PE_TEXT(" noFile=")) + EString::FromInt(bNoFileWritten) +
				EString(PE_TEXT(" nameEmpty=")) + EString::FromInt(bNameStillEmpty) +
				EString(PE_TEXT(" firstSaved=")) + EString::FromInt(bFirstSaved) +
				EString(PE_TEXT(" refused=")) + EString::FromInt(bOverwriteRefused) +
				EString(PE_TEXT(" overwritten=")) + EString::FromInt(bOverwritten) +
				EString(PE_TEXT(" content=")) + EString::FromInt(bOverwriteContent) +
				EString(PE_TEXT(" firstError=[")) + Error + EString(PE_TEXT("] refuseError=[")) + RefuseError +
				EString(PE_TEXT("] overwriteError=[")) + OverwriteError + EString(PE_TEXT("]")));
		}
		return bOk;
	}

	// Two triangles forming one flat world space rectangle on the y = 0 plane, wound so the
	// normal points up - which is what Recast tests walkability by.
	static void AddSanityNavigationQuad(ENavigationSourceGeometry& InOutGeometry
		, const FLOAT InMinX, const FLOAT InMaxX, const FLOAT InMinZ, const FLOAT InMaxZ
		, const ENavigationAreaFlagType InArea)
	{
		ENavigationSourceTriangle First;
		First.Vertex0 = Vector3(InMinX, 0.0f, InMinZ);
		First.Vertex1 = Vector3(InMinX, 0.0f, InMaxZ);
		First.Vertex2 = Vector3(InMaxX, 0.0f, InMinZ);
		First.Area = InArea;
		InOutGeometry.Triangles.Add(First);

		ENavigationSourceTriangle Second;
		Second.Vertex0 = Vector3(InMaxX, 0.0f, InMinZ);
		Second.Vertex1 = Vector3(InMinX, 0.0f, InMaxZ);
		Second.Vertex2 = Vector3(InMaxX, 0.0f, InMaxZ);
		Second.Area = InArea;
		InOutGeometry.Triangles.Add(Second);
	}

	// Two ground platforms joined by two parallel corridors: the short one along z = 0 is water,
	// the long one behind it is ground. A filter that may cross water walks straight; one that
	// may not, or one that prices water high, has to take the ground corridor. The vertical
	// extent of the pinned bounds is what gives the flat floor the clearance Recast's walkable
	// height filter needs.
	static ENavigationSourceGeometry MakeSanityNavigationAreaGeometry()
	{
		ENavigationSourceGeometry Geometry;
		Geometry.Bounds = EBoundAABB(Vector3(-500.0f, -50.0f, -200.0f), Vector3(500.0f, 400.0f, 360.0f), TRUE);
		AddSanityNavigationQuad(Geometry, -500.0f, -300.0f, -200.0f, 360.0f, ENavigationAreaFlagType::NAVIGATION_AREA_GROUND);
		AddSanityNavigationQuad(Geometry, 300.0f, 500.0f, -200.0f, 360.0f, ENavigationAreaFlagType::NAVIGATION_AREA_GROUND);
		AddSanityNavigationQuad(Geometry, -300.0f, 300.0f, -200.0f, 200.0f, ENavigationAreaFlagType::NAVIGATION_AREA_WATER);
		AddSanityNavigationQuad(Geometry, -300.0f, 300.0f, 200.0f, 360.0f, ENavigationAreaFlagType::NAVIGATION_AREA_GROUND);
		return Geometry;
	}

	static FLOAT GetSanityPathLength(const ENavigationPath& InPath)
	{
		FLOAT Length = 0.0f;
		for (UINT32 Index = 1u; Index < InPath.Num(); Index++)
		{
			Length += Vector3::Distance(InPath.Points[Index - 1u], InPath.Points[Index]);
		}
		return Length;
	}

	// Runs the same four queries against one bake: crossing water, ground only, ground with
	// expensive water, and a destination that only exists on the water. The length thresholds
	// separate the straight route (~800 cm) from the ground detour (~980 cm).
	static BOOL32 CheckNavigationAreaProvider(ERecastNavigationProvider& InProvider)
	{
		const Vector3 Start(-400.0f, 0.0f, 0.0f);
		const Vector3 End(400.0f, 0.0f, 0.0f);
		const FLOAT StraightLimit = 900.0f;
		const FLOAT DetourLimit = 900.0f;

		ENavigationQueryFilter CrossWaterFilter;
		ENavigationPath CrossWaterPath;
		FLOAT CrossWaterLength = 0.0f;
		const BOOL32 bCrossWater = (InProvider.FindPath(Start, End, CrossWaterFilter, CrossWaterPath)
			&& (CrossWaterPath.Status == ENavigationPathStatusType::NAVIGATION_PATH_COMPLETE)
			&& ((CrossWaterLength = GetSanityPathLength(CrossWaterPath)) < StraightLimit)) ? TRUE : FALSE;

		ENavigationQueryFilter GroundOnlyFilter;
		GroundOnlyFilter.IncludeAreaFlags = static_cast<UINT16>(ENavigationAreaFlagType::NAVIGATION_AREA_GROUND);
		ENavigationPath GroundOnlyPath;
		FLOAT GroundOnlyLength = 0.0f;
		const BOOL32 bGroundOnly = (InProvider.FindPath(Start, End, GroundOnlyFilter, GroundOnlyPath)
			&& (GroundOnlyPath.Status == ENavigationPathStatusType::NAVIGATION_PATH_COMPLETE)
			&& ((GroundOnlyLength = GetSanityPathLength(GroundOnlyPath)) > DetourLimit)) ? TRUE : FALSE;

		// The path may cross the water here, but at this price the detour is cheaper.
		ENavigationQueryFilter CostsFilter;
		CostsFilter.AreaCosts.SetNum(4u);
		CostsFilter.AreaCosts[0u] = 1.0f;
		CostsFilter.AreaCosts[1u] = 20.0f;
		CostsFilter.AreaCosts[2u] = 1.0f;
		CostsFilter.AreaCosts[3u] = 1.0f;
		ENavigationPath CostsPath;
		FLOAT CostsLength = 0.0f;
		const BOOL32 bCosts = (InProvider.FindPath(Start, End, CostsFilter, CostsPath)
			&& (CostsPath.Status == ENavigationPathStatusType::NAVIGATION_PATH_COMPLETE)
			&& ((CostsLength = GetSanityPathLength(CostsPath)) > DetourLimit)) ? TRUE : FALSE;

		// The middle of the water corridor has no ground within the projection box, so a ground
		// only agent cannot be given a destination there.
		ENavigationPath BlockedPath;
		const BOOL32 bBlocked = (InProvider.FindPath(Start, Vector3(0.0f, 0.0f, 0.0f), GroundOnlyFilter, BlockedPath) == FALSE) ? TRUE : FALSE;

		const BOOL32 bOk = (bCrossWater && bGroundOnly && bCosts && bBlocked) ? TRUE : FALSE;
		if (!bOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("navmesh area detail: crossWater=")) + EString::FromInt(bCrossWater)
				+ EString(PE_TEXT(" (len ")) + EString::FromFloat(CrossWaterLength) + EString(PE_TEXT(")"))
				+ EString(PE_TEXT(" groundOnly=")) + EString::FromInt(bGroundOnly)
				+ EString(PE_TEXT(" (len ")) + EString::FromFloat(GroundOnlyLength) + EString(PE_TEXT(")"))
				+ EString(PE_TEXT(" costs=")) + EString::FromInt(bCosts)
				+ EString(PE_TEXT(" (len ")) + EString::FromFloat(CostsLength) + EString(PE_TEXT(")"))
				+ EString(PE_TEXT(" blocked=")) + EString::FromInt(bBlocked));
		}
		return bOk;
	}

	// Bakes the same two-corridor geometry twice: once as a single tile, which stamps the
	// polygon areas in the direct build path, and once tiled, where the tile cache's mesh
	// process stamps the rebuilt polygons. Both bakes have to answer the area queries alike.
	static BOOL32 CheckNavMeshAreaBake()
	{
		const ENavigationSourceGeometry Geometry = MakeSanityNavigationAreaGeometry();

		ENavMeshBuildSettings SingleSettings;
		SingleSettings.TileSizeCells = 0u;
		ERecastNavigationProvider SingleProvider;
		SingleProvider.SetSettings(SingleSettings);
		ENavigationBuildStats SingleStats;
		const BOOL32 bSingleBuilt = SingleProvider.Build(Geometry, SingleStats);
		const BOOL32 bSingle = (bSingleBuilt && (SingleStats.PolygonCount > 0u) && CheckNavigationAreaProvider(SingleProvider)) ? TRUE : FALSE;

		ENavMeshBuildSettings TiledSettings;
		TiledSettings.TileSizeCells = 32u;
		ERecastNavigationProvider TiledProvider;
		TiledProvider.SetSettings(TiledSettings);
		ENavigationBuildStats TiledStats;
		const BOOL32 bTiledBuilt = TiledProvider.Build(Geometry, TiledStats);
		const BOOL32 bTiled = (bTiledBuilt && (TiledStats.PolygonCount > 0u) && CheckNavigationAreaProvider(TiledProvider)) ? TRUE : FALSE;

		const BOOL32 bOk = (bSingle && bTiled) ? TRUE : FALSE;
		if (!bOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("navmesh area bake detail: singleBuilt=")) + EString::FromInt(bSingleBuilt)
				+ EString(PE_TEXT(" singlePolys=")) + EString::FromInt(SingleStats.PolygonCount)
				+ EString(PE_TEXT(" singleQueries=")) + EString::FromInt(bSingle)
				+ EString(PE_TEXT(" tiledBuilt=")) + EString::FromInt(bTiledBuilt)
				+ EString(PE_TEXT(" tiledPolys=")) + EString::FromInt(TiledStats.PolygonCount)
				+ EString(PE_TEXT(" tiledQueries=")) + EString::FromInt(bTiled));
		}
		return bOk;
	}

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
			AddClassPropertyAccessors<FSanityContainerData, TArray<EWaterGroundType>>(ClassHash, PE_TEXT("GroundTypes"),
				[](const FSanityContainerData* Data) -> TArray<EWaterGroundType> { return Data->GroundTypes; },
				[](FSanityContainerData* Data, const TArray<EWaterGroundType>& Value) -> void { Data->GroundTypes = Value; },
				static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
			AddClassPropertyAccessors<FSanityContainerData, TMap<EWaterGroundType, INT32>>(ClassHash, PE_TEXT("EnumKeys"),
				[](const FSanityContainerData* Data) -> TMap<EWaterGroundType, INT32> { return Data->EnumKeys; },
				[](FSanityContainerData* Data, const TMap<EWaterGroundType, INT32>& Value) -> void { Data->EnumKeys = Value; },
				static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
			AddClassPropertyAccessors<FSanityContainerData, TArray<TArray<INT32>>>(ClassHash, PE_TEXT("NestedNumbers"),
				[](const FSanityContainerData* Data) -> TArray<TArray<INT32>> { return Data->NestedNumbers; },
				[](FSanityContainerData* Data, const TArray<TArray<INT32>>& Value) -> void { Data->NestedNumbers = Value; },
				static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
			AddClassPropertyAccessors<FSanityContainerData, TMap<EString, TArray<FLOAT>>>(ClassHash, PE_TEXT("NestedWeights"),
				[](const FSanityContainerData* Data) -> TMap<EString, TArray<FLOAT>> { return Data->NestedWeights; },
				[](FSanityContainerData* Data, const TMap<EString, TArray<FLOAT>>& Value) -> void { Data->NestedWeights = Value; },
				static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
			bPropertiesRegistered = TRUE;
		}

		FSanityContainerData Source;
		Source.Numbers.Add(1);
		Source.Numbers.Add(2);
		Source.Numbers.Add(3);
		Source.Weights.Add(EString(PE_TEXT("A")), 1.5f);
		Source.Weights.Add(EString(PE_TEXT("B")), 2.5f);
		Source.GroundTypes.Add(EWaterGroundType::WATER_GROUND_FLAT);
		Source.GroundTypes.Add(EWaterGroundType::WATER_GROUND_HEIGHT_MAP);
		Source.EnumKeys.Add(EWaterGroundType::WATER_GROUND_FLAT, 10);
		Source.EnumKeys.Add(EWaterGroundType::WATER_GROUND_HEIGHT_MAP, 20);
		{
			TArray<INT32> FirstRow;
			FirstRow.Add(1);
			FirstRow.Add(2);
			Source.NestedNumbers.Add(FirstRow);
			TArray<INT32> SecondRow;
			SecondRow.Add(3);
			Source.NestedNumbers.Add(SecondRow);
			TArray<FLOAT> FirstWeights;
			FirstWeights.Add(0.5f);
			FirstWeights.Add(0.75f);
			Source.NestedWeights.Add(EString(PE_TEXT("A")), FirstWeights);
		}

		// Editor metadata: the array element type and the map key/value types are available
		// without knowing the concrete C++ class.
		const FProperty* NumberProperty = nullptr;
		const FProperty* WeightProperty = nullptr;
		const FProperty* GroundTypesProperty = nullptr;
		const FProperty* EnumKeysProperty = nullptr;
		const FProperty* NestedNumbersProperty = nullptr;
		const FProperty* NestedWeightsProperty = nullptr;
		EClassPropertyRegistry::ForEachProperty(ClassHash, [&NumberProperty, &WeightProperty, &GroundTypesProperty, &EnumKeysProperty, &NestedNumbersProperty, &NestedWeightsProperty](const FProperty& Property) -> void
		{
			if (Property.Name == EString(PE_TEXT("Numbers")))
			{
				NumberProperty = &Property;
			}
			else if (Property.Name == EString(PE_TEXT("Weights")))
			{
				WeightProperty = &Property;
			}
			else if (Property.Name == EString(PE_TEXT("GroundTypes")))
			{
				GroundTypesProperty = &Property;
			}
			else if (Property.Name == EString(PE_TEXT("EnumKeys")))
			{
				EnumKeysProperty = &Property;
			}
			else if (Property.Name == EString(PE_TEXT("NestedNumbers")))
			{
				NestedNumbersProperty = &Property;
			}
			else if (Property.Name == EString(PE_TEXT("NestedWeights")))
			{
				NestedWeightsProperty = &Property;
			}
		});

		BOOL32 bMetadataOk = FALSE;
		BOOL32 bShapeOk = FALSE;
		if (NumberProperty && WeightProperty && GroundTypesProperty && EnumKeysProperty &&
			NestedNumbersProperty && NestedWeightsProperty &&
			NumberProperty->GetValue && WeightProperty->GetValue &&
			GroundTypesProperty->GetValue && EnumKeysProperty->GetValue &&
			NestedNumbersProperty->GetValue && NestedWeightsProperty->GetValue)
		{
			FPropertyValue NumbersValue;
			NumberProperty->GetValue(&Source, NumbersValue);
			FPropertyValue WeightsValue;
			WeightProperty->GetValue(&Source, WeightsValue);
			FPropertyValue GroundTypesValue;
			GroundTypesProperty->GetValue(&Source, GroundTypesValue);
			FPropertyValue EnumKeysValue;
			EnumKeysProperty->GetValue(&Source, EnumKeysValue);
			FPropertyValue NestedNumbersValue;
			NestedNumbersProperty->GetValue(&Source, NestedNumbersValue);
			FPropertyValue NestedWeightsValue;
			NestedWeightsProperty->GetValue(&Source, NestedWeightsValue);
			const size_t GroundTypeEnumHash = GetClassHashCode<EWaterGroundType>();
			bMetadataOk = (NumbersValue.Type == EPropertyValueType::ARRAY) &&
				(NumbersValue.ElementType == EPropertyValueType::INT32) &&
				(NumbersValue.ArrayValues.Num<UINT32>() == 3u) &&
				(WeightsValue.Type == EPropertyValueType::MAP) &&
				(WeightsValue.KeyType == EPropertyValueType::STRING) &&
				(WeightsValue.MapValueType == EPropertyValueType::FLOAT) &&
				(WeightsValue.MapEntries.Num<UINT32>() == 2u) &&
				(GroundTypesValue.Type == EPropertyValueType::ARRAY) &&
				(GroundTypesValue.ElementType == EPropertyValueType::ENUM) &&
				(GroundTypesValue.ElementEnumHash == GroundTypeEnumHash) &&
				(GroundTypesValue.ArrayValues.Num<UINT32>() == 2u) &&
				(EnumKeysValue.Type == EPropertyValueType::MAP) &&
				(EnumKeysValue.KeyType == EPropertyValueType::ENUM) &&
				(EnumKeysValue.KeyEnumHash == GroundTypeEnumHash) &&
				(EnumKeysValue.MapValueType == EPropertyValueType::INT32) &&
				(EnumKeysValue.MapEntries.Num<UINT32>() == 2u) &&
				(NestedNumbersValue.Type == EPropertyValueType::ARRAY) &&
				(NestedNumbersValue.ElementType == EPropertyValueType::ARRAY) &&
				(NestedNumbersValue.ArrayValues.Num<UINT32>() == 2u) &&
				(NestedNumbersValue.ArrayValues[0].ContainerValue) &&
				(NestedNumbersValue.ArrayValues[0].ContainerValue->ElementType == EPropertyValueType::INT32) &&
				(NestedNumbersValue.ArrayValues[0].ContainerValue->ArrayValues.Num<UINT32>() == 2u) &&
				(NestedWeightsValue.Type == EPropertyValueType::MAP) &&
				(NestedWeightsValue.MapValueType == EPropertyValueType::ARRAY) &&
				(NestedWeightsValue.MapEntries.Num<UINT32>() == 1u) &&
				(NestedWeightsValue.MapEntries[0].Value.ContainerValue) &&
				(NestedWeightsValue.MapEntries[0].Value.ContainerValue->ElementType == EPropertyValueType::FLOAT) &&
				(NestedWeightsValue.MapEntries[0].Value.ContainerValue->ArrayValues.Num<UINT32>() == 2u) ? TRUE : FALSE;

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

			FPropertyValue NestedShape;
			MakePropertyShape<TArray<INT32>>(NestedShape);
			FPropertyElement AddedNested;
			EPropertyEditor::MakeDefaultElement(&NestedShape, AddedNested);
			FPropertyElement AddedInner;
			if (AddedNested.ContainerValue && AddedNested.ContainerValue->ElementShape)
			{
				EPropertyEditor::MakeDefaultElement(AddedNested.ContainerValue->ElementShape.get(), AddedInner);
			}
			bShapeOk = (AddedNested.Type == EPropertyValueType::ARRAY) &&
				AddedNested.ContainerValue &&
				(AddedNested.ContainerValue->ElementType == EPropertyValueType::INT32) &&
				(AddedNested.ContainerValue->ArrayValues.Num<UINT32>() == 0u) &&
				(AddedInner.Type == EPropertyValueType::INT32) ? TRUE : FALSE;
			if (bShapeOk)
			{
				AddedInner.IntValue = 9;
				AddedNested.ContainerValue->ArrayValues.Add(AddedInner);
				NestedNumbersValue.ArrayValues.Add(AddedNested);
				NestedNumbersProperty->SetValue(&Source, NestedNumbersValue);
			}
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
		INT32 FlatValue = 0;
		INT32 HeightValue = 0;
		const BOOL32 bEnumMapValues = Target.EnumKeys.FindValue(EWaterGroundType::WATER_GROUND_FLAT, FlatValue) &&
			Target.EnumKeys.FindValue(EWaterGroundType::WATER_GROUND_HEIGHT_MAP, HeightValue) ? TRUE : FALSE;
		TArray<FLOAT> NestedWeightValues;
		const BOOL32 bNestedWeightsFound = Target.NestedWeights.FindValue(EString(PE_TEXT("A")), NestedWeightValues) ? TRUE : FALSE;
		const BOOL32 bNestedValuesOk =
			(Target.NestedNumbers.Num<UINT32>() == 3u) &&
			(Target.NestedNumbers[0].Num<UINT32>() == 2u) &&
			(Target.NestedNumbers[0][0] == 1) && (Target.NestedNumbers[0][1] == 2) &&
			(Target.NestedNumbers[1].Num<UINT32>() == 1u) &&
			(Target.NestedNumbers[1][0] == 3) &&
			(Target.NestedNumbers[2].Num<UINT32>() == 1u) &&
			(Target.NestedNumbers[2][0] == 9) &&
			bNestedWeightsFound &&
			(NestedWeightValues.Num<UINT32>() == 2u) &&
			(EMath::Abs(NestedWeightValues[0] - 0.5f) <= 1.0e-5f) &&
			(EMath::Abs(NestedWeightValues[1] - 0.75f) <= 1.0e-5f) ? TRUE : FALSE;
		const BOOL32 bValuesOk = (Target.Numbers.Num<UINT32>() == 4u) &&
			(Target.Numbers[0] == 1) && (Target.Numbers[1] == 2) && (Target.Numbers[2] == 3) && (Target.Numbers[3] == 4) &&
			bMapValues &&
			(EMath::Abs(AValue - 1.5f) <= 1.0e-4f) &&
			(EMath::Abs(BValue - 2.5f) <= 1.0e-4f) &&
			(EMath::Abs(CValue - 3.5f) <= 1.0e-4f) &&
			(Target.GroundTypes.Num<UINT32>() == 2u) &&
			(Target.GroundTypes[0] == EWaterGroundType::WATER_GROUND_FLAT) &&
			(Target.GroundTypes[1] == EWaterGroundType::WATER_GROUND_HEIGHT_MAP) &&
			bEnumMapValues && (FlatValue == 10) && (HeightValue == 20) &&
			bNestedValuesOk ? TRUE : FALSE;

		if (!(bMetadataOk && bValuesOk && bShapeOk))
		{
			PE_LOG_ERROR(EString(PE_TEXT("container detail: metadata=")) + EString::FromInt(bMetadataOk) +
				EString(PE_TEXT(" values=")) + EString::FromInt(bValuesOk) +
				EString(PE_TEXT(" shape=")) + EString::FromInt(bShapeOk) +
				EString(PE_TEXT(" sourceNumbers=")) + EString::FromInt(Source.Numbers.Num<UINT32>()) +
				EString(PE_TEXT(" targetNumbers=")) + EString::FromInt(Target.Numbers.Num<UINT32>()) +
				EString(PE_TEXT(" targetWeights=")) + EString::FromInt(Target.Weights.Num<UINT32>()) +
				EString(PE_TEXT(" A=")) + EString::FromFloat(AValue) +
				EString(PE_TEXT(" B=")) + EString::FromFloat(BValue) +
				EString(PE_TEXT(" C=")) + EString::FromFloat(CValue));
		}
		return (bMetadataOk && bValuesOk && bShapeOk) ? TRUE : FALSE;
	}

	static const FProperty* FindReflectedProperty(const size_t& InClassHash, const EString& InName)
	{
		const FProperty* Found = nullptr;
		EClassPropertyRegistry::ForEachProperty(InClassHash, [&InName, &Found](const FProperty& Property) -> void
		{
			if (Property.Name == InName)
			{
				Found = &Property;
			}
		});
		return Found;
	}

	static const FProperty* FindStructField(const FStructDescriptor& InDescriptor, const EString& InName)
	{
		for (UINT32 Index = 0u, Num = InDescriptor.Fields.Num<UINT32>(); Index < Num; Index++)
		{
			if (InDescriptor.Fields[Index].Name == InName)
			{
				return &InDescriptor.Fields[Index];
			}
		}
		return nullptr;
	}

	// Exercises the struct value tree end to end: descriptor metadata, scalar/bool/enum/asset
	// fields, nested structs, arrays and maps of structs, custom-serializer round trip, shape
	// isolation for newly added elements, and the unchanged CUSTOM fallback.
	static BOOL32 CheckStructProperties()
	{
		RegisterSanityStructProperties();
		const size_t ClassHash = GetClassHashCode<FSanityStructData>();
		const size_t StructHash = GetClassHashCode<FSanityStructData>();
		const size_t NestedHash = GetClassHashCode<FSanityStructNested>();
		const FStructDescriptor* Descriptor = EStructPropertyRegistry::Find(StructHash);
		const FStructDescriptor* NestedDescriptor = EStructPropertyRegistry::Find(NestedHash);
		const FProperty* DataProperty = FindReflectedProperty(ClassHash, EString(PE_TEXT("Data")));

		BOOL32 bMetadataOk = (Descriptor && NestedDescriptor && DataProperty &&
			(Descriptor->Fields.Num<UINT32>() == 7u) &&
			(NestedDescriptor->Fields.Num<UINT32>() == 2u) &&
			(DataProperty->ValueType == EPropertyValueType::STRUCT) &&
			(DataProperty->StructHash == StructHash) &&
			DataProperty->GetValue && DataProperty->SetValue) ? TRUE : FALSE;
		if (bMetadataOk)
		{
			const FProperty* IntegerField = FindStructField(*Descriptor, EString(PE_TEXT("Integer")));
			const FProperty* FlagField = FindStructField(*Descriptor, EString(PE_TEXT("Flag")));
			const FProperty* AssetField = FindStructField(*Descriptor, EString(PE_TEXT("AssetName")));
			const FProperty* GroundField = FindStructField(*Descriptor, EString(PE_TEXT("GroundType")));
			const FProperty* NestedField = FindStructField(*Descriptor, EString(PE_TEXT("Nested")));
			const FProperty* ItemsField = FindStructField(*Descriptor, EString(PE_TEXT("Items")));
			const FProperty* NamedField = FindStructField(*Descriptor, EString(PE_TEXT("NamedItems")));
			bMetadataOk = (IntegerField && IntegerField->ValueType == EPropertyValueType::INT32 &&
				FlagField && FlagField->ValueType == EPropertyValueType::BOOL &&
				AssetField && AssetField->ValueType == EPropertyValueType::STRING && AssetField->AssetType == EPropertyAssetType::MATERIAL &&
				GroundField && GroundField->ValueType == EPropertyValueType::ENUM && GroundField->EnumHash == GetClassHashCode<EWaterGroundType>() &&
				NestedField && NestedField->ValueType == EPropertyValueType::STRUCT && NestedField->StructHash == NestedHash &&
				ItemsField && ItemsField->ValueType == EPropertyValueType::ARRAY && ItemsField->ElementType == EPropertyValueType::STRUCT &&
				ItemsField->ElementStructHash == NestedHash &&
				NamedField && NamedField->ValueType == EPropertyValueType::MAP && NamedField->KeyType == EPropertyValueType::STRING &&
				NamedField->MapValueType == EPropertyValueType::STRUCT && NamedField->MapValueStructHash == NestedHash) ? TRUE : FALSE;
		}

		FSanityStructData Source;
		Source.Integer = 42;
		Source.bFlag = TRUE;
		Source.AssetName = EString(PE_TEXT("M_EngineFallback"));
		Source.GroundType = EWaterGroundType::WATER_GROUND_HEIGHT_MAP;
		Source.Nested.Count = 7;
		Source.Nested.Label = EString(PE_TEXT("Nested"));
		Source.Items.Add(FSanityStructNested{ 1, EString(PE_TEXT("First")) });
		Source.Items.Add(FSanityStructNested{ 2, EString(PE_TEXT("Second")) });
		Source.NamedItems.Add(EString(PE_TEXT("A")), FSanityStructNested{ 3, EString(PE_TEXT("Named")) });

		BOOL32 bValueOk = FALSE;
		BOOL32 bSetOk = FALSE;
		BOOL32 bShapeOk = FALSE;
		BOOL32 bRoundTripOk = FALSE;
		if (DataProperty && DataProperty->GetValue && DataProperty->SetValue)
		{
			FPropertyValue Value;
			DataProperty->GetValue(&Source, Value);
			bValueOk = (Value.Type == EPropertyValueType::STRUCT) &&
				(Value.StructHash == StructHash) &&
				(Value.StructFields.Num<UINT32>() == 7u) &&
				Value.StructFields[0] && (Value.StructFields[0]->Type == EPropertyValueType::INT32) && (Value.StructFields[0]->IntValue == 42) &&
				Value.StructFields[1] && (Value.StructFields[1]->Type == EPropertyValueType::BOOL) && (Value.StructFields[1]->BoolValue != FALSE) &&
				Value.StructFields[2] && (Value.StructFields[2]->Type == EPropertyValueType::STRING) &&
				(Value.StructFields[2]->StringValue == EString(PE_TEXT("M_EngineFallback"))) &&
				Value.StructFields[3] && (Value.StructFields[3]->Type == EPropertyValueType::ENUM) &&
				(Value.StructFields[3]->EnumHash == GetClassHashCode<EWaterGroundType>()) &&
				Value.StructFields[4] && (Value.StructFields[4]->Type == EPropertyValueType::STRUCT) &&
				(Value.StructFields[4]->StructFields.Num<UINT32>() == 2u) &&
				Value.StructFields[4]->StructFields[0]->IntValue == 7 &&
				Value.StructFields[5] && (Value.StructFields[5]->Type == EPropertyValueType::ARRAY) &&
				(Value.StructFields[5]->ElementType == EPropertyValueType::STRUCT) &&
				(Value.StructFields[5]->ElementStructHash == NestedHash) &&
				(Value.StructFields[5]->ArrayValues.Num<UINT32>() == 2u) &&
				Value.StructFields[5]->ArrayValues[0].ContainerValue &&
				(Value.StructFields[5]->ArrayValues[0].ContainerValue->StructFields[0]->IntValue == 1) &&
				Value.StructFields[6] && (Value.StructFields[6]->Type == EPropertyValueType::MAP) &&
				(Value.StructFields[6]->KeyType == EPropertyValueType::STRING) &&
				(Value.StructFields[6]->MapValueType == EPropertyValueType::STRUCT) &&
				(Value.StructFields[6]->MapValueStructHash == NestedHash) &&
				(Value.StructFields[6]->MapEntries.Num<UINT32>() == 1u) &&
				Value.StructFields[6]->MapEntries[0].Value.ContainerValue &&
				(Value.StructFields[6]->MapEntries[0].Value.ContainerValue->StructFields[0]->IntValue == 3) ? TRUE : FALSE;

			// Modify a scalar, a nested field, an array element and a map value, then read the
			// value tree back into the C++ struct.
			Value.StructFields[0]->IntValue = 99;
			Value.StructFields[4]->StructFields[0]->IntValue = 77;
			Value.StructFields[5]->ArrayValues[0].ContainerValue->StructFields[1]->StringValue = EString(PE_TEXT("Changed"));
			Value.StructFields[6]->MapEntries[0].Value.ContainerValue->StructFields[0]->IntValue = 55;
			DataProperty->SetValue(&Source, Value);
			bSetOk = (Source.Integer == 99) &&
				(Source.Nested.Count == 77) &&
				(Source.Items.Num<UINT32>() == 2u) &&
				(Source.Items[0].Label == EString(PE_TEXT("Changed"))) &&
				(Source.Items[1].Label == EString(PE_TEXT("Second"))) ? TRUE : FALSE;
			FSanityStructNested NamedValue;
			bSetOk = bSetOk && (Source.NamedItems.FindValue(EString(PE_TEXT("A")), NamedValue) ? (NamedValue.Count == 55) : FALSE);

			// Two elements made from the same shape must own independent field trees, and
			// editing one must not affect the other or the shape itself.
			FPropertyValue Shape;
			MakePropertyShape<FSanityStructData>(Shape);
			FPropertyElement First;
			FPropertyElement Second;
			EPropertyEditor::MakeDefaultElement(&Shape, First);
			EPropertyEditor::MakeDefaultElement(&Shape, Second);
			bShapeOk = (First.Type == EPropertyValueType::STRUCT) && First.ContainerValue &&
				(Second.Type == EPropertyValueType::STRUCT) && Second.ContainerValue &&
				(First.ContainerValue->StructFields.Num<UINT32>() == 7u) &&
				(Second.ContainerValue->StructFields.Num<UINT32>() == 7u) ? TRUE : FALSE;
			if (bShapeOk)
			{
				First.ContainerValue->StructFields[0]->IntValue = 123;
				bShapeOk = (Second.ContainerValue->StructFields[0]->IntValue == 0) &&
					(Shape.StructFields[0]->IntValue == 0) ? TRUE : FALSE;
			}

			CJsonObject Document;
			EJsonArchiveWriter Writer(Document);
			EClassPropertyRegistry::SerializeObject(Writer, ClassHash, &Source);
			FSanityStructData Target;
			EJsonArchiveReader Reader(Document);
			EClassPropertyRegistry::SerializeObject(Reader, ClassHash, &Target);
			FSanityStructNested TargetNamed;
			bRoundTripOk = (Target.Integer == 99) &&
				(Target.bFlag != FALSE) &&
				(Target.AssetName == EString(PE_TEXT("M_EngineFallback"))) &&
				(Target.GroundType == EWaterGroundType::WATER_GROUND_HEIGHT_MAP) &&
				(Target.Nested.Count == 77) &&
				(Target.Nested.Label == EString(PE_TEXT("Nested"))) &&
				(Target.Items.Num<UINT32>() == 2u) &&
				(Target.Items[0].Label == EString(PE_TEXT("Changed"))) &&
				(Target.Items[1].Count == 2) &&
				(Target.NamedItems.FindValue(EString(PE_TEXT("A")), TargetNamed) ? (TargetNamed.Count == 55) : FALSE) ? TRUE : FALSE;
		}

		// A property still registered through AddClassPropertyCustom has no reflected value
		// accessors and must remain a CUSTOM entry the generated panel skips.
		const size_t SkeletalHash = GetClassHashCode<PSkeletalMeshComponent>();
		const FProperty* MeshAssetsProperty = FindReflectedProperty(SkeletalHash, EString(PE_TEXT("MeshAssets")));
		BOOL32 bCustomFallbackOk = (MeshAssetsProperty &&
			(MeshAssetsProperty->ValueType == EPropertyValueType::CUSTOM) &&
			(!MeshAssetsProperty->GetValue) && (!MeshAssetsProperty->SetValue)) ? TRUE : FALSE;
		PSkeletalMeshComponent* Skeletal = new PSkeletalMeshComponent();
		FPropertyValue EmptyValue;
		bCustomFallbackOk = bCustomFallbackOk &&
			(!EClassPropertyRegistry::SetPropertyValue(SkeletalHash, Skeletal, EString(PE_TEXT("MeshAssets")), EmptyValue)) ? TRUE : FALSE;
		Skeletal->Destroy();

		// Metadata of the migrated properties: the file format stays custom, while the same
		// registration now exposes a STRUCT/struct-container value tree.
		const FProperty* NavSettings = FindReflectedProperty(GetClassHashCode<PNavAgentComponent>(), EString(PE_TEXT("Settings")));
		const FProperty* CascadeData = FindReflectedProperty(GetClassHashCode<PDirectionalLightComponent>(), EString(PE_TEXT("CascadeShadowData")));
		const FProperty* Animation = FindReflectedProperty(SkeletalHash, EString(PE_TEXT("Animation")));
		const FProperty* MaterialSlots = FindReflectedProperty(GetClassHashCode<PMeshComponent>(), EString(PE_TEXT("MaterialSlots")));
		const FStructDescriptor* NavDescriptor = EStructPropertyRegistry::Find(GetClassHashCode<ENavigationAgentSettings>());
		const FStructDescriptor* CascadeDescriptor = EStructPropertyRegistry::Find(GetClassHashCode<ECascadeShadowData>());
		const FStructDescriptor* AnimationDescriptor = Animation ? EStructPropertyRegistry::Find(Animation->StructHash) : nullptr;
		const FStructDescriptor* MaterialDescriptor = EStructPropertyRegistry::Find(GetClassHashCode<FMaterialSlotReference>());
		BOOL32 bMigratedMetadataOk = (NavSettings && NavSettings->ValueType == EPropertyValueType::STRUCT &&
			NavDescriptor && NavDescriptor->Fields.Num<UINT32>() == 18u &&
			CascadeData && CascadeData->ValueType == EPropertyValueType::STRUCT &&
			CascadeDescriptor && CascadeDescriptor->Fields.Num<UINT32>() == 3u &&
			Animation && Animation->ValueType == EPropertyValueType::STRUCT &&
			AnimationDescriptor && AnimationDescriptor->Fields.Num<UINT32>() == 4u &&
			MaterialSlots && MaterialSlots->ValueType == EPropertyValueType::ARRAY &&
			(MaterialSlots->ElementType == EPropertyValueType::STRUCT) &&
			(MaterialSlots->ElementStructHash == GetClassHashCode<FMaterialSlotReference>()) &&
			MaterialDescriptor && MaterialDescriptor->Fields.Num<UINT32>() == 3u) ? TRUE : FALSE;
		if (bMigratedMetadataOk)
		{
			const FProperty* AnimationAsset = FindStructField(*AnimationDescriptor, EString(PE_TEXT("Asset")));
			const FProperty* AnimationPlayRate = FindStructField(*AnimationDescriptor, EString(PE_TEXT("PlayRate")));
			const FProperty* MaterialName = FindStructField(*MaterialDescriptor, EString(PE_TEXT("MaterialName")));
			const FProperty* ShaderName = FindStructField(*MaterialDescriptor, EString(PE_TEXT("ShaderName")));
			bMigratedMetadataOk = (AnimationAsset && AnimationAsset->AssetType == EPropertyAssetType::ANIMATION &&
				AnimationPlayRate && ((AnimationPlayRate->Flags & static_cast<UINT32>(EPropertyFlagsType::COMMIT_ONLY)) != 0u) &&
				MaterialName && MaterialName->AssetType == EPropertyAssetType::MATERIAL &&
				ShaderName && (!ShaderName->SetValue)) ? TRUE : FALSE;
		}

		const BOOL32 bOk = (bMetadataOk && bValueOk && bSetOk && bShapeOk && bRoundTripOk &&
			bCustomFallbackOk && bMigratedMetadataOk) ? TRUE : FALSE;
		if (!bOk)
		{
			PE_LOG_ERROR(EString(PE_TEXT("struct properties detail: metadata=")) + EString::FromInt(bMetadataOk) +
				EString(PE_TEXT(" value=")) + EString::FromInt(bValueOk) +
				EString(PE_TEXT(" set=")) + EString::FromInt(bSetOk) +
				EString(PE_TEXT(" shape=")) + EString::FromInt(bShapeOk) +
				EString(PE_TEXT(" roundtrip=")) + EString::FromInt(bRoundTripOk) +
				EString(PE_TEXT(" custom=")) + EString::FromInt(bCustomFallbackOk) +
				EString(PE_TEXT(" migrated=")) + EString::FromInt(bMigratedMetadataOk));
		}
		return bOk;
	}

	// Undo/redo of a struct array property through the same command path the Details panel
	// uses. Material slots are the migrated property with the richest setter side effects.
	static BOOL32 CheckStructPropertyUndo(PWorld* InWorld)
	{
		PActor* Actor = new PActor();
		POBJ_DEBUGNAME_SET(Actor, "SanityStructUndoActor");
		InWorld->AddActor(Actor);
		PStaticMeshComponent* Mesh = new PStaticMeshComponent();
		Actor->AddComponent(Mesh);

		FMaterialSlotReference Reference;
		Reference.MaterialName = EString(PE_TEXT("M_EngineFallback"));
		Reference.ShaderName = EString(PE_TEXT("M_EngineFallback"));
		Reference.ProgramName = EString(PE_TEXT("BasePass"));
		Mesh->SetMaterialSlotReference(0u, Reference);

		const size_t ClassHash = GetClassHashCode(Mesh);
		const FProperty* MaterialSlots = FindReflectedProperty(ClassHash, EString(PE_TEXT("MaterialSlots")));
		BOOL32 bUndoOk = FALSE;
		BOOL32 bRedoOk = FALSE;
		BOOL32 bValueOk = FALSE;
		if (MaterialSlots && MaterialSlots->GetValue && MaterialSlots->SetValue)
		{
			FPropertyValue Before;
			MaterialSlots->GetValue(Mesh, Before);
			FPropertyValue After;
			ClonePropertyValue(Before, After);
			if ((After.ArrayValues.Num<UINT32>() == 1u) &&
				After.ArrayValues[0].ContainerValue &&
				(After.ArrayValues[0].ContainerValue->StructFields.Num<UINT32>() >= 1u))
			{
				After.ArrayValues[0].ContainerValue->StructFields[0]->StringValue = EString(PE_TEXT("Projects/Demo/M_DemoUnlit"));
				MaterialSlots->SetValue(Mesh, After);
				FWorldEditorCommandStack Stack;
				Stack.GetContext().World = InWorld;
				const BOOL32 bPushed = PushSetPropertyCommand(Stack, MakeObjectHandle(Mesh), TRUE, ClassHash,
					EString(PE_TEXT("MaterialSlots")), Before, After);
				bValueOk = (Mesh->GetMaterialSlotReferences().Num<UINT32>() == 1u) &&
					(Mesh->GetMaterialSlotReferences()[0].MaterialName == EString(PE_TEXT("Projects/Demo/M_DemoUnlit"))) ? TRUE : FALSE;
				bUndoOk = Stack.Undo() && (Mesh->GetMaterialSlotReferences()[0].MaterialName == EString(PE_TEXT("M_EngineFallback"))) ? TRUE : FALSE;
				bRedoOk = Stack.Redo() && (Mesh->GetMaterialSlotReferences()[0].MaterialName == EString(PE_TEXT("Projects/Demo/M_DemoUnlit"))) ? TRUE : FALSE;
				bUndoOk = bUndoOk && bPushed ? TRUE : FALSE;
				Stack.Clear();
			}
		}

		InWorld->DestroyActor(Actor);
		InWorld->FlushPendingDestroy();
		return (bValueOk && bUndoOk && bRedoOk) ? TRUE : FALSE;
	}

	// A character's physics body and movement component are constructed by InitCharacter,
	// not by the class factory. Loading one must therefore run PostDeserialize after the
	// serialized properties and components are installed, and the movement component must
	// not be written to the file as a disconnected factory object.
	static BOOL32 CheckCharacterSerialization(PWorld* InWorld)
	{
		PCharacter* Character = new PCharacter();
		Character->CharacterHeightStanding = 150.f;
		{
			FCharacterVirtualSettings Settings;
			Character->InitCharacter(Settings);
		}
		POBJ_DEBUGNAME_SET(Character, "SanityCharacter");
		InWorld->AddActor(Character);

		TArray<PActor*> Actors;
		Actors.Add(Character);
		CJsonObject Document;
		const BOOL32 bSaved = ELevelSerializer::SaveActors(Actors, Document);

		InWorld->DestroyActor(Character);
		InWorld->FlushPendingDestroy();

		TArray<PActor*> LoadedActors;
		const BOOL32 bLoaded = ELevelSerializer::LoadActors(*InWorld, Document, &LoadedActors);
		PCharacter* LoadedCharacter = nullptr;
		if (bLoaded && (LoadedActors.Num<UINT32>() == 1u))
		{
			LoadedCharacter = dynamic_cast<PCharacter*>(LoadedActors[0]);
		}
		const BOOL32 bBodyOk = (LoadedCharacter && LoadedCharacter->GetPhysicsCharacter()) ? TRUE : FALSE;
		const BOOL32 bMovementOk = (LoadedCharacter && LoadedCharacter->GetMovementComponent() &&
			(LoadedCharacter->GetMovementComponent()->GetOwnerActor() == LoadedCharacter)) ? TRUE : FALSE;
		const BOOL32 bHeightOk = (LoadedCharacter &&
			(EMath::Abs(LoadedCharacter->CharacterHeightStanding - 150.f) <= 1.0e-4f)) ? TRUE : FALSE;

		for (UINT32 Index = 0u, Num = LoadedActors.Num<UINT32>(); Index < Num; Index++)
		{
			if (LoadedActors[Index])
			{
				InWorld->DestroyActor(LoadedActors[Index]);
			}
		}
		InWorld->FlushPendingDestroy();
		return (bSaved && bLoaded && bBodyOk && bMovementOk && bHeightOk) ? TRUE : FALSE;
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
		const BOOL32 bWorldTickScheduler = CheckWorldTickScheduler();
		ReportSanityResult(EString(PE_TEXT("world tick scheduler domains and isolation")), bWorldTickScheduler);
		if (!bWorldTickScheduler)
		{
			FailedCount += 1u;
		}
		const BOOL32 bRenderTreeDirtySkip = CheckRenderTreeDirtySkip(InWorld);
		ReportSanityResult(EString(PE_TEXT("render tree dirty skip")), bRenderTreeDirtySkip);
		if (!bRenderTreeDirtySkip)
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
		const BOOL32 bLevelAcceptance = CheckLevelAcceptanceRoundTrip();
		ReportSanityResult(EString(PE_TEXT("level acceptance round trip")), bLevelAcceptance);
		if (!bLevelAcceptance)
		{
			FailedCount += 1u;
		}
		const BOOL32 bWorldEditorCommands = CheckWorldEditorCommands();
		ReportSanityResult(EString(PE_TEXT("world editor command stack")), bWorldEditorCommands);
		const BOOL32 bAttachmentReparent = CheckWorldAttachmentReparent();
		ReportSanityResult(EString(PE_TEXT("attachment reparent commands")), bAttachmentReparent);
		const BOOL32 bEditorCustomization = CheckEditorCustomizationRegistry();
		ReportSanityResult(EString(PE_TEXT("editor customization registry")), bEditorCustomization);
		const BOOL32 bEditorWorldContext = CheckEditorWorldContextSelection();
		ReportSanityResult(EString(PE_TEXT("editor world context selection")), bEditorWorldContext);
		const BOOL32 bLevelFileState = CheckLevelFileStateTransitions();
		ReportSanityResult(EString(PE_TEXT("level file state transitions")), bLevelFileState);
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
		const BOOL32 bStructProperties = CheckStructProperties();
		ReportSanityResult(EString(PE_TEXT("struct property serialization and metadata")), bStructProperties);
		if (!bStructProperties)
		{
			FailedCount += 1u;
		}
		const BOOL32 bStructPropertyUndo = CheckStructPropertyUndo(InWorld);
		ReportSanityResult(EString(PE_TEXT("struct property undo and redo")), bStructPropertyUndo);
		if (!bStructPropertyUndo)
		{
			FailedCount += 1u;
		}
		const BOOL32 bMaterialCatalog = CheckMaterialSourceCatalog();
		ReportSanityResult(EString(PE_TEXT("material source catalog")), bMaterialCatalog);
		if (!bMaterialCatalog)
		{
			FailedCount += 1u;
		}
		const BOOL32 bNavMeshCacheReload = CheckNavMeshAssetCacheReload();
		ReportSanityResult(EString(PE_TEXT("navmesh asset cache reload")), bNavMeshCacheReload);
		if (!bNavMeshCacheReload)
		{
			FailedCount += 1u;
		}
		const BOOL32 bNavMeshAutoAssign = CheckNavMeshSaveAutoAssign();
		ReportSanityResult(EString(PE_TEXT("navmesh save auto-assign")), bNavMeshAutoAssign);
		if (!bNavMeshAutoAssign)
		{
			FailedCount += 1u;
		}
		const BOOL32 bNavMeshAreaBake = CheckNavMeshAreaBake();
		ReportSanityResult(EString(PE_TEXT("navmesh area bake and filters")), bNavMeshAreaBake);
		if (!bNavMeshAreaBake)
		{
			FailedCount += 1u;
		}
		const BOOL32 bCharacterSerialization = CheckCharacterSerialization(InWorld);
		ReportSanityResult(EString(PE_TEXT("character serialization round trip")), bCharacterSerialization);
		if (!bCharacterSerialization)
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
