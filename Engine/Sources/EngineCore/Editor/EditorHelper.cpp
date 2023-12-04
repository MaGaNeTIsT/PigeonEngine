#include "EditorHelper.h"
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>
#include <RenderConfig/RenderConfig.h>

namespace PigeonEngine
{

	PActor* SelectObjectInViewport(const PCameraComponent* InCamera, TArray<PActor*>& InActors, const INT32 InMouseX, const INT32 InMouseY, const ERect& InScreenRect)
	{
		const UINT32 ActorNum = InActors.Length();
		const FLOAT UsedMouseX = (FLOAT)InMouseX;
		const FLOAT UsedMouseY = (FLOAT)InMouseY;

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

		TArray<PActor*> TargetActors;
		TArray<FLOAT> TargetDepths;
		{
			Vector2 MouseScreenCoord(UsedMouseX - InScreenRect.Left, UsedMouseY - InScreenRect.Top);
			TargetActors.Recapacity(ActorNum);
			TargetDepths.Recapacity(ActorNum);
			TArray<Vector3> SeparateAxis;
			{
				SeparateAxis.Add(Vector3::XVector());
				SeparateAxis.Add(Vector3::YVector());
				SeparateAxis.Add(Vector3::ZVector());
			}
			for (UINT32 i = 0u; i < ActorNum; i++)
			{
				EBoundAABB TempActorBound(InActors[i]->GetBounds());
				if (!(TempActorBound.IsValid))
				{
					continue;
				}
				TArray<Vector3> TempBoundPoints;
				TempActorBound.CalculateBoundWorldSpace(
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
				for (UINT32 SeparateAxisIndex = 0u, SeparateAxisNum = SeparateAxis.Length(); SeparateAxisIndex < SeparateAxisNum; SeparateAxisIndex++)
				{
					if (IsOverlapNear)
					{
						const FLOAT PointProjection = Vector3::Dot(SeparateAxis[SeparateAxisIndex], CameraLocation);
						if ((PointProjection < TempProjection[SeparateAxisIndex].x) || (PointProjection > TempProjection[SeparateAxisIndex].y))
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
		if (const UINT32 TargetActorNum = TargetActors.Length(); TargetActorNum > 0u)
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

};