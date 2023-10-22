#include "../../Headers/Character/Character.h"
#include "../../Headers/PhysicsManager.h"

PIGEONENGINE_NAMESPACE_BEGIN

FCharacter::FCharacter(const FCharacterSettings* inSettings) :
	FCharacterBase(inSettings)
{
	m_CharacterCreateSettings = inSettings;
	CharacterBaseSettings = inSettings;

	m_CharacterSettings = new CharacterSettings();
	m_CharacterSettings->mUp = PhysicsUtility::Convert(inSettings->Up);
	m_CharacterSettings->mMaxSlopeAngle = inSettings->MaxSlopeAngle;
	if(inSettings->Shape)
		m_CharacterSettings->mShape = inSettings->Shape->CreateShape();
	m_CharacterSettings->mSupportingVolume = Plane(PhysicsUtility::Convert(inSettings->PlaneVector),inSettings->CharacterRadiusStanding);
	m_CharacterSettings->mLayer = Layers::MOVING;
	m_CharacterSettings->mMass = inSettings->Mass;
	m_CharacterSettings->mFriction = inSettings->Friction;
	m_CharacterSettings->mGravityFactor = inSettings->GravityFactor;
}

FCharacter::~FCharacter()
{
	delete m_CharacterCreateSettings->Shape;
	delete m_CharacterCreateSettings;
	m_CharacterCreateSettings = nullptr;
	delete m_CharacterSettings;
	m_CharacterSettings = nullptr;
	if (m_Character)
		RemoveFromPhysicsSystem();
}
void FCharacter::AddToPhysicsSystem(EActivate inActivationMode, Vector3 inPosition, Quaternion inRotation, UINT64 inUserData, BOOL32 inLockBodies)
{
	m_Character = new Character(m_CharacterSettings, PhysicsUtility::Convert2Meter(inPosition), PhysicsUtility::Convert(inRotation),inUserData, FPhysicsManager::GetSingleton()->GetPhysicsData()->PhysicsSystem);
	m_Character->AddToPhysicsSystem(inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate, inLockBodies);
	CharacterBase = m_Character;
}
void FCharacter::RemoveFromPhysicsSystem(BOOL32 inLockBodies)
{
	m_Character->RemoveFromPhysicsSystem(inLockBodies);
	delete m_Character;
	m_Character = nullptr;
}
void FCharacter::Activate(BOOL32 inLockBodies)
{
	m_Character->Activate(inLockBodies);
}
void FCharacter::PostSimulation(BOOL32 inLockBodies)
{
	m_Character->PostSimulation(m_CharacterCreateSettings->MaxSeparationDistance, inLockBodies);
}
void FCharacter::SetLinearAndAngularVelocity(Vector3 inLinearVelocity, Vector3 inAngularVelocity, BOOL32 inLockBodies)
{
	m_Character->SetLinearAndAngularVelocity(PhysicsUtility::Convert(inLinearVelocity), PhysicsUtility::Convert(inAngularVelocity), inLockBodies);
}
Vector3 FCharacter::GetLinearVelocity(BOOL32 inLockBodies) const
{
	return PhysicsUtility::Convert(m_Character->GetLinearVelocity(inLockBodies));
}
void FCharacter::SetLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies)
{
	return m_Character->SetLinearVelocity(PhysicsUtility::Convert(inLinearVelocity), inLockBodies);
}
void FCharacter::AddLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies)
{
	m_Character->AddLinearVelocity(PhysicsUtility::Convert(inLinearVelocity),inLockBodies);
}
void FCharacter::AddImpulse(Vector3 inImpulse, BOOL32 inLockBodies)
{
	m_Character->AddImpulse(PhysicsUtility::Convert(inImpulse),inLockBodies);
}
void FCharacter::GetPositionAndRotation(Vector3& outPosition, Quaternion& outRotation, BOOL32 inLockBodies) const
{
	Vec3 position;
	Quat rotation;
	m_Character->GetPositionAndRotation(position, rotation, inLockBodies);
	outPosition = PhysicsUtility::Convert2Centimeter(position);
	outRotation = PhysicsUtility::Convert(rotation);
}
void FCharacter::SetPositionAndRotation(Vector3 inPosition, Quaternion inRotation, EActivate inActivationMode, BOOL32 inLockBodies) const
{
	m_Character->SetPositionAndRotation(PhysicsUtility::Convert2Meter(inPosition), PhysicsUtility::Convert(inRotation), inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate, inLockBodies);
}
Vector3 FCharacter::GetPosition(BOOL32 inLockBodies) const
{
	return PhysicsUtility::Convert2Centimeter(m_Character->GetPosition(inLockBodies));
}
void FCharacter::SetPosition(Vector3 inPosition, EActivate inActivationMode, BOOL32 inLockBodies)
{
	m_Character->SetPosition(PhysicsUtility::Convert2Meter(inPosition), inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate, inLockBodies);
}
Quaternion FCharacter::GetRotation(BOOL32 inLockBodies) const
{
	return PhysicsUtility::Convert(m_Character->GetRotation(inLockBodies));
}
void FCharacter::SetRotation(Quaternion inRotation, EActivate inActivationMode, BOOL32 inLockBodies)
{
	m_Character->SetRotation(PhysicsUtility::Convert(inRotation), inActivationMode == EActivate::Activate ? EActivation::Activate : EActivation::DontActivate, inLockBodies);
}
Vector3 FCharacter::GetCenterOfMassPosition(BOOL32 inLockBodies) const
{
	return PhysicsUtility::Convert2Centimeter(m_Character->GetCenterOfMassPosition(inLockBodies));
}
void FCharacter::SetLayer(UINT8 inLayer, BOOL32 inLockBodies)
{
	m_Character->SetLayer(inLayer, inLockBodies);
}
BOOL32 FCharacter::SetShape(FShape* inShape, FLOAT inMaxPenetrationDepth, BOOL32 inLockBodies)
{
	return m_Character->SetShape(inShape->CreateShape(), inMaxPenetrationDepth, inLockBodies);
}
PIGEONENGINE_NAMESPACE_END