#pragma once

#include "NavigationComponent.h"

#include <NavMeshAsset/NavMeshAsset.h>
#include <Navigation/RecastNavigationProvider.h>

namespace PigeonEngine
{

	class PWorld;

	/*
	* One navmesh profile - the engine's RecastNavMesh actor.
	*
	* The component holds the profile's bake settings, asset and off-mesh links. It does not
	* build itself: every build is one world-level pass driven by ENavigationManager (level
	* load, editor Build Navigation, level save, runtime start), so a level with several agent
	* sizes builds all its navmeshes together instead of one actor at a time.
	*
	* Coverage is authored with PNavMeshBoundsComponent. A volume feeds every profile whose
	* name it lists, or every profile when its list is empty - the usual case, one volume set
	* shared by the default agent and a larger one. An empty NavMeshName is the default profile.
	*/
	class PNavMeshComponent final : public PNavigationComponent
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PNavMeshComponent)
	public:
		virtual EName	GetProviderName() const override;
		// Asks the manager to run an editor build pass over the whole world (all profiles).
		virtual BOOL32	BuildNavigation(ENavigationBuildStats& OutStats) override;

	public:
		PE_NODISCARD const ENavMeshBuildSettings&	GetBuildSettings() const;
		void										SetBuildSettings(const ENavMeshBuildSettings& InSettings);

		// Profile name. Empty is the level's default navmesh; agents, obstacles and bounds
		// volumes select a profile by this exact name.
		PE_NODISCARD const EString&	GetNavMeshName() const;
		void						SetNavMeshName(const EString& InName);

		// Half extent of one extra region this profile contributes, around the owner's world
		// location. All three axes have to be positive, otherwise no box is contributed. A
		// level without any bounds volume falls back to the whole world for the default profile.
		PE_NODISCARD const Vector3&	GetBakeVolumeExtent() const;
		void						SetBakeVolumeExtent(const Vector3& InExtent);

		// Asks the manager for an editor build pass (bake everything from the world).
		void	RequestBuildNavigation();

		// Runtime switch: a LOAD/RUNTIME pass skips this profile when it is off. Editor builds
		// and saves always include every profile.
		PE_NODISCARD BOOL32	GetBuildOnBeginPlay() const;
		void				SetBuildOnBeginPlay(const BOOL32& InBuildOnBeginPlay);

		// Authored off-mesh connections baked into this profile. They are part of the pass, so a
		// change takes effect on the next build, and an assigned navmesh asset carries the
		// connections it was baked with.
		PE_NODISCARD const TArray<ENavigationLink>&	GetNavigationLinks() const;
		void										SetNavigationLinks(const TArray<ENavigationLink>& InLinks);

	public:
		// One owner per profile: the oldest component of a name publishes, the others warn once
		// and stay inert instead of shadowing it or overwriting its asset.
		PE_NODISCARD BOOL32	IsPrimaryNavMeshOwner() const;
		// Appends the primary owner of every profile in the world, one per NavMeshName. This is
		// what the world build pass iterates.
		static void	CollectPrimaryOwnersInWorld(const PWorld& InWorld, TArray<PNavMeshComponent*>& OutOwners);

		// Regions a build of this profile uses: every bounds volume that supports it plus the
		// owner's own extent. An empty list means the whole world and is only valid for the
		// default profile (a named profile without volumes is reported, not built).
		PE_NODISCARD TArray<EBoundAABB>	CollectBakeVolumesInWorld() const;

		// The backend provider configured with this profile's settings, not published yet. The
		// caller owns it until it is handed to the manager.
		PE_NODISCARD ERecastNavigationProvider*	CreateConfiguredProvider() const;

	public:
		// Baked mesh to use instead of a runtime bake on a level load.
		PE_NODISCARD const ENavMeshAsset*	GetNavMeshAsset() const;
		// Live asset name, or the last requested name while the asynchronous load is pending.
		PE_NODISCARD const EString&			GetNavMeshAssetName() const;
		void								SetNavMeshAsset(const ENavMeshAsset* InAsset);
		/*
		* Requests a navmesh asset by name from ENGINE_NAV_MESH_PATH. The request is one shot: a
		* missing or rejected file is reported once and the component falls back to a world bake.
		*/
		void	RequestNavMeshAsset(const EString& InAssetName);
		// Drops the current navmesh asset and any pending request.
		void	ClearNavMeshAsset();
		// Imports the assigned asset and publishes it. Used by the world build's LOAD/RUNTIME
		// pass; it is a memory copy, not a bake.
		BOOL32	ImportAssignedAsset(ENavigationBuildStats& OutStats);
		// A named asset is still being requested and has not failed yet: a runtime build should
		// wait for it instead of baking a second mesh from the world.
		PE_NODISCARD BOOL32	HasOutstandingAssetRequest() const;

#if ENGINE_EDITOR_ONLY
	public:
		virtual void	EditorTick(FLOAT InDeltaTime) override;

		// Manual authoring shortcut: bakes this one profile from the world, publishes it and
		// writes the asset. The level-wide path is Build Navigation / level save.
		BOOL32						SaveBakeAsAsset(const EString& InAssetName, EString& OutError, const BOOL32& bInAllowOverwrite = FALSE);
		// Writes an already built provider as an asset and points the component at it; does not
		// publish. The world save pass uses this so the file and the level agree.
		BOOL32						SaveProviderAsAsset(ENavigationProviderInterface& InProvider, const EString& InAssetName, const ENavigationBuildStats& InStats, EString& OutError, const BOOL32& bInAllowOverwrite = FALSE);
		// Re-reads the current asset from disk and publishes it immediately.
		BOOL32						ReloadNavMeshAsset(EString& OutError);
		// Builds this profile from the world on this thread; the caller owns the provider.
		ERecastNavigationProvider*	BakeProviderSynchronously(ENavigationBuildStats& OutStats);

		// <LevelStem>_NavMesh for the default profile, <LevelStem>_<profile>_NavMesh otherwise,
		// with characters a file name cannot hold replaced by '_'. Static so the naming rule is
		// testable without a world.
		static EString				MakeNavMeshAssetName(const EString& InLevelStem, const EString& InNavMeshName);

		// The single queued asset request failed to load; the component fell back to the world bake.
		PE_NODISCARD BOOL32			HasNavMeshAssetFailedToLoad() const;
		// The last error recorded by SaveBakeAsAsset/ReloadNavMeshAsset; both clear it at entry.
		PE_NODISCARD const EString&	GetLastNavMeshAssetError() const;

	private:
		EString	LastAssetError;
#endif

	private:
		ENavMeshBuildSettings	BuildSettings;
		EString					NavMeshName;
		Vector3					BakeVolumeExtent = Vector3::Zero();
		BOOL32					bBuildOnBeginPlay = TRUE;
		TArray<ENavigationLink>	NavigationLinks;

		const ENavMeshAsset*	NavMeshAsset = nullptr;
		EString					NavMeshAssetName;
		// Set once the single asset request has been queued, so it is never queued twice.
		BOOL32					bAssetRequestIssued = FALSE;
		// Set once a requested asset failed to load, so the component stops waiting for it.
		BOOL32					bAssetLoadFailed = FALSE;
		// Drops deferred steps from an older request when the asset name changes or reloads.
		UINT32					AssetRequestGeneration = 0u;
		// The non-primary warning is one shot per component, not per frame.
		mutable BOOL32			bWarnedNotPrimary = FALSE;
	};

}  // namespace PigeonEngine
