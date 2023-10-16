#include "../Headers/JoltPhysics.h"
#include "../Headers/Character/Character.h"
#include "../../../Sources/EngineCore/PhysicsCore/PhysicsConfig/PhysicsConfig.h"

namespace PigeonEngine
{
	void FPhysics_Jolt::InitPhysics()
	{
		// Register allocation hook
		RegisterDefaultAllocator();

		Trace = TraceImpl;

		// Create a factory
		Factory::sInstance = new Factory();

		// Register all Jolt physics types
		RegisterTypes();

		//create PhysicsData
		PhysicsData = new FPhysicsData();

		FCommonSettings* CommonSettings = FCommonSettings::GetSingleton();

		//pre-allocated memory for simulation.
		PhysicsData->TempAllocator = new TempAllocatorImpl(CommonSettings->PHYSICS_PRE_ALLOCATED_SIZE);

		//an example implementation for jobsystem
		//used for multiple threads
		PhysicsData->JobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, EMath::Clamp(EMath::Max(thread::hardware_concurrency(), 1u) - 1u, 1u, 4u));

		// Create mapping table from object layer to broadphase layer
		PhysicsData->BPLayerInterface = new CBPLayerInterfaceImpl();

		PhysicsData->ObjectLayerPairFilterImpl = new CObjectLayerPairFilterImpl();
		PhysicsData->ObjectVsBroadPhaseLayerFilterImpl = new CObjectVsBroadPhaseLayerFilterImpl();

		PhysicsData->PhysicsSystem = new PhysicsSystem();
		PhysicsData->PhysicsSystem->Init(CommonSettings->PHYSICS_MAX_BODIES, CommonSettings->PHYSICS_NUM_BODY_MUTEXES, CommonSettings->PHYSICS_MAX_BODY_PAIRS, CommonSettings->PHYSICS_MAX_CONTACT_CONSTRAINTS, *PhysicsData->BPLayerInterface, *PhysicsData->ObjectVsBroadPhaseLayerFilterImpl, *PhysicsData->ObjectLayerPairFilterImpl);

		PhysicsData->BodyActivationListener = new FBodyActivationListener();
		PhysicsData->PhysicsSystem->SetBodyActivationListener(PhysicsData->BodyActivationListener);

		PhysicsData->ContactListener = new FContactListener();
		PhysicsData->PhysicsSystem->SetContactListener(PhysicsData->ContactListener);

		PhysicsData->BodyInterface = &PhysicsData->PhysicsSystem->GetBodyInterface();

		PhysicsData->PhysicsSystem->SetGravity(JPH::Vec3(0, -9.81f, 0));
	}

	void FPhysics_Jolt::PhysicsUpdate(FLOAT InDeltaTime)
	{
		FCommonSettings* CommonSettings = FCommonSettings::GetSingleton();
		PhysicsData->PhysicsSystem->Update(InDeltaTime, CommonSettings->PHYSICS_COLLISION_STEPS, PhysicsData->TempAllocator, PhysicsData->JobSystem);
		for (auto Character = m_Characters.Begin(); Character != m_Characters.End(); ++Character)
		{
			//TODO:unsafe;
			(*Character)->PostSimulation();
		}
	}

	void FPhysics_Jolt::UninitPhysics()
	{
		for (auto body = m_Bodys.Begin(); body != m_Bodys.End(); ++body)
		{
			PhysicsData->BodyInterface->RemoveBody(body->second.ID);
			PhysicsData->BodyInterface->DestroyBody(body->second.ID);
		}

		for (auto shape = m_Shapes.Begin(); shape != m_Shapes.End(); ++shape)
		{
			delete shape->second;
		}

		PhysicsData->BodyInterface = nullptr;
		delete PhysicsData->PhysicsSystem;
		PhysicsData->PhysicsSystem = nullptr;
		delete PhysicsData->TempAllocator;
		PhysicsData->TempAllocator = nullptr;
		delete PhysicsData->JobSystem;
		PhysicsData->JobSystem = nullptr;
		delete PhysicsData->BPLayerInterface;
		PhysicsData->BPLayerInterface = nullptr;
		delete PhysicsData->BodyActivationListener;
		PhysicsData->BodyActivationListener = nullptr;
		delete PhysicsData->ContactListener;
		PhysicsData->ContactListener = nullptr;
		delete PhysicsData->ObjectLayerPairFilterImpl;
		PhysicsData->ObjectLayerPairFilterImpl = nullptr;
		delete PhysicsData->ObjectVsBroadPhaseLayerFilterImpl;
		PhysicsData->ObjectVsBroadPhaseLayerFilterImpl = nullptr;

		// Destroy the factory
		delete Factory::sInstance;
		Factory::sInstance = nullptr;
		delete PhysicsData;
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
		m_Bodys.Add(GameObjectId, inBodyID);
	}

	void FPhysics_Jolt::RemoveBody(const ObjectIdentityType& GameObjectId, BOOL32 bDeleteShape/* = TRUE*/)
	{
		FPhysicsBodyId ID;
		if (m_Bodys.FindValue(GameObjectId, ID))
		{
			PhysicsData->BodyInterface->RemoveBody(ID.ID);
			m_Bodys.Remove(GameObjectId);
			FShape* Shape;
			if (m_Shapes.FindValue(ID, Shape))
			{
				m_Shapes.Remove(ID);
				if(bDeleteShape)
					delete Shape;
			}
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