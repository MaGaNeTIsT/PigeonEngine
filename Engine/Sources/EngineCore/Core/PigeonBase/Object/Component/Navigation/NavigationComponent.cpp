#include "NavigationComponent.h"

#include <Development/EditorLogManager.h>
#include <Navigation/NavigationManager.h>
#include <Navigation/NavigationProviderInterface.h>


namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PNavigationComponent, PSceneComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PNavigationComponent::PNavigationComponent()
	{
		POBJ_DEBUGNAME_SET(this, "NavigationComponent");
	}

	PNavigationComponent::~PNavigationComponent()
	{
	}

	void PNavigationComponent::Uninit()
	{
		// A component that leaves the world hands its navigation data back. The manager keys
		// providers by profile inside one world, so a stale provider would answer queries for a
		// volume that no longer exists. Uninit runs after the scene exit, when the world pointer
		// is already gone, so remove by source instead of by world.
		ENavigationManager::GetManagerSingleton()->UnregisterProfilesBySource(nullptr, GetUniqueID());
		PSceneComponent::Uninit();
	}

	ENavigationProviderInterface* PNavigationComponent::GetRuntimeProvider() const
	{
		return ENavigationManager::GetManagerSingleton()->GetProvider(GetWorld(), GetPrimaryProfileName());
	}

	const ENavigationBuildStats& PNavigationComponent::GetLastBuildStats() const
	{
		// The manager owns the published mesh, so it also owns the statistics of the build that
		// produced it; the component keeps no copy that could go stale after a rebuild.
		return ENavigationManager::GetManagerSingleton()->GetBuildStats(GetWorld(), GetPrimaryProfileName());
	}

	void PNavigationComponent::SetDebugDrawEnabled(const BOOL32& InEnabled)
	{
		bDebugDrawEnabled = InEnabled;
		// Forwarded only when a provider is already published; otherwise the value is applied
		// by the next build, which is the frame the overlay could show anything anyway.
		ENavigationManager::GetManagerSingleton()->SetProviderDebugDrawEnabled(GetWorld(), GetPrimaryProfileName(), InEnabled);
	}

	BOOL32 PNavigationComponent::IsDebugDrawEnabled() const
	{
		return bDebugDrawEnabled;
	}

	void PNavigationComponent::RemovedFromScene()
	{
		ENavigationManager::GetManagerSingleton()->UnregisterProfilesBySource(GetWorld(), GetUniqueID());
		PSceneComponent::RemovedFromScene();
	}


}  // namespace PigeonEngine
