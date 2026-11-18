#pragma once
#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/ObjectBase.h>
#include "JoltPhysics.h"
#include "PhysicsQuery.h"

PIGEONENGINE_NAMESPACE_BEGIN
using FPhysicsInterface = FPhysics_Jolt;

class FPhysicsManager final : public EManagerBase, public FPhysicsInterface
{
	CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(FPhysicsManager)
public:
	void Initialize() override;
	void ShutDown() override;
	void Init();
	void Uninit();
	void Update();
	void FixedUpdate(FLOAT DeltaTime);
	void Draw();

	// ---- Raycast ----

	static BOOL32 RaycastSingle(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		FRayCastHit& OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FPhysicsDefaultBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FPhysicsDefaultQueryLayerFilter::Get())
	{
		return FPhysicsQuery::RaycastSingle(InOrigin, InDirection, InLength, OutHit, InBPLayerFilter, InObjLayerFilter);
	}

	static BOOL32 RaycastMulti(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FPhysicsDefaultBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FPhysicsDefaultQueryLayerFilter::Get())
	{
		return FPhysicsQuery::RaycastMulti(InOrigin, InDirection, InLength, OutHits, InMaxHits, InBPLayerFilter, InObjLayerFilter);
	}

	// ---- Shape Cast ----

	static BOOL32 CastShapeSingle(
		FShape* InShape,
		const Vector3& InOrigin,
		const Quaternion& InRotation,
		const Vector3& InDirection,
		FLOAT InLength,
		FRayCastHit& OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FPhysicsDefaultBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FPhysicsDefaultQueryLayerFilter::Get())
	{
		return FPhysicsQuery::CastShapeSingle(InShape, InOrigin, InRotation, InDirection, InLength, OutHit, InBPLayerFilter, InObjLayerFilter);
	}

	static BOOL32 CastShapeMulti(
		FShape* InShape,
		const Vector3& InOrigin,
		const Quaternion& InRotation,
		const Vector3& InDirection,
		FLOAT InLength,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FPhysicsDefaultBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FPhysicsDefaultQueryLayerFilter::Get())
	{
		return FPhysicsQuery::CastShapeMulti(InShape, InOrigin, InRotation, InDirection, InLength, OutHits, InMaxHits, InBPLayerFilter, InObjLayerFilter);
	}

	// ---- Shape Overlap (bodies + characters) ----

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
		BOOL32 InIsIncludeCharacters = TRUE)
	{
		return FPhysicsQuery::OverlapShape(InShape, InPosition, InRotation, OutOverlaps, InMaxHits, InBPLayerFilter, InObjLayerFilter, InIgnoreBody, InIgnoreObjectId, InIsIncludeStaticBodies, InIsIncludeSensorBodies, InIsIncludeCharacters);
	}

	static BOOL32 OverlapCharacters(
		FShape* InShape,
		const Vector3& InPosition,
		const Quaternion& InRotation,
		TArray<FOverlapResult>& OutOverlaps,
		UINT32 InMaxHits = 16u,
		ObjectIdentityType InIgnoreObjectId = 0ull)
	{
		return FPhysicsQuery::OverlapCharacters(InShape, InPosition, InRotation, OutOverlaps, InMaxHits, InIgnoreObjectId);
	}

	// ---- Point Overlap ----

	static BOOL32 OverlapPoint(
		const Vector3& InPoint,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FPhysicsDefaultBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FPhysicsDefaultQueryLayerFilter::Get())
	{
		return FPhysicsQuery::OverlapPoint(InPoint, OutHits, InMaxHits, InBPLayerFilter, InObjLayerFilter);
	}
};

PIGEONENGINE_NAMESPACE_END
