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
		// on the unique id, so a stale provider would answer queries for a volume that no
		// longer exists.
		ENavigationManager::GetManagerSingleton()->UnregisterProvider(GetUniqueID());
		PSceneComponent::Uninit();
	}

	ENavigationProviderInterface* PNavigationComponent::GetRuntimeProvider() const
	{
		return ENavigationManager::GetManagerSingleton()->GetProvider(GetUniqueID());
	}

	const ENavigationBuildStats& PNavigationComponent::GetLastBuildStats() const
	{
		return LastBuildStats;
	}

	void PNavigationComponent::SetDebugDrawEnabled(const BOOL32& InEnabled)
	{
		bDebugDrawEnabled = InEnabled;
		// Forwarded only when a provider is already published; otherwise the value is applied
		// by the next build, which is the frame the overlay could show anything anyway.
		ENavigationManager::GetManagerSingleton()->SetProviderDebugDrawEnabled(GetUniqueID(), InEnabled);
	}

	BOOL32 PNavigationComponent::IsDebugDrawEnabled() const
	{
		return bDebugDrawEnabled;
	}

	void PNavigationComponent::RemovedFromScene()
	{
		ENavigationManager::GetManagerSingleton()->UnregisterProvider(GetUniqueID());
	PSceneComponent::RemovedFromScene();
}


}  // namespace PigeonEngine
