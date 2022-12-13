#include "../Headers/CPhysics_API_Jolt.h"


CPhysics_API_Jolt::CPhysics_API_Jolt():
	m_BodyInterface(nullptr),
	m_PhysicsSystem(nullptr),
	m_TempAllocator(nullptr),
	m_JobSystem(nullptr),
	m_BPLayerInterface(nullptr),
	m_BodyActivationListener(nullptr),
	m_ContactListener(nullptr)
{
	
}

CPhysics_API_Jolt::~CPhysics_API_Jolt()
{
	for (const auto& body : m_Bodys)
	{
		m_BodyInterface->RemoveBody(body.second);
		m_BodyInterface->DestroyBody(body.second);
	}

	for (const auto& bodyCreateSettings: m_BodyCreationSettings)
	{
		if(bodyCreateSettings.second->GetShapeSettings())
			delete bodyCreateSettings.second->GetShapeSettings();
		if (bodyCreateSettings.second->GetShape())
			delete bodyCreateSettings.second->GetShape();
		delete bodyCreateSettings.second;
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

void CPhysics_API_Jolt::Init()
{
	using namespace PhysicsAPI_Jolt;

	Trace = TraceImpl;

	// Create a factory
	Factory::sInstance = new Factory();
	m_PhysicsSystem = new PhysicsSystem();

	// Register all Jolt physics types
	RegisterTypes();

	//pre-allocated memory for simulation.
	m_TempAllocator = new TempAllocatorImpl(cPreAllocatedSize);

	//an example implementation for jobsystem
	//used for multiple threads
	m_JobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

	// Create mapping table from object layer to broadphase layer
	m_BPLayerInterface = new CBPLayerInterfaceImpl();

	m_PhysicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *m_BPLayerInterface, BroadPhaseCanCollide, ObjectCanCollide);

	m_BodyActivationListener = new CBodyActivationListener();
	m_PhysicsSystem->SetBodyActivationListener(m_BodyActivationListener);

	m_ContactListener = new CContactListener();
	m_PhysicsSystem->SetContactListener(m_ContactListener);

	m_BodyInterface = &m_PhysicsSystem->GetBodyInterface();
}

void CPhysics_API_Jolt::Tick(const float cDeltaTime)
{
	m_PhysicsSystem->Update(cDeltaTime, cCollisionSteps, cIntegrationSubSteps, m_TempAllocator, m_JobSystem);
}

BodyCreationSettings* CPhysics_API_Jolt::CreateBodyCreationSettings()
{
	auto bodyCreationSettings = new JPH::BodyCreationSettings();
	return bodyCreationSettings;
}
BodyCreationSettings* CPhysics_API_Jolt::CreateBodyCreationSettings(ShapeSettings* settings,
	const Vec3& position,
	const Quat& rotation,
	EMotionType motionType,
	ObjectLayer objectLayer)
{
	auto bodyCreationSettings = new JPH::BodyCreationSettings(
		settings,
		position,
		rotation,
		motionType,
		objectLayer
	);
	return bodyCreationSettings;
}
BodyCreationSettings* CPhysics_API_Jolt::CreateBodyCreationSettings(Shape* shape,
	const Vec3& position,
	const Quat& rotation,
	EMotionType motionType,
	ObjectLayer objectLayer)
{
	auto bodyCreationSettings = new JPH::BodyCreationSettings(
		shape,
		position,
		rotation,
		motionType,
		objectLayer
	);
	return bodyCreationSettings;
}

BoxShape* CPhysics_API_Jolt::CreateBoxShape(const Vec3& inHalfExtent, float ConvexRadius)
{
	return new BoxShape(inHalfExtent, ConvexRadius);
}
BoxShapeSettings* CPhysics_API_Jolt::CreateBoxShapeSettings(const Vec3& inHalfExtent, float ConvexRadius)
{
	return new BoxShapeSettings(inHalfExtent, ConvexRadius);
}
SphereShape* CPhysics_API_Jolt::CreateSphereShape(float Radius)
{
	return new SphereShape(Radius);
}
SphereShapeSettings* CPhysics_API_Jolt::CreateSphereShapeSettings(float Radius)
{
	return new SphereShapeSettings(Radius);
}
TriangleShapeSettings* CPhysics_API_Jolt::CreateTriangleShapeSettings(const Vec3& inV1, const Vec3& inV2, const Vec3& inV3, float ConvexRadius)
{
	return new TriangleShapeSettings(inV1, inV2, inV3, ConvexRadius);
}
CapsuleShapeSettings* CPhysics_API_Jolt::CreateCapsuleShapeSettings(float HalfHeightOfCylinder, float Radius)
{
	return new CapsuleShapeSettings(HalfHeightOfCylinder, Radius);
}
CylinderShapeSettings* CPhysics_API_Jolt::CreateCylinderShapeSettings(float HalfHeight, float Radius, float ConvexRadius)
{
	return new CylinderShapeSettings(HalfHeight, Radius, ConvexRadius);
}
ConvexHullShapeSettings* CPhysics_API_Jolt::CreateConvexHullShapeSettings(const vector<Vec3>& inPoints, float MaxConvexRadius)
{
	return new ConvexHullShapeSettings(inPoints, MaxConvexRadius);
}
MeshShapeSettings* CPhysics_API_Jolt::CreateMeshShapeSettings(const vector<Triangle>& inTriangles)
{
	return new MeshShapeSettings(inTriangles);
}
MeshShapeSettings* CPhysics_API_Jolt::CreateMeshShapeSettings(const vector<Vec3>& inVertices, const vector<IndexedTriangle>& inTriangles)
{
	VertexList VertexList;
	VertexList.resize(inVertices.size());
	for (uint32_t i = 0; i < inVertices.size(); ++i)
	{
		VertexList[i] = Float3(inVertices[i].GetX(), inVertices[i].GetY(), inVertices[i].GetZ());
	}
	return new MeshShapeSettings(VertexList, inTriangles);
}
HeightFieldShapeSettings* CPhysics_API_Jolt::CreateHeightFieldShapeSettings(const float* inSamples, const Vec3& inOffset, const Vec3& inScale, uint32_t SampleCount)
{
	return new HeightFieldShapeSettings(inSamples, inOffset, inScale,SampleCount);
}
TaperedCapsuleShapeSettings* CPhysics_API_Jolt::CreateTaperedCapsuleShapeSettings(float HalfHeightOfTaperedCylinder, float TopRadius, float BottomRadius)
{
	return new TaperedCapsuleShapeSettings(HalfHeightOfTaperedCylinder, TopRadius, BottomRadius);
}

BodyID CPhysics_API_Jolt::CreateAndAddBody(const ULONGLONG& GameObjectId, const BodyCreationSettings* inBodyCreateSettings, EActivation Activation)
{
	BodyID bodyID = m_BodyInterface->CreateAndAddBody(*inBodyCreateSettings, Activation);
	m_BodyInterface->AddBody(bodyID, Activation);
	m_Bodys.insert_or_assign(GameObjectId, bodyID);
	m_BodyCreationSettings.insert_or_assign(bodyID, inBodyCreateSettings);
	return bodyID;
}

bool CPhysics_API_Jolt::TryCreateBody(const BodyCreationSettings* inBodyCreateSettings,BodyID& outBodyID)
{
	Body* body = m_BodyInterface->CreateBody(*inBodyCreateSettings);
	if (body)
	{
		outBodyID = body->GetID();
		m_BodyCreationSettings.insert_or_assign(outBodyID, inBodyCreateSettings);
		return true;
	}
	return false;
}

void CPhysics_API_Jolt::AddBody(const ULONGLONG& GameObjectId, const BodyID& inBodyID, EActivation Activation)
{
	m_BodyInterface->AddBody(inBodyID, Activation);
	m_Bodys.insert_or_assign(GameObjectId, inBodyID);
}

BodyID CPhysics_API_Jolt::GetBodyID(const ULONGLONG& GameObjectId)
{
	return m_Bodys.find(GameObjectId)->second;
}

JPH::Vec3 CPhysics_API_Jolt::GetPosition(const BodyID& BodyId)
{
	return m_BodyInterface->GetPosition(BodyId);
}
JPH::Quat CPhysics_API_Jolt::GetRotation(const BodyID& BodyId)
{
	return m_BodyInterface->GetRotation(BodyId);
}