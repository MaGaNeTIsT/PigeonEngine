#pragma once
#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Map.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveComponent.h>
#include "JoltIncludes.h"
#include "JoltPhysicsLayer.h"
#include "JoltPhysicsListener.h"
#include "PhysicsManagerInterface.h"
#include "PhysicsUtility.h"
#include "Shapes.h"

PIGEONENGINE_NAMESPACE_BEGIN
// All Jolt symbols are in the JPH namespace
using namespace JPH;
using namespace PhysicsUtility;

struct FPhysicsData
{
	//Initialized Parameters
	UINT32 PreAllocatedSize = 70 * 1024 * 1024;			//pre-allocated memory for simulation.
	UINT32 MaxBodies = 65535;							//Max amount of rigid bodies.
	UINT32 NumBodyMutexes = 0;							//Mutexes count,0 to default setting.
	UINT32 MaxBodyPairs = 65535;						//Max amount of body pairs that can be queued at any time.
	UINT32 MaxContactConstraints = 65535;				//This is the maximum size of the contact constraint buffer.

	//Runtime Parameters
	INT32 CollisionSteps = 1;							// Do n collision step per cDeltaTime

	//Physics Parameters
	PhysicsSystem*										PhysicsSystem;
	BodyInterface*										BodyInterface;
	TempAllocatorImpl*									TempAllocator;
	JobSystemThreadPool*								JobSystem;
	PigeonEngine::CBPLayerInterfaceImpl*				BPLayerInterface;
	PigeonEngine::CObjectLayerPairFilterImpl*			ObjectLayerPairFilterImpl;
	PigeonEngine::CObjectVsBroadPhaseLayerFilterImpl*	ObjectVsBroadPhaseLayerFilterImpl;
	PigeonEngine::FBodyActivationListener*				BodyActivationListener;
	PigeonEngine::FContactListener*						ContactListener;
};

struct PhysicsBodyId
{
	BodyID ID;

	/// Equals check
	inline BOOL32				operator == (const PhysicsBodyId& inRHS) const
	{
		return ID == inRHS.ID;
	}

	/// Not equals check
	inline BOOL32				operator != (const PhysicsBodyId& inRHS) const
	{
		return ID != inRHS.ID;
	}

	/// Smaller than operator, can be used for sorting bodies
	inline BOOL32				operator < (const PhysicsBodyId& inRHS) const
	{
		return ID < inRHS.ID;
	}

	/// Greater than operator, can be used for sorting bodies
	inline BOOL32				operator > (const PhysicsBodyId& inRHS) const
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
	inline BOOL32				IsInvalid() const
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

class FPhysics_Jolt : public IPhysicsManagerInterface
{
public:
	FPhysics_Jolt() : PhysicsData(nullptr) {}
	virtual ~FPhysics_Jolt() {}
public:
	virtual void InitPhysics();
	virtual void UninitPhysics();
	virtual void PrePhysicsUpdate();
	virtual void PhysicsUpdate(FLOAT DeltaTime);
	virtual void PostPhysicsUpdate();

	FPhysicsData* GetPhysicsData() { return PhysicsData; }

	void AddCharacter(class FCharacter* Character);
	void RemoveCharacter(class FCharacter* Character);

public:
	BOOL32 TryCreateBody(FShape* inShape, BOOL32 CreateNew, Vector3 inPosition, Quaternion inRotation, PhysicsUtility::EMotionType inMotionType, UINT16 inLayer, PhysicsBodyId& outBodyID);
	void AddBody(const ObjectIdentityType& GameObjectId, const PhysicsBodyId& inBodyID, EActivate inActivationMode = EActivate::DontActivate);
	/// <summary>
	/// remove a rigid body by a GameObjectId.
	/// </summary>
	/// <param name="GameObjectId">Id</param>
	/// <param name="bDeleteShape">true to delete shape at sametime,if your shape is shared to other bodys, set false.</param>
	void RemoveBody(const ObjectIdentityType& GameObjectId, BOOL32 bDeleteShape = true);

	Vector3 GetPosition(const PhysicsBodyId& PhysicsBodyId);
	Quaternion GetRotation(const PhysicsBodyId& PhysicsBodyId);
	void SetPosition(const PhysicsBodyId& inPhysicsBodyId, Vector3 inPosition, EActivate inActivationMode = EActivate::DontActivate);
	void SetRoation(const PhysicsBodyId& inPhysicsBodyId, Quaternion inRotation, EActivate inActivationMode = EActivate::DontActivate);
	void AddForce(const PhysicsBodyId& inPhysicsBodyId, Vector3 inForce);
	void AddForce(const PhysicsBodyId& inPhysicsBodyId, Vector3 inForce, Vector3 inPoint);
	void AddImpulse(const PhysicsBodyId& inPhysicsBodyId, Vector3 inImpulse); ///< Applied at center of mass
	void AddImpulse(const PhysicsBodyId& inPhysicsBodyId, Vector3 inImpulse, Vector3 inPoint); ///< Applied at inPoint

	void SetGravity(Vector3 inGravity);
private:
	TMap<ObjectIdentityType, PhysicsBodyId>					m_Bodys;
	TMap<PhysicsBodyId, FShape*>							m_Shapes;
	TSet<class FCharacter*>									m_Characters;
private:
	FPhysicsData* PhysicsData;
};
PIGEONENGINE_NAMESPACE_END