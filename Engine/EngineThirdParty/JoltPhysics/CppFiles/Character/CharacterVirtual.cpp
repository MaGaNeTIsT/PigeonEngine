#include "../../Headers/PhysicsManager.h"
#include "../../Headers/Character/CharacterVirtual.h"

PIGEONENGINE_NAMESPACE_BEGIN
using namespace JPH;

namespace
{
	static auto GetBroadPhaseLayerFilter(const FCharacterVirtualSettings& InSettings)
	{
		return FPhysicsManager::GetSingleton()->GetPhysicsData()->PhysicsSystem->GetDefaultBroadPhaseLayerFilter(InSettings.Layer.ToJolt());
	}

	static auto GetObjectLayerFilter(const FCharacterVirtualSettings& InSettings)
	{
		return FPhysicsManager::GetSingleton()->GetPhysicsData()->PhysicsSystem->GetDefaultLayerFilter(InSettings.Layer.ToJolt());
	}
}

FCharacterVirtual::FCharacterVirtual(const FCharacterVirtualSettings& inSettings)
	: m_CharacterCreateSettings(inSettings)
	, FCharacterBase(inSettings)
{
	m_CharacterSettings = New<CharacterVirtualSettings>();
	m_CharacterSettings->mUp = PhysicsUtility::Convert(inSettings.Up);
	m_CharacterSettings->mSupportingVolume = Plane(PhysicsUtility::Convert(inSettings.PlaneVector), inSettings.CharacterRadiusStanding);
	m_CharacterSettings->mMaxSlopeAngle = inSettings.MaxSlopeAngle;
	if (inSettings.Shape)
		m_CharacterSettings->mShape = inSettings.Shape->CreateShape();
	m_CharacterSettings->mMass = inSettings.Mass;
	m_CharacterSettings->mMaxStrength = inSettings.MaxStrength;
	m_CharacterSettings->mInnerBodyLayer = inSettings.Layer.ToJolt();
	m_CharacterSettings->mShapeOffset = PhysicsUtility::Convert2Meter(inSettings.ShapeOffset);
	m_CharacterSettings->mBackFaceMode = inSettings.CanBackFaceMode ? EBackFaceMode::CollideWithBackFaces : EBackFaceMode::IgnoreBackFaces;
	m_CharacterSettings->mPredictiveContactDistance = inSettings.PredictiveContactDistance;
	m_CharacterSettings->mMaxCollisionIterations = inSettings.MaxCollisionIterations;
	m_CharacterSettings->mMaxConstraintIterations = inSettings.MaxConstraintIterations;
	m_CharacterSettings->mMinTimeRemaining = inSettings.MinTimeRemaining;
	m_CharacterSettings->mCollisionTolerance = inSettings.CollisionTolerance;
	m_CharacterSettings->mCharacterPadding = inSettings.CharacterPadding;
	m_CharacterSettings->mMaxNumHits = inSettings.MaxNumHits;
	m_CharacterSettings->mHitReductionCosMaxAngle = inSettings.HitReductionCosMaxAngle;
	m_CharacterSettings->mPenetrationRecoverySpeed = inSettings.PenetrationRecoverySpeed;
}

FCharacterVirtual::~FCharacterVirtual()
{
	if (m_Character)
		RemoveFromPhysicsSystem();
	Delete(m_CharacterSettings);
	m_CharacterSettings = nullptr;
}

void FCharacterVirtual::AddToPhysicsSystem(PhysicsUtility::EActivate inActivationMode, Vector3 inPosition, Quaternion inRotation, const ObjectIdentityType& InObjectID, BOOL32 inLockBodies)
{
	(void)inActivationMode;
	(void)inLockBodies;
	if (m_Character)
	{
		RemoveFromPhysicsSystem();
	}

	m_Character = New<CharacterVirtual>(m_CharacterSettings, PhysicsUtility::Convert2Meter(inPosition), PhysicsUtility::Convert(inRotation), static_cast<uint64>(InObjectID), FPhysicsManager::GetSingleton()->GetPhysicsData()->PhysicsSystem);
	m_Character->SetListener(&m_CharacterContactListener);
	CharacterBase = m_Character;
}

void FCharacterVirtual::RemoveFromPhysicsSystem(BOOL32 inLockBodies)
{
	(void)inLockBodies;
	Delete(m_Character);
	m_Character = nullptr;
	CharacterBase = nullptr;
}

void FCharacterVirtual::Activate(BOOL32 inLockBodies)
{
	(void)inLockBodies;
}

void FCharacterVirtual::SetLinearAndAngularVelocity(Vector3 inLinearVelocity, Vector3 inAngularVelocity, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	(void)inAngularVelocity;
	SetLinearVelocity(inLinearVelocity);
}

Vector3 FCharacterVirtual::GetLinearVelocity(BOOL32 inLockBodies) const
{
	(void)inLockBodies;
	return PhysicsUtility::Convert(m_Character->GetLinearVelocity());
}

void FCharacterVirtual::SetLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	m_Character->SetLinearVelocity(PhysicsUtility::Convert(inLinearVelocity));
}

void FCharacterVirtual::AddLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	m_Character->SetLinearVelocity(m_Character->GetLinearVelocity() + PhysicsUtility::Convert(inLinearVelocity));
}

void FCharacterVirtual::AddImpulse(Vector3 inImpulse, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	const FLOAT Mass = EMath::Max(m_Character->GetMass(), 1.0e-6f);
	m_Character->SetLinearVelocity(m_Character->GetLinearVelocity() + PhysicsUtility::Convert(inImpulse / Mass));
}

void FCharacterVirtual::GetPositionAndRotation(Vector3& outPosition, Quaternion& outRotation, BOOL32 inLockBodies) const
{
	(void)inLockBodies;
	outPosition = PhysicsUtility::Convert2Centimeter(m_Character->GetPosition());
	outRotation = PhysicsUtility::Convert(m_Character->GetRotation());
}

void FCharacterVirtual::SetPositionAndRotation(Vector3 inPosition, Quaternion inRotation, PhysicsUtility::EActivate inActivationMode, BOOL32 inLockBodies) const
{
	(void)inActivationMode;
	(void)inLockBodies;
	m_Character->SetPosition(PhysicsUtility::Convert2Meter(inPosition));
	m_Character->SetRotation(PhysicsUtility::Convert(inRotation));
}

Vector3 FCharacterVirtual::GetPosition(BOOL32 inLockBodies) const
{
	(void)inLockBodies;
	return PhysicsUtility::Convert2Centimeter(m_Character->GetPosition());
}

void FCharacterVirtual::SetPosition(Vector3 inPosition, PhysicsUtility::EActivate inActivationMode, BOOL32 inLockBodies)
{
	(void)inActivationMode;
	(void)inLockBodies;
	m_Character->SetPosition(PhysicsUtility::Convert2Meter(inPosition));
}

Quaternion FCharacterVirtual::GetRotation(BOOL32 inLockBodies) const
{
	(void)inLockBodies;
	return PhysicsUtility::Convert(m_Character->GetRotation());
}

void FCharacterVirtual::SetRotation(Quaternion inRotation, PhysicsUtility::EActivate inActivationMode, BOOL32 inLockBodies)
{
	(void)inActivationMode;
	(void)inLockBodies;
	m_Character->SetRotation(PhysicsUtility::Convert(inRotation));
}

Vector3 FCharacterVirtual::GetCenterOfMassPosition(BOOL32 inLockBodies) const
{
	(void)inLockBodies;
	return PhysicsUtility::Convert2Centimeter(m_Character->GetCenterOfMassPosition());
}

void FCharacterVirtual::SetLayer(FPhysicsObjectLayer inLayer, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	m_CharacterSettings->mInnerBodyLayer = inLayer.ToJolt();
}

BOOL32 FCharacterVirtual::SetShape(FShape* inShape, FLOAT inMaxPenetrationDepth, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	const auto BroadPhaseLayerFilter = GetBroadPhaseLayerFilter(m_CharacterCreateSettings);
	const auto ObjectLayerFilter = GetObjectLayerFilter(m_CharacterCreateSettings);
	IgnoreSingleBodyFilter BodyFilter(m_Character->GetInnerBodyID());
	ShapeFilter ShapeFilter;
	if (m_Character->SetShape(inShape->CreateShape(), PhysicsUtility::Convert2Meter(inMaxPenetrationDepth), BroadPhaseLayerFilter, ObjectLayerFilter, BodyFilter, ShapeFilter, *FPhysicsManager::GetSingleton()->GetPhysicsData()->TempAllocator))
	{
		Shape = inShape;
		return TRUE;
	}
	return FALSE;
}

void FCharacterVirtual::PostSimulation(FLOAT inDeltaTime, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	if (inDeltaTime > 0.0f)
	{
		const Vector3 Gravity = PhysicsUtility::Convert(FPhysicsManager::GetSingleton()->GetPhysicsData()->PhysicsSystem->GetGravity()) * m_CharacterCreateSettings.GravityFactor;
		m_Character->SetLinearVelocity(m_Character->GetLinearVelocity() + PhysicsUtility::Convert(Gravity * inDeltaTime));
	}

	const auto BroadPhaseLayerFilter = GetBroadPhaseLayerFilter(m_CharacterCreateSettings);
	const auto ObjectLayerFilter = GetObjectLayerFilter(m_CharacterCreateSettings);
	IgnoreSingleBodyFilter BodyFilter(m_Character->GetInnerBodyID());
	ShapeFilter ShapeFilter;
	CharacterVirtual::ExtendedUpdateSettings UpdateSettings;
	m_Character->ExtendedUpdate(inDeltaTime, FPhysicsManager::GetSingleton()->GetPhysicsData()->PhysicsSystem->GetGravity(), UpdateSettings, BroadPhaseLayerFilter, ObjectLayerFilter, BodyFilter, ShapeFilter, *FPhysicsManager::GetSingleton()->GetPhysicsData()->TempAllocator);
}

void FCharacterVirtual::AddListener(FCharacterContactEventListenerInterface* InListener)
{
	m_CharacterContactListener.AddListener(InListener);
	if (m_Character)
	{
		m_Character->SetListener(&m_CharacterContactListener);
	}
}

void FCharacterVirtual::RemoveListener(FCharacterContactEventListenerInterface* InListener)
{
	m_CharacterContactListener.RemoveListener(InListener);
}

FCharacterContactListener* FCharacterVirtual::GetListener()
{
	return &m_CharacterContactListener;
}

const FCharacterContactListener* FCharacterVirtual::GetListener() const
{
	return &m_CharacterContactListener;
}

PIGEONENGINE_NAMESPACE_END