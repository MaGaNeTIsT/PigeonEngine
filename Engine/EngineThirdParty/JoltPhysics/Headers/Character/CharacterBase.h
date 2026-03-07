#pragma once
#include <CoreMinimal.h>
#include "../PhysicsUtility.h"
#include "../JoltIncludes.h"
#include "../Shapes.h"

PIGEONENGINE_NAMESPACE_BEGIN
class FCharacterBaseSettings
{
public:
	virtual								~FCharacterBaseSettings() = default;

	Vector3								Up = Vector3::YVector();

	FLOAT								MaxSlopeAngle = DegreesToRadians(50.0f);

	FShape*								Shape;

	Vector3								PlaneVector = Vector3::YVector();

	const FLOAT							CharacterRadiusStanding = -0.3f;
};

/// <summary>
/// abstract class, dont create it
/// </summary>
class FCharacterBase
{
public:
	FCharacterBase(const FCharacterBaseSettings* inSettings)
	{
		CharacterBaseSettings = inSettings;
		Shape = inSettings->Shape;
	}

	/// Destructor
	virtual								~FCharacterBase() = default;

	void								SetMaxSlopeAngle(FLOAT inMaxSlopeAngle) { return CharacterBase->SetMaxSlopeAngle(inMaxSlopeAngle); }

	FLOAT								GetCosMaxSlopeAngle() const { return CharacterBase->GetCosMaxSlopeAngle(); }

	void								SetUp(Vector3 inUp) { return CharacterBase->SetUp(PhysicsUtility::Convert(inUp)); }

	Vector3								GetUp() const { return PhysicsUtility::Convert(CharacterBase->GetUp()); }

	BOOL32								IsSlopeTooSteep(Vector3 inNormal) const
	{
		return CharacterBase->IsSlopeTooSteep(PhysicsUtility::Convert(inNormal));
	}

	const FShape* GetShape() const { return Shape; }
	enum class EGroundState
	{
		OnGround,						///< Character is on the ground and can move freely.
		OnSteepGround,					///< Character is on a slope that is too steep and can't climb up any further. The caller should start applying downward velocity if sliding from the slope is desired.
		NotSupported,					///< Character is touching an object, but is not supported by it and should fall. The GetGroundXXX functions will return information about the touched object.
		InAir,							///< Character is in the air and is not touching anything.
	};

	EGroundState						GetGroundState() const 
	{ 
		switch(CharacterBase->GetGroundState())
		{
			case JPH::CharacterBase::EGroundState::OnGround:
				return EGroundState::OnGround;
			case JPH::CharacterBase::EGroundState::OnSteepGround:
				return EGroundState::OnSteepGround;
			case JPH::CharacterBase::EGroundState::NotSupported:
				return EGroundState::NotSupported;
			case JPH::CharacterBase::EGroundState::InAir:
				return EGroundState::InAir;
			default:
				PE_ASSERT(FALSE);
				return EGroundState::NotSupported;
		}
	}

	BOOL32								IsSupported() const { return GetGroundState() == EGroundState::OnGround || GetGroundState() == EGroundState::OnSteepGround; }

	Vector3 							GetGroundPosition() const { return PhysicsUtility::Convert2Centimeter(CharacterBase->GetGroundPosition()); }

	Vector3	 							GetGroundNormal() const { return PhysicsUtility::Convert(CharacterBase->GetGroundNormal()); }

	Vector3								GetGroundVelocity() const { return PhysicsUtility::Convert(CharacterBase->GetGroundVelocity()); }

	const PhysicsMaterial*				GetGroundMaterial() const { return CharacterBase->GetGroundMaterial(); }

protected:
	JPH::CharacterBase*					CharacterBase = nullptr;
	const FCharacterBaseSettings*		CharacterBaseSettings = nullptr;
	const FShape*						Shape = nullptr;
};
PIGEONENGINE_NAMESPACE_END