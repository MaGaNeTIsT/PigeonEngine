#pragma once
//Engine Includes
#include "../../../../Entry/EngineMain.h"

#include "CPhysicsInterface.h"

//Jolt Includes
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/TriangleShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/TaperedCapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>


// All Jolt symbols are in the JPH namespace
using namespace JPH;

// We're also using STL classes in this example
using namespace std;

//Jolt define
//#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
//#define JPH_USE_SSE4_1
//#define JPH_USE_SSE4_2
//#define JPH_USE_LZCNT
//#define JPH_USE_TZCNT
//#define JPH_USE_F16C
//#define JPH_USE_FMADD
//#define NDEBUG
#define JPH_PROFILE_ENABLED
#define JPH_DEBUG_RENDERER


// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

namespace PhysicsAPI_Jolt
{
	namespace Layers
	{
		static constexpr uint8 NON_MOVING = 0;
		static constexpr uint8 MOVING = 1;
		static constexpr uint8 NUM_LAYERS = 2;
	};

	namespace BroadPhaseLayers
	{
		static constexpr BroadPhaseLayer NON_MOVING(0);
		static constexpr BroadPhaseLayer MOVING(1);
		static constexpr uint NUM_LAYERS(2);
	};

	// Callback for traces, connect this to your own trace function if you have one
	static void TraceImpl(const char* inFMT, ...)
	{
		// Format the message
		va_list list;
		va_start(list, inFMT);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFMT, list);

		// Print to the TTY
		cout << buffer << endl;
	}

	// BroadPhaseLayerInterface implementation
	// This defines a mapping between object and broadphase layers.
	class CBPLayerInterfaceImpl final : public BroadPhaseLayerInterface
	{
	public:
		CBPLayerInterfaceImpl()
		{
			// Create a mapping table from object to broad phase layer
			mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
		}

		virtual uint					GetNumBroadPhaseLayers() const override
		{
			return BroadPhaseLayers::NUM_LAYERS;
		}

		virtual BroadPhaseLayer			GetBroadPhaseLayer(ObjectLayer inLayer) const override
		{
			JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
			return mObjectToBroadPhase[inLayer];
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const
		{
			switch ((BroadPhaseLayer::Type)inLayer)
			{
			case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
			case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
			default:													JPH_ASSERT(false); return "INVALID";
			}
		}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

	private:
		BroadPhaseLayer					mObjectToBroadPhase[Layers::NUM_LAYERS];
	};

	// Function that determines if two broadphase layers can collide
	static bool BroadPhaseCanCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2)
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}

	// Function that determines if two object layers can collide
	static bool ObjectCanCollide(ObjectLayer inObject1, ObjectLayer inObject2)
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	};

	class CBodyActivationListener : public BodyActivationListener
	{
	public:
		virtual void		OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override
		{
			cout << "A body got activated" << endl;
		}

		virtual void		OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override
		{
			cout << "A body went to sleep" << endl;
		}
	};

	class CContactListener : public ContactListener
	{
	public:
		// See: ContactListener
		virtual ValidateResult	OnContactValidate(const Body& inBody1, const Body& inBody2, const CollideShapeResult& inCollisionResult) override
		{
			cout << "Contact validate callback" << endl;

			// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
			return ValidateResult::AcceptAllContactsForThisBodyPair;
		}

		virtual void			OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
		{
			cout << "A contact was added" << endl;
		}

		virtual void			OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
		{
			cout << "A contact was persisted" << endl;
		}

		virtual void			OnContactRemoved(const SubShapeIDPair& inSubShapePair) override
		{
			cout << "A contact was removed" << endl;
		}
	};
}


class CPhysics_API_Jolt : public CPhysicsInterface
{
public:
	virtual void Init() override;
	virtual void Tick(const float cDeltaTime) override;
	virtual void Uninit() = 0;

	JPH::BodyCreationSettings* CreateBodyCreationSettings();
	JPH::BodyCreationSettings* CreateBodyCreationSettings(ShapeSettings* inSettings,const Vec3& inPosition,const Quat& inRotation,EMotionType MotionType,ObjectLayer ObjectLayer);
	JPH::BodyCreationSettings* CreateBodyCreationSettings(Shape* inShape,const Vec3& inPosition,const Quat& inRotation,EMotionType MotionType,ObjectLayer ObjectLayer);
	//Use CreateXXXShape funtions when you dont want to change params,like static object or some object cant be changed.
	BoxShape* CreateBoxShape(const Vec3& inHalfExtent, float ConvexRadius);
	BoxShapeSettings* CreateBoxShapeSettings(const Vec3& inHalfExtent, float ConvexRadius);
	SphereShape* CreateSphereShape(float Radius);
	SphereShapeSettings* CreateSphereShapeSettings(float Radius);
	TriangleShapeSettings* CreateTriangleShapeSettings(const Vec3& inV1, const Vec3& inV2, const Vec3& inV3, float ConvexRadius);
	CapsuleShapeSettings* CreateCapsuleShapeSettings(float HalfHeightOfCylinder,float Radius);
	CylinderShapeSettings* CreateCylinderShapeSettings(float HalfHeight, float Radius, float ConvexRadius);
	ConvexHullShapeSettings* CreateConvexHullShapeSettings(const vector<Vec3>& inPoints, float MaxConvexRadius);
	MeshShapeSettings* CreateMeshShapeSettings(const vector<Triangle>& inTriangles);
	MeshShapeSettings* CreateMeshShapeSettings(const vector<Vec3>& inVertices, const vector<IndexedTriangle>& inTriangles);
	HeightFieldShapeSettings* CreateHeightFieldShapeSettings(const float* inSamples, const Vec3& inOffset, const Vec3& inScale, uint32_t SampleCount);
	TaperedCapsuleShapeSettings* CreateTaperedCapsuleShapeSettings(float HalfHeightOfTaperedCylinder, float TopRadius, float BottomRadius);
	//CreateBody
	BodyID CreateAndAddBody(const ULONGLONG& GameObjectId, BodyCreationSettings* inBodyCreateSettings, EActivation Activation);
	/// <summary>
	/// try to create a body and return success or not
	/// </summary>
	/// <param name="inBodyCreateSettings">Body create settings input</param>
	/// <param name="outBodyID">output body id</param>
	/// <returns>true ,if create success</returns>
	bool TryCreateBody(BodyCreationSettings* inBodyCreateSettings, BodyID& outBodyID);
	void AddBody(const ULONGLONG& GameObjectId, const BodyID& inBodyID, EActivation Activation);
	/// <summary>
	/// maybe unsafe.
	/// </summary>
	/// <param name="GameObjectId">object id input</param>
	/// <returns></returns>
	JPH_INLINE BodyID GetBodyID(const ULONGLONG& GameObjectId);
	JPH_INLINE JPH::Vec3 GetPosition(const BodyID& BodyId);
	JPH_INLINE JPH::Quat GetRotation(const BodyID& BodyId);
	JPH_INLINE void SetPosition(const BodyID& inBodyID, JPH::Vec3 inPosition, EActivation inActivationMode = EActivation::DontActivate);
	JPH_INLINE void SetRoation(const BodyID& inBodyID, JPH::Quat inRotation, EActivation inActivationMode = EActivation::DontActivate);
	JPH_INLINE void AddForce(const BodyID& inBodyID, JPH::Vec3 inForce);
	JPH_INLINE void AddForce(const BodyID& inBodyID, JPH::Vec3 inForce, JPH::Vec3 inPoint);
	JPH_INLINE void AddImpulse(const BodyID& inBodyID, JPH::Vec3 inImpulse); ///< Applied at center of mass
	JPH_INLINE void AddImpulse(const BodyID& inBodyID, JPH::Vec3 inImpulse, JPH::Vec3 inPoint); ///< Applied at inPoint

	JPH_INLINE void SetGravity(JPH::Vec3 inGravity);
	CPhysics_API_Jolt();
	virtual ~CPhysics_API_Jolt();

	//Physics Parameters
public:
	//Initialized Parameters
	uint cPreAllocatedSize			= 70 * 1024 * 1024;		//pre-allocated memory for simulation.
	uint cMaxBodies					= 65535;				//Max amount of rigid bodies.
	uint cNumBodyMutexes			= 0;					//Mutexes count,0 to default setting.
	uint cMaxBodyPairs				= 65535;				//Max amount of body pairs that can be queued at any time.
	uint cMaxContactConstraints		= 65535;				//This is the maximum size of the contact constraint buffer.

	//Runtime Parameters
	int cCollisionSteps				= 1;					// Do n collision step per cDeltaTime
	int cIntegrationSubSteps		= 1;					// If you want more accurate step results you can do multiple sub steps within a collision step. Usually you would set this to 1.

	//Jolt Parameters
private:
	PhysicsSystem*											m_PhysicsSystem;
	BodyInterface*											m_BodyInterface;
	TempAllocatorImpl*										m_TempAllocator;
	JobSystemThreadPool*									m_JobSystem;
	PhysicsAPI_Jolt::CBPLayerInterfaceImpl*					m_BPLayerInterface;
	PhysicsAPI_Jolt::CBodyActivationListener*				m_BodyActivationListener;
	PhysicsAPI_Jolt::CContactListener*						m_ContactListener;
	unordered_map<ULONGLONG, BodyID>						m_Bodys;
	unordered_map<BodyID, BodyCreationSettings*>			m_BodyCreationSettings;
};