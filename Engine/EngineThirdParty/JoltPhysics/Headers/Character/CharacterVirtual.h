#pragma once
#include "CharacterBase.h"
#include "../JoltPhysicsLayer.h"
#include "../JoltPhysicsListener.h"

PIGEONENGINE_NAMESPACE_BEGIN
using JPH::CharacterVirtual;
using JPH::CharacterVirtualSettings;

class FCharacterVirtualSettings : public FCharacterBaseSettings
{
public:
	FPhysicsObjectLayer Layer = Layers::NON_MOVING;							
	/// Character mass (kg). Used to push down objects with gravity when the character is standing on top.
	FLOAT								Mass = 70.0f;
	FLOAT								GravityFactor = 1.0f;

	/// Maximum force with which the character can push other bodies (N).
	FLOAT								MaxStrength = 100.0f;

	/// An extra offset applied to the shape in local space. This allows applying an extra offset to the shape in local space.
	Vector3								ShapeOffset = Vector3::Zero();

	///@name Movement settings
	BOOL32								CanBackFaceMode = TRUE;									///< When colliding with back faces, the character will not be able to move through back facing triangles. Use this if you have triangles that need to collide on both sides.
	FLOAT								PredictiveContactDistance = 0.1f;						///< How far to scan outside of the shape for predictive contacts. A value of 0 will most likely cause the character to get stuck as it properly calculate a sliding direction anymore. A value that's too high will cause ghost collisions.
	UINT32								MaxCollisionIterations = 5;								///< Max amount of collision loops
	UINT32								MaxConstraintIterations = 15;							///< How often to try stepping in the constraint solving
	FLOAT								MinTimeRemaining = 1.0e-4f;								///< Early out condition: If this much time is left to simulate we are done
	FLOAT								CollisionTolerance = 1.0e-3f;							///< How far we're willing to penetrate geometry
	FLOAT								CharacterPadding = 0.02f;								///< How far we try to stay away from the geometry, this ensures that the sweep will hit as little as possible lowering the collision cost and reducing the risk of getting stuck
	UINT32								MaxNumHits = 256;										///< Max num hits to collect in order to avoid excess of contact points collection
	FLOAT								HitReductionCosMaxAngle = 0.999f;						///< Cos(angle) where angle is the maximum angle between two hits contact normals that are allowed to be merged during hit reduction. Default is around 2.5 degrees. Set to -1 to turn off.
	FLOAT								PenetrationRecoverySpeed = 1.0f;						///< This value governs how fast a penetration will be resolved, 0 = nothing is resolved, 1 = everything in one update
};

class FCharacterVirtual final : public FCharacterBase
{
public:
	FCharacterVirtual(const FCharacterVirtualSettings& inSettings);
	virtual ~FCharacterVirtual() override;

	void AddToPhysicsSystem(PhysicsUtility::EActivate inActivationMode, Vector3 inPosition, Quaternion inRotation, const ObjectIdentityType& InObjectID, BOOL32 inLockBodies = TRUE);
	void RemoveFromPhysicsSystem(BOOL32 inLockBodies = TRUE);
	void Activate(BOOL32 inLockBodies = TRUE);
	void SetLinearAndAngularVelocity(Vector3 inLinearVelocity, Vector3 inAngularVelocity, BOOL32 inLockBodies = TRUE);
	Vector3 GetLinearVelocity(BOOL32 inLockBodies = TRUE) const;
	void SetLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies = TRUE);
	void AddLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies = TRUE);
	void AddImpulse(Vector3 inImpulse, BOOL32 inLockBodies = TRUE);
	void GetPositionAndRotation(Vector3& outPosition, Quaternion& outRotation, BOOL32 inLockBodies = TRUE) const;
	void SetPositionAndRotation(Vector3 inPosition, Quaternion inRotation, PhysicsUtility::EActivate inActivationMode = PhysicsUtility::EActivate::Activate, BOOL32 inLockBodies = TRUE) const;
	Vector3 GetPosition(BOOL32 inLockBodies = TRUE) const;
	void SetPosition(Vector3 inPostion, PhysicsUtility::EActivate inActivationMode = PhysicsUtility::EActivate::Activate, BOOL32 inLockBodies = TRUE);
	Quaternion GetRotation(BOOL32 inLockBodies = TRUE) const;
	void SetRotation(Quaternion inRotation, PhysicsUtility::EActivate inActivationMode = PhysicsUtility::EActivate::Activate, BOOL32 inLockBodies = TRUE);
	Vector3 GetCenterOfMassPosition(BOOL32 inLockBodies = TRUE) const;
	void SetLayer(FPhysicsObjectLayer inLayer, BOOL32 inLockBodies = TRUE);
	BOOL32 SetShape(FShape* inShape, FLOAT inMaxPenetrationDepth, BOOL32 inLockBodies = TRUE);
	void PostSimulation(FLOAT inDeltaTime, BOOL32 inLockBodies = TRUE);

	void AddListener(FCharacterContactEventListenerInterface* InListener);
	void RemoveListener(FCharacterContactEventListenerInterface* InListener);
	FCharacterContactListener* GetListener();
	const FCharacterContactListener* GetListener() const;

private:
	const FCharacterVirtualSettings m_CharacterCreateSettings;
	CharacterVirtual* m_Character = nullptr;
	CharacterVirtualSettings* m_CharacterSettings = nullptr;
	FCharacterContactListener m_CharacterContactListener;
};

using FCharacterSettings = FCharacterVirtualSettings;
using FCharacter = FCharacterVirtual;

PIGEONENGINE_NAMESPACE_END