#pragma once
#include "CharacterBase.h"
#include "../JoltPhysicsLayer.h"

PIGEONENGINE_NAMESPACE_BEGIN
using namespace PhysicsUtility;
class FCharacterSettings : public FCharacterBaseSettings
{
public:
	/// Layer that this character will be added to
	UINT8								Layer = 0;

	/// Mass of the character
	FLOAT								Mass = 80.0f;

	/// Friction for the character
	FLOAT								Friction = 0.2f;

	/// Value to multiply gravity with for this character
	FLOAT								GravityFactor = 1.0f;

	//  Max distance between the floor and the character to still consider the character standing on the floor
	FLOAT								MaxSeparationDistance = 0.05f;
};
class FCharacter final : public FCharacterBase
{
public:
	FCharacter(const FCharacterSettings* inSettings);

	/// Destructor
	virtual								~FCharacter() override;

	/// Add bodies and constraints to the system and optionally activate the bodies
	void								AddToPhysicsSystem(EActivate inActivationMode, Vector3 inPosition, Quaternion inRotation, UINT64 inUserData, BOOL32 inLockBodies = TRUE);

	/// Remove bodies and constraints from the system
	void								RemoveFromPhysicsSystem(BOOL32 inLockBodies = TRUE);

	/// Wake up the character
	void								Activate(BOOL32 inLockBodies = TRUE);

	/// Control the velocity of the character
	void								SetLinearAndAngularVelocity(Vector3 inLinearVelocity, Vector3 inAngularVelocity, BOOL32 inLockBodies = TRUE);

	/// Get the linear velocity of the character (m / s)
	Vector3								GetLinearVelocity(BOOL32 inLockBodies = TRUE) const;

	/// Set the linear velocity of the character (m / s)
	void								SetLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies = TRUE);

	/// Add world space linear velocity to current velocity (m / s)
	void								AddLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies = TRUE);

	/// Add impulse to the center of mass of the character
	void								AddImpulse(Vector3 inImpulse, BOOL32 inLockBodies = TRUE);

	/// Get position / rotation of the body
	void								GetPositionAndRotation(Vector3& outPosition, Quaternion& outRotation, BOOL32 inLockBodies = TRUE) const;

	/// Set the position / rotation of the body, optionally activating it.
	void								SetPositionAndRotation(Vector3 inPosition, Quaternion inRotation, EActivate inActivationMode = EActivate::Activate, BOOL32 inLockBodies = TRUE) const;

	/// Get the position of the character
	Vector3								GetPosition(BOOL32 inLockBodies = TRUE) const;

	/// Set the position of the character, optionally activating it.
	void								SetPosition(Vector3 inPostion, EActivate inActivationMode = EActivate::Activate, BOOL32 inLockBodies = TRUE);

	/// Get the rotation of the character
	Quaternion							GetRotation(BOOL32 inLockBodies = TRUE) const;

	/// Set the rotation of the character, optionally activating it.
	void								SetRotation(Quaternion inRotation, EActivate inActivationMode = EActivate::Activate, BOOL32 inLockBodies = TRUE);

	/// Position of the center of mass of the underlying rigid body
	Vector3								GetCenterOfMassPosition(BOOL32 inLockBodies = TRUE) const;

	/// Update the layer of the character
	void								SetLayer(UINT8 inLayer, BOOL32 inLockBodies = TRUE);

	/// Switch the shape of the character (e.g. for stance). When inMaxPenetrationDepth is not FLT_MAX, it checks
	/// if the new shape collides before switching shape. Returns TRUE if the switch succeeded.
	BOOL32								SetShape(FShape* inShape, FLOAT inMaxPenetrationDepth, BOOL32 inLockBodies = TRUE);

	/// Needs to be called after every PhysicsSystem::Update
	/// @param inMaxSeparationDistance Max distance between the floor and the character to still consider the character standing on the floor
	/// @param inLockBodies If the collision query should use the locking body interface (TRUE) or the non locking body interface (FALSE)
	void								PostSimulation(BOOL32 inLockBodies = TRUE);
private:
	const FCharacterSettings*			m_CharacterCreateSettings = nullptr;
	Character*							m_Character = nullptr;
	CharacterSettings*					m_CharacterSettings = nullptr;
};
PIGEONENGINE_NAMESPACE_END