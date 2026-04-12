#pragma once
#include <CoreMinimal.h>
#include "JoltPhysics.h"
#include "PhysicsQuery.h"

PIGEONENGINE_NAMESPACE_BEGIN
typedef FPhysics_Jolt FPhysicsInterface;

class FPhysicsManager final : public FPhysicsInterface
{
	CLASS_SINGLETON_BODY(FPhysicsManager)
public:
	static void Initialize() {}
	static void ShutDown() {}
	static void StaticUpdate() {}
public:
	static void Init() { GetSingleton()->InitPhysics(); }
	static void Uninit() { GetSingleton()->UninitPhysics(); }
	static void Update() {}
	static void SetLayerConfig(const FPhysicsLayerConfig& InLayerConfig) { GetSingleton()->SetLayerConfig(InLayerConfig); }
	static void FixedUpdate(FLOAT DeltaTime)
	{
		GetSingleton()->PrePhysicsUpdate();
		GetSingleton()->PhysicsUpdate(DeltaTime);
		GetSingleton()->PostPhysicsUpdate();
	}
	static void Draw() {}

	// ---- Raycast ----

	static BOOL32 RaycastSingle(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		FRayCastHit& OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::RaycastSingle(InOrigin, InDirection, InLength, OutHit, InBPLayerFilter, InObjLayerFilter);
	}

	static BOOL32 RaycastMulti(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::RaycastMulti(InOrigin, InDirection, InLength, OutHits, InMaxHits, InBPLayerFilter, InObjLayerFilter);
	}

	// ---- Sphere Cast ----

	static BOOL32 SphereCastSingle(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		FLOAT InRadius,
		FRayCastHit& OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::SphereCastSingle(InOrigin, InDirection, InLength, InRadius, OutHit, InBPLayerFilter, InObjLayerFilter);
	}

	static BOOL32 SphereCastMulti(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		FLOAT InRadius,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::SphereCastMulti(InOrigin, InDirection, InLength, InRadius, OutHits, InMaxHits, InBPLayerFilter, InObjLayerFilter);
	}

	// ---- Box Cast ----

	static BOOL32 BoxCastSingle(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		const Vector3& InHalfExtents,
		const Quaternion& InRotation,
		FRayCastHit& OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::BoxCastSingle(InOrigin, InDirection, InLength, InHalfExtents, InRotation, OutHit, InBPLayerFilter, InObjLayerFilter);
	}

	static BOOL32 BoxCastMulti(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		const Vector3& InHalfExtents,
		const Quaternion& InRotation,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::BoxCastMulti(InOrigin, InDirection, InLength, InHalfExtents, InRotation, OutHits, InMaxHits, InBPLayerFilter, InObjLayerFilter);
	}

	// ---- Capsule Cast ----

	static BOOL32 CapsuleCastSingle(
		const Vector3& InOrigin,
		const Vector3& InDirection,
		FLOAT InLength,
		FLOAT InRadius,
		FLOAT InHalfHeight,
		const Quaternion& InRotation,
		FRayCastHit& OutHit,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::CapsuleCastSingle(InOrigin, InDirection, InLength, InRadius, InHalfHeight, InRotation, OutHit, InBPLayerFilter, InObjLayerFilter);
	}

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
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::CapsuleCastMulti(InOrigin, InDirection, InLength, InRadius, InHalfHeight, InRotation, OutHits, InMaxHits, InBPLayerFilter, InObjLayerFilter);
	}

	// ---- Overlap ----

	static BOOL32 OverlapSphere(
		const Vector3& InCenter,
		FLOAT InRadius,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::OverlapSphere(InCenter, InRadius, OutHits, InMaxHits, InBPLayerFilter, InObjLayerFilter);
	}

	static BOOL32 OverlapBox(
		const Vector3& InCenter,
		const Vector3& InHalfExtents,
		const Quaternion& InRotation,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::OverlapBox(InCenter, InHalfExtents, InRotation, OutHits, InMaxHits, InBPLayerFilter, InObjLayerFilter);
	}

	static BOOL32 OverlapCapsule(
		const Vector3& InCenter,
		FLOAT InRadius,
		FLOAT InHalfHeight,
		const Quaternion& InRotation,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::OverlapCapsule(InCenter, InRadius, InHalfHeight, InRotation, OutHits, InMaxHits, InBPLayerFilter, InObjLayerFilter);
	}

	// ---- Point Overlap ----

	static BOOL32 OverlapPoint(
		const Vector3& InPoint,
		TArray<FRayCastHit>& OutHits,
		UINT32 InMaxHits = 16u,
		const FPhysicsBroadPhaseLayerFilterBase& InBPLayerFilter = FDefaultPhysicsBroadPhaseLayerFilter::Get(),
		const FPhysicsQueryLayerFilterBase& InObjLayerFilter = FDefaultPhysicsQueryLayerFilter::Get())
	{
		return FPhysicsQuery::OverlapPoint(InPoint, OutHits, InMaxHits, InBPLayerFilter, InObjLayerFilter);
	}
#if _EDITOR_ONLY
public:
	static void EditorUpdate() {}
private:
	BOOL32 m_EditorOpen = FALSE;
#endif
};

PIGEONENGINE_NAMESPACE_END