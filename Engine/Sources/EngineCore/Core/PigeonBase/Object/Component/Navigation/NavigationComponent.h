#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Navigation/NavigationTypes.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

	class ENavigationProviderInterface;

	/*
	* Base class for a component that owns one navigation provider.
	*
	* This is where the backend independent half of a navigation volume lives: registering the
	* provider with ENavigationManager, unloading it when the component leaves the world, and
	* the editor section that shows the last build result. A different navigation type (a
	* waypoint graph, a grid) is added by deriving from this class and from
	* ENavigationProviderInterface; nothing here mentions Recast.
	*
	* The provider itself is owned by ENavigationManager once a build publishes it, so it
	* survives until the component removes it or the manager shuts down.
	*/
	class PNavigationComponent : public PSceneComponent
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PNavigationComponent)
	public:
		virtual void	Uninit() override;

	public:
		// Name the backend registers itself under in ENavigationProviderRegistry.
		PE_NODISCARD virtual EName GetProviderName() const = 0;
		// Collects the source geometry, bakes it and publishes the result to the manager.
		// Returns FALSE when the bake did not produce a usable provider.
		virtual BOOL32	BuildNavigation(ENavigationBuildStats& OutStats) = 0;

		// The provider currently registered for this component, or null before the first build.
		PE_NODISCARD ENavigationProviderInterface*	GetRuntimeProvider() const;
		PE_NODISCARD const ENavigationBuildStats&	GetLastBuildStats() const;

		// Whether this volume's polygons take part in the editor's navigation overlay.
		void						SetDebugDrawEnabled(const BOOL32& InEnabled);
		PE_NODISCARD BOOL32			IsDebugDrawEnabled() const;

	protected:
		virtual void	RemovedFromScene() override;

	protected:
		ENavigationBuildStats	LastBuildStats;
		BOOL32					bDebugDrawEnabled = TRUE;

#if _EDITOR_ONLY
	public:
		virtual void	GenerateComponentDetail() override;
#endif
	};

}  // namespace PigeonEngine
