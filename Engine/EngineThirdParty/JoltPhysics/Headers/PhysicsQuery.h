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
	Raycast      = 0,
	ShapeCast    = 1,
	PointOverlap = 2,
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
///  PointOverlap - BodyId, ObjectId
struct FRayCastHit
{
	/// Type of query that produced this hit.
	EQueryType		QueryType			= EQueryType::Raycast;

	/// World-space primary hit / contact point on the query shape (cm).
	/// Raycast   : world-space hit position.
	/// ShapeCast : contact point on the cast shape.
	Vector3			HitPosition;

	/// World-space surface normal (Raycast) or penetration axis (ShapeCast),
	/// pointing from the hit body toward the query shape.
	Vector3			HitNormal;

	/// Fraction along the ray / cast direction [0, 1] at first contact
	/// (Raycast and ShapeCast only).
	FLOAT			Fraction			= 0.f;

	/// Distance from the origin to the first contact (cm).
	/// (Raycast and ShapeCast only).
	FLOAT			Distance			= 0.f;

	/// Penetration depth at the contact point (cm).
	/// (ShapeCast only).
	FLOAT			PenetrationDepth	= 0.f;

	/// Physics body ID of the hit body.
	FPhysicsBodyId	BodyId;

	/// Object identity of the hit actor.
	ObjectIdentityType ObjectId = 0u;

	/// Returns TRUE when this result represents a valid hit.
	inline BOOL32 IsValid() const { return !BodyId.IsInvalid(); }
};

class FCharacterVirtual;

/// Result of a shape overlap test that can contain rigid bodies and virtual
/// characters. Bodies carry a BodyId; characters have an invalid BodyId and set
/// Character instead (CharacterVirtual is not part of the broad phase, so a
/// regular physics query cannot see it).
struct FOverlapResult
{
	/// Overlapping rigid body (invalid for a character overlap).
	FPhysicsBodyId		BodyId;

	/// Identity of the overlapping object: the component ID for a rigid body (a
	/// PPhysicsComponent or a trigger's sensor body), the actor ID for a character.
	/// IDs are globally unique across actors and components, so resolve against the
	/// world's component registry first and the actor registry second.
	ObjectIdentityType	ObjectId = 0ull;

	/// Non-null when the overlap is a CharacterVirtual.
	const FCharacterVirtual* Character = nullptr;

	/// World-space contact information (cm). Zero for characters if the
	/// collision collector produced no usable point.
	Vector3				ContactPoint = Vector3::Zero();
	Vector3				ContactNormal = Vector3::Zero();
	FLOAT				PenetrationDepth = 0.f;

	inline BOOL32 IsCharacter() const { return Character != nullptr; }
	inline BOOL32 IsValid() const { return (Character != nullptr) || !BodyId.IsInvalid(); }
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
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FPhysicsDefaultBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FPhysicsDefaultQueryLayerFilter::Get());

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
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FPhysicsDefaultBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FPhysicsDefaultQueryLayerFilter::Get());

	// ------------------------------------------------------------
	//  Shape Cast (swept arbitrary shape)
	// ------------------------------------------------------------

	/// Sweep an engine shape along a direction and return the closest hit.
	///
	/// This is the generic form of the former sphere/box/capsule casts: the
	/// caller supplies an FShape (box, sphere, capsule, cylinder or compound).
	/// CharacterVirtual is not part of the broad phase and is not returned.
	///
	/// The shape is owned by the caller and is cached by FShape::CreateShape();
	/// the query never takes ownership of it. InOrigin is the shape origin; the
	/// center-of-mass offset inside the shape is applied by the query.
	///
	/// @param InShape          Engine shape to sweep.
	/// @param InOrigin         World-space origin of the shape in centimeters.
	/// @param InRotation       World-space orientation at the start of the sweep.
	/// @param InDirection      Sweep direction (does NOT need to be normalised).
	/// @param InLength         Sweep length in centimeters.
	/// @param OutHit           Filled when the function returns TRUE.
	/// @return TRUE if any geometry was hit.
	static BOOL32 CastShapeSingle(
		FShape* InShape,
		const Vector3& InOrigin,
		const Quaternion& InRotation,
		const Vector3& InDirection,
		FLOAT InLength,
		FRayCastHit& OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FPhysicsDefaultBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FPhysicsDefaultQueryLayerFilter::Get());

	/// Sweep an engine shape and collect ALL hits up to InMaxHits.
	/// Hits are appended to OutHits and sorted nearest-first.
	/// @param OutHits          Appended with every hit, sorted nearest-first.
	/// @param InMaxHits        Maximum number of hits to append.
	static BOOL32 CastShapeMulti(
		FShape* InShape,
		const Vector3& InOrigin,
		const Quaternion& InRotation,
		const Vector3& InDirection,
		FLOAT InLength,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FPhysicsDefaultBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FPhysicsDefaultQueryLayerFilter::Get());

	// ------------------------------------------------------------
	//  Overlap (shape test, no sweep)
	// ------------------------------------------------------------

	/// Test an arbitrary engine shape for overlapping bodies and characters.
	///
	/// This is the general spatial detection entry point: AI perception, scene
	/// queries and PTriggerComponent's query backend all use it. CharacterVirtual
	/// is not in the broad phase and is not returned by the body pass; the
	/// optional character pass walks FPhysics_Jolt's character registry and tests
	/// each character's shape against InShape.
	///
	/// The shape is owned by the caller and is cached by FShape::CreateShape();
	/// the query never takes ownership of it.
	///
	/// @param InShape              Engine shape to test (box/sphere/capsule/compound).
	/// @param InPosition           World-space origin of the shape in centimeters.
	/// @param InRotation           World-space orientation of the shape.
	/// @param OutOverlaps          Every overlapping body / character (unordered).
	/// @param InMaxHits            Maximum number of results to collect.
	/// @param InBPLayerFilter      Optional broad-phase layer filter.
	/// @param InObjLayerFilter     Optional object-layer filter.
	/// @param InIgnoreBody         Body to ignore (e.g. the query owner).
	/// @param InIgnoreObjectId     Actor identity to ignore (e.g. the query owner).
	/// @param InIsIncludeStaticBodies Include static bodies in the result.
	/// @param InIsIncludeSensorBodies Include sensor bodies (e.g. other trigger volumes).
	/// @param InIsIncludeCharacters   Run the CharacterVirtual pass.
	/// @return TRUE if at least one overlap was found.
	static BOOL32 OverlapShape(
		FShape* InShape,
		const Vector3& InPosition,
		const Quaternion& InRotation,
		TArray<FOverlapResult>& OutOverlaps,
		UINT32 InMaxHits = 64u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FPhysicsDefaultBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FPhysicsDefaultQueryLayerFilter::Get(),
		const FPhysicsBodyId& InIgnoreBody = FPhysicsBodyId(),
		ObjectIdentityType InIgnoreObjectId = 0ull,
		BOOL32 InIsIncludeStaticBodies = TRUE,
		BOOL32 InIsIncludeSensorBodies = FALSE,
		BOOL32 InIsIncludeCharacters = TRUE);

	/// Test an arbitrary engine shape against registered CharacterVirtual only.
	/// Used by trigger backends that already receive rigid-body contacts through
	/// the physics contact pipeline (sensor mode) and only need the character pass.
	static BOOL32 OverlapCharacters(
		FShape* InShape,
		const Vector3& InPosition,
		const Quaternion& InRotation,
		TArray<FOverlapResult>& OutOverlaps,
		UINT32 InMaxHits = 16u,
		ObjectIdentityType InIgnoreObjectId = 0ull);

	// ------------------------------------------------------------
	//  Point Overlap
	// ------------------------------------------------------------

	/// Test whether a world-space point lies inside any physics body.
	/// This is a CollidePoint query, not a shape overlap; CharacterVirtual is
	/// not part of the broad phase and is not returned.
	/// @param InPoint          World-space point in centimeters.
	/// @param OutHits          Every body that contains the point.
	/// @param InMaxHits        Maximum number of results to collect.
	/// @return TRUE if the point is inside at least one body.
	static BOOL32 OverlapPoint(
		const Vector3& InPoint,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FPhysicsDefaultBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FPhysicsDefaultQueryLayerFilter::Get());
};

PIGEONENGINE_NAMESPACE_END
