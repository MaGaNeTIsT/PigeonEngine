#pragma once
#include <CoreMinimal.h>
#include <Base/DataStructure/Container/HashMap.h>
#include "JoltIncludes.h"
#include "JoltPhysicsLayer.h"
#include "JoltPhysicsListener.h"
#include "PhysicsManagerInterface.h"
#include "PhysicsUtility.h"

namespace PigeonEngine
{
	// All Jolt symbols are in the JPH namespace
	using namespace JPH;

	struct CPhysicsData
	{
		//Initialized Parameters
		uint PreAllocatedSize = 70 * 1024 * 1024;		//pre-allocated memory for simulation.
		uint MaxBodies = 65535;							//Max amount of rigid bodies.
		uint NumBodyMutexes = 0;						//Mutexes count,0 to default setting.
		uint MaxBodyPairs = 65535;						//Max amount of body pairs that can be queued at any time.
		uint MaxContactConstraints = 65535;				//This is the maximum size of the contact constraint buffer.

		//Runtime Parameters
		int CollisionSteps = 1;							// Do n collision step per cDeltaTime
		int IntegrationSubSteps = 1;					// If you want more accurate step results you can do multiple sub steps within a collision step. Usually you would set this to 1.

		//Physics Parameters
		PhysicsSystem*										PhysicsSystem;
		BodyInterface*										BodyInterface;
		TempAllocatorImpl*									TempAllocator;
		JobSystemThreadPool*								JobSystem;
		PigeonEngine::CBPLayerInterfaceImpl*				BPLayerInterface;
		PigeonEngine::CObjectLayerPairFilterImpl*			ObjectLayerPairFilterImpl;
		PigeonEngine::CObjectVsBroadPhaseLayerFilterImpl*	ObjectVsBroadPhaseLayerFilterImpl;
		PigeonEngine::CBodyActivationListener*				BodyActivationListener;
		PigeonEngine::CContactListener*						ContactListener;
	};

	struct PhysicsBodyId
	{
		BodyID ID;
	};
	
	enum class EActive
	{
		Active,				///< Activate the body, making it part of the simulation
		DontActive			///< Leave activation state as it is (will not deactivate an active body)
	};

	static void TraceImpl(const char* inFMT, ...)
	{
		// Format the message
		va_list list;
		va_start(list, inFMT);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFMT, list);

		// Print to the TTY
	}

	class CPhysics_Jolt : public IPhysicsManagerInterface
	{
	public:
		CPhysics_Jolt() {}
		virtual ~CPhysics_Jolt() {}
	public:
		virtual void InitPhysics();
		virtual void UninitPhysics();
		virtual void PrePhysicsUpdate();
		virtual void PhysicsUpdate(float DeltaTime);
		virtual void PostPhysicsUpdate();

	public:
		Vector3 GetPosition(const PhysicsBodyId& PhysicsBodyId);
		Quaternion GetRotation(const PhysicsBodyId& PhysicsBodyId);
		void SetPosition(const PhysicsBodyId& inPhysicsBodyId, Vector3 inPosition, EActive inActivationMode = EActive::DontActive);
		void SetRoation(const PhysicsBodyId& inPhysicsBodyId, Quaternion inRotation, EActive inActivationMode = EActive::DontActive);
		void AddForce(const PhysicsBodyId& inPhysicsBodyId, Vector3 inForce);
		void AddForce(const PhysicsBodyId& inPhysicsBodyId, Vector3 inForce, Vector3 inPoint);
		void AddImpulse(const PhysicsBodyId& inPhysicsBodyId, Vector3 inImpulse); ///< Applied at center of mass
		void AddImpulse(const PhysicsBodyId& inPhysicsBodyId, Vector3 inImpulse, Vector3 inPoint); ///< Applied at inPoint

		void SetGravity(Vector3 inGravity);
	private:
		THashMap<SIZE_T, PhysicsBodyId>							m_Bodys;
		THashMap<PhysicsBodyId, BodyCreationSettings*>			m_BodyCreationSettings;
	private:
		CPhysicsData* PhysicsData;
	};
};