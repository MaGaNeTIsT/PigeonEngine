#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Navigation/NavigationTypes.h>
#include <PigeonBase/Object/Component/ActorComponent.h>

namespace PigeonEngine
{

	/*
	* Movement settings for one navmesh agent. All lengths are in engine units (centimeters)
	* and speeds in centimeters per second.
	*
	* Radius and Height are only used to project the agent onto the mesh; they do not have to
	* match the bake settings, but a smaller radius than the bake used lets the agent cut
	* corners the mesh was not eroded for.
	*/
	struct ENavigationAgentSettings
	{
	public:
		FLOAT	Radius = 35.0f;
		FLOAT	Height = 150.0f;
		FLOAT	MoveSpeed = 300.0f;
		FLOAT	TurnSpeedDegreesPerSecond = 240.0f;
		// Distance at which a corner counts as reached. Too small and the agent jitters around
		// a corner it can never exactly hit; too large and it cuts the path short.
		FLOAT	AcceptanceRadius = 20.0f;
		// Minimum time between two path queries. A target that moves every frame would
		// otherwise run a graph search every frame per agent.
		FLOAT	RepathInterval = 0.5f;
	};

	/*
	* Moves the owner actor along a path found by the navigation manager.
	*
	* The component is backend agnostic: it asks ENavigationManager for a path and works with
	* engine positions only, so swapping the navigation backend changes nothing here. Movement
	* writes the owner's root component transform directly; hooking the agent up to a physics
	* character instead is a separate component, not a mode of this one.
	*/
	class PNavAgentComponent final : public PActorComponent
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PNavAgentComponent)
	public:
		virtual void	Tick(FLOAT InDeltaTime) override;

	public:
		// Records the request; the path query runs on the next tick so a caller inside a frame
		// callback never pays for a graph search in the middle of its own logic.
		BOOL32	MoveToLocation(const Vector3& InTarget);
		void	StopMovement();
		PE_NODISCARD BOOL32	IsMoving() const;
		PE_NODISCARD const ENavigationPath&	GetCurrentPath() const;
		// Status of the most recently finished move, or INVALID when none has finished.
		PE_NODISCARD ENavigationPathStatusType	GetLastMoveStatus() const;

		PE_NODISCARD const ENavigationAgentSettings&	GetSettings() const;
		void											SetSettings(const ENavigationAgentSettings& InSettings);

		// Invoked once per finished move with COMPLETE, PARTIAL or INVALID.
		TFunction<void(ENavigationPathStatusType)>	OnMoveFinished;

	private:
		// Publishes CurrentPath for MoveTarget. Leaves the request pending when no navigation
		// data exists yet; finishes the move when the query itself fails.
		void	RequestPath(const Vector3& InTarget);
		void	FollowPath(FLOAT InDeltaTime);
		void	FinishMove(ENavigationPathStatusType InStatus);
		PE_NODISCARD PSceneComponent*	GetOwnerRootComponent() const;
		PE_NODISCARD Vector3			GetAgentLocation() const;

	private:
		ENavigationAgentSettings	Settings;
		ENavigationPath				CurrentPath;
		Vector3						MoveTarget = Vector3::Zero();
		Vector3						RequestedTarget = Vector3::Zero();
		BOOL32						bHasMoveRequest = FALSE;
		BOOL32						bHasPath = FALSE;
		UINT32						NextCornerIndex = 0u;
		ENavigationPathStatusType	LastMoveStatus = ENavigationPathStatusType::NAVIGATION_PATH_INVALID;
		FLOAT						SecondsSincePathRequest = 0.0f;

#if _EDITOR_ONLY
	public:
		virtual void	GenerateComponentDetail() override;
		virtual void	EditorTick(FLOAT InDeltaTime) override;
#endif
	};

}  // namespace PigeonEngine
