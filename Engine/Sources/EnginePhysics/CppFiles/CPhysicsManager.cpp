#include "../Headers/CPhysicsManager.h"

CPhysicsManager* CPhysicsManager::m_PhysicsManager = new CPhysicsManager();
void CPhysicsManager::Init()
{
	using namespace PhysicsManager;

	Trace = TraceImpl;

	// Create a factory
	Factory::sInstance = new Factory();
	m_PhysicsSystem = new PhysicsSystem();

	// Register all Jolt physics types
	RegisterTypes();

	//pre-allocated memory for simulation.
	m_TempAllocator = new TempAllocatorImpl(70 * 1024 * 1024);

	//an example implementation for jobsystem
	//used for multiple threads
	m_JobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

	const uint cMaxBodies = 65535;	//Max amount of rigid bodies.
	const uint cNumBodyMutexes = 0;		//Mutexes count,0 to default setting.
	const uint cMaxBodyPairs = 65535;	//Max amount of body pairs that can be queued at any time.
	const uint cMaxContactConstraints = 65535;	//This is the maximum size of the contact constraint buffer.

	// Create mapping table from object layer to broadphase layer
	m_BPLayerInterface = new CBPLayerInterfaceImpl();

	m_PhysicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *m_BPLayerInterface, BroadPhaseCanCollide, ObjectCanCollide);

	m_BodyActivationListener = new CBodyActivationListener();
	m_PhysicsSystem->SetBodyActivationListener(m_BodyActivationListener);

	m_ContactListener = new CContactListener();
	m_PhysicsSystem->SetContactListener(m_ContactListener);

	m_BodyInterface = &m_PhysicsSystem->GetBodyInterface();
}

void CPhysicsManager::Tick(const float cDeltaTime)
{
	// Do n collision step per cDeltaTime
	const int cCollisionSteps = 1;
	// If you want more accurate step results you can do multiple sub steps within a collision step. Usually you would set this to 1.
	const int cIntegrationSubSteps = 1;

	m_PhysicsSystem->Update(cDeltaTime, cCollisionSteps, cIntegrationSubSteps, m_TempAllocator, m_JobSystem);
}

Body* CPhysicsManager::CreateBody(const ULONGLONG& GameObjectId, const ShapeSettings* inShape, const Vec3Arg& inPosition, const QuatArg& inRotation, EMotionType inMotionType, const ObjectLayer& inObjectLayer, EActivation inActivation)
{
	BodyCreationSettings bodyCreateSettings(inShape, inPosition, inRotation, inMotionType, inObjectLayer);
	return CreateBody(GameObjectId, bodyCreateSettings, inActivation);
}

Body* CPhysicsManager::CreateBody(const ULONGLONG& GameObjectId, const Shape* inShape, const Vec3Arg& inPosition, const QuatArg& inRotation, EMotionType inMotionType, const ObjectLayer& inObjectLayer, EActivation inActivation)
{
	BodyCreationSettings bodyCreateSettings(inShape, inPosition, inRotation, inMotionType, inObjectLayer);
	return CreateBody(GameObjectId, bodyCreateSettings, inActivation);
}

Body* CPhysicsManager::CreateBoxBody(const ULONGLONG& GameObjectId, const Vec3Arg& inHalfExtent, const Vec3Arg& inPosition, const QuatArg& inRotation, EMotionType inMotionType, const ObjectLayer& inObjectLayer, EActivation inActivation)
{
	BoxShape* boxShape = new BoxShape(inHalfExtent);
	return CreateBody(GameObjectId, boxShape, inPosition, inRotation, inMotionType, inObjectLayer, inActivation);
}

Body* CPhysicsManager::CreateHeightFieldBody(const ULONGLONG& GameObjectId, const float* inSamples, const Vec3Arg& inOffset, const Vec3Arg& inScale, uint32 inSampleCount, const Vec3Arg& inPosition, const QuatArg& inRotation, EMotionType inMotionType, const ObjectLayer& inObjectLayer, EActivation inActivation)
{
	HeightFieldShapeSettings heightFieldShapeSetting(inSamples, inOffset, inScale, inSampleCount);
	ShapeSettings::ShapeResult heightFieldShapeResult = heightFieldShapeSetting.Create();
	ShapeRefC heightFieldShape = heightFieldShapeResult.Get();
	BodyCreationSettings heightFieldSetting(heightFieldShape, inPosition, inRotation, inMotionType, inObjectLayer);
	return CreateBody(GameObjectId, heightFieldSetting, inActivation);
}

Body* CPhysicsManager::GetBody(const ULONGLONG& GameObjectId)
{
	return m_Bodys.find(GameObjectId)->second;
}

CPhysicsManager::~CPhysicsManager()
{
	for (const auto& body : m_Bodys)
	{
		m_BodyInterface->RemoveBody(body.second->GetID());
		m_BodyInterface->DestroyBody(body.second->GetID());
	}

	m_BodyInterface = nullptr;
	delete m_PhysicsSystem;
	m_PhysicsSystem = nullptr;
	delete m_TempAllocator;
	m_TempAllocator = nullptr;
	delete m_JobSystem;
	m_JobSystem = nullptr;
	delete m_BPLayerInterface;
	m_BPLayerInterface = nullptr;
	delete m_BodyActivationListener;
	m_BodyActivationListener = nullptr;
	delete m_ContactListener;
	m_ContactListener = nullptr;

	// Destroy the factory
	delete Factory::sInstance;
	Factory::sInstance = nullptr;

}