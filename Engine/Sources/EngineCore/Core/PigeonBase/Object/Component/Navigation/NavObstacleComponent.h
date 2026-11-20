#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Navigation/NavigationTypes.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

	enum ENavigationObstacleShapeType : UINT8
	{
		NAVIGATION_OBSTACLE_CYLINDER = 0,
		NAVIGATION_OBSTACLE_BOX
	};

	/*
	* Registers a runtime obstacle with the navigation volume covering it, so the tile cache rebuilds
	* the tiles it touches and agents path around it.
	*
	* The obstacle takes this component's world transform: a cylinder stands with its base at the
	* component location, a box is centred on it and yawed with it. Moving the component re-registers
	* the obstacle once the move passes FollowThreshold, because Detour has no way to move an obstacle
	* in place.
	*
	* Only a tiled volume (PNavMeshComponent with TileSizeCells > 0) supports obstacles. On a single
	* tile volume the registration is refused; that is reported once rather than every frame.
	*/
	class PNavObstacleComponent final : public PSceneComponent
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PNavObstacleComponent)
	public:
		virtual void	Tick(FLOAT InDeltaTime) override;

	public:
		PE_NODISCARD ENavigationObstacleShapeType	GetShapeType() const;
		void										SetShapeType(const ENavigationObstacleShapeType& InShapeType);
		PE_NODISCARD FLOAT	GetCylinderRadius() const;
		void				SetCylinderRadius(const FLOAT& InRadius);
		PE_NODISCARD FLOAT	GetCylinderHeight() const;
		void				SetCylinderHeight(const FLOAT& InHeight);
		PE_NODISCARD const Vector3&	GetBoxHalfExtents() const;
		void						SetBoxHalfExtents(const Vector3& InHalfExtents);

	public:
		// Re-register after the component moved further than this, in centimeters.
		FLOAT	FollowThreshold = 5.0f;
		BOOL32	bFollowTransform = TRUE;

	protected:
		virtual void	BeginAddedToScene(PWorld* InWorld) override;
		virtual void	RemovedFromScene() override;

	private:
		void	RegisterObstacle();
		void	UnregisterObstacle();

	private:
		ENavigationObstacleShapeType	ShapeType = ENavigationObstacleShapeType::NAVIGATION_OBSTACLE_CYLINDER;
		FLOAT							CylinderRadius = 40.0f;
		FLOAT							CylinderHeight = 150.0f;
		Vector3							BoxHalfExtents = Vector3(40.0f, 75.0f, 40.0f);
		// The id the manager handed out; stable across provider rebuilds.
		ENavigationObstacleHandle		ObstacleId = NavigationInvalidObstacleHandle;
		Vector3							LastCenter = Vector3::Zero();
		FLOAT							LastYawDegrees = 0.0f;
		BOOL32							bWarnedUnsupported = FALSE;

#if ENGINE_EDITOR_ONLY
	public:
		virtual void	EditorTick(FLOAT InDeltaTime) override;
#endif
	};

}  // namespace PigeonEngine
