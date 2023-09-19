#include "../Headers/JoltPhysics.h"
#include "../Headers/Character/Character.h"

namespace PigeonEngine
{
	void FPhysics_Jolt::InitPhysics()
	{
		//create PhysicsData
		PhysicsData = new FPhysicsData();

		Trace = TraceImpl;

		// Create a factory
		Factory::sInstance = new Factory();
		PhysicsData->PhysicsSystem = new PhysicsSystem();

		// Register all Jolt physics types
		RegisterTypes();

		//pre-allocated memory for simulation.
		PhysicsData->TempAllocator = new TempAllocatorImpl(PhysicsData->PreAllocatedSize);

		//an example implementation for jobsystem
		//used for multiple threads
		PhysicsData->JobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

		// Create mapping table from object layer to broadphase layer
		PhysicsData->BPLayerInterface = new CBPLayerInterfaceImpl();

		PhysicsData->ObjectLayerPairFilterImpl = new CObjectLayerPairFilterImpl();
		PhysicsData->ObjectVsBroadPhaseLayerFilterImpl = new CObjectVsBroadPhaseLayerFilterImpl();
		PhysicsData->PhysicsSystem->Init(PhysicsData->MaxBodies, PhysicsData->NumBodyMutexes, PhysicsData->MaxBodyPairs, PhysicsData->MaxContactConstraints, *PhysicsData->BPLayerInterface, *PhysicsData->ObjectVsBroadPhaseLayerFilterImpl, *PhysicsData->ObjectLayerPairFilterImpl);

		PhysicsData->BodyActivationListener = new FBodyActivationListener();
		PhysicsData->PhysicsSystem->SetBodyActivationListener(PhysicsData->BodyActivationListener);

		PhysicsData->ContactListener = new FContactListener();
		PhysicsData->PhysicsSystem->SetContactListener(PhysicsData->ContactListener);

		PhysicsData->BodyInterface = &PhysicsData->PhysicsSystem->GetBodyInterface();

		PhysicsData->PhysicsSystem->SetGravity(JPH::Vec3(0, -9.81f, 0));
	}

	void FPhysics_Jolt::PhysicsUpdate(FLOAT InDeltaTime)
	{
		PhysicsData->PhysicsSystem->Update(InDeltaTime, PhysicsData->CollisionSteps, PhysicsData->TempAllocator, PhysicsData->JobSystem);
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

	BOOL32 FPhysics_Jolt::TryCreateBody(FShape* inShape, BOOL32 CreateNew, Vector3 inPosition, Quaternion inRotation, PhysicsUtility::EMotionType inMotionType, UINT16 inLayer, PhysicsBodyId& outBodyID)
	{
		Body* body = PhysicsData->BodyInterface->CreateBody(BodyCreationSettings(inShape->CreateShapeSettings(CreateNew), PhysicsUtility::Convert2Meter(inPosition), PhysicsUtility::Convert(inRotation), GetMotionType(inMotionType), inLayer));
		if (body)
		{
			outBodyID.ID = body->GetID();
			m_Shapes.Add(outBodyID, inShape);
			return true;
		}
		return false;
	}

	void FPhysics_Jolt::AddBody(const ObjectIdentityType& GameObjectId, const PhysicsBodyId& inBodyID, EActivate inActivationMode)
	{
		PhysicsData->BodyInterface->AddBody(inBodyID.ID, inActivationMode == EActivate::Activate? EActivation::Activate : EActivation::DontActivate);
		m_Bodys.Add(GameObjectId, inBodyID);
	}

	void FPhysics_Jolt::RemoveBody(const ObjectIdentityType& GameObjectId, BOOL32 bDeleteShape/* = true*/)
	{
		PhysicsBodyId ID;
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

	Vector3 FPhysics_Jolt::GetPosition(const PhysicsBodyId& PhysicsBodyId)
	{
		return PhysicsUtility::Convert2Centimeter(PhysicsData->BodyInterface->GetPosition(PhysicsBodyId.ID));
	}
	Quaternion FPhysics_Jolt::GetRotation(const PhysicsBodyId& PhysicsBodyId)
	{
		return PhysicsUtility::Convert(PhysicsData->BodyInterface->GetRotation(PhysicsBodyId.ID));
	}

	void FPhysics_Jolt::SetPosition(const PhysicsBodyId& inPhysicsBodyId, Vector3 inPosition, EActivate inActivationMode)
	{
		PhysicsData->BodyInterface->SetPosition(inPhysicsBodyId.ID, PhysicsUtility::Convert2Meter(inPosition), inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate);
	}

	void FPhysics_Jolt::SetRoation(const PhysicsBodyId& inPhysicsBodyId, Quaternion inRotation, EActivate inActivationMode)
	{
		PhysicsData->BodyInterface->SetRotation(inPhysicsBodyId.ID, PhysicsUtility::Convert(inRotation), inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate);
	}

	void FPhysics_Jolt::AddForce(const PhysicsBodyId& inPhysicsBodyId, Vector3 inForce)
	{
		PhysicsData->BodyInterface->AddForce(inPhysicsBodyId.ID, PhysicsUtility::Convert(inForce));
	}
	void FPhysics_Jolt::AddForce(const PhysicsBodyId& inPhysicsBodyId, Vector3 inForce, Vector3 inPoint)
	{
		PhysicsData->BodyInterface->AddForce(inPhysicsBodyId.ID, PhysicsUtility::Convert(inForce), PhysicsUtility::Convert2Meter(inPoint));
	}

	void FPhysics_Jolt::AddImpulse(const PhysicsBodyId& inPhysicsBodyId, Vector3 inImpulse)
	{
		PhysicsData->BodyInterface->AddImpulse(inPhysicsBodyId.ID, PhysicsUtility::Convert(inImpulse));
	}
	void FPhysics_Jolt::AddImpulse(const PhysicsBodyId& inPhysicsBodyId, Vector3 inImpulse, Vector3 inPoint)
	{
		PhysicsData->BodyInterface->AddImpulse(inPhysicsBodyId.ID, PhysicsUtility::Convert(inImpulse), PhysicsUtility::Convert2Meter(inPoint));
	}

	void FPhysics_Jolt::SetGravity(Vector3 inGravity)
	{
		PhysicsData->PhysicsSystem->SetGravity(PhysicsUtility::Convert2Meter(inGravity));
	}
}