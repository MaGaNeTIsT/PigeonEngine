#pragma once
#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Map.h>
#include <PigeonBase/Object/Component/PrimitiveComponent.h>
#include "JoltIncludes.h"
#include "JoltPhysicsLayer.h"
#include "JoltPhysicsListener.h"
#include "PhysicsManagerInterface.h"
#include "PhysicsUtility.h"
#include "Shapes.h"

namespace PigeonEngine
{
	// All Jolt symbols are in the JPH namespace
	using namespace JPH;
	using namespace PhysicsUtility;

	struct CPhysicsData
	{
		//Initialized Parameters
		UINT32 PreAllocatedSize = 70 * 1024 * 1024;		//pre-allocated memory for simulation.
		UINT32 MaxBodies = 65535;							//Max amount of rigid bodies.
		UINT32 NumBodyMutexes = 0;						//Mutexes count,0 to default setting.
		UINT32 MaxBodyPairs = 65535;						//Max amount of body pairs that can be queued at any time.
		UINT32 MaxContactConstraints = 65535;				//This is the maximum size of the contact constraint buffer.

		//Runtime Parameters
		int CollisionSteps = 1;							// Do n collision step per cDeltaTime

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

		/// Equals check
		inline bool				operator == (const PhysicsBodyId& inRHS) const
		{
			return ID == inRHS.ID;
		}

		/// Not equals check
		inline bool				operator != (const PhysicsBodyId& inRHS) const
		{
			return ID != inRHS.ID;
		}

		/// Smaller than operator, can be used for sorting bodies
		inline bool				operator < (const PhysicsBodyId& inRHS) const
		{
			return ID < inRHS.ID;
		}

		/// Greater than operator, can be used for sorting bodies
		inline bool				operator > (const PhysicsBodyId& inRHS) const
		{
			return ID > inRHS.ID;
		}

		/// Get index in body array
		inline UINT32			GetIndex() const
		{
			return ID.GetIndex();;
		}

		/// Get sequence number of body.
		/// The sequence number can be used to check if a body ID with the same body index has been reused by another body.
		/// It is mainly used in multi threaded situations where a body is removed and its body index is immediately reused by a body created from another thread.
		/// Functions querying the broadphase can (after aquiring a body lock) detect that the body has been removed (we assume that this won't happen more than 128 times in a row).
		inline UINT8			GetSequenceNumber() const
		{
			return ID.GetSequenceNumber();
		}

		/// Returns the index and sequence number combined in an uint32
		inline UINT32			GetIndexAndSequenceNumber() const
		{
			return ID.GetIndexAndSequenceNumber();
		}

		/// Check if the ID is valid
		inline bool				IsInvalid() const
		{
			return ID.IsInvalid();
		}
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

	//template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Alloc = std::allocator<std::pair<const Key, T>>> using THashMap = std::unordered_map<Key, T, Hash, KeyEqual, Alloc>;

	class CPhysics_Jolt : public IPhysicsManagerInterface
	{
	public:
		CPhysics_Jolt() : PhysicsData(nullptr) {}
		virtual ~CPhysics_Jolt() {}
	public:
		virtual void InitPhysics();
		virtual void UninitPhysics();
		virtual void PrePhysicsUpdate();
		virtual void PhysicsUpdate(float DeltaTime);
		virtual void PostPhysicsUpdate();

	public:
		bool TryCreateBody(FShape* inShape, bool CreateNew, Vector3 inPosition, Quaternion inRotation, PhysicsUtility::EMotionType inMotionType, UINT16 inLayer, PhysicsBodyId& outBodyID);
		void AddBody(const ObjectIdentityType& GameObjectId, const PhysicsBodyId& inBodyID, EActive inActivationMode = EActive::DontActive);
		/// <summary>
		/// remove a rigid body by a GameObjectId.
		/// </summary>
		/// <param name="GameObjectId">Id</param>
		/// <param name="bDeleteShape">true to delete shape at sametime,if your shape is shared to other bodys, set false.</param>
		void RemoveBody(const ObjectIdentityType& GameObjectId, bool bDeleteShape = true);

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
		TMap<ObjectIdentityType, PhysicsBodyId>					m_Bodys;
		TMap<PhysicsBodyId, FShape*>							m_Shapes;
	private:
		CPhysicsData* PhysicsData;
	};
};