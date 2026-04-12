#pragma once
#include <CoreMinimal.h>
#include "JoltIncludes.h"
#include "PhysicsUtility.h"
#include "JoltPhysics.h"

PIGEONENGINE_NAMESPACE_BEGIN

// ============================================================
//  Query type enum
// ============================================================

enum class EQueryType : UINT8
{
	Raycast     = 0,
	ShapeCast   = 1,
	Overlap     = 2,
	PointOverlap = 3,
};

// ============================================================
//  Unified hit result
// ============================================================

/// Unified result struct returned by every physics query.
/// Which fields are populated depends on QueryType:
///
///  Raycast      - HitPosition, HitNormal, Fraction, Distance, BodyId
///  ShapeCast    - HitPosition, HitNormal, Fraction, Distance,
///                 PenetrationDepth, BodyId
///  Overlap      - HitPosition, ContactOnHit, HitNormal,
///                 PenetrationDepth, BodyId
///  PointOverlap - BodyId only
struct FRayCastHit
{
	/// Type of query that produced this hit.
	EQueryType		QueryType			= EQueryType::Raycast;

	/// World-space primary hit / contact point on the query shape (cm).
	/// Raycast   : world-space hit position.
	/// ShapeCast : contact point on the cast shape.
	/// Overlap   : contact point on the query shape (ContactOnQuery).
	Vector3			HitPosition;

	/// World-space surface normal (Raycast) or penetration axis
	/// (ShapeCast / Overlap), pointing from the hit body toward the query shape.
	Vector3			HitNormal;

	/// World-space contact point on the overlapping body (Overlap only).
	Vector3			ContactOnHit;

	/// Fraction along the ray / cast direction [0, 1] at first contact
	/// (Raycast and ShapeCast only).
	FLOAT			Fraction			= 0.f;

	/// Distance from the origin to the first contact (cm).
	/// (Raycast and ShapeCast only).
	FLOAT			Distance			= 0.f;

	/// Penetration depth at the contact point (cm).
	/// (ShapeCast and Overlap only).
	FLOAT			PenetrationDepth	= 0.f;

	/// Physics body ID of the hit / overlapping body.
	FPhysicsBodyId	BodyId;

	/// Object identity of the hit / overlapping actor.
	ObjectIdentityType ObjectId = 0u;

	/// Returns TRUE when this result represents a valid hit.
	inline BOOL32 IsValid() const { return !BodyId.IsInvalid(); }
};

// ============================================================
//  FPhysicsQuery
// ============================================================

/// Physics query helpers.
/// All positions / directions / sizes must be in engine units (centimeters).
/// Layer filtering defaults to ALL layers (pass custom engine layer filters to
/// restrict the query).
class FPhysicsQuery
{
public:
	// ------------------------------------------------------------
	//  Raycast
	// ------------------------------------------------------------

	/// Cast a ray and return the closest hit.
	/// @param InOrigin         Ray origin in centimeters.
	/// @param InDirection      Ray direction (does NOT need to be normalised).
	/// @param InLength         Ray length in centimeters.
	/// @param OutHit           Filled when the function returns TRUE.
	/// @param InBPLayerFilter  Optional broad-phase layer filter.
	/// @param InObjLayerFilter Optional object-layer filter.
	/// @return TRUE if any geometry was hit.
	static BOOL32 RaycastSingle(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		FRayCastHit& OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());

	/// Cast a ray and collect ALL hits up to MaxHits.
	/// @param InOrigin         Ray origin in centimeters.
	/// @param InDirection      Ray direction (does NOT need to be normalised).
	/// @param InLength         Ray length in centimeters.
	/// @param OutHits          Filled with every hit, sorted nearest-first.
	/// @param InMaxHits        Maximum number of hits to collect.
	/// @param InBPLayerFilter  Optional broad-phase layer filter.
	/// @param InObjLayerFilter Optional object-layer filter.
	/// @return TRUE if at least one hit was found.
	static BOOL32 RaycastMulti(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());

	// ------------------------------------------------------------
	//  Sphere Cast (swept sphere)
	// ------------------------------------------------------------

	/// Sweep a sphere along a direction and return the closest hit.
	/// @param InOrigin         Sphere center origin in centimeters.
	/// @param InDirection      Sweep direction (does NOT need to be normalised).
	/// @param InLength         Sweep length in centimeters.
	/// @param InRadius         Sphere radius in centimeters.
	/// @param OutHit           Filled when the function returns TRUE.
	static BOOL32 SphereCastSingle(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		FLOAT InRadius,
		FRayCastHit& OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());

	/// Sweep a sphere along a direction and collect ALL hits up to MaxHits.
	/// @param InOrigin         Sphere center origin in centimeters.
	/// @param InDirection      Sweep direction (does NOT need to be normalised).
	/// @param InLength         Sweep length in centimeters.
	/// @param InRadius         Sphere radius in centimeters.
	/// @param OutHits          Sorted nearest-first.
	/// @param InMaxHits        Maximum number of hits to collect.
	static BOOL32 SphereCastMulti(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		FLOAT InRadius,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());

	// ------------------------------------------------------------
	//  Box Cast (swept box)
	// ------------------------------------------------------------

	/// Sweep a box along a direction and return the closest hit.
	/// @param InOrigin         Box center origin in centimeters.
	/// @param InDirection      Sweep direction (does NOT need to be normalised).
	/// @param InLength         Sweep length in centimeters.
	/// @param InHalfExtents    Box half-extents in centimeters (X, Y, Z).
	/// @param InRotation       Box orientation at the start of the sweep.
	/// @param OutHit           Filled when the function returns TRUE.
	static BOOL32 BoxCastSingle(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		const Vector3& InHalfExtents,
		const Quaternion& InRotation,
		FRayCastHit& OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());

	/// Sweep a box along a direction and collect ALL hits up to MaxHits.
	/// @param InOrigin         Box center origin in centimeters.
	/// @param InDirection      Sweep direction (does NOT need to be normalised).
	/// @param InLength         Sweep length in centimeters.
	/// @param InHalfExtents    Box half-extents in centimeters (X, Y, Z).
	/// @param InRotation       Box orientation at the start of the sweep.
	/// @param OutHits          Sorted nearest-first.
	/// @param InMaxHits        Maximum number of hits to collect.
	static BOOL32 BoxCastMulti(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		const Vector3& InHalfExtents,
		const Quaternion& InRotation,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());

	// ------------------------------------------------------------
	//  Capsule Cast (swept capsule)
	// ------------------------------------------------------------

	/// Sweep a capsule along a direction and return the closest hit.
	/// @param InOrigin         Capsule center origin in centimeters.
	/// @param InDirection      Sweep direction (does NOT need to be normalised).
	/// @param InLength         Sweep length in centimeters.
	/// @param InRadius         Capsule radius in centimeters.
	/// @param InHalfHeight     Half-height of the cylindrical part in centimeters.
	/// @param InRotation       Capsule orientation at the start of the sweep.
	/// @param OutHit           Filled when the function returns TRUE.
	static BOOL32 CapsuleCastSingle(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		FLOAT InRadius,
		FLOAT InHalfHeight,
		const Quaternion& InRotation,
		FRayCastHit& OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());

	/// Sweep a capsule along a direction and collect ALL hits up to MaxHits.
	/// @param InOrigin         Capsule center origin in centimeters.
	/// @param InDirection      Sweep direction (does NOT need to be normalised).
	/// @param InLength         Sweep length in centimeters.
	/// @param InRadius         Capsule radius in centimeters.
	/// @param InHalfHeight     Half-height of the cylindrical part in centimeters.
	/// @param InRotation       Capsule orientation at the start of the sweep.
	/// @param OutHits          Sorted nearest-first.
	/// @param InMaxHits        Maximum number of hits to collect.
	static BOOL32 CapsuleCastMulti(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		FLOAT InRadius,
		FLOAT InHalfHeight,
		const Quaternion& InRotation,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());

	// ------------------------------------------------------------
	//  Overlap (static shape test, no sweep)
	// ------------------------------------------------------------

	/// Test a sphere for overlapping bodies.
	/// @param InCenter         Sphere center in centimeters.
	/// @param InRadius         Sphere radius in centimeters.
	/// @param OutHits          Every overlapping body (unordered).
	/// @param InMaxHits        Maximum number of overlaps to collect.
	/// @return TRUE if at least one overlap was found.
	static BOOL32 OverlapSphere(
		const Vector3& InCenter,
		FLOAT InRadius,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());

	/// Test a box for overlapping bodies.
	/// @param InCenter         Box center in centimeters.
	/// @param InHalfExtents    Box half-extents in centimeters (X, Y, Z).
	/// @param InRotation       Box orientation.
	/// @param OutHits          Every overlapping body (unordered).
	/// @param InMaxHits        Maximum number of overlaps to collect.
	/// @return TRUE if at least one overlap was found.
	static BOOL32 OverlapBox(
		const Vector3& InCenter,
		const Vector3& InHalfExtents,
		const Quaternion& InRotation,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());

	/// Test a capsule for overlapping bodies.
	/// @param InCenter         Capsule center in centimeters.
	/// @param InRadius         Capsule radius in centimeters.
	/// @param InHalfHeight     Half-height of the cylindrical part in centimeters.
	/// @param InRotation       Capsule orientation.
	/// @param OutHits          Every overlapping body (unordered).
	/// @param InMaxHits        Maximum number of overlaps to collect.
	/// @return TRUE if at least one overlap was found.
	static BOOL32 OverlapCapsule(
		const Vector3& InCenter,
		FLOAT InRadius,
		FLOAT InHalfHeight,
		const Quaternion& InRotation,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());

	// ------------------------------------------------------------
	//  Point Overlap
	// ------------------------------------------------------------

	/// Test whether a world-space point lies inside any physics body.
	/// @param InPoint          World-space point in centimeters.
	/// @param OutBodyIds       Every body that contains the point.
	/// @param InMaxHits        Maximum number of results to collect.
	/// @return TRUE if the point is inside at least one body.
	static BOOL32 OverlapPoint(
		const Vector3& InPoint,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get());
};

PIGEONENGINE_NAMESPACE_END
