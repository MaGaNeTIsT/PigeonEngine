#pragma once

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

//DXProject includes
#include "../../../../Entry/EngineMain.h"
#include <Jolt/Jolt.h>

//Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// We're also using STL classes in this example
using namespace std;

namespace PhysicsManager
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
		virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
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

class CPhysicsManager
{
public:
	static CPhysicsManager* GetPhysicsManager() { return m_PhysicsManager; }
	static void Uninit() { delete m_PhysicsManager; }
public:
	void Init();
	void Tick(const float cDeltaTime);
	Body* CreateBody(const ULONGLONG& GameObjectId, 
		const ShapeSettings* inShape, 
		const Vec3Arg& inPosition, 
		const QuatArg& inRotation, 
		EMotionType inMotionType, 
		const ObjectLayer& inObjectLayer, 
		EActivation inActivation);
	Body* CreateBody(const ULONGLONG& GameObjectId, 
		const Shape* inShape, 
		const Vec3Arg& inPosition, 
		const QuatArg& inRotation, 
		EMotionType inMotionType, 
		const ObjectLayer& inObjectLayer, 
		EActivation inActivation);
	Body* CreateBoxBody(const ULONGLONG& GameObjectId, 
		const Vec3Arg& inHalfExtent, 
		const Vec3Arg& inPosition, 
		const QuatArg& inRotation, 
		EMotionType inMotionType, 
		const ObjectLayer& inObjectLayer,
		EActivation inActivation);
	Body* CreateHeightFieldBody(const ULONGLONG& GameObjectId, 
		const float* inSamples, 
		const Vec3Arg& inOffset, 
		const Vec3Arg& inScale, 
		uint32 inSampleCount, 
		const Vec3Arg& inPosition, 
		const QuatArg& inRotation, 
		EMotionType inMotionType, 
		const ObjectLayer& inObjectLayer, 
		EActivation inActivation);
	Body* GetBody(const ULONGLONG& GameObjectId);

private:
	CPhysicsManager() {}
	~CPhysicsManager();
private:
	PhysicsSystem*															m_PhysicsSystem;
	BodyInterface*															m_BodyInterface;
	TempAllocatorImpl*														m_TempAllocator;
	JobSystemThreadPool*													m_JobSystem;
	PhysicsManager::CBPLayerInterfaceImpl*									m_BPLayerInterface;
	PhysicsManager::CBodyActivationListener*								m_BodyActivationListener;
	PhysicsManager::CContactListener*										m_ContactListener;
	unordered_map<ULONGLONG, Body*>											m_Bodys;

private:
	JPH_INLINE Body* CreateBody(const ULONGLONG& GameObjectId, BodyCreationSettings inBodyCreateSettings, EActivation inActivation)
	{
		Body* body = m_BodyInterface->CreateBody(inBodyCreateSettings);
		m_BodyInterface->AddBody(body->GetID(), inActivation);
		m_Bodys.insert_or_assign(GameObjectId, body);
		return body;
	}

private:
	static CPhysicsManager* m_PhysicsManager;
};