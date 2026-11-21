#pragma once

#include <CoreMinimal.h>
#include <Navigation/NavigationTypes.h>
#include <Navigation/RecastNavigationProvider.h>

namespace PigeonEngine
{

	class EArchive;
	class ENavigationProviderInterface;

	/*
	* One agent profile of the navigation system - the engine's RecastNavMesh settings.
	*
	* A PNavMeshActor owns exactly one profile: the agent name (empty is the default agent),
	* the bake settings that make this mesh fit that agent, the navmesh asset, and the off-mesh
	* links that agent may use. Bounds volumes select the profiles they feed by name, so one
	* volume set serves every agent size.
	*
	* The struct is serialized explicitly (not through the struct descriptor registry) because
	* the build settings have a hand written layout; the descriptor below exists for the Details
	* grid, which edits a profile in place with the normal property undo.
	*/
	struct FNavMeshProfile
	{
	public:
		// Agent name this mesh is built for. Empty is the level's default agent.
		EString					Name;
		ENavMeshBuildSettings	BuildSettings;
		// Asset name from ENGINE_NAV_MESH_PATH; empty means "bake from the world" and the save
		// pass generates <Level>_<Agent>_NavMesh.
		EString					NavMeshAsset;
		// Off-mesh connections this agent can cross. Per profile: a jump that only fits the large
		// agent belongs to that agent's mesh.
		TArray<ENavigationLink>	NavLinks;
	};

	// One profile as an object: Name, BuildSettings, NavMeshAsset, NavLinks.
	void SerializeNavMeshProfile(EArchive& InArchive, const CHAR* InName, FNavMeshProfile& InOutProfile);
	// The nested pieces, shared with the older single-profile component and the level file.
	void SerializeNavMeshBuildSettings(EArchive& InArchive, const CHAR* InName, ENavMeshBuildSettings& InOutValue);
	void SerializeNavigationLinks(EArchive& InArchive, const CHAR* InName, TArray<ENavigationLink>& InOutLinks);

	// <LevelStem>_NavMesh for the default agent, <LevelStem>_<Agent>_NavMesh otherwise, with
	// characters a file name cannot hold replaced by '_'.
	PE_NODISCARD EString MakeNavMeshAssetName(const EString& InLevelStem, const EString& InAgentName);

	/*
	* Writes an already built provider as a profile asset and leaves the file describing the
	* provider (bounds, polygons, tiles). The caller updates the profile's asset name afterwards;
	* the level save pass uses this so the file exists before the level is serialized.
	*/
	BOOL32 WriteNavMeshAsset(ENavigationProviderInterface& InProvider, const ENavMeshBuildSettings& InBuildSettings,
		const ENavigationBuildStats& InStats, const EString& InAssetName,
		EString& OutError, const BOOL32& bInAllowOverwrite = FALSE);

	/*
	* Registers the struct descriptors the Details grid needs: ENavMeshBuildSettings (17 fields),
	* ENavigationLink, then FNavMeshProfile. Order matters, because a referring struct resolves
	* its nested descriptor at registration time. Idempotent, so every class that registers a
	* profile property can call it without duplicating fields.
	*/
	void RegisterNavMeshProfileTypes();

}  // namespace PigeonEngine
