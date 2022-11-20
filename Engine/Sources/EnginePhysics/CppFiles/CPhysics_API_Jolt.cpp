#include "../Headers/CPhysics_API_Jolt.h"

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

void CPhysics_API_Jolt::Tick(const float cDeltaTime)
{
	// Do n collision step per cDeltaTime
	const int cCollisionSteps = 1;
	// If you want more accurate step results you can do multiple sub steps within a collision step. Usually you would set this to 1.
	const int cIntegrationSubSteps = 1;

	m_PhysicsSystem->Update(cDeltaTime, cCollisionSteps, cIntegrationSubSteps, m_TempAllocator, m_JobSystem);
}