#pragma once
#include <CoreMinimal.h>
#include "../../../Sources/EngineCore/Core/Base/DataStructure/Container/Array.h"
#include "PhysicsBodyId.h"
#include "PhysicsUtility.h"
#include "JoltIncludes.h"

PIGEONENGINE_NAMESPACE_BEGIN
using JPH::Body;
using JPH::BodyActivationListener;
using JPH::BodyID;
using JPH::CharacterContactListener;
using JPH::CharacterContactSettings;
using JPH::CharacterVirtual;
using JPH::CollideShapeResult;
using JPH::ContactListener;
using JPH::ContactManifold;
using JPH::ContactSettings;
using JPH::PhysicsMaterial;
using JPH::RVec3Arg;
using JPH::SoftBodyContactListener;
using JPH::SoftBodyContactSettings;
using JPH::SoftBodyManifold;
using JPH::SoftBodyValidateResult;
using JPH::SubShapeID;
using JPH::SubShapeIDPair;
using JPH::ValidateResult;
using JPH::Vec3;
using JPH::Vec3Arg;
using JPH::uint64;

class FPhysics_Jolt;

enum class EPhysicsContactValidateResult : UINT8
{
	AcceptAllContactsForThisBodyPair = 0,
	AcceptContact = 1,
	RejectContact = 2,
	RejectAllContactsForThisBodyPair = 3,
};

enum class EPhysicsSoftBodyContactValidateResult : UINT8
{
	AcceptContact = 0,
	RejectContact = 1,
};

struct FPhysicsContactSettings
{
	FLOAT CombinedFriction = 0.f;
	FLOAT CombinedRestitution = 0.f;
	FLOAT InvMassScale1 = 1.f;
	FLOAT InvInertiaScale1 = 1.f;
	FLOAT InvMassScale2 = 1.f;
	FLOAT InvInertiaScale2 = 1.f;
	BOOL32 IsSensor = FALSE;
	Vector3 RelativeLinearSurfaceVelocity = Vector3::Zero();
	Vector3 RelativeAngularSurfaceVelocity = Vector3::Zero();

	void FromJolt(const ContactSettings& InSettings)
	{
		CombinedFriction = InSettings.mCombinedFriction;
		CombinedRestitution = InSettings.mCombinedRestitution;
		InvMassScale1 = InSettings.mInvMassScale1;
		InvInertiaScale1 = InSettings.mInvInertiaScale1;
		InvMassScale2 = InSettings.mInvMassScale2;
		InvInertiaScale2 = InSettings.mInvInertiaScale2;
		IsSensor = InSettings.mIsSensor;
		RelativeLinearSurfaceVelocity = PhysicsUtility::Convert2Centimeter(InSettings.mRelativeLinearSurfaceVelocity);
		RelativeAngularSurfaceVelocity = PhysicsUtility::Convert(InSettings.mRelativeAngularSurfaceVelocity);
	}

	void ToJolt(ContactSettings& OutSettings) const
	{
		OutSettings.mCombinedFriction = CombinedFriction;
		OutSettings.mCombinedRestitution = CombinedRestitution;
		OutSettings.mInvMassScale1 = InvMassScale1;
		OutSettings.mInvInertiaScale1 = InvInertiaScale1;
		OutSettings.mInvMassScale2 = InvMassScale2;
		OutSettings.mInvInertiaScale2 = InvInertiaScale2;
		OutSettings.mIsSensor = IsSensor;
		OutSettings.mRelativeLinearSurfaceVelocity = PhysicsUtility::Convert2Meter(RelativeLinearSurfaceVelocity);
		OutSettings.mRelativeAngularSurfaceVelocity = PhysicsUtility::Convert(RelativeAngularSurfaceVelocity);
	}

	static FPhysicsContactSettings FromJoltSettings(const ContactSettings& InSettings)
	{
		FPhysicsContactSettings Result;
		Result.FromJolt(InSettings);
		return Result;
	}

	static ContactSettings ToJoltSettings(const FPhysicsContactSettings& InSettings)
	{
		ContactSettings Result;
		InSettings.ToJolt(Result);
		return Result;
	}

	static FPhysicsContactSettings CreateDefault()
	{
		return FromJoltSettings(ContactSettings());
	}

	static FPhysicsContactSettings Create(const ContactSettings& InSettings)
	{
		return FromJoltSettings(InSettings);
	}
};

struct FPhysicsContactManifold
{
	Vector3 BaseOffset = Vector3::Zero();
	Vector3 WorldSpaceNormal = Vector3::Zero();
	FLOAT PenetrationDepth = 0.f;
	UINT32 SubShapeID1 = UINT32_MAX;
	UINT32 SubShapeID2 = UINT32_MAX;
	TArray<Vector3> RelativeContactPointsOn1;
	TArray<Vector3> RelativeContactPointsOn2;

	static FPhysicsContactManifold Create(const ContactManifold& InManifold)
	{
		FPhysicsContactManifold Result;
		Result.BaseOffset = PhysicsUtility::Convert2Centimeter(Vec3(InManifold.mBaseOffset.GetX(), InManifold.mBaseOffset.GetY(), InManifold.mBaseOffset.GetZ()));
		Result.WorldSpaceNormal = PhysicsUtility::Convert(InManifold.mWorldSpaceNormal);
		Result.PenetrationDepth = InManifold.mPenetrationDepth * 100.f;
		Result.SubShapeID1 = InManifold.mSubShapeID1.GetValue();
		Result.SubShapeID2 = InManifold.mSubShapeID2.GetValue();
		for (UINT32 i = 0u; i < static_cast<UINT32>(InManifold.mRelativeContactPointsOn1.size()); ++i)
		{
			Result.RelativeContactPointsOn1.Add(PhysicsUtility::Convert2Centimeter(InManifold.mRelativeContactPointsOn1[i]));
		}
		for (UINT32 i = 0u; i < static_cast<UINT32>(InManifold.mRelativeContactPointsOn2.size()); ++i)
		{
			Result.RelativeContactPointsOn2.Add(PhysicsUtility::Convert2Centimeter(InManifold.mRelativeContactPointsOn2[i]));
		}
		return Result;
	}
};

struct FPhysicsContactValidateResult
{
	Vector3 ContactPointOn1 = Vector3::Zero();
	Vector3 ContactPointOn2 = Vector3::Zero();
	Vector3 PenetrationAxis = Vector3::Zero();
	FLOAT PenetrationDepth = 0.f;
	UINT32 SubShapeID1 = UINT32_MAX;
	UINT32 SubShapeID2 = UINT32_MAX;
	FPhysicsBodyId BodyId2;

	static FPhysicsContactValidateResult Create(const CollideShapeResult& InResult)
	{
		FPhysicsContactValidateResult Result;
		Result.ContactPointOn1 = PhysicsUtility::Convert2Centimeter(InResult.mContactPointOn1);
		Result.ContactPointOn2 = PhysicsUtility::Convert2Centimeter(InResult.mContactPointOn2);
		Result.PenetrationAxis = PhysicsUtility::Convert(InResult.mPenetrationAxis);
		Result.PenetrationDepth = InResult.mPenetrationDepth * 100.f;
		Result.SubShapeID1 = InResult.mSubShapeID1.GetValue();
		Result.SubShapeID2 = InResult.mSubShapeID2.GetValue();
		Result.BodyId2.ID = InResult.mBodyID2;
		return Result;
	}
};

struct FPhysicsSubShapePair
{
	FPhysicsBodyId BodyId1;
	UINT32 SubShapeID1 = UINT32_MAX;
	FPhysicsBodyId BodyId2;
	UINT32 SubShapeID2 = UINT32_MAX;

	static FPhysicsSubShapePair Create(const SubShapeIDPair& InPair)
	{
		FPhysicsSubShapePair Result;
		Result.BodyId1.ID = InPair.GetBody1ID();
		Result.SubShapeID1 = InPair.GetSubShapeID1().GetValue();
		Result.BodyId2.ID = InPair.GetBody2ID();
		Result.SubShapeID2 = InPair.GetSubShapeID2().GetValue();
		return Result;
	}
};

struct FPhysicsCharacterContactSettings
{
	BOOL32 CanPushCharacter = TRUE;
	BOOL32 CanReceiveImpulses = TRUE;

	void FromJolt(const CharacterContactSettings& InSettings)
	{
		CanPushCharacter = InSettings.mCanPushCharacter;
		CanReceiveImpulses = InSettings.mCanReceiveImpulses;
	}

	void ToJolt(CharacterContactSettings& OutSettings) const
	{
		OutSettings.mCanPushCharacter = CanPushCharacter;
		OutSettings.mCanReceiveImpulses = CanReceiveImpulses;
	}

	static FPhysicsCharacterContactSettings Create(const CharacterContactSettings& InSettings)
	{
		FPhysicsCharacterContactSettings Result;
		Result.FromJolt(InSettings);
		return Result;
	}
};

struct FPhysicsSoftBodyContactSettings
{
	FLOAT InvMassScale1 = 1.f;
	FLOAT InvMassScale2 = 1.f;
	FLOAT InvInertiaScale2 = 1.f;
	BOOL32 IsSensor = FALSE;

	void FromJolt(const SoftBodyContactSettings& InSettings)
	{
		InvMassScale1 = InSettings.mInvMassScale1;
		InvMassScale2 = InSettings.mInvMassScale2;
		InvInertiaScale2 = InSettings.mInvInertiaScale2;
		IsSensor = InSettings.mIsSensor;
	}

	void ToJolt(SoftBodyContactSettings& OutSettings) const
	{
		OutSettings.mInvMassScale1 = InvMassScale1;
		OutSettings.mInvMassScale2 = InvMassScale2;
		OutSettings.mInvInertiaScale2 = InvInertiaScale2;
		OutSettings.mIsSensor = IsSensor;
	}

	static FPhysicsSoftBodyContactSettings Create(const SoftBodyContactSettings& InSettings)
	{
		FPhysicsSoftBodyContactSettings Result;
		Result.FromJolt(InSettings);
		return Result;
	}
};

struct FPhysicsSoftBodyManifoldVertex
{
	Vector3 Position = Vector3::Zero();
	BOOL32 HasContact = FALSE;
	Vector3 ContactPoint = Vector3::Zero();
	Vector3 ContactNormal = Vector3::Zero();
	FPhysicsBodyId ContactBodyId;
};

struct FPhysicsSoftBodyManifold
{
	TArray<FPhysicsSoftBodyManifoldVertex> Vertices;
	TArray<FPhysicsBodyId> SensorContactBodyIds;

	static FPhysicsSoftBodyManifold Create(const SoftBodyManifold& InManifold)
	{
		FPhysicsSoftBodyManifold Result;
		for (const auto& Vertex : InManifold.GetVertices())
		{
			FPhysicsSoftBodyManifoldVertex& NewVertex = Result.Vertices.AddDefaultGetRef();
			NewVertex.Position = PhysicsUtility::Convert2Centimeter(Vertex.mPosition);
			NewVertex.HasContact = InManifold.HasContact(Vertex);
			if (NewVertex.HasContact)
			{
				NewVertex.ContactPoint = PhysicsUtility::Convert2Centimeter(InManifold.GetLocalContactPoint(Vertex));
				NewVertex.ContactNormal = PhysicsUtility::Convert(InManifold.GetContactNormal(Vertex));
				NewVertex.ContactBodyId.ID = InManifold.GetContactBodyID(Vertex);
			}
		}

		for (UINT32 i = 0u; i < static_cast<UINT32>(InManifold.GetNumSensorContacts()); ++i)
		{
			FPhysicsBodyId& NewBodyId = Result.SensorContactBodyIds.AddDefaultGetRef();
			NewBodyId.ID = InManifold.GetSensorContactBodyID(i);
		}

		return Result;
	}
};

inline ValidateResult ToJoltValidateResult(EPhysicsContactValidateResult InResult)
{
	switch (InResult)
	{
	case EPhysicsContactValidateResult::AcceptContact:
		return ValidateResult::AcceptContact;
	case EPhysicsContactValidateResult::RejectContact:
		return ValidateResult::RejectContact;
	case EPhysicsContactValidateResult::RejectAllContactsForThisBodyPair:
		return ValidateResult::RejectAllContactsForThisBodyPair;
	case EPhysicsContactValidateResult::AcceptAllContactsForThisBodyPair:
	default:
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}
}

inline EPhysicsContactValidateResult FromJoltValidateResult(ValidateResult InResult)
{
	switch (InResult)
	{
	case ValidateResult::AcceptContact:
		return EPhysicsContactValidateResult::AcceptContact;
	case ValidateResult::RejectContact:
		return EPhysicsContactValidateResult::RejectContact;
	case ValidateResult::RejectAllContactsForThisBodyPair:
		return EPhysicsContactValidateResult::RejectAllContactsForThisBodyPair;
	case ValidateResult::AcceptAllContactsForThisBodyPair:
	default:
		return EPhysicsContactValidateResult::AcceptAllContactsForThisBodyPair;
	}
}

inline SoftBodyValidateResult ToJoltSoftBodyValidateResult(EPhysicsSoftBodyContactValidateResult InResult)
{
	return InResult == EPhysicsSoftBodyContactValidateResult::RejectContact ? SoftBodyValidateResult::RejectContact : SoftBodyValidateResult::AcceptContact;
}

inline EPhysicsSoftBodyContactValidateResult FromJoltSoftBodyValidateResult(SoftBodyValidateResult InResult)
{
	return InResult == SoftBodyValidateResult::RejectContact ? EPhysicsSoftBodyContactValidateResult::RejectContact : EPhysicsSoftBodyContactValidateResult::AcceptContact;
}

class FBodyActivationEventListenerInterface
{
public:
	virtual ~FBodyActivationEventListenerInterface() = default;

	virtual void OnBodyActivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID) {}
	virtual void OnBodyDeactivated(const FPhysicsBodyId& inBodyID, const ObjectIdentityType& inObjectID) {}
};

class FContactEventListenerInterface
{
public:
	virtual ~FContactEventListenerInterface() = default;

	virtual EPhysicsContactValidateResult OnContactValidate(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const Vector3& inBaseOffset, const FPhysicsContactValidateResult& inCollisionResult)
	{
		return EPhysicsContactValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings) {}
	virtual void OnContactPersisted(const FPhysicsBodyId& inBodyID1, const FPhysicsBodyId& inBodyID2, const FPhysicsContactManifold& inManifold, FPhysicsContactSettings& ioSettings) {}
	virtual void OnContactRemoved(const FPhysicsSubShapePair& inSubShapePair) {}
};

class FCharacterContactEventListenerInterface
{
public:
	virtual ~FCharacterContactEventListenerInterface() = default;

	virtual void OnAdjustBodyVelocity(const FPhysicsBodyId& inBodyID2, Vector3& ioLinearVelocity, Vector3& ioAngularVelocity) {}
	virtual bool OnContactValidate(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2) { return TRUE; }
	virtual void OnContactAdded(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2, const Vector3& inContactPosition, const Vector3& inContactNormal, FPhysicsCharacterContactSettings& ioSettings) {}
	virtual void OnContactSolve(const FPhysicsBodyId& inBodyID2, UINT32 inSubShapeID2, const Vector3& inContactPosition, const Vector3& inContactNormal, const Vector3& inContactVelocity, const Vector3& inCharacterVelocity, Vector3& ioNewCharacterVelocity) {}
};

class FSoftBodyContactEventListenerInterface
{
public:
	virtual ~FSoftBodyContactEventListenerInterface() = default;

	virtual EPhysicsSoftBodyContactValidateResult OnSoftBodyContactValidate(const FPhysicsBodyId& inSoftBodyID, const FPhysicsBodyId& inOtherBodyID, FPhysicsSoftBodyContactSettings& ioSettings)
	{
		return EPhysicsSoftBodyContactValidateResult::AcceptContact;
	}

	virtual void OnSoftBodyContactAdded(const FPhysicsBodyId& inSoftBodyID, const FPhysicsSoftBodyManifold& inManifold) {}
};

class FBodyActivationListener : public BodyActivationListener
{
public:
	void AddListener(FBodyActivationEventListenerInterface* InListener)
	{
		if (InListener && !m_Listeners.Contains(InListener))
		{
			m_Listeners.Add(InListener);
		}
	}

	void RemoveListener(FBodyActivationEventListenerInterface* InListener)
	{
		if (InListener)
		{
			m_Listeners.Remove(InListener);
		}
	}

	virtual void OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override
	{
		FPhysicsBodyId PhysicsBodyId;
		PhysicsBodyId.ID = inBodyID;
		const ObjectIdentityType ObjectID = static_cast<ObjectIdentityType>(inBodyUserData);
		for (auto* Listener : m_Listeners)
		{
			if (Listener)
			{
				Listener->OnBodyActivated(PhysicsBodyId, ObjectID);
			}
		}
	}

	virtual void OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override
	{
		FPhysicsBodyId PhysicsBodyId;
		PhysicsBodyId.ID = inBodyID;
		const ObjectIdentityType ObjectID = static_cast<ObjectIdentityType>(inBodyUserData);
		for (auto* Listener : m_Listeners)
		{
			if (Listener)
			{
				Listener->OnBodyDeactivated(PhysicsBodyId, ObjectID);
			}
		}
	}

private:
	TArray<FBodyActivationEventListenerInterface*> m_Listeners;
};

class FContactListener : public ContactListener
{
public:
	void AddListener(FContactEventListenerInterface* InListener)
	{
		if (InListener && !m_Listeners.Contains(InListener))
		{
			m_Listeners.Add(InListener);
		}
	}

	void RemoveListener(FContactEventListenerInterface* InListener)
	{
		if (InListener)
		{
			m_Listeners.Remove(InListener);
		}
	}

	// See: ContactListener
	virtual ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override
	{
		FPhysicsBodyId BodyId1;
		BodyId1.ID = inBody1.GetID();
		FPhysicsBodyId BodyId2;
		BodyId2.ID = inBody2.GetID();
		const Vector3 BaseOffset = PhysicsUtility::Convert2Centimeter(Vec3(inBaseOffset.GetX(), inBaseOffset.GetY(), inBaseOffset.GetZ()));
		const FPhysicsContactValidateResult CollisionResult = FPhysicsContactValidateResult::Create(inCollisionResult);
		for (auto* Listener : m_Listeners)
		{
			if (Listener)
			{
				const EPhysicsContactValidateResult Result = Listener->OnContactValidate(BodyId1, BodyId2, BaseOffset, CollisionResult);
				if (Result != EPhysicsContactValidateResult::AcceptAllContactsForThisBodyPair)
				{
					return ToJoltValidateResult(Result);
				}
			}
		}

		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
	{
		FPhysicsBodyId BodyId1;
		BodyId1.ID = inBody1.GetID();
		FPhysicsBodyId BodyId2;
		BodyId2.ID = inBody2.GetID();
		const FPhysicsContactManifold Manifold = FPhysicsContactManifold::Create(inManifold);
		FPhysicsContactSettings Settings = FPhysicsContactSettings::Create(ioSettings);
		for (auto* Listener : m_Listeners)
		{
			if (Listener)
			{
				Listener->OnContactAdded(BodyId1, BodyId2, Manifold, Settings);
			}
		}
		Settings.ToJolt(ioSettings);
	}

	virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
	{
		FPhysicsBodyId BodyId1;
		BodyId1.ID = inBody1.GetID();
		FPhysicsBodyId BodyId2;
		BodyId2.ID = inBody2.GetID();
		const FPhysicsContactManifold Manifold = FPhysicsContactManifold::Create(inManifold);
		FPhysicsContactSettings Settings = FPhysicsContactSettings::Create(ioSettings);
		for (auto* Listener : m_Listeners)
		{
			if (Listener)
			{
				Listener->OnContactPersisted(BodyId1, BodyId2, Manifold, Settings);
			}
		}
		Settings.ToJolt(ioSettings);
	}

	virtual void OnContactRemoved(const SubShapeIDPair& inSubShapePair) override
	{
		const FPhysicsSubShapePair SubShapePair = FPhysicsSubShapePair::Create(inSubShapePair);
		for (auto* Listener : m_Listeners)
		{
			if (Listener)
			{
				Listener->OnContactRemoved(SubShapePair);
			}
		}
	}

private:
	TArray<FContactEventListenerInterface*> m_Listeners;
};

/// This class receives callbacks when a virtual character hits something.
class FCharacterContactListener :public CharacterContactListener
{
public:
	void AddListener(FCharacterContactEventListenerInterface* InListener)
	{
		if (InListener && !m_Listeners.Contains(InListener))
		{
			m_Listeners.Add(InListener);
		}
	}

	void RemoveListener(FCharacterContactEventListenerInterface* InListener)
	{
		if (InListener)
		{
			m_Listeners.Remove(InListener);
		}
	}

	/// Destructor
	virtual	~FCharacterContactListener() = default;

	/// Callback to adjust the velocity of a body as seen by the character. Can be adjusted to e.g. implement a conveyor belt or an inertial dampener system of a sci-fi space ship.
	/// Note that inBody2 is locked during the callback so you can read its properties freely.
	virtual void OnAdjustBodyVelocity(const CharacterVirtual* inCharacter, const Body& inBody2, Vec3& ioLinearVelocity, Vec3& ioAngularVelocity)
	{
		FPhysicsBodyId BodyId2;
		BodyId2.ID = inBody2.GetID();
		Vector3 LinearVelocity = PhysicsUtility::Convert(ioLinearVelocity);
		Vector3 AngularVelocity = PhysicsUtility::Convert(ioAngularVelocity);
		for (auto* Listener : m_Listeners)
		{
			if (Listener)
			{
				Listener->OnAdjustBodyVelocity(BodyId2, LinearVelocity, AngularVelocity);
			}
		}
		ioLinearVelocity = PhysicsUtility::Convert(LinearVelocity);
		ioAngularVelocity = PhysicsUtility::Convert(AngularVelocity);
	}

	/// Checks if a character can collide with specified body. Return true if the contact is valid.
	virtual bool OnContactValidate(const CharacterVirtual* inCharacter, const BodyID& inBodyID2, const SubShapeID& inSubShapeID2)
	{
		FPhysicsBodyId BodyId2;
		BodyId2.ID = inBodyID2;
		for (auto* Listener : m_Listeners)
		{
			if (Listener && !Listener->OnContactValidate(BodyId2, inSubShapeID2.GetValue()))
			{
				return FALSE;
			}
		}

		return TRUE;
	}

	/// Called whenever the character collides with a body. Returns true if the contact can push the character.
	/// @param inCharacter Character that is being solved
	/// @param inBodyID2 Body ID of body that is being hit
	/// @param inSubShapeID2 Sub shape ID of shape that is being hit
	/// @param inContactPosition World space contact position
	/// @param inContactNormal World space contact normal
	/// @param ioSettings Settings returned by the contact callback to indicate how the character should behave
	virtual void OnContactAdded(const CharacterVirtual* inCharacter, const BodyID& inBodyID2, const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings& ioSettings)
	{
		FPhysicsBodyId BodyId2;
		BodyId2.ID = inBodyID2;
		const Vector3 ContactPosition = PhysicsUtility::Convert2Centimeter(Vec3(inContactPosition.GetX(), inContactPosition.GetY(), inContactPosition.GetZ()));
		const Vector3 ContactNormal = PhysicsUtility::Convert(inContactNormal);
		FPhysicsCharacterContactSettings Settings = FPhysicsCharacterContactSettings::Create(ioSettings);
		for (auto* Listener : m_Listeners)
		{
			if (Listener)
			{
				Listener->OnContactAdded(BodyId2, inSubShapeID2.GetValue(), ContactPosition, ContactNormal, Settings);
			}
		}
		Settings.ToJolt(ioSettings);
	}

	/// Called whenever a contact is being used by the solver. Allows the listener to override the resulting character velocity (e.g. by preventing sliding along certain surfaces).
	/// @param inCharacter Character that is being solved
	/// @param inBodyID2 Body ID of body that is being hit
	/// @param inSubShapeID2 Sub shape ID of shape that is being hit
	/// @param inContactPosition World space contact position
	/// @param inContactNormal World space contact normal
	/// @param inContactVelocity World space velocity of contact point (e.g. for a moving platform)
	/// @param inContactMaterial Material of contact point
	/// @param inCharacterVelocity World space velocity of the character prior to hitting this contact
	/// @param ioNewCharacterVelocity Contains the calculated world space velocity of the character after hitting this contact, this velocity slides along the surface of the contact. Can be modified by the listener to provide an alternative velocity.
	virtual void OnContactSolve(const CharacterVirtual* inCharacter, const BodyID& inBodyID2, const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, Vec3Arg inContactVelocity, const PhysicsMaterial* inContactMaterial, Vec3Arg inCharacterVelocity, Vec3& ioNewCharacterVelocity)
	{
		FPhysicsBodyId BodyId2;
		BodyId2.ID = inBodyID2;
		const Vector3 ContactPosition = PhysicsUtility::Convert2Centimeter(Vec3(inContactPosition.GetX(), inContactPosition.GetY(), inContactPosition.GetZ()));
		const Vector3 ContactNormal = PhysicsUtility::Convert(inContactNormal);
		const Vector3 ContactVelocity = PhysicsUtility::Convert2Centimeter(inContactVelocity);
		const Vector3 CharacterVelocity = PhysicsUtility::Convert2Centimeter(inCharacterVelocity);
		Vector3 NewCharacterVelocity = PhysicsUtility::Convert2Centimeter(ioNewCharacterVelocity);
		for (auto* Listener : m_Listeners)
		{
			if (Listener)
			{
				Listener->OnContactSolve(BodyId2, inSubShapeID2.GetValue(), ContactPosition, ContactNormal, ContactVelocity, CharacterVelocity, NewCharacterVelocity);
			}
		}
		ioNewCharacterVelocity = PhysicsUtility::Convert2Meter(NewCharacterVelocity);
	}

private:
	TArray<FCharacterContactEventListenerInterface*> m_Listeners;
};

class FSoftBodyContactListener : public SoftBodyContactListener
{
public:
	void AddListener(FSoftBodyContactEventListenerInterface* InListener)
	{
		if (InListener && !m_Listeners.Contains(InListener))
		{
			m_Listeners.Add(InListener);
		}
	}

	void RemoveListener(FSoftBodyContactEventListenerInterface* InListener)
	{
		if (InListener)
		{
			m_Listeners.Remove(InListener);
		}
	}

	virtual SoftBodyValidateResult OnSoftBodyContactValidate(const Body& inSoftBody, const Body& inOtherBody, SoftBodyContactSettings& ioSettings) override
	{
		FPhysicsBodyId SoftBodyId;
		SoftBodyId.ID = inSoftBody.GetID();
		FPhysicsBodyId OtherBodyId;
		OtherBodyId.ID = inOtherBody.GetID();
		FPhysicsSoftBodyContactSettings Settings = FPhysicsSoftBodyContactSettings::Create(ioSettings);
		for (auto* Listener : m_Listeners)
		{
			if (Listener)
			{
				const EPhysicsSoftBodyContactValidateResult Result = Listener->OnSoftBodyContactValidate(SoftBodyId, OtherBodyId, Settings);
				if (Result != EPhysicsSoftBodyContactValidateResult::AcceptContact)
				{
					Settings.ToJolt(ioSettings);
					return ToJoltSoftBodyValidateResult(Result);
				}
			}
		}

		Settings.ToJolt(ioSettings);
		return SoftBodyValidateResult::AcceptContact;
	}

	virtual void OnSoftBodyContactAdded(const Body& inSoftBody, const SoftBodyManifold& inManifold) override
	{
		FPhysicsBodyId SoftBodyId;
		SoftBodyId.ID = inSoftBody.GetID();
		const FPhysicsSoftBodyManifold Manifold = FPhysicsSoftBodyManifold::Create(inManifold);
		for (auto* Listener : m_Listeners)
		{
			if (Listener)
			{
				Listener->OnSoftBodyContactAdded(SoftBodyId, Manifold);
			}
		}
	}

private:
	TArray<FSoftBodyContactEventListenerInterface*> m_Listeners;
};

PIGEONENGINE_NAMESPACE_END