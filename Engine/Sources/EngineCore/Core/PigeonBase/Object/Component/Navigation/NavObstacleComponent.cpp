#include "NavObstacleComponent.h"

#include <Editor/EditorLogManager.h>
#include <Navigation/NavigationManager.h>
#include <PigeonBase/Object/Actor.h>

#if _EDITOR_ONLY
#include <RenderProxy/RenderSingletonObject.h>
#include <imgui.h>
#endif

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PNavObstacleComponent, PSceneComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PNavObstacleComponent::PNavObstacleComponent()
	{
		POBJ_DEBUGNAME_SET(this, "NavObstacleComponent");
		// The obstacle follows its transform, so it needs a tick of its own.
		SetIsTickable(TRUE);
	}

	PNavObstacleComponent::~PNavObstacleComponent()
	{
	}

	ENavigationObstacleShapeType PNavObstacleComponent::GetShapeType() const
	{
		return ShapeType;
	}

	void PNavObstacleComponent::SetShapeType(const ENavigationObstacleShapeType& InShapeType)
	{
		if (ShapeType == InShapeType)
		{
			return;
		}
		// The shape changed, so the registered obstacle no longer describes it.
		UnregisterObstacle();
		ShapeType = InShapeType;
	}

	FLOAT PNavObstacleComponent::GetCylinderRadius() const
	{
		return CylinderRadius;
	}

	void PNavObstacleComponent::SetCylinderRadius(const FLOAT& InRadius)
	{
		CylinderRadius = EMath::Max(InRadius, 1.0f);
		UnregisterObstacle();
	}

	FLOAT PNavObstacleComponent::GetCylinderHeight() const
	{
		return CylinderHeight;
	}

	void PNavObstacleComponent::SetCylinderHeight(const FLOAT& InHeight)
	{
		CylinderHeight = EMath::Max(InHeight, 1.0f);
		UnregisterObstacle();
	}

	const Vector3& PNavObstacleComponent::GetBoxHalfExtents() const
	{
		return BoxHalfExtents;
	}

	void PNavObstacleComponent::SetBoxHalfExtents(const Vector3& InHalfExtents)
	{
		BoxHalfExtents = Vector3(EMath::Max(InHalfExtents.x, 1.0f), EMath::Max(InHalfExtents.y, 1.0f), EMath::Max(InHalfExtents.z, 1.0f));
		UnregisterObstacle();
	}

	void PNavObstacleComponent::BeginAddedToScene(PWorld* InWorld)
	{
		PSceneComponent::BeginAddedToScene(InWorld);
		RegisterObstacle();
	}

	void PNavObstacleComponent::RemovedFromScene()
	{
		UnregisterObstacle();
		PSceneComponent::RemovedFromScene();
	}

	void PNavObstacleComponent::RegisterObstacle()
	{
		if (ObstacleId != NavigationInvalidObstacleHandle)
		{
			return;
		}
		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		const Vector3 Center = GetComponentWorldLocation();
		const FLOAT YawDegrees = MakeEuler(GetComponentWorldRotation()).Yaw;

		ENavigationObstacleHandle NewId = NavigationInvalidObstacleHandle;
		if (ShapeType == ENavigationObstacleShapeType::NAVIGATION_OBSTACLE_BOX)
		{
			NewId = Manager->AddBoxObstacle(GetUniqueID(), Center, BoxHalfExtents, EMath::DegreesToRadians(YawDegrees));
		}
		else
		{
			NewId = Manager->AddCylinderObstacle(GetUniqueID(), Center, CylinderRadius, CylinderHeight);
		}

		if (NewId == NavigationInvalidObstacleHandle)
		{
			if (!bWarnedUnsupported)
			{
				// The manager refuses only when no volume is ready; the usual cause is a single tile
				// bake, which has no tile cache to rebuild. Say it once.
				bWarnedUnsupported = TRUE;
				PE_LOG_WARN(EString(PE_TEXT("Navigation: obstacle registration was refused; the volume needs Tile Size (cells) > 0 to support dynamic obstacles.")));
			}
			return;
		}

		ObstacleId = NewId;
		LastCenter = Center;
		LastYawDegrees = YawDegrees;
		bWarnedUnsupported = FALSE;
	}

	void PNavObstacleComponent::UnregisterObstacle()
	{
		if (ObstacleId == NavigationInvalidObstacleHandle)
		{
			return;
		}
		ENavigationManager::GetManagerSingleton()->RemoveObstacle(GetUniqueID(), ObstacleId);
		ObstacleId = NavigationInvalidObstacleHandle;
	}

	void PNavObstacleComponent::Tick(FLOAT InDeltaTime)
	{
		PSceneComponent::Tick(InDeltaTime);

		if (!bFollowTransform)
		{
			return;
		}

		// Retried here, because the volume may not have been baked when this component was added.
		if (ObstacleId == NavigationInvalidObstacleHandle)
		{
			RegisterObstacle();
			return;
		}

		const Vector3 Center = GetComponentWorldLocation();
		const FLOAT YawDegrees = MakeEuler(GetComponentWorldRotation()).Yaw;
		const FLOAT Moved = Vector3::Distance(Center, LastCenter);
		const FLOAT YawDelta = EMath::Abs(Euler::NormalizeAxis(YawDegrees - LastYawDegrees));
		if ((Moved <= FollowThreshold) && (YawDelta <= 1.0f))
		{
			return;
		}

		// Detour cannot move an obstacle in place, so a moved obstacle is removed and re-added. The
		// manager keeps the description, so the new position is what future rebuilds replay.
		UnregisterObstacle();
		RegisterObstacle();
	}

#if _EDITOR_ONLY
	void PNavObstacleComponent::GenerateComponentDetail()
	{
		PSceneComponent::GenerateComponentDetail();

		if (!ImGui::TreeNodeEx("Navigation Obstacle", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return;
		}

		INT32 ShapeIndex = static_cast<INT32>(ShapeType);
		if (ImGui::Combo("Shape", &ShapeIndex, "Cylinder\0Box\0"))
		{
			SetShapeType(static_cast<ENavigationObstacleShapeType>(ShapeIndex));
		}

		if (ShapeType == ENavigationObstacleShapeType::NAVIGATION_OBSTACLE_BOX)
		{
			Vector3 Extents = BoxHalfExtents;
			if (ImGui::DragFloat3("Half Extents (cm)", &Extents.x, 1.0f, 1.0f, 5000.0f, "%.0f"))
			{
				SetBoxHalfExtents(Extents);
			}
		}
		else
		{
			FLOAT Radius = CylinderRadius;
			if (ImGui::DragFloat("Radius (cm)", &Radius, 1.0f, 1.0f, 5000.0f, "%.0f"))
			{
				SetCylinderRadius(Radius);
			}
			FLOAT Height = CylinderHeight;
			if (ImGui::DragFloat("Height (cm)", &Height, 1.0f, 1.0f, 5000.0f, "%.0f"))
			{
				SetCylinderHeight(Height);
			}
		}

		bool bFollow = (bFollowTransform != FALSE);
		if (ImGui::Checkbox("Follow Transform", &bFollow))
		{
			bFollowTransform = (bFollow ? TRUE : FALSE);
		}
		ImGui::DragFloat("Re-register Threshold (cm)", &FollowThreshold, 0.5f, 0.0f, 100.0f, "%.1f");

		ImGui::Text("State: %s", (ObstacleId != NavigationInvalidObstacleHandle) ? "Registered" : "Not registered");
		ImGui::TreePop();
	}

	void PNavObstacleComponent::EditorTick(FLOAT InDeltaTime)
	{
		PSceneComponent::EditorTick(InDeltaTime);

		if (!ENavigationManager::GetManagerSingleton()->IsDebugDrawEnabled())
		{
			return;
		}

		// The obstacle itself, so its footprint and its yaw are visible without a physics debug draw.
		RDebugWireframePrimitiveManager* WireframeManager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		const Vector3 Location = GetComponentWorldLocation();
		const Quaternion Rotation = GetComponentWorldRotation();
		if (ShapeType == ENavigationObstacleShapeType::NAVIGATION_OBSTACLE_BOX)
		{
			WireframeManager->DrawCuboid(Location, Rotation, BoxHalfExtents.x * 2.0f, BoxHalfExtents.y * 2.0f, BoxHalfExtents.z * 2.0f, Color4::Blue());
		}
		else
		{
			WireframeManager->DrawCylinder(Location, Location + Vector3(0.0f, CylinderHeight, 0.0f), CylinderRadius, CylinderRadius, Color4::Blue());
		}
	}
#endif

}  // namespace PigeonEngine
