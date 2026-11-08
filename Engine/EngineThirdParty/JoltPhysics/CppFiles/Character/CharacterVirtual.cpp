#include "../../Headers/PhysicsManager.h"
#include "../../Headers/Character/CharacterVirtual.h"

PIGEONENGINE_NAMESPACE_BEGIN
using namespace JPH;

namespace
{
	static auto GetBroadPhaseLayerFilter(const FCharacterVirtualSettings& InSettings)
	{
		return FPhysicsManager::GetManagerSingleton()->GetPhysicsData()->PhysicsSystem->GetDefaultBroadPhaseLayerFilter(InSettings.Layer.ToJolt());
	}

	static auto GetObjectLayerFilter(const FCharacterVirtualSettings& InSettings)
	{
		return FPhysicsManager::GetManagerSingleton()->GetPhysicsData()->PhysicsSystem->GetDefaultLayerFilter(InSettings.Layer.ToJolt());
	}
}

FCharacterVirtual::FCharacterVirtual(const FCharacterVirtualSettings& inSettings)
	: CharacterCreateSettings(inSettings)
	, FCharacterBase(inSettings)
{
	CharacterSettingsInstance = new CharacterVirtualSettings();
	CharacterSettingsInstance->mUp = PhysicsUtility::Convert(inSettings.Up);
	CharacterSettingsInstance->mSupportingVolume = Plane(PhysicsUtility::Convert(inSettings.PlaneVector), inSettings.CharacterRadiusStanding);
	CharacterSettingsInstance->mMaxSlopeAngle = inSettings.MaxSlopeAngle;
	if (inSettings.Shape)
		CharacterSettingsInstance->mShape = inSettings.Shape->CreateShape();
	CharacterSettingsInstance->mMass = inSettings.Mass;
	CharacterSettingsInstance->mMaxStrength = inSettings.MaxStrength;
	CharacterSettingsInstance->mInnerBodyLayer = inSettings.Layer.ToJolt();
	CharacterSettingsInstance->mShapeOffset = PhysicsUtility::Convert2Meter(inSettings.ShapeOffset);
	CharacterSettingsInstance->mBackFaceMode = inSettings.CanBackFaceMode ? EBackFaceMode::CollideWithBackFaces : EBackFaceMode::IgnoreBackFaces;
	CharacterSettingsInstance->mPredictiveContactDistance = inSettings.PredictiveContactDistance;
	CharacterSettingsInstance->mMaxCollisionIterations = inSettings.MaxCollisionIterations;
	CharacterSettingsInstance->mMaxConstraintIterations = inSettings.MaxConstraintIterations;
	CharacterSettingsInstance->mMinTimeRemaining = inSettings.MinTimeRemaining;
	CharacterSettingsInstance->mCollisionTolerance = inSettings.CollisionTolerance;
	CharacterSettingsInstance->mCharacterPadding = inSettings.CharacterPadding;
	CharacterSettingsInstance->mMaxNumHits = inSettings.MaxNumHits;
	CharacterSettingsInstance->mHitReductionCosMaxAngle = inSettings.HitReductionCosMaxAngle;
	CharacterSettingsInstance->mPenetrationRecoverySpeed = inSettings.PenetrationRecoverySpeed;
}

FCharacterVirtual::~FCharacterVirtual()
{
	if (CharacterInstance)
		RemoveFromPhysicsSystem();
	delete CharacterSettingsInstance;
	CharacterSettingsInstance = nullptr;
}

void FCharacterVirtual::AddToPhysicsSystem(PhysicsUtility::EActivate inActivationMode, Vector3 inPosition, Quaternion inRotation, const ObjectIdentityType& InObjectID, BOOL32 inLockBodies)
{
	(void)inActivationMode;
	(void)inLockBodies;
	if (CharacterInstance)
	{
		RemoveFromPhysicsSystem();
	}

	CharacterInstance = new CharacterVirtual(CharacterSettingsInstance, PhysicsUtility::Convert2Meter(inPosition), PhysicsUtility::Convert(inRotation), static_cast<uint64>(InObjectID), FPhysicsManager::GetManagerSingleton()->GetPhysicsData()->PhysicsSystem);
	CharacterInstance->SetListener(&ContactListenerInstance);
	CharacterBase = CharacterInstance;
}

void FCharacterVirtual::RemoveFromPhysicsSystem(BOOL32 inLockBodies)
{
	(void)inLockBodies;
	delete CharacterInstance;
	CharacterInstance = nullptr;
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
	return PhysicsUtility::Convert(CharacterInstance->GetLinearVelocity());
}

void FCharacterVirtual::SetLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	CharacterInstance->SetLinearVelocity(PhysicsUtility::Convert(inLinearVelocity));
}

void FCharacterVirtual::AddLinearVelocity(Vector3 inLinearVelocity, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	CharacterInstance->SetLinearVelocity(CharacterInstance->GetLinearVelocity() + PhysicsUtility::Convert(inLinearVelocity));
}

void FCharacterVirtual::AddImpulse(Vector3 inImpulse, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	const FLOAT Mass = EMath::Max(CharacterInstance->GetMass(), 1.0e-6f);
	CharacterInstance->SetLinearVelocity(CharacterInstance->GetLinearVelocity() + PhysicsUtility::Convert(inImpulse / Mass));
}

void FCharacterVirtual::GetPositionAndRotation(Vector3& outPosition, Quaternion& outRotation, BOOL32 inLockBodies) const
{
	(void)inLockBodies;
	outPosition = PhysicsUtility::Convert2Centimeter(CharacterInstance->GetPosition());
	outRotation = PhysicsUtility::Convert(CharacterInstance->GetRotation());
}

void FCharacterVirtual::SetPositionAndRotation(Vector3 inPosition, Quaternion inRotation, PhysicsUtility::EActivate inActivationMode, BOOL32 inLockBodies) const
{
	(void)inActivationMode;
	(void)inLockBodies;
	CharacterInstance->SetPosition(PhysicsUtility::Convert2Meter(inPosition));
	CharacterInstance->SetRotation(PhysicsUtility::Convert(inRotation));
}

Vector3 FCharacterVirtual::GetPosition(BOOL32 inLockBodies) const
{
	(void)inLockBodies;
	return PhysicsUtility::Convert2Centimeter(CharacterInstance->GetPosition());
}

void FCharacterVirtual::SetPosition(Vector3 inPosition, PhysicsUtility::EActivate inActivationMode, BOOL32 inLockBodies)
{
	(void)inActivationMode;
	(void)inLockBodies;
	CharacterInstance->SetPosition(PhysicsUtility::Convert2Meter(inPosition));
}

Quaternion FCharacterVirtual::GetRotation(BOOL32 inLockBodies) const
{
	(void)inLockBodies;
	return PhysicsUtility::Convert(CharacterInstance->GetRotation());
}

void FCharacterVirtual::SetRotation(Quaternion inRotation, PhysicsUtility::EActivate inActivationMode, BOOL32 inLockBodies)
{
	(void)inActivationMode;
	(void)inLockBodies;
	CharacterInstance->SetRotation(PhysicsUtility::Convert(inRotation));
}

Vector3 FCharacterVirtual::GetCenterOfMassPosition(BOOL32 inLockBodies) const
{
	(void)inLockBodies;
	return PhysicsUtility::Convert2Centimeter(CharacterInstance->GetCenterOfMassPosition());
}

void FCharacterVirtual::SetLayer(FPhysicsObjectLayer inLayer, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	CharacterSettingsInstance->mInnerBodyLayer = inLayer.ToJolt();
}

BOOL32 FCharacterVirtual::SetShape(FShape* inShape, FLOAT inMaxPenetrationDepth, BOOL32 inLockBodies)
{
	(void)inLockBodies;
	const auto BroadPhaseLayerFilter = GetBroadPhaseLayerFilter(CharacterCreateSettings);
	const auto ObjectLayerFilter = GetObjectLayerFilter(CharacterCreateSettings);
	IgnoreSingleBodyFilter BodyFilter(CharacterInstance->GetInnerBodyID());
	ShapeFilter ShapeFilter;
	if (CharacterInstance->SetShape(inShape->CreateShape(), PhysicsUtility::Convert2Meter(inMaxPenetrationDepth), BroadPhaseLayerFilter, ObjectLayerFilter, BodyFilter, ShapeFilter, *FPhysicsManager::GetManagerSingleton()->GetPhysicsData()->TempAllocator))
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
		const Vector3 Gravity = PhysicsUtility::Convert(FPhysicsManager::GetManagerSingleton()->GetPhysicsData()->PhysicsSystem->GetGravity()) * CharacterCreateSettings.GravityFactor;
		CharacterInstance->SetLinearVelocity(CharacterInstance->GetLinearVelocity() + PhysicsUtility::Convert(Gravity * inDeltaTime));
	}

	const auto BroadPhaseLayerFilter = GetBroadPhaseLayerFilter(CharacterCreateSettings);
	const auto ObjectLayerFilter = GetObjectLayerFilter(CharacterCreateSettings);
	IgnoreSingleBodyFilter BodyFilter(CharacterInstance->GetInnerBodyID());
	ShapeFilter ShapeFilter;
	CharacterVirtual::ExtendedUpdateSettings UpdateSettings;
	CharacterInstance->ExtendedUpdate(inDeltaTime, FPhysicsManager::GetManagerSingleton()->GetPhysicsData()->PhysicsSystem->GetGravity(), UpdateSettings, BroadPhaseLayerFilter, ObjectLayerFilter, BodyFilter, ShapeFilter, *FPhysicsManager::GetManagerSingleton()->GetPhysicsData()->TempAllocator);
}

void FCharacterVirtual::AddListener(FCharacterContactEventListenerInterface* InListener)
{
	ContactListenerInstance.AddListener(InListener);
	if (CharacterInstance)
	{
		CharacterInstance->SetListener(&ContactListenerInstance);
	}
}

void FCharacterVirtual::RemoveListener(FCharacterContactEventListenerInterface* InListener)
{
	ContactListenerInstance.RemoveListener(InListener);
}

FCharacterContactListener* FCharacterVirtual::GetListener()
{
	return &ContactListenerInstance;
}

const FCharacterContactListener* FCharacterVirtual::GetListener() const
{
	return &ContactListenerInstance;
}

PIGEONENGINE_NAMESPACE_END
