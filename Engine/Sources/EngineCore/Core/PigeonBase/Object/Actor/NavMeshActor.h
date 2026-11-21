#pragma once

#include <CoreMinimal.h>
#include "../Actor.h"
#include <Navigation/NavMeshProfile.h>

namespace PigeonEngine
{

	/*
	* One navmesh instance - the engine's RecastNavMesh actor.
	*
	* The actor holds exactly one agent profile (name, bake settings, asset, off-mesh links).
	* A level has one actor per agent size; more than one actor may carry the same agent name,
	* in which case the build pass uses the earliest created one and logs the others. Bounds
	* volumes select the agents they feed by name, so one volume set serves every actor.
	*
	* The profile is a reflected property, so the Details grid edits it in place with the
	* normal property undo. The actor has no build controls: building is a level-wide action
	* driven by ENavigationManager.
	*/
	class PNavMeshActor final : public PActor
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PNavMeshActor)
	public:
		PE_NODISCARD const FNavMeshProfile&			GetProfile() const;
		void										SetProfile(const FNavMeshProfile& InProfile);
		// Empty for the default agent; this is the name agents and bounds volumes reference.
		PE_NODISCARD const EString&					GetAgentName() const;
		PE_NODISCARD const ENavMeshBuildSettings&	GetBuildSettings() const;
		PE_NODISCARD const TArray<ENavigationLink>&	GetNavigationLinks() const;
		PE_NODISCARD const EString&					GetNavMeshAssetName() const;
		void										SetNavMeshAssetName(const EString& InAssetName);

		// Runtime switch: a LOAD/RUNTIME pass skips this agent when it is off. Editor builds and
		// saves always include every agent.
		PE_NODISCARD BOOL32	GetBuildOnBeginPlay() const;
		void				SetBuildOnBeginPlay(const BOOL32& InBuildOnBeginPlay);

	public:
		// Appends the primary (earliest created) actor of every agent name in the world. This is
		// what the world build pass iterates; duplicate names are the build pass's warning.
		static void CollectPrimaryActorsInWorld(const PWorld& InWorld, TArray<PNavMeshActor*>& OutActors);

	protected:
		virtual void	BeginAddedToScene(PWorld* World) override;
		virtual void	RemovedFromScene() override;

	private:
		FNavMeshProfile	Profile;
		BOOL32			bBuildOnBeginPlay = TRUE;
	};

}  // namespace PigeonEngine
