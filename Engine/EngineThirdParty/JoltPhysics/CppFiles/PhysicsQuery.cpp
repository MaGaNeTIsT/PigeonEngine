#include "../Headers/PhysicsQuery.h"
#include "../Headers/PhysicsManager.h"

namespace PigeonEngine
{
	static void FillHitObjectIdentity(FRayCastHit& InOutHit)
	{
		InOutHit.ObjectId = 0u;
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
		if (Manager)
		{
			Manager->FindObjectIdentityByBodyId(InOutHit.BodyId, InOutHit.ObjectId);
		}
	}

	// -----------------------------------------------------------------------
	// Internal helpers – Raycast
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

		BodyLockRead Lock(
			FPhysicsManager::GetSingleton()->GetPhysicsData()->PhysicsSystem->GetBodyLockInterfaceNoLock(),
			InResult.mBodyID);
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
	// Internal helpers – Shape Cast
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
		const JPH::Mat44 StartTransform = JPH::Mat44::sRotationTranslation(
			PhysicsUtility::Convert(InRotation),
			PhysicsUtility::Convert2Meter(InOrigin));

		const JPH::Vec3 DirNorm   = PhysicsUtility::Convert(InDirection).NormalizedOr(JPH::Vec3::sAxisZ());
		const JPH::Vec3 Direction = DirNorm * PhysicsUtility::Convert2Meter(InLength);

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
	// Internal helpers – Overlap / CollideShape
	// -----------------------------------------------------------------------

	/// Build a Jolt CollideShape center-of-mass transform (in meters) from engine-unit inputs.
	static JPH::Mat44 BuildJoltOverlapTransform(const Vector3& InCenter, const Quaternion& InRotation)
	{
		return JPH::Mat44::sRotationTranslation(
			PhysicsUtility::Convert(InRotation),
			PhysicsUtility::Convert2Meter(InCenter));
	}

	/// Populate an FRayCastHit from a Jolt CollideShapeResult.
	static FRayCastHit BuildOverlapHit(const CollideShapeResult& InResult)
	{
		FRayCastHit Hit;
		Hit.QueryType = EQueryType::Overlap;
		// Contact points relative to base offset Vec3::sZero → world space.
		Hit.HitPosition      = PhysicsUtility::Convert(InResult.mContactPointOn1) * 100.f;
		Hit.ContactOnHit     = PhysicsUtility::Convert(InResult.mContactPointOn2) * 100.f;
		Hit.HitNormal        = PhysicsUtility::Convert(InResult.mPenetrationAxis);
		Hit.PenetrationDepth = InResult.mPenetrationDepth * 100.f;
		Hit.BodyId.ID        = InResult.mBodyID2;
		FillHitObjectIdentity(Hit);
		return Hit;
	}

	/// Compute a safe convex radius for a box shape used in queries.
	/// Jolt requires convexRadius >= 0 and < each half-extent. Using 0 is valid.
	static FLOAT SafeBoxConvexRadius(const JPH::Vec3& InHalfExtentsM)
	{
		return 0.f;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::RaycastSingle
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::RaycastSingle(
		const Vector3&					InOrigin,
		const Vector3&					InDirection,
		FLOAT							InLength,
		FRayCastHit&					OutHit,
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
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
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
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
	// FPhysicsQuery::SphereCastSingle
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::SphereCastSingle(
		const Vector3&					InOrigin,
		const Vector3&					InDirection,
		FLOAT							InLength,
		FLOAT							InRadius,
		FRayCastHit&					OutHit,
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;
		if (InRadius <= 0.f)
			return FALSE;

		Ref<SphereShape> Shape = new SphereShape(PhysicsUtility::Convert2Meter(InRadius));
		const RShapeCast ShapeCast = BuildJoltShapeCast(Shape.GetPtr(), InOrigin, InDirection, InLength, Quaternion::Identity());

		ShapeCastSettings Settings;
		ClosestHitCollisionCollector<CastShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CastShape(
			ShapeCast, Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
			return FALSE;

		OutHit = BuildShapeCastHit(ShapeCast, Collector.mHit);
		return TRUE;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::SphereCastMulti
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::SphereCastMulti(
		const Vector3&					InOrigin,
		const Vector3&					InDirection,
		FLOAT							InLength,
		FLOAT							InRadius,
		TArray<FRayCastHit>&			OutHits,
		UINT32							InMaxHits,
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;
		if (InRadius <= 0.f)
			return FALSE;

		Ref<SphereShape> Shape = new SphereShape(PhysicsUtility::Convert2Meter(InRadius));
		const RShapeCast ShapeCast = BuildJoltShapeCast(Shape.GetPtr(), InOrigin, InDirection, InLength, Quaternion::Identity());

		ShapeCastSettings Settings;
		AllHitCollisionCollector<CastShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CastShape(
			ShapeCast, Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
			return FALSE;

		Collector.Sort();

		const UINT32 Count = EMath::Min(static_cast<UINT32>(Collector.mHits.size()), InMaxHits);
		for (UINT32 i = 0u; i < Count; ++i)
			OutHits.Add(BuildShapeCastHit(ShapeCast, Collector.mHits[i]));

		return TRUE;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::BoxCastSingle
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::BoxCastSingle(
		const Vector3&					InOrigin,
		const Vector3&					InDirection,
		FLOAT							InLength,
		const Vector3&					InHalfExtents,
		const Quaternion&				InRotation,
		FRayCastHit&					OutHit,
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;

		const JPH::Vec3 HalfExtM = PhysicsUtility::Convert2Meter(InHalfExtents);
		Ref<BoxShape> Shape = new BoxShape(HalfExtM, SafeBoxConvexRadius(HalfExtM));
		const RShapeCast ShapeCast = BuildJoltShapeCast(Shape.GetPtr(), InOrigin, InDirection, InLength, InRotation);

		ShapeCastSettings Settings;
		ClosestHitCollisionCollector<CastShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CastShape(
			ShapeCast, Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
			return FALSE;

		OutHit = BuildShapeCastHit(ShapeCast, Collector.mHit);
		return TRUE;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::BoxCastMulti
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::BoxCastMulti(
		const Vector3&					InOrigin,
		const Vector3&					InDirection,
		FLOAT							InLength,
		const Vector3&					InHalfExtents,
		const Quaternion&				InRotation,
		TArray<FRayCastHit>&			OutHits,
		UINT32							InMaxHits,
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;

		const JPH::Vec3 HalfExtM = PhysicsUtility::Convert2Meter(InHalfExtents);
		Ref<BoxShape> Shape = new BoxShape(HalfExtM, SafeBoxConvexRadius(HalfExtM));
		const RShapeCast ShapeCast = BuildJoltShapeCast(Shape.GetPtr(), InOrigin, InDirection, InLength, InRotation);

		ShapeCastSettings Settings;
		AllHitCollisionCollector<CastShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CastShape(
			ShapeCast, Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
			return FALSE;

		Collector.Sort();

		const UINT32 Count = EMath::Min(static_cast<UINT32>(Collector.mHits.size()), InMaxHits);
		for (UINT32 i = 0u; i < Count; ++i)
			OutHits.Add(BuildShapeCastHit(ShapeCast, Collector.mHits[i]));

		return TRUE;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::CapsuleCastSingle
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::CapsuleCastSingle(
		const Vector3&					InOrigin,
		const Vector3&					InDirection,
		FLOAT							InLength,
		FLOAT							InRadius,
		FLOAT							InHalfHeight,
		const Quaternion&				InRotation,
		FRayCastHit&					OutHit,
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;
		if (InRadius <= 0.f)
			return FALSE;

		Ref<CapsuleShape> Shape = new CapsuleShape(
			PhysicsUtility::Convert2Meter(InHalfHeight),
			PhysicsUtility::Convert2Meter(InRadius));
		const RShapeCast ShapeCast = BuildJoltShapeCast(Shape.GetPtr(), InOrigin, InDirection, InLength, InRotation);

		ShapeCastSettings Settings;
		ClosestHitCollisionCollector<CastShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CastShape(
			ShapeCast, Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
			return FALSE;

		OutHit = BuildShapeCastHit(ShapeCast, Collector.mHit);
		return TRUE;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::CapsuleCastMulti
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::CapsuleCastMulti(
		const Vector3&					InOrigin,
		const Vector3&					InDirection,
		FLOAT							InLength,
		FLOAT							InRadius,
		FLOAT							InHalfHeight,
		const Quaternion&				InRotation,
		TArray<FRayCastHit>&			OutHits,
		UINT32							InMaxHits,
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;
		if (InRadius <= 0.f)
			return FALSE;

		Ref<CapsuleShape> Shape = new CapsuleShape(
			PhysicsUtility::Convert2Meter(InHalfHeight),
			PhysicsUtility::Convert2Meter(InRadius));
		const RShapeCast ShapeCast = BuildJoltShapeCast(Shape.GetPtr(), InOrigin, InDirection, InLength, InRotation);

		ShapeCastSettings Settings;
		AllHitCollisionCollector<CastShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CastShape(
			ShapeCast, Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
			return FALSE;

		Collector.Sort();

		const UINT32 Count = EMath::Min(static_cast<UINT32>(Collector.mHits.size()), InMaxHits);
		for (UINT32 i = 0u; i < Count; ++i)
			OutHits.Add(BuildShapeCastHit(ShapeCast, Collector.mHits[i]));

		return TRUE;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::OverlapSphere
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::OverlapSphere(
		const Vector3&					InCenter,
		FLOAT							InRadius,
		TArray<FRayCastHit>&			OutHits,
		UINT32							InMaxHits,
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;
		if (InRadius <= 0.f)
			return FALSE;

		Ref<SphereShape> Shape = new SphereShape(PhysicsUtility::Convert2Meter(InRadius));
		const JPH::Mat44 Transform = BuildJoltOverlapTransform(InCenter, Quaternion::Identity());

		CollideShapeSettings Settings;
		AllHitCollisionCollector<CollideShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CollideShape(
			Shape.GetPtr(), JPH::Vec3::sReplicate(1.0f), Transform,
			Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
			return FALSE;

		const UINT32 Count = EMath::Min(static_cast<UINT32>(Collector.mHits.size()), InMaxHits);
		for (UINT32 i = 0u; i < Count; ++i)
			OutHits.Add(BuildOverlapHit(Collector.mHits[i]));

		return TRUE;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::OverlapBox
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::OverlapBox(
		const Vector3&					InCenter,
		const Vector3&					InHalfExtents,
		const Quaternion&				InRotation,
		TArray<FRayCastHit>&			OutHits,
		UINT32							InMaxHits,
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;

		const JPH::Vec3 HalfExtM = PhysicsUtility::Convert2Meter(InHalfExtents);
		Ref<BoxShape> Shape = new BoxShape(HalfExtM, SafeBoxConvexRadius(HalfExtM));
		const JPH::Mat44 Transform = BuildJoltOverlapTransform(InCenter, InRotation);

		CollideShapeSettings Settings;
		AllHitCollisionCollector<CollideShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CollideShape(
			Shape.GetPtr(), JPH::Vec3::sReplicate(1.0f), Transform,
			Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
			return FALSE;

		const UINT32 Count = EMath::Min(static_cast<UINT32>(Collector.mHits.size()), InMaxHits);
		for (UINT32 i = 0u; i < Count; ++i)
			OutHits.Add(BuildOverlapHit(Collector.mHits[i]));

		return TRUE;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::OverlapCapsule
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::OverlapCapsule(
		const Vector3&					InCenter,
		FLOAT							InRadius,
		FLOAT							InHalfHeight,
		const Quaternion&				InRotation,
		TArray<FRayCastHit>&			OutHits,
		UINT32							InMaxHits,
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;
		if (InRadius <= 0.f)
			return FALSE;

		Ref<CapsuleShape> Shape = new CapsuleShape(
			PhysicsUtility::Convert2Meter(InHalfHeight),
			PhysicsUtility::Convert2Meter(InRadius));
		const JPH::Mat44 Transform = BuildJoltOverlapTransform(InCenter, InRotation);

		CollideShapeSettings Settings;
		AllHitCollisionCollector<CollideShapeCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CollideShape(
			Shape.GetPtr(), JPH::Vec3::sReplicate(1.0f), Transform,
			Settings, JPH::Vec3::sZero(), Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
			return FALSE;

		const UINT32 Count = EMath::Min(static_cast<UINT32>(Collector.mHits.size()), InMaxHits);
		for (UINT32 i = 0u; i < Count; ++i)
			OutHits.Add(BuildOverlapHit(Collector.mHits[i]));

		return TRUE;
	}

	// -----------------------------------------------------------------------
	// FPhysicsQuery::OverlapPoint
	// -----------------------------------------------------------------------

	BOOL32 FPhysicsQuery::OverlapPoint(
		const Vector3&					InPoint,
		TArray<FRayCastHit>&			OutHits,
		UINT32							InMaxHits,
		const BroadPhaseLayerFilter&	InBPLayerFilter,
		const ObjectLayerFilter&		InObjLayerFilter)
	{
		FPhysicsManager* Manager = FPhysicsManager::GetSingleton();
		if (!Manager || !Manager->GetPhysicsData() || !Manager->GetPhysicsData()->PhysicsSystem)
			return FALSE;

		const JPH::Vec3 PointM = PhysicsUtility::Convert2Meter(InPoint);

		AllHitCollisionCollector<CollidePointCollector> Collector;
		Manager->GetPhysicsData()->PhysicsSystem->GetNarrowPhaseQuery().CollidePoint(
			PointM, Collector, InBPLayerFilter, InObjLayerFilter);

		if (!Collector.HadHit())
			return FALSE;

		const UINT32 Count = EMath::Min(static_cast<UINT32>(Collector.mHits.size()), InMaxHits);
		for (UINT32 i = 0u; i < Count; ++i)
		{
			FRayCastHit Hit;
			Hit.QueryType = EQueryType::PointOverlap;
			Hit.BodyId.ID = Collector.mHits[i].mBodyID;
			OutHits.Add(Hit);
		}

		return TRUE;
	}
}
