#pragma once

#include "NavigationComponent.h"

#include <NavMeshAsset/NavMeshAsset.h>
#include <Navigation/RecastNavigationProvider.h>

namespace PigeonEngine
{

	/*
	* A navigation volume baked with Recast and queried through Detour.
	*
	* The component is the author facing half of the Recast backend: it holds the bake settings,
	* collects the static geometry inside its volume and republishes the result when asked. It is
	* deliberately the only navigation component that knows which backend is in use, which is why
	* the settings type comes from the wrapper header rather than from the provider interface.
	*
	* Two sources of a navigation mesh, in this order:
	*   1. An assigned ENavMeshAsset, whose blob is imported instead of baked. This is the shipped
	*      path: level load does not pay for a bake.
	*   2. The world itself, baked on demand. Used when no asset is assigned, when the asset is
	*      missing or rejected, and by the explicit rebuild button.
	*/
	class PNavMeshComponent final : public PNavigationComponent
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PNavMeshComponent)
	public:
		virtual void	Tick(FLOAT InDeltaTime) override;

	public:
		virtual EName	GetProviderName() const override;
		virtual BOOL32	BuildNavigation(ENavigationBuildStats& OutStats) override;

	public:
		PE_NODISCARD const ENavMeshBuildSettings&	GetBuildSettings() const;
		void										SetBuildSettings(const ENavMeshBuildSettings& InSettings);

		// Half extent of the volume, around the component's world location. Zero on all axes
		// means "the whole world", which is what a level with a single navigation volume wants.
		PE_NODISCARD const Vector3&	GetBakeVolumeExtent() const;
		void						SetBakeVolumeExtent(const Vector3& InExtent);

		// Rebuilds from the world on the next tick, ignoring any assigned asset, so a caller
		// inside a frame callback never runs a multi millisecond bake in the middle of its own
		// logic.
		void	RequestBuildNavigation();

		// When on, the component publishes once by itself: from the assigned asset if there is
		// one, otherwise by baking after the assets it needs are loaded.
		PE_NODISCARD BOOL32	GetBuildOnBeginPlay() const;
		void				SetBuildOnBeginPlay(const BOOL32& InBuildOnBeginPlay);

	public:
		// Baked mesh to use instead of a runtime bake. Assigning one drops the current provider and
		// republishes from the asset on the next tick.
		PE_NODISCARD const ENavMeshAsset*	GetNavMeshAsset() const;
		// Live asset name, or the last requested name while the asynchronous load is pending.
		PE_NODISCARD const EString&			GetNavMeshAssetName() const;
		void								SetNavMeshAsset(const ENavMeshAsset* InAsset);
		/*
		* Requests a navmesh asset by name from ENGINE_NAV_MESH_PATH. The request is one shot: a
		* missing or rejected file is reported once and the component falls back to baking, rather
		* than retrying every frame.
		*/
		void	RequestNavMeshAsset(const EString& InAssetName);
		// Drops the current navmesh asset and any pending request, returning the component to
		// its runtime bake source.
		void	ClearNavMeshAsset();

	private:
		// Runs the automatic publish once the asset queue drained, or once waiting longer would be
		// worse than baking an incomplete mesh.
		void	TryBuildOnBeginPlay(FLOAT InDeltaTime);
		// World space box the collector filters against. An invalid bound collects everything.
		PE_NODISCARD EBoundAABB	MakeBakeBoundsInWorld() const;

		// Creates the Recast provider this component talks to and copies the bake settings over.
		// The caller owns it until it is published or deleted.
		ERecastNavigationProvider*	CreateProvider();
		// Collects the world geometry on the game thread and queues the bake on the manager's
		// worker. The result is published by the manager's tick; nothing is published here.
		BOOL32						StartBakeFromWorld();
		// Imports the assigned asset's blob into a new provider. Synchronous: it is a memory copy.
		BOOL32						BuildFromAsset(ENavigationBuildStats& OutStats);
		// Hands the provider to the manager and records the stats the panel shows.
		void						PublishProvider(ENavigationProviderInterface* InProvider, const ENavigationBuildStats& InStats);

	private:
		ENavMeshBuildSettings	BuildSettings;
		Vector3					BakeVolumeExtent = Vector3::Zero();
		BOOL32					bBuildOnBeginPlay = TRUE;
		BOOL32					bBuildRequested = FALSE;
		BOOL32					bHasBuilt = FALSE;
		// Seconds spent waiting for the asset queue. Once it passes the threshold the bake runs
		// with whatever is loaded and says so, rather than never producing a navmesh.
		FLOAT					WaitedSeconds = 0.0f;
		FLOAT					NextAttemptSeconds = 0.0f;

		const ENavMeshAsset*	NavMeshAsset = nullptr;
		EString					NavMeshAssetName;
		// Set once the single asset request has been queued, so it is never queued twice.
		BOOL32					bAssetRequestIssued = FALSE;
		// Set once a requested asset failed to load, so the component stops waiting for it.
		BOOL32					bAssetLoadFailed = FALSE;
		// Drops deferred steps from an older request when the asset name changes or reloads.
		UINT32					AssetRequestGeneration = 0u;
		// Set when the last queued bake reported failure, so the retry policy can react without
		// re-requesting blindly.
		BOOL32					bBakeFailed = FALSE;

		// Whether the component currently holds a published provider (baked here, or imported
		// from its asset). The level-save auto-assign uses it to skip components with nothing
		// to export.
		PE_NODISCARD BOOL32			HasBuiltNavMesh() const;

#if ENGINE_EDITOR_ONLY
	public:
		virtual void	EditorTick(FLOAT InDeltaTime) override;

		// Bakes from the world on this thread, exports the result and writes a new asset. Saving is
		// an explicit editor action and has to publish exactly the bytes the file holds, so it does
		// not go through the worker. Both actions record their failure into the last-asset error,
		// so the editor panel only needs the getters below.
		BOOL32						SaveBakeAsAsset(const EString& InAssetName, EString& OutError, const BOOL32& bInAllowOverwrite = FALSE);
		// Re-reads the current asset from disk and publishes it immediately. Used by the editor
		// panel after an external bake/replace, without restarting the editor.
		BOOL32						ReloadNavMeshAsset(EString& OutError);
		ERecastNavigationProvider*	BakeProviderSynchronously(ENavigationBuildStats& OutStats);

		// The single queued asset request failed to load; the component fell back to the world bake.
		PE_NODISCARD BOOL32			HasNavMeshAssetFailedToLoad() const;
		// The last error recorded by SaveBakeAsAsset/ReloadNavMeshAsset; both clear it at entry.
		PE_NODISCARD const EString&	GetLastNavMeshAssetError() const;

		// Level-save auto-assign. A component the author already named is left alone; one with a
		// published provider but no name gets the generated convention name <InLevelStem>_NavMesh,
		// re-baked from the world being saved and written with overwrite allowed (the convention
		// name is generated from this level, so re-saving regenerates it; manual saves keep
		// refusing to overwrite). Returns TRUE when the component was left alone or was assigned.
		BOOL32						AutoAssignNamedNavMeshAsset(const EString& InLevelStem, EString& OutError);

	private:
		EString	LastAssetError;
#endif
	};

}  // namespace PigeonEngine
