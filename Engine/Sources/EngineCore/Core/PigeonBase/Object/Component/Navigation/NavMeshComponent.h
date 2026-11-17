#pragma once

#include "NavigationComponent.h"

#include <RecastNavigation/Headers/RecastNavigationProvider.h>

namespace PigeonEngine
{

	/*
	* A navigation volume baked with Recast and queried through Detour.
	*
	* The component is the author facing half of the Recast backend: it holds the bake
	* settings, collects the static geometry inside its volume and republishes the result when
	* asked. It is deliberately the only navigation component that knows which backend is in
	* use, which is why the settings type comes from the wrapper header rather than from the
	* provider interface.
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

		// Bakes on the next tick instead of immediately, so a caller inside a frame callback
		// never runs a multi millisecond bake in the middle of its own logic.
		void	RequestBuildNavigation();

		// When on, the component bakes once by itself after the assets it needs are loaded.
		PE_NODISCARD BOOL32	GetBuildOnBeginPlay() const;
		void				SetBuildOnBeginPlay(const BOOL32& InBuildOnBeginPlay);

	private:
		// Runs the automatic bake once the asset queue drained, or once waiting longer would be
		// worse than baking an incomplete mesh.
		void	TryBuildOnBeginPlay(FLOAT InDeltaTime);
		// World space box the collector filters against. An invalid bound collects everything.
		PE_NODISCARD EBoundAABB	MakeBakeBoundsInWorld() const;

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

#if _EDITOR_ONLY
	public:
		virtual void	GenerateComponentDetail() override;
		virtual void	EditorTick(FLOAT InDeltaTime) override;
#endif
	};

}  // namespace PigeonEngine
