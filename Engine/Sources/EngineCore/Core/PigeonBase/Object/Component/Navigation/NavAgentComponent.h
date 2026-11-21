#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Navigation/NavigationTypes.h>
#include <PigeonBase/Object/Component/ActorComponent.h>

namespace PigeonEngine
{

	class PSceneComponent;

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

		// Navmesh group this agent paths on. Empty is the level's default navmesh; a group
		// never answers for another one, so two navmeshes can coexist in one level.
		PE_NODISCARD const EString&	GetNavMeshName() const;
		void						SetNavMeshName(const EString& InName);

		/*
		* When on, the agent is registered with the backend's local avoidance and steered there
		* instead of following a path here. The radius, height, acceleration and avoidance settings
		* then apply; a directly moved agent is projected with the bake settings instead.
		*/
		PE_NODISCARD BOOL32	GetUseCrowd() const;
		void				SetUseCrowd(const BOOL32& InUseCrowd);

		// A target moved to once after the component joins the scene. Levels use this to
		// store patrol/demo behaviour as data instead of calling MoveToLocation from a
		// hardcoded builder; MoveOnBeginPlay stays true when the file is saved again.
		PE_NODISCARD const Vector3&	GetTargetLocation() const;
		void						SetTargetLocation(const Vector3& InTarget);
		PE_NODISCARD BOOL32			GetMoveOnBeginPlay() const;
		void						SetMoveOnBeginPlay(const BOOL32& bInMoveOnBeginPlay);

		// Invoked once per finished move with COMPLETE, PARTIAL or INVALID.
		TFunction<void(ENavigationPathStatusType)>	OnMoveFinished;

	private:
		// Crowd mode: register with the local avoidance, hand over the target, and drive the owner
		// from the pose the crowd reports.
		void	TickCrowd(FLOAT InDeltaTime);
		// Publishes CurrentPath for MoveTarget. Leaves the request pending when no navigation
		// data exists yet; finishes the move when the query itself fails.
		void	RequestPath(const Vector3& InTarget);
		void	FollowPath(FLOAT InDeltaTime);
		void	FinishMove(ENavigationPathStatusType InStatus);
		PE_NODISCARD PSceneComponent*	GetOwnerRootComponent() const;

	protected:
		virtual void	RemovedFromScene() override;
		PE_NODISCARD Vector3			GetAgentLocation() const;

	private:
		ENavigationAgentSettings	Settings;
		EString						NavMeshName;
		ENavigationPath				CurrentPath;
		Vector3						MoveTarget = Vector3::Zero();
		Vector3						RequestedTarget = Vector3::Zero();
		BOOL32						bHasMoveRequest = FALSE;
		BOOL32						bHasPath = FALSE;
		UINT32						NextCornerIndex = 0u;
		ENavigationPathStatusType	LastMoveStatus = ENavigationPathStatusType::NAVIGATION_PATH_INVALID;
		FLOAT						SecondsSincePathRequest = 0.0f;
		BOOL32						bUseCrowd = FALSE;
		Vector3						TargetLocation = Vector3::Zero();
		BOOL32						bMoveOnBeginPlay = FALSE;
		BOOL32						bAutoMoveIssued = FALSE;
		// TRUE while the crowd holds the current target. Without it a freshly registered agent,
		// whose target state starts as "none", would look like a finished move.
		BOOL32						bCrowdMoveRequested = FALSE;

#if ENGINE_EDITOR_ONLY
	public:
		virtual void	EditorTick(FLOAT InDeltaTime) override;
#endif
	};

}  // namespace PigeonEngine
