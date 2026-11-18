#include "../Headers/PhysicsQuery.h"
#include "../Headers/PhysicsManager.h"
#include "../Headers/Character/CharacterVirtual.h"

namespace PigeonEngine
{
	using namespace JPH;

	static void FillHitObjectIdentity(FRayCastHit& InOutHit)
	{
		InOutHit.ObjectId = 0u;
		FPhysicsManager* Manager = FPhysicsManager::GetManagerSingleton();
		if (Manager)
		{
			Manager->FindObjectIdentityByBodyId(InOutHit.BodyId, InOutHit.ObjectId);
		}
	}

	/// Build the world-space center-of-mass transform for an engine shape. The
	/// engine position is the shape origin; Jolt collides in center-of-mass space.
	static JPH::Mat44 BuildShapeTransform(const JPH::Shape* InShape, const Vector3& InPosition, const Quaternion& InRotation)
	{
		const JPH::Quat Rotation = PhysicsUtility::Convert(InRotation);
		const JPH::Vec3 CenterOfMass = PhysicsUtility::Convert2Meter(InPosition) + Rotation * InShape->GetCenterOfMass();
		return JPH::Mat44::sRotationTranslation(Rotation, CenterOfMass);
	}

	// -----------------------------------------------------------------------
	// Internal helpers - Raycast
	// -----------------------------------------------------------------------

	/// Build a Jolt world-space ray in meters from engine-unit (cm) inputs.
	static RRayCast BuildJoltRay(const Vector3& InOrigin, const Vector3& InDirection, FLOAT InLength)
	{
		const JPH::Vec3 Origin    = PhysicsUtility::Convert2Meter(InOrigin);
		const JPH::Vec3 DirNorm   = PhysicsUtility::Convert(InDirection).NormalizedOr(JPH::Vec3::sAxisZ());
		const JPH::Vec3 Direction = DirNorm * PhysicsUtility::Convert2Meter(InLength);
		return RRayCast{ Origin, Direction };
	}

	/// Populate an FRayCastHit from a Jolt RayCastResult.
	static FRayCastHit BuildRaycastHit(const RRayCast& InRay, const RayCastResult& InResult)
	{
		FRayCastHit Hit;
		Hit.QueryType = EQueryType::Raycast;
		Hit.Fraction = InResult.mFraction;

		const JPH::Vec3 HitPosMeter = InRay.GetPointOnRay(InResult.mFraction);
		Hit.HitPosition = PhysicsUtility::Convert(HitPosMeter) * 100.f;

		const FLOAT RayLengthCm = InRay.mDirection.Length() * 100.f;
		Hit.Distance = Hit.Fraction * RayLengthCm;

		Hit.BodyId.ID = InResult.mBodyID;

		BodyLockRead Lock(FPhysicsManager::GetManagerSingleton()->GetPhysicsData()->PhysicsSystem->GetBodyLockInterfaceNoLock(), InResult.mBodyID);
		if (Lock.Succeeded())
		{
			const Body& HitBody = Lock.GetBody();
			JPH::Vec3 Normal = HitBody.GetWorldSpaceSurfaceNormal(InResult.mSubShapeID2, HitPosMeter);
			Hit.HitNormal = PhysicsUtility::Convert(Normal);
		}
		FillHitObjectIdentity(Hit);

		return Hit;
	}

	// -----------------------------------------------------------------------
	// Internal helpers - Shape Cast
	// -----------------------------------------------------------------------

	/// Build a Jolt RShapeCast from engine-unit inputs.
	/// @param InRotation  Orientation of the shape at the start of the sweep.
	static RShapeCast BuildJoltShapeCast(
		const Shape*		InShape,
		const Vector3&		InOrigin,
		const Vector3&		InDirection,
		FLOAT				InLength,
		const Quaternion&	InRotation)
	{
		const JPH::Mat44 StartTransform = BuildShapeTransform(InShape, InOrigin, InRotation);

		const JPH::Vec3 DirectionNormalized = PhysicsUtility::Convert(InDirection).NormalizedOr(JPH::Vec3::sAxisZ());
		const JPH::Vec3 Direction = DirectionNormalized * PhysicsUtility::Convert2Meter(InLength);

		return RShapeCast::sFromWorldTransform(InShape, JPH::Vec3::sReplicate(1.0f), StartTransform, Direction);
	}

	/// Populate an FRayCastHit from a Jolt ShapeCastResult.
	static FRayCastHit BuildShapeCastHit(const RShapeCast& InCast, const ShapeCastResult& InResult)
	{
		FRayCastHit Hit;
		Hit.QueryType = EQueryType::ShapeCast;
		Hit.Fraction = InResult.mFraction;

		// Contact point on the cast shape in world space (base offset is Vec3::sZero).
		Hit.HitPosition = PhysicsUtility::Convert(InResult.mContactPointOn1) * 100.f;
		Hit.HitNormal = PhysicsUtility::Convert(InResult.mPenetrationAxis);

		// Cast length in cm (Jolt direction vector is in meters).
		const FLOAT CastLengthCm = InCast.mDirection.Length() * 100.f;
		Hit.Distance         = Hit.Fraction * CastLengthCm;
		Hit.PenetrationDepth = InResult.mPenetrationDepth * 100.f;
		Hit.BodyId.ID        = InResult.mBodyID2;
		FillHitObjectIdentity(Hit);

		return Hit;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::RaycastSingle
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::RaycastSingle(
		const Vector3&					InOrigin,
		const Vector3&					InDirection,
		FLOAT							InLength,
		FRayCastHit&					OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter,
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetManagerSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;

		const RRayCast Ray = BuildJoltRay(InOrigin, InDirection, InLength);

		RayCastResult Result;
		const BOOL32 bHit = Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CastRay(
			Ray, Result, InBPLayerFilter, InObjLayerFilter);

		if (bHit)
			OutHit = BuildRaycastHit(Ray, Result);

		return bHit;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::RaycastMulti
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::RaycastMulti(
		const Vector3&					InOrigin,
		const Vector3&					InDirection,
		FLOAT							InLength,
		TArray<FRayCastHit>&			OutHits,
		UINT32							InMaxHits,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter,
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetManagerSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;

		const RRayCast Ray = BuildJoltRay(InOrigin, InDirection, InLength);

		AllHitCollisionCollector<CastRayCollector> Collector;
		RayCastSettings Settings;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CastRay(
			Ray, Settings, Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
			return FALSE;

		Collector.Sort();

		const UINT32 Count = EMath::Min(static_cast<UINT32>(Collector.mHits.size()), InMaxHits);
		for (UINT32 i = 0u; i < Count; ++i)
			OutHits.Add(BuildRaycastHit(Ray, Collector.mHits[i]));

		return TRUE;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::CastShapeSingle
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::CastShapeSingle(
		FShape*							InShape,
		const Vector3&					InOrigin,
		const Quaternion&				InRotation,
		const Vector3&					InDirection,
		FLOAT							InLength,
		FRayCastHit&					OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter,
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetManagerSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
		{
			return FALSE;
		}
		if (!InShape)
		{
			return FALSE;
		}

		Shape* JoltShape = InShape->CreateShape();
		if (!JoltShape)
		{
			return FALSE;
		}

		const RShapeCast ShapeCast = BuildJoltShapeCast(JoltShape, InOrigin, InDirection, InLength, InRotation);

		ShapeCastSettings Settings;
		ClosestHitCollisionCollector<CastShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CastShape(
			ShapeCast, Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
		{
			return FALSE;
		}

		OutHit = BuildShapeCastHit(ShapeCast, Collector.mHit);
		return TRUE;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::CastShapeMulti
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::CastShapeMulti(
		FShape*							InShape,
		const Vector3&					InOrigin,
		const Quaternion&				InRotation,
		const Vector3&					InDirection,
		FLOAT							InLength,
		TArray<FRayCastHit>&			OutHits,
		UINT32							InMaxHits,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter,
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetManagerSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
		{
			return FALSE;
		}
		if (!InShape)
		{
			return FALSE;
		}

		Shape* JoltShape = InShape->CreateShape();
		if (!JoltShape)
		{
			return FALSE;
		}

		const RShapeCast ShapeCast = BuildJoltShapeCast(JoltShape, InOrigin, InDirection, InLength, InRotation);

		ShapeCastSettings Settings;
		AllHitCollisionCollector<CastShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CastShape(
			ShapeCast, Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
		{
			return FALSE;
		}

		Collector.Sort();

		const UINT32 HitNum = EMath::Min(static_cast<UINT32>(Collector.mHits.size()), InMaxHits);
		for (UINT32 HitIndex = 0u; HitIndex < HitNum; HitIndex++)
		{
			OutHits.Add(BuildShapeCastHit(ShapeCast, Collector.mHits[HitIndex]));
		}

		return TRUE;
	}

	// -----------------------------------------------------------------------
	// Overlap shape helpers
	// -----------------------------------------------------------------------

	/// CharacterVirtual is not part of the broad phase. Collide the query shape
	/// against every registered character, with an AABB reject first.
	static void CollectCharacterOverlaps(
		JPH::Shape* InJoltShape,
		const JPH::Mat44& InCOMTransform,
		UINT32 InMaxHits,
		ObjectIdentityType InIgnoreObjectId,
		TArray<FOverlapResult>& OutOverlaps)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetManagerSingleton();
		if (!Manager)
		{
			return;
		}

		const JPH::AABox ShapeBounds = InJoltShape->GetWorldSpaceBounds(InCOMTransform, JPH::Vec3::sOne());
		Manager->ForEachCharacter([&](FCharacterVirtual* Character)
		{
			if (!Character || static_cast<UINT32>(OutOverlaps.Num()) >= InMaxHits)
			{
				return;
			}

			const JPH::Shape* CharacterShape = Character->GetShapeInstance();
			if (!CharacterShape)
			{
				return;
			}

			const JPH::RMat44 CharacterTransform = Character->GetCenterOfMassTransformInstance();
			const JPH::AABox CharacterBounds = CharacterShape->GetWorldSpaceBounds(CharacterTransform, JPH::Vec3::sOne());
			if (!ShapeBounds.Overlaps(CharacterBounds))
			{
				return;
			}

			CollideShapeSettings CharacterSettings;
			CharacterSettings.mMaxSeparationDistance = 0.0f;
			AllHitCollisionCollector<CollideShapeCollector> CharacterCollector;
			Character->GetTransformedShapeInstance().CollideShape(
				InJoltShape, JPH::Vec3::sOne(), InCOMTransform,
				CharacterSettings, JPH::Vec3::sZero(), CharacterCollector);

			if (!CharacterCollector.HadHit())
			{
				return;
			}

			FOverlapResult Overlap;
			Overlap.ObjectId = Character->GetObjectId();
			Overlap.Character = Character;
			if (!CharacterCollector.mHits.empty())
			{
				const CollideShapeResult& Result = CharacterCollector.mHits.front();
				Overlap.ContactPoint = PhysicsUtility::Convert(Result.mContactPointOn1) * 100.f;
				Overlap.ContactNormal = PhysicsUtility::Convert(Result.mPenetrationAxis);
				Overlap.PenetrationDepth = Result.mPenetrationDepth * 100.f;
			}

			if (InIgnoreObjectId != 0ull && Overlap.ObjectId == InIgnoreObjectId)
			{
				return;
			}

			OutOverlaps.Add(static_cast<FOverlapResult&&>(Overlap));
		});
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::OverlapShape
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::OverlapShape(
		FShape*							InShape,
		const Vector3&					InPosition,
		const Quaternion&				InRotation,
		TArray<FOverlapResult>&			OutOverlaps,
		UINT32							InMaxHits,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter,
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter,
		const FPhysicsBodyId&			InIgnoreBody,
		ObjectIdentityType				InIgnoreObjectId,
		BOOL32							InIsIncludeStaticBodies,
		BOOL32							InIsIncludeSensorBodies,
		BOOL32							InIsIncludeCharacters)
	{
		OutOverlaps.Empty();

		FPhysicsManager* Manager = FPhysicsManager::GetManagerSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
		{
			return FALSE;
		}
		if (!InShape || InMaxHits == 0u)
		{
			return FALSE;
		}

		// The shape belongs to the caller (FShape caches the Jolt instance), the
		// query never deletes it.
		JPH::Shape* JoltShape = InShape->CreateShape();
		if (!JoltShape)
		{
			return FALSE;
		}

		const JPH::Mat44 COMTransform = BuildShapeTransform(JoltShape, InPosition, InRotation);

		CollideShapeSettings Settings;
		Settings.mMaxSeparationDistance = 0.0f;
		AllHitCollisionCollector<CollideShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CollideShape(
			JoltShape, JPH::Vec3::sReplicate(1.0f), COMTransform,
			Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		const UINT32 HitNum = static_cast<UINT32>(Collector.mHits.size());
		for (UINT32 HitIndex = 0u; HitIndex < HitNum; HitIndex++)
		{
			if (OutOverlaps.Num<UINT32>() >= InMaxHits)
			{
				break;
			}

			const CollideShapeResult& Result = Collector.mHits[HitIndex];
			FPhysicsBodyId BodyId;
			BodyId.ID = Result.mBodyID2;
			if (!InIgnoreBody.IsInvalid() && BodyId == InIgnoreBody)
			{
				continue;
			}

			BOOL32 bAlreadyAdded = FALSE;
			const UINT32 ExistingNum = OutOverlaps.Num<UINT32>();
			for (UINT32 ExistingIndex = 0u; ExistingIndex < ExistingNum; ExistingIndex++)
			{
				const FOverlapResult& Existing = OutOverlaps[ExistingIndex];
				if (!Existing.IsCharacter() && Existing.BodyId == BodyId)
				{
					bAlreadyAdded = TRUE;
					break;
				}
			}
			if (bAlreadyAdded)
			{
				continue;
			}

			if (!InIsIncludeStaticBodies || !InIsIncludeSensorBodies)
			{
				BodyLockRead Lock(Manager->GetPhysicsData()->PhysicsSystem->GetBodyLockInterfaceNoLock(), BodyId.ID);
				if (!Lock.Succeeded())
				{
					continue;
				}
				const Body& OverlapBody = Lock.GetBody();
				if (!InIsIncludeStaticBodies && OverlapBody.IsStatic())
				{
					continue;
				}
				if (!InIsIncludeSensorBodies && OverlapBody.IsSensor())
				{
					continue;
				}
			}

			FOverlapResult Overlap;
			Overlap.BodyId = BodyId;
			Overlap.ContactPoint = PhysicsUtility::Convert(Result.mContactPointOn1) * 100.f;
			Overlap.ContactNormal = PhysicsUtility::Convert(Result.mPenetrationAxis);
			Overlap.PenetrationDepth = Result.mPenetrationDepth * 100.f;
			Manager->FindObjectIdentityByBodyId(BodyId, Overlap.ObjectId);

			if (InIgnoreObjectId != 0ull && Overlap.ObjectId == InIgnoreObjectId)
			{
				continue;
			}

			OutOverlaps.Add(static_cast<FOverlapResult&&>(Overlap));
		}

		if (InIsIncludeCharacters)
		{
			CollectCharacterOverlaps(JoltShape, COMTransform, InMaxHits, InIgnoreObjectId, OutOverlaps);
		}

		return (OutOverlaps.Num() > 0);
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::OverlapCharacters
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::OverlapCharacters(
		FShape*							InShape,
		const Vector3&					InPosition,
		const Quaternion&				InRotation,
		TArray<FOverlapResult>&			OutOverlaps,
		UINT32							InMaxHits,
		ObjectIdentityType				InIgnoreObjectId)
	{
		OutOverlaps.Empty();

		if (!InShape || InMaxHits == 0u)
		{
			return FALSE;
		}

		JPH::Shape* JoltShape = InShape->CreateShape();
		if (!JoltShape)
		{
			return FALSE;
		}

		const JPH::Mat44 COMTransform = BuildShapeTransform(JoltShape, InPosition, InRotation);
		CollectCharacterOverlaps(JoltShape, COMTransform, InMaxHits, InIgnoreObjectId, OutOverlaps);
		return (OutOverlaps.Num() > 0);
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::OverlapPoint
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::OverlapPoint(
		const Vector3&					InPoint,
		TArray<FRayCastHit>&			OutHits,
		UINT32							InMaxHits,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter,
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetManagerSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
		{
			return FALSE;
		}

		const JPH::Vec3 PointMeter = PhysicsUtility::Convert2Meter(InPoint);

		AllHitCollisionCollector<CollidePointCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CollidePoint(
			PointMeter, Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
		{
			return FALSE;
		}

		const UINT32 HitNum = EMath::Min(static_cast<UINT32>(Collector.mHits.size()), InMaxHits);
		for (UINT32 HitIndex = 0u; HitIndex < HitNum; HitIndex++)
		{
			FRayCastHit Hit;
			Hit.QueryType = EQueryType::PointOverlap;
			Hit.BodyId.ID = Collector.mHits[HitIndex].mBodyID;
			FillHitObjectIdentity(Hit);
			OutHits.Add(Hit);
		}

		return TRUE;
	}
}   // namespace PigeonEngine
