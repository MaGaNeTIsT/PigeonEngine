#include "NavigationComponent.h"

#include <Editor/EditorLogManager.h>
#include <Navigation/NavigationManager.h>
#include <Navigation/NavigationProviderInterface.h>

#if ENGINE_EDITOR_ONLY
#include <imgui.h>
#endif

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

#if ENGINE_EDITOR_ONLY
	void PNavigationComponent::GenerateComponentDetail()
	{
		PSceneComponent::GenerateComponentDetail();

		if (!ImGui::TreeNodeEx("Navigation", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return;
		}

		ImGui::Text("Backend: %s", *GetProviderName().ToString());
		bool bShowNavigation = (bDebugDrawEnabled != FALSE);
		if (ImGui::Checkbox("Include In Overlay", &bShowNavigation))
		{
			// Through the setter: it forwards the flag to the manager, so the overlay drops this
			// provider's lines in the same frame instead of waiting for the next build.
			SetDebugDrawEnabled((bShowNavigation ? TRUE : FALSE));
		}

		if (LastBuildStats.PolygonCount > 0u)
		{
			ImGui::Text("Polygons: %u  Tiles: %u", LastBuildStats.PolygonCount, LastBuildStats.TileCount);
			if ((LastBuildStats.TriangleCount > 0u) || (LastBuildStats.Milliseconds > 0.0f))
			{
				ImGui::Text("Triangles: %u (%u walkable)", LastBuildStats.TriangleCount, LastBuildStats.WalkableTriangleCount);
				ImGui::Text("Bake: %.2f ms", LastBuildStats.Milliseconds);
			}
			else
			{
				ImGui::TextDisabled("Imported from navmesh asset.");
			}
		}
		else
		{
			ImGui::TextDisabled("Not built yet.");
		}
		if (LastBuildStats.SkippedMeshCount > 0u)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Skipped meshes: %u", LastBuildStats.SkippedMeshCount);
		}

		ImGui::TreePop();
	}
#endif

}  // namespace PigeonEngine
