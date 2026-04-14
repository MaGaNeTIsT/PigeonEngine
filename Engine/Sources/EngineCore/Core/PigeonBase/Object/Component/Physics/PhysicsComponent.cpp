#include "PhysicsComponent.h"
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/World/World.h>
#include <PigeonBase/Object/World/WorldTickManager.h>
#include <PhysicsConfig/PhysicsConfig.h>
#if _EDITOR_ONLY
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
		if (m_Shape)
			RemoveShape();
	}

	void PPhysicsComponent::Init()
	{
		PActorComponent::Init();
		FPhysicsManager::GetSingleton()->AddPhysicsListener(static_cast<FBodyActivationEventListenerInterface*>(this));
		FPhysicsManager::GetSingleton()->AddPhysicsListener(static_cast<FContactEventListenerInterface*>(this));
		InitPhysicsComponent();
	}

	void PPhysicsComponent::BeginAddedToScene(PWorld* World)
	{
		PActorComponent::BeginAddedToScene(World);
		TryRegisterPostPhysicsTick();
	}

	void PPhysicsComponent::RemovedFromScene()
	{
		TryUnregisterPostPhysicsTick();
		PActorComponent::RemovedFromScene();
	}

	void PPhysicsComponent::TryRegisterPostPhysicsTick()
	{
		if (GetWorld() && !bPostPhysicsTickRegistered)
		{
			PWorld* World = GetWorld();
			const ObjectIdentityType ComponentID = GetUniqueID();
			PostPhysicsTickHandler = TFunction<void(FLOAT)>([World, ComponentID](FLOAT deltaTime)
			{
				if (!World)
				{
					return;
				}

				const PActorComponent* FoundComponent = World->GetComponentByUniqueID(ComponentID, TRUE);
				PPhysicsComponent* PhysicsComponent = dynamic_cast<PPhysicsComponent*>(const_cast<PActorComponent*>(FoundComponent));
				if (PhysicsComponent)
				{
					PhysicsComponent->HandlePostPhysicsTick(deltaTime);
				}
			});
            EWorldTickManager::GetManagerSingleton()->RegisterPostPhysicsTick(PostPhysicsTickHandler);
			bPostPhysicsTickRegistered = TRUE;
		}
	}

	void PPhysicsComponent::TryUnregisterPostPhysicsTick()
	{
		if (bPostPhysicsTickRegistered)
		{
			EWorldTickManager::GetManagerSingleton()->UnregisterPostPhysicsTick(PostPhysicsTickHandler);
			bPostPhysicsTickRegistered = FALSE;
		}
	}

	void PPhysicsComponent::Uninit()
	{
		FPhysicsManager::GetSingleton()->RemovePhysicsListener(static_cast<FBodyActivationEventListenerInterface*>(this));
		FPhysicsManager::GetSingleton()->RemovePhysicsListener(static_cast<FContactEventListenerInterface*>(this));
		PActorComponent::Uninit();
		UninitPhysicsComponent();
	}

	void PPhysicsComponent::Destroy()
	{
		if (m_Shape)
			RemoveShape();
		RemoveShape();
		PActorComponent::Destroy();
	}

	void PPhysicsComponent::HandlePostPhysicsTick(FLOAT deltaTime)
	{
		(void)deltaTime;
		if (m_BodyId.IsInvalid() || !GetOwnerActor())
		{
			return;
		}

		if (MotionType != PhysicsUtility::EMotionType::Static)
		{
			GetOwnerActor()->SetActorLocation(FPhysicsManager::GetSingleton()->GetPosition(m_BodyId));
			GetOwnerActor()->SetActorRotation(FPhysicsManager::GetSingleton()->GetRotation(m_BodyId));
		}
	}
#if _EDITOR_ONLY
	void PPhysicsComponent::EditorTick(FLOAT deltaTime)
	{
		if(FPhysicsCommonSettings::GetSingleton()->PHYSICS_DRAW_DEBUG_PRIMITIVE)
			DrawPrimitive();
	}
	void PPhysicsComponent::DrawPrimitive()
	{
		RDebugWireframePrimitiveManager* Manager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		const Vector3 Pos = GetOwnerActor()->GetActorLocation();
		const Quaternion Rot = GetOwnerActor()->GetActorRotation();
		if (m_Shape)
		{
			m_Shape->DrawPrimitive(Manager, Pos, Rot);
		}
	}
#endif
	void PPhysicsComponent::SetShape(FShape* InShape)
	{
		if (m_Shape)
		{
			UninitPhysicsComponent();
			m_Shape = InShape;
			InitPhysicsComponent();
		}
		else
			m_Shape = InShape;
	}

	void PPhysicsComponent::InitPhysicsComponent()
	{
		Vector3 pos = GetOwnerActor()->GetRootComponent()->GetComponentWorldLocation();
		Quaternion rot = GetOwnerActor()->GetRootComponent()->GetComponentWorldRotation();
		if (FPhysicsManager::GetSingleton()->TryCreateBody(m_Shape, FALSE, pos, rot, MotionType, Layer, GetOwnerActor()->GetUniqueID(), m_BodyId))
		{
			FPhysicsManager::GetSingleton()->AddBody(GetOwnerActor()->GetUniqueID(), m_BodyId);
		}
	}

	void PPhysicsComponent::UninitPhysicsComponent()
	{
		RemoveShape();
	}

	FShape* PPhysicsComponent::GetShape()
	{
		return m_Shape;
	}

	void PPhysicsComponent::RemoveShape()
	{
		FPhysicsManager::GetSingleton()->RemoveBody(GetOwnerActor()->GetUniqueID(), TRUE);
		m_BodyId = FPhysicsBodyId();
		m_Shape = nullptr;
	}

	void PPhysicsComponent::AddForce(Vector3 InForce)
	{
		FPhysicsManager::GetSingleton()->AddForce(m_BodyId, InForce);
	}

	void PPhysicsComponent::AddImpluse(Vector3 InImpluse)
	{
		FPhysicsManager::GetSingleton()->AddImpulse(m_BodyId, InImpluse);
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
		return m_BodyId;
	}

	void PPhysicsComponent::OnBodyActivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID)
	{
		if (ContainsBody(inBodyID))
		{
			OnPhysicsBodyActivated(inBodyID, inObjectID);
		}
	}

	void PPhysicsComponent::OnBodyDeactivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID)
	{
		if (ContainsBody(inBodyID))
		{
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

	void PPhysicsComponent::OnContactAdded(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings)
	{
		if (ContainsAnyBody(inBodyID1, inBodyID2))
		{
			OnPhysicsContactAdded(inBodyID1, inBodyID2, inManifold, ioSettings);
		}
	}

	void PPhysicsComponent::OnContactPersisted(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings)
	{
		if (ContainsAnyBody(inBodyID1, inBodyID2))
		{
			OnPhysicsContactPersisted(inBodyID1, inBodyID2, inManifold, ioSettings);
		}
	}

	void PPhysicsComponent::OnContactRemoved(const FPhysicsSubShapePair& inSubShapePair)
	{
		if (ContainsAnyBody(inSubShapePair.BodyId1, inSubShapePair.BodyId2))
		{
			OnPhysicsContactRemoved(inSubShapePair);
		}
	}

	BOOL32 PPhysicsComponent::ContainsBody(const FPhysicsBodyId& InBodyID) const
	{
		return m_BodyId == InBodyID;
	}

	BOOL32 PPhysicsComponent::ContainsAnyBody(const FPhysicsBodyId& InBodyID1, const FPhysicsBodyId& InBodyID2) const
	{
		return ContainsBody(InBodyID1) || ContainsBody(InBodyID2);
	}


}