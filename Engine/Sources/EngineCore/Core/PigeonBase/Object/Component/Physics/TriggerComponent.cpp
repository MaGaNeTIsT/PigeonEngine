#include "TriggerComponent.h"
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/World/World.h>
#include <PigeonBase/Object/World/WorldTickManager.h>

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		RegisterClassType<PTriggerComponent, PSceneComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	static UINT64 PackSubShapePair(UINT32 InSelfSubShape, UINT32 InOtherSubShape)
	{
		return ((static_cast<UINT64>(InSelfSubShape) << 32) | static_cast<UINT64>(InOtherSubShape));
	}

	PTriggerComponent::PTriggerComponent()
	{
		POBJ_DEBUGNAME_SET(this, PE_TEXT("TriggerComponent"));
	}

	PTriggerComponent::~PTriggerComponent()
	{
		// Uninit normally released the shape. The safety net covers a component
		// that is destroyed outside the normal scene teardown path.
		if (Shape && bOwnShape)
		{
			delete Shape;
		}
		Shape = nullptr;
		bOwnShape = FALSE;
	}

	void PTriggerComponent::Init()
	{
		PSceneComponent::Init();
	}

	void PTriggerComponent::Uninit()
	{
		if (Shape && bOwnShape)
		{
			delete Shape;
		}
		Shape = nullptr;
		bOwnShape = FALSE;
		PSceneComponent::Uninit();
	}

	void PTriggerComponent::SetShape(FShape* InShape, BOOL32 InIsOwnShape)
	{
		if (Shape == InShape && bOwnShape == InIsOwnShape)
		{
			return;
		}

		ClearOverlaps(TRUE);

		if (GetWorld() && Backend == ETriggerBackendType::SENSOR)
		{
			DestroySensorBody();
		}

		if (Shape && bOwnShape)
		{
			delete Shape;
		}

		Shape = InShape;
		bOwnShape = InIsOwnShape;

		if (GetWorld() && Backend == ETriggerBackendType::SENSOR)
		{
			CreateSensorBody();
		}
	}

	FShape* PTriggerComponent::GetShape() const
	{
		return Shape;
	}

	void PTriggerComponent::SetBackend(ETriggerBackendType InBackend)
	{
		if (Backend == InBackend)
		{
			return;
		}

		ClearOverlaps(TRUE);

		if (GetWorld() && Backend == ETriggerBackendType::SENSOR)
		{
			DestroySensorBody();
		}

		Backend = InBackend;

		if (GetWorld() && Backend == ETriggerBackendType::SENSOR)
		{
			CreateSensorBody();
		}
	}

	ETriggerBackendType PTriggerComponent::GetBackend() const
	{
		return Backend;
	}

	const FPhysicsBodyId& PTriggerComponent::GetSensorBodyId() const
	{
		return SensorBodyId;
	}

	const TArray<FOverlapResult>& PTriggerComponent::GetOverlaps() const
	{
		return CurrentOverlaps;
	}

	BOOL32 PTriggerComponent::IsOverlapping(const ObjectIdentityType& InObjectId) const
	{
		for (UINT32 Index = 0u, Num = CurrentOverlaps.Num<UINT32>(); Index < Num; Index++)
		{
			if (CurrentOverlaps[Index].ObjectId == InObjectId)
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	void PTriggerComponent::OnTriggerEnter(const FOverlapResult& InOther)
	{
		if (OnTriggerEnterCallback.IsValid())
		{
			OnTriggerEnterCallback(InOther);
		}
	}

	void PTriggerComponent::OnTriggerExit(const FOverlapResult& InOther)
	{
		if (OnTriggerExitCallback.IsValid())
		{
			OnTriggerExitCallback(InOther);
		}
	}

	void PTriggerComponent::OnTriggerStay(const FOverlapResult& InOther)
	{
		if (OnTriggerStayCallback.IsValid())
		{
			OnTriggerStayCallback(InOther);
		}
	}

	void PTriggerComponent::BeginAddedToScene(PWorld* World)
	{
		PSceneComponent::BeginAddedToScene(World);
		TryRegisterPostPhysicsTick();

		if (Backend == ETriggerBackendType::SENSOR)
		{
			CreateSensorBody();
		}
	}

	void PTriggerComponent::RemovedFromScene()
	{
		// Fire exits while the world/owner are still valid, then drop the body
		// and listener. Uninit runs after the owner has been cleared.
		ClearOverlaps(TRUE);

		if (Backend == ETriggerBackendType::SENSOR)
		{
			DestroySensorBody();
		}

		TryUnregisterPostPhysicsTick();
		PSceneComponent::RemovedFromScene();
	}

	void PTriggerComponent::TryRegisterPostPhysicsTick()
	{
		if (GetWorld() && !bPostPhysicsTickRegistered)
		{
			PWorld* World = GetWorld();
			const ObjectIdentityType ComponentID = GetUniqueID();
			PostPhysicsTickHandler = TFunction<void(FLOAT)>([World, ComponentID](FLOAT DeltaTime)
			{
				if (!World)
				{
					return;
				}

				const PActorComponent* FoundComponent = World->GetComponentByUniqueID(ComponentID, TRUE);
				PTriggerComponent* Trigger = dynamic_cast<PTriggerComponent*>(const_cast<PActorComponent*>(FoundComponent));
				if (Trigger)
				{
					Trigger->HandlePostPhysicsTick(DeltaTime);
				}
			});
			EWorldTickManager::GetManagerSingleton()->RegisterPostPhysicsTick(PostPhysicsTickHandler);
			bPostPhysicsTickRegistered = TRUE;
		}
	}

	void PTriggerComponent::TryUnregisterPostPhysicsTick()
	{
		if (bPostPhysicsTickRegistered)
		{
			EWorldTickManager::GetManagerSingleton()->UnregisterPostPhysicsTick(PostPhysicsTickHandler);
			bPostPhysicsTickRegistered = FALSE;
		}
	}

	void PTriggerComponent::HandlePostPhysicsTick(FLOAT DeltaTime)
	{
		if (!GetWorld())
		{
			return;
		}
		UpdateOverlaps(DeltaTime);
	}

	void PTriggerComponent::UpdateOverlaps(FLOAT DeltaTime)
	{
		(void)DeltaTime;

		if (!Shape)
		{
			return;
		}

		if (Backend == ETriggerBackendType::SENSOR)
		{
			if (SensorBodyId.IsInvalid())
			{
				CreateSensorBody();
			}
			UpdateSensorBodyTransform();
		}

		CollectCurrentOverlaps();
		DispatchOverlapEvents();

		PreviousBodies = CurrentBodies;
		PreviousCharacters = CurrentCharacters;
	}

	void PTriggerComponent::CollectCurrentOverlaps()
	{
		CurrentBodies.Empty();
		CurrentCharacters.Empty();
		CurrentOverlaps.Empty();

		if (!Shape)
		{
			return;
		}

		const Vector3 Location = GetComponentWorldLocation();
		const Quaternion Rotation = GetComponentWorldRotation();
		const ObjectIdentityType IgnoredObjectId = GetIgnoredObjectId();

		if (Backend == ETriggerBackendType::QUERY)
		{
			TArray<FOverlapResult> Hits;
			FPhysicsManager::GetManagerSingleton()->OverlapShape(
				Shape, Location, Rotation, Hits, MaxOverlaps,
				FPhysicsDefaultBroadPhaseLayerFilter::Get(), QueryLayerFilter,
				FPhysicsBodyId(), IgnoredObjectId, bDetectStaticBodies, FALSE, bDetectCharacters);

			for (UINT32 Index = 0u, Num = Hits.Num<UINT32>(); Index < Num; Index++)
			{
				const FOverlapResult& Hit = Hits[Index];
				if (Hit.IsCharacter())
				{
					CurrentCharacters.Add(Hit.ObjectId, Hit);
				}
				else
				{
					CurrentBodies.Add(Hit.BodyId, Hit);
				}
			}
		}
		else
		{
			for (TMap<FPhysicsBodyId, FSensorBodyOverlap>::TConstIterator It = SensorOverlaps.Begin(); It != SensorOverlaps.End(); It++)
			{
				FOverlapResult Hit;
				Hit.BodyId = It->first;
				Hit.ObjectId = It->second.ObjectId;

				if (IgnoredObjectId != 0ull && Hit.ObjectId == IgnoredObjectId)
				{
					continue;
				}
				CurrentBodies.Add(It->first, Hit);
			}

			if (bDetectCharacters)
			{
				TArray<FOverlapResult> CharacterHits;
				FPhysicsManager::GetManagerSingleton()->OverlapCharacters(
					Shape, Location, Rotation, CharacterHits, MaxOverlaps, IgnoredObjectId);

				for (UINT32 Index = 0u, Num = CharacterHits.Num<UINT32>(); Index < Num; Index++)
				{
					CurrentCharacters.Add(CharacterHits[Index].ObjectId, CharacterHits[Index]);
				}
			}
		}

		for (TMap<FPhysicsBodyId, FOverlapResult>::TConstIterator It = CurrentBodies.Begin(); It != CurrentBodies.End(); It++)
		{
			CurrentOverlaps.Add(It->second);
		}
		for (TMap<ObjectIdentityType, FOverlapResult>::TConstIterator It = CurrentCharacters.Begin(); It != CurrentCharacters.End(); It++)
		{
			CurrentOverlaps.Add(It->second);
		}
	}

	void PTriggerComponent::DispatchOverlapEvents()
	{
		for (TMap<FPhysicsBodyId, FOverlapResult>::TConstIterator It = CurrentBodies.Begin(); It != CurrentBodies.End(); It++)
		{
			const FOverlapResult* Previous = PreviousBodies.FindValueAsPtr(It->first);
			if (!Previous)
			{
				OnTriggerEnter(It->second);
			}
			else if (bFireStayEvents)
			{
				OnTriggerStay(It->second);
			}
		}

		for (TMap<FPhysicsBodyId, FOverlapResult>::TConstIterator It = PreviousBodies.Begin(); It != PreviousBodies.End(); It++)
		{
			if (!CurrentBodies.ContainsKey(It->first))
			{
				OnTriggerExit(It->second);
			}
		}

		for (TMap<ObjectIdentityType, FOverlapResult>::TConstIterator It = CurrentCharacters.Begin(); It != CurrentCharacters.End(); It++)
		{
			const FOverlapResult* Previous = PreviousCharacters.FindValueAsPtr(It->first);
			if (!Previous)
			{
				OnTriggerEnter(It->second);
			}
			else if (bFireStayEvents)
			{
				OnTriggerStay(It->second);
			}
		}

		for (TMap<ObjectIdentityType, FOverlapResult>::TConstIterator It = PreviousCharacters.Begin(); It != PreviousCharacters.End(); It++)
		{
			if (!CurrentCharacters.ContainsKey(It->first))
			{
				OnTriggerExit(It->second);
			}
		}
	}

	void PTriggerComponent::ClearOverlaps(BOOL32 bFireExitEvents)
	{
		if (bFireExitEvents)
		{
			for (TMap<FPhysicsBodyId, FOverlapResult>::TConstIterator It = PreviousBodies.Begin(); It != PreviousBodies.End(); It++)
			{
				OnTriggerExit(It->second);
			}
			for (TMap<ObjectIdentityType, FOverlapResult>::TConstIterator It = PreviousCharacters.Begin(); It != PreviousCharacters.End(); It++)
			{
				OnTriggerExit(It->second);
			}
		}

		PreviousBodies.Empty();
		CurrentBodies.Empty();
		PreviousCharacters.Empty();
		CurrentCharacters.Empty();
		CurrentOverlaps.Empty();
		SensorOverlaps.Empty();
	}

	void PTriggerComponent::CreateSensorBody()
	{
		if (!SensorBodyId.IsInvalid() || !Shape || !GetOwnerActor())
		{
			return;
		}

		FPhysicsManager* Manager = FPhysicsManager::GetManagerSingleton();
		const Vector3 Location = GetComponentWorldLocation();
		const Quaternion Rotation = GetComponentWorldRotation();
		// Key the sensor body by the component identity, not the actor identity:
		// FPhysics_Jolt::RemoveBody removes every body registered under one object
		// id, so using the actor id would also remove an unrelated PPhysicsComponent
		// body that lives on the same actor.
		const ObjectIdentityType BodyObjectId = GetUniqueID();

		FPhysicsBodyId NewBodyId;
		if (!Manager->TryCreateBody(Shape, FALSE, Location, Rotation, SensorMotionType, SensorLayer, BodyObjectId, NewBodyId, TRUE))
		{
			return;
		}

		SensorBodyId = NewBodyId;
		LastSensorLocation = Location;
		LastSensorRotation = Rotation;

		// Register before adding: the first contacts are produced by the next
		// PhysicsSystem::Update, but keeping the listener first avoids relying on
		// the add order.
		Manager->AddPhysicsListener(static_cast<FContactEventListenerInterface*>(this));

		const PhysicsUtility::EActivate Activation = (SensorMotionType == PhysicsUtility::EMotionType::Static)
			? PhysicsUtility::EActivate::DontActivate
			: PhysicsUtility::EActivate::Activate;
		Manager->AddBody(BodyObjectId, SensorBodyId, Activation);
	}

	void PTriggerComponent::DestroySensorBody()
	{
		if (SensorBodyId.IsInvalid())
		{
			SensorOverlaps.Empty();
			return;
		}

		FPhysicsManager* Manager = FPhysicsManager::GetManagerSingleton();
		Manager->RemovePhysicsListener(static_cast<FContactEventListenerInterface*>(this));

		// FALSE: the component owns the FShape and releases it in Uninit.
		Manager->RemoveBody(GetUniqueID(), FALSE);

		SensorBodyId = FPhysicsBodyId();
		SensorOverlaps.Empty();
	}

	void PTriggerComponent::UpdateSensorBodyTransform()
	{
		if (SensorBodyId.IsInvalid())
		{
			return;
		}

		const Vector3 Location = GetComponentWorldLocation();
		const Quaternion Rotation = GetComponentWorldRotation();
		if (!PhysicsUtility::HasTransformChanged(Location, Rotation, LastSensorLocation, LastSensorRotation))
		{
			return;
		}

		FPhysicsManager* Manager = FPhysicsManager::GetManagerSingleton();
		const PhysicsUtility::EActivate Activation = (SensorMotionType == PhysicsUtility::EMotionType::Static)
			? PhysicsUtility::EActivate::DontActivate
			: PhysicsUtility::EActivate::Activate;
		Manager->SetPosition(SensorBodyId, Location, Activation);
		Manager->SetRoation(SensorBodyId, Rotation, Activation);

		LastSensorLocation = Location;
		LastSensorRotation = Rotation;
	}

	void PTriggerComponent::OnContactAdded(const FPhysicsContactEvent& InEvent)
	{
		if (Backend != ETriggerBackendType::SENSOR || SensorBodyId.IsInvalid())
		{
			return;
		}
		if (InEvent.BodyId1 != SensorBodyId && InEvent.BodyId2 != SensorBodyId)
		{
			return;
		}

		const BOOL32 bSelfIsOne = (InEvent.BodyId1 == SensorBodyId);
		const FPhysicsBodyId OtherBody = bSelfIsOne ? InEvent.BodyId2 : InEvent.BodyId1;
		const UINT32 SelfSubShape = bSelfIsOne ? InEvent.SubShapeID1 : InEvent.SubShapeID2;
		const UINT32 OtherSubShape = bSelfIsOne ? InEvent.SubShapeID2 : InEvent.SubShapeID1;
		const ObjectIdentityType OtherObject = bSelfIsOne ? InEvent.ObjectId2 : InEvent.ObjectId1;

		FSensorBodyOverlap* Existing = SensorOverlaps.FindValueAsPtr(OtherBody);
		if (!Existing)
		{
			FSensorBodyOverlap NewOverlap;
			NewOverlap.ObjectId = OtherObject;
			SensorOverlaps.Add(OtherBody, NewOverlap);
			Existing = SensorOverlaps.FindValueAsPtr(OtherBody);
		}

		if (Existing)
		{
			if (Existing->ObjectId == 0ull)
			{
				Existing->ObjectId = OtherObject;
			}
			Existing->SubShapePairs.Add(PackSubShapePair(SelfSubShape, OtherSubShape));
		}
	}

	void PTriggerComponent::OnContactPersisted(const FPhysicsContactEvent& InEvent)
	{
		(void)InEvent;
	}

	void PTriggerComponent::OnContactRemoved(const FPhysicsContactEvent& InEvent)
	{
		if (Backend != ETriggerBackendType::SENSOR || SensorBodyId.IsInvalid())
		{
			return;
		}
		if (InEvent.BodyId1 != SensorBodyId && InEvent.BodyId2 != SensorBodyId)
		{
			return;
		}

		const BOOL32 bSelfIsOne = (InEvent.BodyId1 == SensorBodyId);
		const FPhysicsBodyId OtherBody = bSelfIsOne ? InEvent.BodyId2 : InEvent.BodyId1;
		const UINT32 SelfSubShape = bSelfIsOne ? InEvent.SubShapeID1 : InEvent.SubShapeID2;
		const UINT32 OtherSubShape = bSelfIsOne ? InEvent.SubShapeID2 : InEvent.SubShapeID1;

		FSensorBodyOverlap* Existing = SensorOverlaps.FindValueAsPtr(OtherBody);
		if (!Existing)
		{
			return;
		}

		Existing->SubShapePairs.Remove(PackSubShapePair(SelfSubShape, OtherSubShape));
		if (Existing->SubShapePairs.Num() == 0)
		{
			SensorOverlaps.Remove(OtherBody);
		}
	}

	ObjectIdentityType PTriggerComponent::GetIgnoredObjectId() const
	{
		if (!bIgnoreOwnerActor)
		{
			return 0ull;
		}

		const PActor* OwnerActor = GetOwnerActor();
		return OwnerActor ? OwnerActor->GetUniqueID() : 0ull;
	}
}   // namespace PigeonEngine
