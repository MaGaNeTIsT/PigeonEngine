#include "EditorHelper.h"

#if _EDITOR_ONLY

#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>
#include <RenderConfig/RenderConfig.h>
#include <RenderProxy/RenderSingletonObject.h>

namespace PigeonEngine
{

	PActor* SelectObjectInViewport(const PCameraComponent* InCamera, TArray<PActor*>& InActors, const FLOAT InMouseX, const FLOAT InMouseY, const ERect& InScreenRect)
	{
		const UINT32 ActorNum = InActors.Num<UINT32>();
		const FLOAT UsedMouseX = InMouseX;
		const FLOAT UsedMouseY = InMouseY;
		if ((!InCamera) || (ActorNum == 0u) || (InScreenRect.Right <= InScreenRect.Left) || (InScreenRect.Bottom <= InScreenRect.Top))
		{
			return nullptr;
		}
		if ((UsedMouseX < InScreenRect.Left) || (UsedMouseX > InScreenRect.Right) || (UsedMouseY < InScreenRect.Top) || (UsedMouseY > InScreenRect.Bottom))
		{
			return nullptr;
		}

		Vector3 CameraLocation(InCamera->GetComponentWorldLocation());
		const EViewport& UsedViewport = InCamera->GetCameraViewInfo().Viewport;
		EFrustum UsedFrustum(InCamera->GetCameraFrustum());

		TArray<Vector2> UsedFrustumProjection;
		{
			UsedFrustum.GeneratePlaneWorldSpace(
				CameraLocation,
				InCamera->GetComponentWorldRotation()
			);
			UsedFrustum.GenerateSeparatingProjectionWorldSpace();
			UsedFrustumProjection.Add(UsedFrustum.SeparateProjectionX);
			UsedFrustumProjection.Add(UsedFrustum.SeparateProjectionY);
			UsedFrustumProjection.Add(UsedFrustum.SeparateProjectionZ);
		}

		TArray<Vector3> SeparateAxis;
		{
			SeparateAxis.Add(Vector3::XVector());
			SeparateAxis.Add(Vector3::YVector());
			SeparateAxis.Add(Vector3::ZVector());
		}
		const UINT32 SeparateAxisNum = SeparateAxis.Num<UINT32>();
		TArray<FLOAT> CameraProjection;
		for (UINT32 SeparateAxisIndex = 0u; SeparateAxisIndex < SeparateAxisNum; SeparateAxisIndex++)
		{
			CameraProjection.Add(Vector3::Dot(SeparateAxis[SeparateAxisIndex], CameraLocation));
		}

		TArray<PActor*> TargetActors;
		TArray<FLOAT> TargetDepths;
		{
			Vector2 MouseScreenCoord(UsedMouseX - InScreenRect.Left, UsedMouseY - InScreenRect.Top);
			TargetActors.Reserve(ActorNum);
			TargetDepths.Reserve(ActorNum);
			for (UINT32 i = 0u; i < ActorNum; i++)
			{
				EBoundAABB TempActorBound(InActors[i]->GetBounds());
				if (!(TempActorBound.IsValid))
				{
					continue;
				}
				TArray<Vector3> TempBoundPoints;
				TempActorBound.CalculateBoundWithSpace(
					InActors[i]->GetActorWorldLocation(),
					InActors[i]->GetActorWorldRotation(),
					InActors[i]->GetActorWorldScale(),
					TempBoundPoints
				);
				TArray<Vector2> TempProjection;
				EBoundAABB::CalculateSeparatingProjectionWorldSpace(
					TempBoundPoints,
					SeparateAxis,
					TempProjection
				);
				if (!(EBoundAABB::IsProjectionOverlap(UsedFrustumProjection, TempProjection)))
				{
					continue;
				}
				BOOL32 IsOverlapNear = TRUE;
				for (UINT32 SeparateAxisIndex = 0u; SeparateAxisIndex < SeparateAxisNum; SeparateAxisIndex++)
				{
					if (IsOverlapNear)
					{
						if ((CameraProjection[SeparateAxisIndex] < TempProjection[SeparateAxisIndex].x) || (CameraProjection[SeparateAxisIndex] > TempProjection[SeparateAxisIndex].y))
						{
							IsOverlapNear = FALSE;
						}
					}
				}
				if (IsOverlapNear)
				{
					continue;
				}
				Vector3 ActorScreenAABB[2] =
				{
					Vector3(PE_FLOAT32_MAX, PE_FLOAT32_MAX, RCommonSettings::RENDER_DEPTH_MAX),
					Vector3(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, RCommonSettings::RENDER_DEPTH_MIN)
				};
				for (UINT32 BoundPointIndex = 0u; BoundPointIndex < 8u; BoundPointIndex++)
				{
					Vector3 TempScreenCoord;
					if (!(InCamera->TransformWorldToScreen(TempBoundPoints[BoundPointIndex], TempScreenCoord)))
					{
						TempScreenCoord.z = RCommonSettings::RENDER_DEPTH_MAX;
						if (TempScreenCoord.x < UsedViewport.TopLeftX)
						{
							TempScreenCoord.x = UsedViewport.TopLeftX;
						}
						else if (TempScreenCoord.x > (UsedViewport.TopLeftX + UsedViewport.Width))
						{
							TempScreenCoord.x = UsedViewport.TopLeftX + UsedViewport.Width;
						}
						if (TempScreenCoord.y < UsedViewport.TopLeftY)
						{
							TempScreenCoord.y = UsedViewport.TopLeftY;
						}
						else if (TempScreenCoord.y > (UsedViewport.TopLeftY + UsedViewport.Height))
						{
							TempScreenCoord.y = UsedViewport.TopLeftY + UsedViewport.Height;
						}
					}
					ActorScreenAABB[0] = MinVector3(TempScreenCoord, ActorScreenAABB[0]);
					ActorScreenAABB[1] = MaxVector3(TempScreenCoord, ActorScreenAABB[1]);
				}
				if ((MouseScreenCoord.x >= ActorScreenAABB[0].x) &&
					(MouseScreenCoord.x <= ActorScreenAABB[1].x) &&
					(MouseScreenCoord.y >= ActorScreenAABB[0].y) &&
					(MouseScreenCoord.y <= ActorScreenAABB[1].y))
				{
					TargetActors.Add(InActors[i]);
					TargetDepths.Add(ActorScreenAABB[0].z);
				}
			}
		}

		PActor* Result = nullptr;
		if (const UINT32 TargetActorNum = TargetActors.Num<UINT32>(); TargetActorNum > 0u)
		{
			Result = TargetActors[0];
			FLOAT CurrentDepth = RCommonSettings::RENDER_DEPTH_MAX;
			for (UINT32 TargetActorIndex = 0u; TargetActorIndex < TargetActorNum; TargetActorIndex++)
			{
				if (TargetDepths[TargetActorIndex] < CurrentDepth)
				{
					CurrentDepth = TargetDepths[TargetActorIndex];
					Result = TargetActors[TargetActorIndex];
				}
			}
		}

		return Result;
	}

	PSceneComponent* SelectObjectInViewport(const PCameraComponent* InCamera, TArray<PSceneComponent*>& InComponents, const FLOAT InMouseX, const FLOAT InMouseY, const ERect& InScreenRect)
	{
		const UINT32 ComponentNum = InComponents.Num<UINT32>();
		const FLOAT UsedMouseX = InMouseX;
		const FLOAT UsedMouseY = InMouseY;
		
		if ((!InCamera) || (ComponentNum == 0u) || (InScreenRect.Right <= InScreenRect.Left) || (InScreenRect.Bottom <= InScreenRect.Top))
		{
			return nullptr;
		}
		if ((UsedMouseX < InScreenRect.Left) || (UsedMouseX > InScreenRect.Right) || (UsedMouseY < InScreenRect.Top) || (UsedMouseY > InScreenRect.Bottom))
		{
			return nullptr;
		}

		Vector3 CameraLocation(InCamera->GetComponentWorldLocation());
		const EViewport& UsedViewport = InCamera->GetCameraViewInfo().Viewport;
		EFrustum UsedFrustum(InCamera->GetCameraFrustum());

		TArray<Vector2> UsedFrustumProjection;
		{
			UsedFrustum.GeneratePlaneWorldSpace(
				CameraLocation,
				InCamera->GetComponentWorldRotation()
			);
			UsedFrustum.GenerateSeparatingProjectionWorldSpace();
			UsedFrustumProjection.Add(UsedFrustum.SeparateProjectionX);
			UsedFrustumProjection.Add(UsedFrustum.SeparateProjectionY);
			UsedFrustumProjection.Add(UsedFrustum.SeparateProjectionZ);
		}

		TArray<Vector3> SeparateAxis;
		{
			SeparateAxis.Add(Vector3::XVector());
			SeparateAxis.Add(Vector3::YVector());
			SeparateAxis.Add(Vector3::ZVector());
		}
		const UINT32 SeparateAxisNum = SeparateAxis.Num<UINT32>();
		TArray<FLOAT> CameraProjection;
		for (UINT32 SeparateAxisIndex = 0u; SeparateAxisIndex < SeparateAxisNum; SeparateAxisIndex++)
		{
			CameraProjection.Add(Vector3::Dot(SeparateAxis[SeparateAxisIndex], CameraLocation));
		}

		TArray<PSceneComponent*> TargetComponents;
		TArray<FLOAT> TargetDepths;
		{
			Vector2 MouseScreenCoord(UsedMouseX - InScreenRect.Left, UsedMouseY - InScreenRect.Top);
			TargetComponents.Reserve(ComponentNum);
			TargetDepths.Reserve(ComponentNum);
			for (UINT32 i = 0u; i < ComponentNum; i++)
			{
				EBoundAABB TempComponentBound(InComponents[i]->GetLocalBound());
				if (!(TempComponentBound.IsValid))
				{
					continue;
				}
				TArray<Vector3> TempBoundPoints;
				TempComponentBound.CalculateBoundWithSpace(
					InComponents[i]->GetComponentWorldLocation(),
					InComponents[i]->GetComponentWorldRotation(),
					InComponents[i]->GetComponentWorldScale(),
					TempBoundPoints
				);
				TArray<Vector2> TempProjection;
				EBoundAABB::CalculateSeparatingProjectionWorldSpace(
					TempBoundPoints,
					SeparateAxis,
					TempProjection
				);
				if (!(EBoundAABB::IsProjectionOverlap(UsedFrustumProjection, TempProjection)))
				{
					continue;
				}
				BOOL32 IsOverlapNear = TRUE;
				for (UINT32 SeparateAxisIndex = 0u; SeparateAxisIndex < SeparateAxisNum; SeparateAxisIndex++)
				{
					if (IsOverlapNear)
					{
						if ((CameraProjection[SeparateAxisIndex] < TempProjection[SeparateAxisIndex].x) || (CameraProjection[SeparateAxisIndex] > TempProjection[SeparateAxisIndex].y))
						{
							IsOverlapNear = FALSE;
						}
					}
				}
				if (IsOverlapNear)
				{
					continue;
				}
				Vector3 ComponentScreenAABB[2] =
				{
					Vector3(PE_FLOAT32_MAX, PE_FLOAT32_MAX, RCommonSettings::RENDER_DEPTH_MAX),
					Vector3(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, RCommonSettings::RENDER_DEPTH_MIN)
				};
				for (UINT32 BoundPointIndex = 0u; BoundPointIndex < 8u; BoundPointIndex++)
				{
					Vector3 TempScreenCoord;
					if (!(InCamera->TransformWorldToScreen(TempBoundPoints[BoundPointIndex], TempScreenCoord)))
					{
						TempScreenCoord.z = RCommonSettings::RENDER_DEPTH_MAX;
						if (TempScreenCoord.x < UsedViewport.TopLeftX)
						{
							TempScreenCoord.x = UsedViewport.TopLeftX;
						}
						else if (TempScreenCoord.x > (UsedViewport.TopLeftX + UsedViewport.Width))
						{
							TempScreenCoord.x = UsedViewport.TopLeftX + UsedViewport.Width;
						}
						if (TempScreenCoord.y < UsedViewport.TopLeftY)
						{
							TempScreenCoord.y = UsedViewport.TopLeftY;
						}
						else if (TempScreenCoord.y > (UsedViewport.TopLeftY + UsedViewport.Height))
						{
							TempScreenCoord.y = UsedViewport.TopLeftY + UsedViewport.Height;
						}
					}
					ComponentScreenAABB[0] = MinVector3(TempScreenCoord, ComponentScreenAABB[0]);
					ComponentScreenAABB[1] = MaxVector3(TempScreenCoord, ComponentScreenAABB[1]);
				}
				if ((MouseScreenCoord.x >= ComponentScreenAABB[0].x) &&
					(MouseScreenCoord.x <= ComponentScreenAABB[1].x) &&
					(MouseScreenCoord.y >= ComponentScreenAABB[0].y) &&
					(MouseScreenCoord.y <= ComponentScreenAABB[1].y))
				{
					TargetComponents.Add(InComponents[i]);
					TargetDepths.Add(ComponentScreenAABB[0].z);
				}
			}
		}

		PSceneComponent* Result = nullptr;
		if (const UINT32 TargetComponentNum = TargetComponents.Num<UINT32>(); TargetComponentNum > 0u)
		{
			Result = TargetComponents[0];
			FLOAT CurrentDepth = RCommonSettings::RENDER_DEPTH_MAX;
			for (UINT32 TargetComponentIndex = 0u; TargetComponentIndex < TargetComponentNum; TargetComponentIndex++)
			{
				if (TargetDepths[TargetComponentIndex] < CurrentDepth)
				{
					CurrentDepth = TargetDepths[TargetComponentIndex];
					Result = TargetComponents[TargetComponentIndex];
				}
			}
		}

		return Result;
	}

	// ---- Gizmo constants ----
	static constexpr FLOAT GIZMO_ARROW_LENGTH   = 80.0f;   // world units
	static constexpr FLOAT GIZMO_ARROW_RADIUS   = 3.0f;    // cone base radius
	static constexpr FLOAT GIZMO_HIT_RADIUS_PX  = 12.0f;   // screen-space hit tolerance in pixels

	// Returns the world-space tip of a gizmo axis arrow
	static Vector3 GetGizmoAxisTip(const PSceneComponent* InComponent, EGizmoAxis InAxis)
	{
		const Vector3 Origin = InComponent->GetComponentWorldLocation();
		switch (InAxis)
		{
		case EGizmoAxis::AXIS_X: return Origin + Vector3(GIZMO_ARROW_LENGTH, 0.f, 0.f);
		case EGizmoAxis::AXIS_Y: return Origin + Vector3(0.f, GIZMO_ARROW_LENGTH, 0.f);
		case EGizmoAxis::AXIS_Z: return Origin + Vector3(0.f, 0.f, GIZMO_ARROW_LENGTH);
		default:                 return Origin;
		}
	}

	// Returns the world-space direction of a gizmo axis
	static Vector3 GetGizmoAxisDir(EGizmoAxis InAxis)
	{
		switch (InAxis)
		{
		case EGizmoAxis::AXIS_X: return Vector3(1.f, 0.f, 0.f);
		case EGizmoAxis::AXIS_Y: return Vector3(0.f, 1.f, 0.f);
		case EGizmoAxis::AXIS_Z: return Vector3(0.f, 0.f, 1.f);
		default:                 return Vector3::Zero();
		}
	}

	void DrawTranslationGizmo(const PCameraComponent* InCamera, const PSceneComponent* InComponent)
	{
		if (!InCamera || !InComponent) { return; }

		RDebugWireframePrimitiveManager* DbgMgr = RDebugWireframePrimitiveManager::GetManagerSingleton();
		const Vector3 Origin = InComponent->GetComponentWorldLocation();

		// X axis – red
		{
			const Vector3 Tip = GetGizmoAxisTip(InComponent, EGizmoAxis::AXIS_X);
			DbgMgr->DrawSingleLine(Origin, Tip, Color4(1.f, 0.f, 0.f, 1.f));
			DbgMgr->DrawCone(Tip - Vector3(GIZMO_ARROW_RADIUS * 2.f, 0.f, 0.f), Tip, GIZMO_ARROW_RADIUS, Color4(1.f, 0.f, 0.f, 1.f));
		}
		// Y axis – green
		{
			const Vector3 Tip = GetGizmoAxisTip(InComponent, EGizmoAxis::AXIS_Y);
			DbgMgr->DrawSingleLine(Origin, Tip, Color4(0.f, 1.f, 0.f, 1.f));
			DbgMgr->DrawCone(Tip - Vector3(0.f, GIZMO_ARROW_RADIUS * 2.f, 0.f), Tip, GIZMO_ARROW_RADIUS, Color4(0.f, 1.f, 0.f, 1.f));
		}
		// Z axis – blue
		{
			const Vector3 Tip = GetGizmoAxisTip(InComponent, EGizmoAxis::AXIS_Z);
			DbgMgr->DrawSingleLine(Origin, Tip, Color4(0.f, 0.f, 1.f, 1.f));
			DbgMgr->DrawCone(Tip - Vector3(0.f, 0.f, GIZMO_ARROW_RADIUS * 2.f), Tip, GIZMO_ARROW_RADIUS, Color4(0.f, 0.f, 1.f, 1.f));
		}
	}

	// Compute screen-space distance from a point to a line segment (both in screen space)
	static FLOAT ScreenSegmentDistance(const Vector2& P, const Vector2& A, const Vector2& B)
	{
		const Vector2 AB = B - A;
		const FLOAT LenSq = AB.x * AB.x + AB.y * AB.y;
		if (LenSq < PE_SMALL_NUMBER) { return (P - A).Length(); }
		const FLOAT T = EMath::Clamp(((P.x - A.x) * AB.x + (P.y - A.y) * AB.y) / LenSq, 0.f, 1.f);
		const Vector2 Closest = A + Vector2(AB.x * T, AB.y * T);
		return (P - Closest).Length();
	}

	EGizmoAxis HitTestTranslationGizmo(
		const PCameraComponent* InCamera,
		const PSceneComponent* InComponent,
		const FLOAT InMouseX, const FLOAT InMouseY,
		const ERect& InScreenRect,
		Vector3& OutDragOriginWorld)
	{
		if (!InCamera || !InComponent) { return EGizmoAxis::NONE; }

		const EViewport& VP = InCamera->GetCameraViewInfo().Viewport;
		const EViewMatrix& VM = InCamera->GetCameraMatrix();

		const Vector2 MousePx(InMouseX, InMouseY);
		const Vector3 Origin3D = InComponent->GetComponentWorldLocation();

		Vector3 OriginScreen;
		VM.TransformWorldPointToScreen(VP, Origin3D, OriginScreen);
		const Vector2 OriginPx(OriginScreen.x, OriginScreen.y);

		EGizmoAxis BestAxis = EGizmoAxis::NONE;
		FLOAT      BestDist = GIZMO_HIT_RADIUS_PX;

		const EGizmoAxis Axes[3] = { EGizmoAxis::AXIS_X, EGizmoAxis::AXIS_Y, EGizmoAxis::AXIS_Z };
		for (INT32 i = 0; i < 3; ++i)
		{
			const Vector3 Tip3D = GetGizmoAxisTip(InComponent, Axes[i]);
			Vector3 TipScreen;
			VM.TransformWorldPointToScreen(VP, Tip3D, TipScreen);
			const Vector2 TipPx(TipScreen.x, TipScreen.y);

			const FLOAT Dist = ScreenSegmentDistance(MousePx, OriginPx, TipPx);
			if (Dist < BestDist)
			{
				BestDist = Dist;
				BestAxis = Axes[i];
			}
		}

		if (BestAxis != EGizmoAxis::NONE)
		{
			OutDragOriginWorld = Origin3D;
		}
		return BestAxis;
	}

	Vector3 ComputeGizmoDragDelta(
		const PCameraComponent* InCamera,
		const PSceneComponent* InComponent,
		EGizmoAxis InAxis,
		const Vector3& InDragOriginWorld,
		const FLOAT InMouseX, const FLOAT InMouseY,
		const ERect& InScreenRect)
	{
		if (!InCamera || !InComponent || InAxis == EGizmoAxis::NONE) { return Vector3::Zero(); }

		// Build a proper ray by unprojecting at near (z=0) and far (z=1) depth,
		// then taking the direction between them. This avoids the instability of
		// using a single mid-depth point.
		const EViewport& VP = InCamera->GetCameraViewInfo().Viewport;
		const EViewMatrix& VM = InCamera->GetCameraMatrix();

		Vector3 NearWorld, FarWorld;
		VM.TransformScreenPointToWorld(VP, Vector3(InMouseX, InMouseY, 0.f), NearWorld);
		VM.TransformScreenPointToWorld(VP, Vector3(InMouseX, InMouseY, 1.f), FarWorld);

		const Vector3 RayDir = Vector3::Normalize(FarWorld - NearWorld);
		const Vector3 RayOrigin = NearWorld;

		const Vector3 AxisDir    = GetGizmoAxisDir(InAxis);
		const Vector3 AxisOrigin = InDragOriginWorld;

		// Closest point between two lines (parametric):
		// P(t) = AxisOrigin + t * AxisDir
		// Q(s) = RayOrigin  + s * RayDir
		const Vector3 W0    = AxisOrigin - RayOrigin;
		const FLOAT   b     = Vector3::Dot(AxisDir, RayDir);
		const FLOAT   d     = Vector3::Dot(AxisDir, W0);
		const FLOAT   e     = Vector3::Dot(RayDir,  W0);
		const FLOAT   Denom = 1.f - b * b;

		if (EMath::Abs(Denom) < PE_SMALL_NUMBER) { return Vector3::Zero(); }

		const FLOAT t = (d - b * e) / Denom;
		const Vector3 ClosestOnAxis = AxisOrigin + AxisDir * t;

		return ClosestOnAxis - InDragOriginWorld;
	}

	void DrawObjectBounds(TArray<PActor*>& InObjects)
	{
		if (const UINT32 ObjectNum = InObjects.Num<UINT32>(); ObjectNum > 0u)
		{
			const Color4 DebugColor((FLOAT)(0x99) / 255.0f, (FLOAT)(0x32) / 255.0f, (FLOAT)(0xcd) / 255.0f);
			RDebugWireframePrimitiveManager* DebugPrimitiveMangaer = RDebugWireframePrimitiveManager::GetManagerSingleton();
			for (UINT32 i = 0u; i < ObjectNum; i++)
			{
				EBoundAABB TempBound(InObjects[i]->GetBounds());
				if (!(TempBound.IsValid))
				{
					continue;
				}
				const Vector3		BoundWorldLocation(InObjects[i]->GetActorWorldLocation());
				const Quaternion	BoundWorldRotation(InObjects[i]->GetActorWorldRotation());
				const Vector3		BoundWorldScaling(InObjects[i]->GetActorWorldScale());
				TArray<Vector3> BoundWorldPoints;
				TempBound.CalculateBoundWithSpace(
					BoundWorldLocation,
					BoundWorldRotation,
					BoundWorldScaling,
					BoundWorldPoints
				);
				const Vector3 BoundExtent(
					(BoundWorldPoints[1] - BoundWorldPoints[0]).Length(),
					(BoundWorldPoints[0] - BoundWorldPoints[4]).Length(),
					(BoundWorldPoints[0] - BoundWorldPoints[2]).Length()
				);
				Vector3 BoundCenter(0.f, 0.f, 0.f);
				{
					const UINT32 PointNum = BoundWorldPoints.Num<UINT32>();
					for (UINT32 PointIndex = 0u; PointIndex < PointNum; PointIndex++)
					{
						BoundCenter += BoundWorldPoints[PointIndex];
					}
					BoundCenter /= (FLOAT)PointNum;
				}
				DebugPrimitiveMangaer->DrawCuboid(BoundCenter, BoundWorldRotation,
					BoundExtent.x, BoundExtent.y, BoundExtent.z,
					DebugColor);
			}
		}
	}

};

#endif