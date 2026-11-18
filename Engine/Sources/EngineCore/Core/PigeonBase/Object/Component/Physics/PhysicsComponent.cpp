#include "PhysicsComponent.h"
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/World/World.h>
#include <PigeonBase/Object/World/WorldTickManager.h>
#include <PhysicsConfig/PhysicsConfig.h>
#if ENGINE_EDITOR_ONLY
#include <RenderProxy/RenderSingletonObject.h>
#endif

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		RegisterClassType<PPhysicsComponent, PActorComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);


	PPhysicsComponent::~PPhysicsComponent()
	{
		if (Shape)
			RemoveShape();
	}

	void PPhysicsComponent::Init()
	{
		PActorComponent::Init();
		FPhysicsManager::GetManagerSingleton()->AddPhysicsListener(static_cast<FBodyActivationEventListenerInterface*>(this));
		FPhysicsManager::GetManagerSingleton()->AddPhysicsListener(static_cast<FContactEventListenerInterface*>(this));
		FPhysicsManager::GetManagerSingleton()->AddPhysicsListener(static_cast<FPhysicsContactModifierInterface*>(this));
		InitPhysicsComponent();
	}

	void PPhysicsComponent::BeginAddedToScene(PWorld* World)
	{
		PActorComponent::BeginAddedToScene(World);
		RegisterPostPhysicsTick();
	}

	void PPhysicsComponent::RemovedFromScene()
	{
		UnregisterPostPhysicsTick();
		PActorComponent::RemovedFromScene();
	}

	void PPhysicsComponent::Uninit()
	{
		FPhysicsManager::GetManagerSingleton()->RemovePhysicsListener(static_cast<FBodyActivationEventListenerInterface*>(this));
		FPhysicsManager::GetManagerSingleton()->RemovePhysicsListener(static_cast<FContactEventListenerInterface*>(this));
		FPhysicsManager::GetManagerSingleton()->RemovePhysicsListener(static_cast<FPhysicsContactModifierInterface*>(this));
		PActorComponent::Uninit();
		UninitPhysicsComponent();
	}

	void PPhysicsComponent::OnDestroy()
	{
		// Single removal, while the owner actor is still valid (OnPreDelete has not
		// cleared it yet). RemoveShape unregisters the body by the owner's unique ID and
		// is a no-op on a second call, so the later Uninit path is harmless.
		RemoveShape();
	}

	void PPhysicsComponent::HandlePostPhysicsTick(FLOAT deltaTime)
	{
		(void)deltaTime;
		if (BodyId.IsInvalid() || !GetOwnerActor() || MotionType == PhysicsUtility::EMotionType::Static)
		{
			return;
		}

		if (!bBodyActive && bHasSyncedTransform)
		{
			return;
		}

	const Vector3 NewPosition = FPhysicsManager::GetManagerSingleton()->GetPosition(BodyId);
		const Quaternion NewRotation = FPhysicsManager::GetManagerSingleton()->GetRotation(BodyId);
		if (!bHasSyncedTransform || PhysicsUtility::HasTransformChanged(NewPosition, NewRotation, LastSyncedPosition, LastSyncedRotation))
		{
			GetOwnerActor()->SetActorLocation(NewPosition);
			GetOwnerActor()->SetActorRotation(NewRotation);
			LastSyncedPosition = NewPosition;
			LastSyncedRotation = NewRotation;
			bHasSyncedTransform = TRUE;
		}
	}
#if ENGINE_EDITOR_ONLY
	void PPhysicsComponent::EditorTick(FLOAT deltaTime)
	{
		if (FPhysicsCommonSettings::GetSingleton()->bDrawCollision != FALSE)
		{
			DrawPrimitive();
		}
	}
	void PPhysicsComponent::DrawPrimitive()
	{
		RDebugWireframePrimitiveManager* Manager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		const Vector3 Pos = GetOwnerActor()->GetActorLocation();
		const Quaternion Rot = GetOwnerActor()->GetActorRotation();
		if (Shape)
		{
			Shape->DrawPrimitive(Manager, Pos, Rot);
		}
	}
#endif
	void PPhysicsComponent::SetShape(FShape* InShape)
	{
		if (Shape)
		{
			UninitPhysicsComponent();
			Shape = InShape;
			InitPhysicsComponent();
		}
		else
			Shape = InShape;
	}

	void PPhysicsComponent::InitPhysicsComponent()
	{
		Vector3 pos = GetOwnerActor()->GetRootComponent()->GetComponentWorldLocation();
		Quaternion rot = GetOwnerActor()->GetRootComponent()->GetComponentWorldRotation();
		if (FPhysicsManager::GetManagerSingleton()->TryCreateBody(Shape, FALSE, pos, rot, MotionType, Layer, GetOwnerActor()->GetUniqueID(), BodyId))
		{
		FPhysicsManager::GetManagerSingleton()->AddBody(GetOwnerActor()->GetUniqueID(), BodyId);
			LastSyncedPosition = pos;
			LastSyncedRotation = rot;
			bHasSyncedTransform = TRUE;
			bBodyActive = FALSE;
		}
	}

	void PPhysicsComponent::UninitPhysicsComponent()
	{
		RemoveShape();
	}

	FShape* PPhysicsComponent::GetShape()
	{
		return Shape;
	}

	void PPhysicsComponent::RemoveShape()
	{
		// The owner is cleared only in the final destroy step, so it is valid on every
		// orchestrated path; the guard covers a component destroyed before it ever got an
		// owner (constructed for an actor but never added).
		if (GetOwnerActor())
		{
			FPhysicsManager::GetManagerSingleton()->RemoveBody(GetOwnerActor()->GetUniqueID(), TRUE);
		}
		BodyId = FPhysicsBodyId();
		Shape = nullptr;
		bBodyActive = FALSE;
		bHasSyncedTransform = FALSE;
		LastSyncedPosition = Vector3::Zero();
		LastSyncedRotation = Quaternion::Identity();
	}

	void PPhysicsComponent::AddForce(Vector3 InForce)
	{
		FPhysicsManager::GetManagerSingleton()->AddForce(BodyId, InForce);
	}

	void PPhysicsComponent::AddImpluse(Vector3 InImpluse)
	{
		FPhysicsManager::GetManagerSingleton()->AddImpulse(BodyId, InImpluse);
	}

	void PPhysicsComponent::SetLayer(FPhysicsObjectLayer InLayer)
	{
		Layer = InLayer;
	}

	FPhysicsObjectLayer PPhysicsComponent::GetLayer() const
	{
		return Layer;
	}

	const FPhysicsBodyId& PPhysicsComponent::GetBodyId() const
	{
		return BodyId;
	}

	void PPhysicsComponent::OnBodyActivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID)
	{
		if (ContainsBody(inBodyID))
		{
			bBodyActive = TRUE;
			OnPhysicsBodyActivated(inBodyID, inObjectID);
		}
	}

	void PPhysicsComponent::OnBodyDeactivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID)
	{
		if (ContainsBody(inBodyID))
		{
			bBodyActive = FALSE;
			OnPhysicsBodyDeactivated(inBodyID, inObjectID);
		}
	}

	EPhysicsContactValidateResult PPhysicsComponent::OnContactValidate(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const Vector3& inBaseOffset, const FPhysicsContactValidateResult& inCollisionResult)
	{
		if (!ContainsAnyBody(inBodyID1, inBodyID2))
		{
			return EPhysicsContactValidateResult::AcceptAllContactsForThisBodyPair;
		}

		return OnPhysicsContactValidate(inBodyID1, inBodyID2, inBaseOffset, inCollisionResult);
	}

	void PPhysicsComponent::OnContactAdded(const FPhysicsContactEvent& InEvent)
	{
		if (ContainsAnyBody(InEvent.BodyId1, InEvent.BodyId2))
		{
			OnPhysicsContactAdded(InEvent);
		}
	}

	void PPhysicsComponent::OnContactPersisted(const FPhysicsContactEvent& InEvent)
	{
		if (ContainsAnyBody(InEvent.BodyId1, InEvent.BodyId2))
		{
			OnPhysicsContactPersisted(InEvent);
		}
	}

	void PPhysicsComponent::OnContactRemoved(const FPhysicsContactEvent& InEvent)
	{
		if (ContainsAnyBody(InEvent.BodyId1, InEvent.BodyId2))
		{
			OnPhysicsContactRemoved(InEvent);
		}
	}

	void PPhysicsComponent::OnContactSettingsAdded(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings)
	{
		if (ContainsAnyBody(inBodyID1, inBodyID2))
		{
			OnPhysicsContactSettingsAdded(inBodyID1, inBodyID2, inManifold, ioSettings);
		}
	}

	void PPhysicsComponent::OnContactSettingsPersisted(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings)
	{
		if (ContainsAnyBody(inBodyID1, inBodyID2))
		{
			OnPhysicsContactSettingsPersisted(inBodyID1, inBodyID2, inManifold, ioSettings);
		}
	}

	BOOL32 PPhysicsComponent::ContainsBody(const FPhysicsBodyId& InBodyID) const
	{
		return BodyId == InBodyID;
	}

	BOOL32 PPhysicsComponent::ContainsAnyBody(const FPhysicsBodyId& InBodyID1, const FPhysicsBodyId& InBodyID2) const
	{
		return ContainsBody(InBodyID1) || ContainsBody(InBodyID2);
	}


}
