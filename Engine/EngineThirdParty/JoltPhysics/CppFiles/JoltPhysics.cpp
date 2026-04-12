#include "../Headers/JoltPhysics.h"
#include "../Headers/Character/Character.h"

namespace PigeonEngine
{
	void FPhysics_Jolt::InitPhysics()
	{
		// Register allocation hook
		RegisterDefaultAllocator();

		Trace = TraceImpl;

		// Create a factory
		Factory::sInstance = New<Factory>();

		// Register all Jolt physics types
		RegisterTypes();

		//create PhysicsData
		PhysicsData = New<FPhysicsData>();

		FPhysicsCommonSettings* CommonSettings = FPhysicsCommonSettings::GetSingleton();

		//pre-allocated memory for simulation.
		PhysicsData->TempAllocator = New<TempAllocatorImpl>(CommonSettings->PHYSICS_PRE_ALLOCATED_SIZE);

		//an example implementation for jobsystem
		//used for multiple threads
		PhysicsData->JobSystem = New<JobSystemThreadPool>(cMaxPhysicsJobs, cMaxPhysicsBarriers, EMath::Clamp(EMath::Max(thread::hardware_concurrency(), 1u) - 1u, 1u, 4u));
		// Create mapping table from object layer to broadphase layer
		PhysicsData->BPLayerInterface = New<CBPLayerInterfaceImpl>();

		PhysicsData->ObjectLayerPairFilterImpl = New<CObjectLayerPairFilterImpl>();
		PhysicsData->ObjectVsBroadPhaseLayerFilterImpl = New<CObjectVsBroadPhaseLayerFilterImpl>();

		PhysicsData->PhysicsSystem = New<PhysicsSystem>();
		PhysicsData->PhysicsSystem->Init(CommonSettings->PHYSICS_MAX_BODIES, CommonSettings->PHYSICS_NUM_BODY_MUTEXES, CommonSettings->PHYSICS_MAX_BODY_PAIRS, CommonSettings->PHYSICS_MAX_CONTACT_CONSTRAINTS, *PhysicsData->BPLayerInterface, *PhysicsData->ObjectVsBroadPhaseLayerFilterImpl, *PhysicsData->ObjectLayerPairFilterImpl);

		PhysicsData->BodyActivationListener = New<FBodyActivationListener>();
		PhysicsData->PhysicsSystem->SetBodyActivationListener(PhysicsData->BodyActivationListener);

		PhysicsData->ContactListener = New<FContactListener>();
		PhysicsData->PhysicsSystem->SetContactListener(PhysicsData->ContactListener);

		PhysicsData->BodyInterface = &PhysicsData->PhysicsSystem->GetBodyInterface();

		PhysicsData->PhysicsSystem->SetGravity(JPH::Vec3(0, -9.81f, 0));
	}

	void FPhysics_Jolt::PhysicsUpdate(FLOAT InDeltaTime)
	{
		FPhysicsCommonSettings* CommonSettings = FPhysicsCommonSettings::GetSingleton();
		PhysicsData->PhysicsSystem->Update(InDeltaTime, CommonSettings->PHYSICS_COLLISION_STEPS, PhysicsData->TempAllocator, PhysicsData->JobSystem);
		for (const auto& Character : m_Characters)
		{
			PE_CHECK(ENGINE_THIRD_PARTY_ERROR, ("Has a Character been destory but not removed!"), !!Character);
			Character->PostSimulation();
		}
	}

	void FPhysics_Jolt::UninitPhysics()
	{
		for (const auto& bodyGroup : m_Bodys)
		{
			for (const auto& bodyId : bodyGroup.second)
			{
				PhysicsData->BodyInterface->RemoveBody(bodyId.ID);
				PhysicsData->BodyInterface->DestroyBody(bodyId.ID);
			}
		}

		for (const auto& shape : m_Shapes)
		{
			Delete(shape.second);
		}

		PhysicsData->BodyInterface = nullptr;
		Delete(PhysicsData->PhysicsSystem);
		PhysicsData->PhysicsSystem = nullptr;
		Delete(PhysicsData->TempAllocator);
		PhysicsData->TempAllocator = nullptr;
		Delete(PhysicsData->JobSystem);
		PhysicsData->JobSystem = nullptr;
		Delete(PhysicsData->BPLayerInterface);
		PhysicsData->BPLayerInterface = nullptr;
		Delete(PhysicsData->BodyActivationListener);
		PhysicsData->BodyActivationListener = nullptr;
		Delete(PhysicsData->ContactListener);
		PhysicsData->ContactListener = nullptr;
		Delete(PhysicsData->ObjectLayerPairFilterImpl);
		PhysicsData->ObjectLayerPairFilterImpl = nullptr;
		Delete(PhysicsData->ObjectVsBroadPhaseLayerFilterImpl);
		PhysicsData->ObjectVsBroadPhaseLayerFilterImpl = nullptr;

		// Destroy the factory
		Delete(Factory::sInstance);
		Factory::sInstance = nullptr;
		m_Bodys.Empty();
		m_BodyToObjectIds.Empty();
		m_Shapes.Empty();
		Delete(PhysicsData);
		PhysicsData = nullptr;
	}

	void FPhysics_Jolt::PrePhysicsUpdate()
	{
		//for (auto body = m_Bodys.Begin(); body != m_Bodys.End(); ++body)
		//{
		//	EGameObject* gameObject = GetScene()->GetGameObjectById(body->first);

		//	SetPosition(obj.second, PhysicsUtility::Convert2Meter(gameObject->GetWorldPosition()));
		//	SetRoation(obj.second, PhysicsUtility::Convert(gameObject->GetWorldRotation()));
		//}
	}

	void FPhysics_Jolt::PostPhysicsUpdate()
	{
		//for (const auto& obj : m_Bodys)
		//{
		//	CGameObject* gameObject = CManager::GetScene()->GetGameObjectById(obj.first);
		//	gameObject->SetWorldPosition(PhysicsUtility::Convert2Centimeter(GetPosition(obj.second)));
		//	gameObject->SetWorldRotation(PhysicsUtility::Convert(GetRotation(obj.second)));
		//}
	}

	void FPhysics_Jolt::AddCharacter(FCharacter* Character)
	{
		m_Characters.Add(Character);
	}

	void FPhysics_Jolt::RemoveCharacter(FCharacter* Character)
	{
		m_Characters.Remove(Character);
	}

	BOOL32 FPhysics_Jolt::TryCreateBody(FShape* inShape, BOOL32 CreateNew, Vector3 inPosition, Quaternion inRotation, PhysicsUtility::EMotionType inMotionType, UINT16 inLayer, FPhysicsBodyId& outBodyID)
	{
		Body* body = PhysicsData->BodyInterface->CreateBody(BodyCreationSettings(inShape->CreateShapeSettings(CreateNew), PhysicsUtility::Convert2Meter(inPosition), PhysicsUtility::Convert(inRotation), GetMotionType(inMotionType), inLayer));
		if (body)
		{
			outBodyID.ID = body->GetID();
			m_Shapes.Add(outBodyID, inShape);
			return TRUE;
		}
		return FALSE;
	}

	void FPhysics_Jolt::AddBody(const ObjectIdentityType& GameObjectId, const FPhysicsBodyId& inBodyID, EActivate inActivationMode)
	{
		PhysicsData->BodyInterface->AddBody(inBodyID.ID, inActivationMode == EActivate::Activate? EActivation::Activate : EActivation::DontActivate);

		TArray<FPhysicsBodyId>* BodyIds = m_Bodys.FindValueAsPtr(GameObjectId);
		if (!BodyIds)
		{
			TArray<FPhysicsBodyId> NewBodyIds;
			NewBodyIds.Add(inBodyID);
			m_Bodys.Add(GameObjectId, NewBodyIds);
		}
		else if (!BodyIds->Contains(inBodyID))
		{
			BodyIds->Add(inBodyID);
		}

		m_BodyToObjectIds.Add(inBodyID, GameObjectId);
	}

	BOOL32 FPhysics_Jolt::FindObjectIdentityByBodyId(const FPhysicsBodyId& inBodyID, ObjectIdentityType& outGameObjectId) const
	{
		return m_BodyToObjectIds.FindValue(inBodyID, outGameObjectId);
	}

	void FPhysics_Jolt::RemoveBody(const ObjectIdentityType& GameObjectId, BOOL32 bDeleteShape/* = TRUE*/)
	{
		TArray<FPhysicsBodyId> BodyIds;
		if (m_Bodys.FindValue(GameObjectId, BodyIds))
		{
			for (const auto& ID : BodyIds)
			{
				PhysicsData->BodyInterface->RemoveBody(ID.ID);
				m_BodyToObjectIds.Remove(ID);
				FShape* Shape;
				if (m_Shapes.FindValue(ID, Shape))
				{
					m_Shapes.Remove(ID);
					if (bDeleteShape)
						Delete(Shape);
				}
			}

			m_Bodys.Remove(GameObjectId);
		}
	}

	Vector3 FPhysics_Jolt::GetPosition(const FPhysicsBodyId& FPhysicsBodyId)
	{
		return PhysicsUtility::Convert2Centimeter(PhysicsData->BodyInterface->GetPosition(FPhysicsBodyId.ID));
	}
	Quaternion FPhysics_Jolt::GetRotation(const FPhysicsBodyId& FPhysicsBodyId)
	{
		return PhysicsUtility::Convert(PhysicsData->BodyInterface->GetRotation(FPhysicsBodyId.ID));
	}

	void FPhysics_Jolt::SetPosition(const FPhysicsBodyId& inPhysicsBodyId, Vector3 inPosition, EActivate inActivationMode)
	{
		PhysicsData->BodyInterface->SetPosition(inPhysicsBodyId.ID, PhysicsUtility::Convert2Meter(inPosition), inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate);
	}

	void FPhysics_Jolt::SetRoation(const FPhysicsBodyId& inPhysicsBodyId, Quaternion inRotation, EActivate inActivationMode)
	{
		PhysicsData->BodyInterface->SetRotation(inPhysicsBodyId.ID, PhysicsUtility::Convert(inRotation), inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate);
	}

	void FPhysics_Jolt::AddForce(const FPhysicsBodyId& inPhysicsBodyId, Vector3 inForce)
	{
		PhysicsData->BodyInterface->AddForce(inPhysicsBodyId.ID, PhysicsUtility::Convert(inForce));
	}
	void FPhysics_Jolt::AddForce(const FPhysicsBodyId& inPhysicsBodyId, Vector3 inForce, Vector3 inPoint)
	{
		PhysicsData->BodyInterface->AddForce(inPhysicsBodyId.ID, PhysicsUtility::Convert(inForce), PhysicsUtility::Convert2Meter(inPoint));
	}

	void FPhysics_Jolt::AddImpulse(const FPhysicsBodyId& inPhysicsBodyId, Vector3 inImpulse)
	{
		PhysicsData->BodyInterface->AddImpulse(inPhysicsBodyId.ID, PhysicsUtility::Convert(inImpulse));
	}
	void FPhysics_Jolt::AddImpulse(const FPhysicsBodyId& inPhysicsBodyId, Vector3 inImpulse, Vector3 inPoint)
	{
		PhysicsData->BodyInterface->AddImpulse(inPhysicsBodyId.ID, PhysicsUtility::Convert(inImpulse), PhysicsUtility::Convert2Meter(inPoint));
	}

	void FPhysics_Jolt::SetGravity(Vector3 inGravity)
	{
		PhysicsData->PhysicsSystem->SetGravity(PhysicsUtility::Convert2Meter(inGravity));
	}
}