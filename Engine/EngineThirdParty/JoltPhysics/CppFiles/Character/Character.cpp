#include "../../Headers/Character/Character.h"
#include "../../Headers/PhysicsManager.h"

PIGEONENGINE_NAMESPACE_BEGIN
using namespace JPH;

FCharacter::FCharacter(const FCharacterSettings& inSettings)
	:CharacterCreateSettings(inSettings)
	,FCharacterBase(inSettings)
{
	MaxSeparationDistance = inSettings.MaxSeparationDistance;

	CharacterSettingsInstance = new CharacterSettings();
	CharacterSettingsInstance->mUp = PhysicsUtility::Convert(inSettings.Up);
	CharacterSettingsInstance->mMaxSlopeAngle = inSettings.MaxSlopeAngle;
	if(inSettings.Shape)
		CharacterSettingsInstance->mShape = inSettings.Shape->CreateShape();
	CharacterSettingsInstance->mSupportingVolume = Plane(PhysicsUtility::Convert(inSettings.PlaneVector),inSettings.CharacterRadiusStanding);
	CharacterSettingsInstance->mLayer = inSettings.Layer.ToJolt();
	CharacterSettingsInstance->mMass = inSettings.Mass;
	CharacterSettingsInstance->mFriction = inSettings.Friction;
	CharacterSettingsInstance->mGravityFactor = inSettings.GravityFactor;
}

FCharacter::~FCharacter()
{
	delete CharacterSettingsInstance;
	CharacterSettingsInstance = nullptr;
	if (CharacterInstance)
		RemoveFromPhysicsSystem();
}
void FCharacter::AddToPhysicsSystem(PhysicsUtility::EActivate inActivationMode, Vector3 inPosition, Quaternion inRotation, UINT64 inUserData, BOOL32 inLockBodies)
{
	CharacterInstance = new Character(CharacterSettingsInstance, PhysicsUtility::Convert2Meter(inPosition), PhysicsUtility::Convert(inRotation),inUserData, FPhysicsManager::GetManagerSingleton()->GetPhysicsData()->PhysicsSystem);
	CharacterInstance->AddToPhysicsSystem(inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate, inLockBodies);
	CharacterBase = CharacterInstance;
}
void FCharacter::RemoveFromPhysicsSystem(BOOL32 inLockBodies)
{
	CharacterInstance->RemoveFromPhysicsSystem(inLockBodies);
	delete CharacterInstance;
	CharacterInstance = nullptr;
}
void FCharacter::Activate(BOOL32 inLockBodies)
{
	CharacterInstance->Activate(inLockBodies);
}
void FCharacter::PostSimulation(BOOL32 inLockBodies)
{
	CharacterInstance->PostSimulation(MaxSeparationDistance, inLockBodies);
}
void FCharacter::SetLinearAndAngularVelocity(Vector3 inLinearVelocity, Vector3 inAngularVelocity, BOOL32 inLockBodies)
{
	CharacterInstance->SetLinearAndAngularVelocity(PhysicsUtility::Convert(inLinearVelocity), PhysicsUtility::Convert(inAngularVelocity), inLockBodies);
}
Vector3 FCharacter::GetLinearVelocity(BOOL32 inLockBodies) const
{
	return PhysicsUtility::Convert(CharacterInstance->GetLinearVelocity(inLockBodies));
}
void FCharacter::SetLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies)
{
	return CharacterInstance->SetLinearVelocity(PhysicsUtility::Convert(inLinearVelocity), inLockBodies);
}
void FCharacter::AddLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies)
{
	CharacterInstance->AddLinearVelocity(PhysicsUtility::Convert(inLinearVelocity),inLockBodies);
}
void FCharacter::AddImpulse(Vector3 inImpulse, BOOL32 inLockBodies)
{
	CharacterInstance->AddImpulse(PhysicsUtility::Convert(inImpulse),inLockBodies);
}
void FCharacter::GetPositionAndRotation(Vector3& outPosition, Quaternion& outRotation, BOOL32 inLockBodies) const
{
	Vec3 position;
	Quat rotation;
	CharacterInstance->GetPositionAndRotation(position, rotation, inLockBodies);
	outPosition = PhysicsUtility::Convert2Centimeter(position);
	outRotation = PhysicsUtility::Convert(rotation);
}
void FCharacter::SetPositionAndRotation(Vector3 inPosition, Quaternion inRotation, PhysicsUtility::EActivate inActivationMode, BOOL32 inLockBodies) const
{
	CharacterInstance->SetPositionAndRotation(PhysicsUtility::Convert2Meter(inPosition), PhysicsUtility::Convert(inRotation), inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate, inLockBodies);
}
Vector3 FCharacter::GetPosition(BOOL32 inLockBodies) const
{
	return PhysicsUtility::Convert2Centimeter(CharacterInstance->GetPosition(inLockBodies));
}
void FCharacter::SetPosition(Vector3 inPosition, PhysicsUtility::EActivate inActivationMode, BOOL32 inLockBodies)
{
	CharacterInstance->SetPosition(PhysicsUtility::Convert2Meter(inPosition), inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate, inLockBodies);
}
Quaternion FCharacter::GetRotation(BOOL32 inLockBodies) const
{
	return PhysicsUtility::Convert(CharacterInstance->GetRotation(inLockBodies));
}
void FCharacter::SetRotation(Quaternion inRotation, PhysicsUtility::EActivate inActivationMode, BOOL32 inLockBodies)
{
	CharacterInstance->SetRotation(PhysicsUtility::Convert(inRotation), inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate, inLockBodies);
}
Vector3 FCharacter::GetCenterOfMassPosition(BOOL32 inLockBodies) const
{
	return PhysicsUtility::Convert2Centimeter(CharacterInstance->GetCenterOfMassPosition(inLockBodies));
}
void FCharacter::SetLayer(FPhysicsObjectLayer inLayer, BOOL32 inLockBodies)
{
	CharacterInstance->SetLayer(inLayer.ToJolt(), inLockBodies);
}
BOOL32 FCharacter::SetShape(FShape* inShape, FLOAT inMaxPenetrationDepth, BOOL32 inLockBodies)
{
	return CharacterInstance->SetShape(inShape->CreateShape(), inMaxPenetrationDepth, inLockBodies);
}
PIGEONENGINE_NAMESPACE_END
