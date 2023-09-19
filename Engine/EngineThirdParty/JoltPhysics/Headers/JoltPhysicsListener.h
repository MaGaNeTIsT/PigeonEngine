#pragma once
#include <CoreMinimal.h>
#include "JoltIncludes.h"

PIGEONENGINE_NAMESPACE_BEGIN
using namespace JPH;

class FBodyActivationListener : public BodyActivationListener
{
public:
	virtual void		OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override
	{
	}

	virtual void		OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override
	{
	}
};

class FContactListener : public ContactListener
{
public:
	// See: ContactListener
	virtual ValidateResult	OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override
	{
		//
		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void			OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
	{
	}

	virtual void			OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
	{
	}

	virtual void			OnContactRemoved(const SubShapeIDPair& inSubShapePair) override
	{
	}
};

/// This class receives callbacks when a virtual character hits something.
class FCharacterContactListener :public CharacterContactListener
{
public:
	/// Destructor
	virtual								~FCharacterContactListener() = default;

	/// Callback to adjust the velocity of a body as seen by the character. Can be adjusted to e.g. implement a conveyor belt or an inertial dampener system of a sci-fi space ship.
	/// Note that inBody2 is locked during the callback so you can read its properties freely.
	virtual void						OnAdjustBodyVelocity(const CharacterVirtual* inCharacter, const Body& inBody2, Vec3& ioLinearVelocity, Vec3& ioAngularVelocity) { /* Do nothing, the linear and angular velocity are already filled in */ }

	/// Checks if a character can collide with specified body. Return true if the contact is valid.
	virtual bool						OnContactValidate(const CharacterVirtual* inCharacter, const BodyID& inBodyID2, const SubShapeID& inSubShapeID2) { return true; }

	/// Called whenever the character collides with a body. Returns true if the contact can push the character.
	/// @param inCharacter Character that is being solved
	/// @param inBodyID2 Body ID of body that is being hit
	/// @param inSubShapeID2 Sub shape ID of shape that is being hit
	/// @param inContactPosition World space contact position
	/// @param inContactNormal World space contact normal
	/// @param ioSettings Settings returned by the contact callback to indicate how the character should behave
	virtual void						OnContactAdded(const CharacterVirtual* inCharacter, const BodyID& inBodyID2, const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings& ioSettings) { /* Default do nothing */ }

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
	virtual void						OnContactSolve(const CharacterVirtual* inCharacter, const BodyID& inBodyID2, const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, Vec3Arg inContactVelocity, const PhysicsMaterial* inContactMaterial, Vec3Arg inCharacterVelocity, Vec3& ioNewCharacterVelocity) { /* Default do nothing */ }
};
PIGEONENGINE_NAMESPACE_END