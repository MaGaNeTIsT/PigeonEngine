#include "NavMeshComponent.h"

#include <AssetLoadManager.h>
#include <Editor/EditorLogManager.h>
#include <Navigation/NavigationGeometry.h>
#include <Navigation/NavigationManager.h>
#include <Navigation/NavigationProviderInterface.h>
#include <Navigation/NavigationProviderRegistry.h>

#if _EDITOR_ONLY
#include <RenderProxy/RenderSingletonObject.h>
#include <imgui.h>
#endif

namespace PigeonEngine
{

	// How long the automatic bake waits for the asset queue to drain before it reports an
	// incomplete mesh instead of waiting forever. Long enough for a level load, short enough
	// that a permanently pending request does not leave the level without navigation.
	PE_CONSTEXPR_STATIC FLOAT NavigationAutoBuildWaitSeconds = 30.0f;
	// A failed automatic attempt (no ground yet, for example) is retried at this interval
	// rather than every frame: a bake walks the whole world.
	PE_CONSTEXPR_STATIC FLOAT NavigationAutoBuildRetrySeconds = 1.0f;

	static void RegisterClassTypes()
	{
		RegisterClassType<PNavMeshComponent, PNavigationComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PNavMeshComponent::PNavMeshComponent()
	{
		POBJ_DEBUGNAME_SET(this, "NavMeshComponent");
		// The component has per frame work of its own: the automatic bake polls the asset
		// queue. Requesting it here is what keeps a level that forgot to mark the component
		// tickable from silently never getting a navmesh.
		SetIsTickable(TRUE);
	}

	PNavMeshComponent::~PNavMeshComponent()
	{
	}

	const ENavMeshBuildSettings& PNavMeshComponent::GetBuildSettings() const
	{
		return BuildSettings;
	}

	void PNavMeshComponent::SetBuildSettings(const ENavMeshBuildSettings& InSettings)
	{
		BuildSettings = InSettings;
	}

	const Vector3& PNavMeshComponent::GetBakeVolumeExtent() const
	{
		return BakeVolumeExtent;
	}

	void PNavMeshComponent::SetBakeVolumeExtent(const Vector3& InExtent)
	{
		BakeVolumeExtent = Vector3(EMath::Max(InExtent.x, 0.0f), EMath::Max(InExtent.y, 0.0f), EMath::Max(InExtent.z, 0.0f));
	}

	void PNavMeshComponent::RequestBuildNavigation()
	{
		bBuildRequested = TRUE;
	}

	BOOL32 PNavMeshComponent::GetBuildOnBeginPlay() const
	{
		return bBuildOnBeginPlay;
	}

	void PNavMeshComponent::SetBuildOnBeginPlay(const BOOL32& InBuildOnBeginPlay)
	{
		bBuildOnBeginPlay = InBuildOnBeginPlay;
	}

	EName PNavMeshComponent::GetProviderName() const
	{
		return ERecastNavigationProvider::GetProviderName();
	}

	EBoundAABB PNavMeshComponent::MakeBakeBoundsInWorld() const
	{
		if ((BakeVolumeExtent.x <= 0.0f) || (BakeVolumeExtent.y <= 0.0f) || (BakeVolumeExtent.z <= 0.0f))
		{
			// No volume authored: the collector treats an invalid bound as the whole world.
			return EBoundAABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), FALSE);
		}
		const Vector3 Location = GetComponentWorldLocation();
		return EBoundAABB(Location - BakeVolumeExtent, Location + BakeVolumeExtent, TRUE);
	}

	BOOL32 PNavMeshComponent::BuildNavigation(ENavigationBuildStats& OutStats)
	{
		OutStats = ENavigationBuildStats();

		PWorld* World = GetWorld();
		if (!World)
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: build requested for a navmesh component that is not in a world.")));
			return FALSE;
		}

		ENavigationProviderInterface* CreatedProvider = ENavigationProviderRegistry::Create(GetProviderName());
		if (!CreatedProvider)
		{
			return FALSE;
		}
		// This component is the Recast backend's author facing half, so the settings it holds
		// are that provider's settings. The cast is the single place where the coupling is
		// visible, and it cannot fail: the provider name comes from the same class the cast
		// targets.
		ERecastNavigationProvider* Provider = dynamic_cast<ERecastNavigationProvider*>(CreatedProvider);
		if (!Provider)
		{
			delete CreatedProvider;
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: the provider registered under the navmesh name is not a Recast provider.")));
			return FALSE;
		}
		Provider->SetSettings(BuildSettings);

		ENavigationSourceGeometry Geometry;
		ENavigationGeometryCollector::CollectStaticMeshTriangles(*World, MakeBakeBoundsInWorld(), BuildSettings.bFlipTriangleWinding, Geometry);

		if (!Provider->Build(Geometry, OutStats))
		{
			// A failed build publishes nothing, so the previous navmesh, if any, stays in use.
			delete Provider;
			return FALSE;
		}

		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		Manager->RegisterProvider(GetUniqueID(), Provider);
		Manager->SetProviderDebugDrawEnabled(GetUniqueID(), bDebugDrawEnabled);
		LastBuildStats = OutStats;
		return TRUE;
	}

	void PNavMeshComponent::Tick(FLOAT InDeltaTime)
	{
		PSceneComponent::Tick(InDeltaTime);

		if (bBuildRequested)
		{
			bBuildRequested = FALSE;
			ENavigationBuildStats Stats;
			if (BuildNavigation(Stats))
			{
				bHasBuilt = TRUE;
			}
			return;
		}

		if ((bBuildOnBeginPlay) && (!bHasBuilt))
		{
			TryBuildOnBeginPlay(InDeltaTime);
		}
	}

	void PNavMeshComponent::TryBuildOnBeginPlay(FLOAT InDeltaTime)
	{
		WaitedSeconds += InDeltaTime;
		if (WaitedSeconds < NextAttemptSeconds)
		{
			return;
		}

		// Baking before the static mesh assets are loaded produces a navmesh with holes, and
		// the cold data of a mesh component is null until its request finishes. The queue is
		// the only signal available, so it gates the first bake.
		const UINT32 PendingNum = EAssetLoadManager::GetManagerSingleton()->GetPendingRequestNum();
		const BOOL32 bQueueDrained = (PendingNum == 0u);
		const BOOL32 bWaitExpired = (WaitedSeconds >= NavigationAutoBuildWaitSeconds);
		if ((!bQueueDrained) && (!bWaitExpired))
		{
			return;
		}

		ENavigationBuildStats Stats;
		if (!BuildNavigation(Stats))
		{
			// Nothing rasterizable yet: an empty level, or assets that are still streaming in.
			// Retrying slowly keeps the cost negligible while a level builds itself.
			NextAttemptSeconds = WaitedSeconds + NavigationAutoBuildRetrySeconds;
			return;
		}
		bHasBuilt = TRUE;
	}

#if _EDITOR_ONLY
	void PNavMeshComponent::GenerateComponentDetail()
	{
		PNavigationComponent::GenerateComponentDetail();

		if (!ImGui::TreeNodeEx("NavMesh Bake", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return;
		}

		// The widgets edit a copy; the settings reach the provider only through a build, so a
		// partially typed value never changes a running query.
		ENavMeshBuildSettings Settings = BuildSettings;
		bool bSettingsChanged = false;
		bSettingsChanged |= ImGui::DragFloat("Cell Size (cm)", &Settings.CellSize, 0.5f, 5.0f, 100.0f, "%.1f");
		bSettingsChanged |= ImGui::DragFloat("Cell Height (cm)", &Settings.CellHeight, 0.5f, 2.0f, 50.0f, "%.1f");
		bSettingsChanged |= ImGui::DragFloat("Agent Radius (cm)", &Settings.AgentRadius, 0.5f, 5.0f, 200.0f, "%.1f");
		bSettingsChanged |= ImGui::DragFloat("Agent Height (cm)", &Settings.AgentHeight, 0.5f, 20.0f, 400.0f, "%.1f");
		bSettingsChanged |= ImGui::DragFloat("Max Climb (cm)", &Settings.AgentMaxClimb, 0.5f, 0.0f, 200.0f, "%.1f");
		bSettingsChanged |= ImGui::DragFloat("Max Slope (deg)", &Settings.AgentMaxSlopeAngle, 0.5f, 1.0f, 89.0f, "%.1f");
		bSettingsChanged |= ImGui::DragFloat("Max Edge Length (cm, 0=auto)", &Settings.MaxEdgeLength, 1.0f, 0.0f, 2000.0f, "%.1f");
		bSettingsChanged |= ImGui::DragFloat("Simplify Error (cm, 0=auto)", &Settings.MaxSimplificationError, 0.1f, 0.0f, 200.0f, "%.2f");
		bSettingsChanged |= ImGui::DragScalar("Min Region Area (cells)", ImGuiDataType_U32, &Settings.MinRegionArea, 1.0f);
		bSettingsChanged |= ImGui::DragScalar("Merge Region Area (cells)", ImGuiDataType_U32, &Settings.MergeRegionArea, 1.0f);

		INT32 VertsPerPoly = static_cast<INT32>(Settings.MaxVertsPerPoly);
		if (ImGui::SliderInt("Max Verts Per Poly", &VertsPerPoly, 3, 6))
		{
			Settings.MaxVertsPerPoly = static_cast<UINT32>(VertsPerPoly);
			bSettingsChanged = TRUE;
		}

		bool bFlipWinding = (Settings.bFlipTriangleWinding != FALSE);
		if (ImGui::Checkbox("Flip Triangle Winding", &bFlipWinding))
		{
			Settings.bFlipTriangleWinding = (bFlipWinding ? TRUE : FALSE);
			bSettingsChanged = TRUE;
		}
		if (bSettingsChanged)
		{
			BuildSettings = Settings;
		}

		Vector3 Extent = BakeVolumeExtent;
		if (ImGui::DragFloat3("Bake Extent (cm, 0=world)", &Extent.x, 10.0f, 0.0f, 100000.0f, "%.0f"))
		{
			SetBakeVolumeExtent(Extent);
		}

		bool bAutoBuild = (bBuildOnBeginPlay != FALSE);
		if (ImGui::Checkbox("Build On Begin Play", &bAutoBuild))
		{
			bBuildOnBeginPlay = (bAutoBuild ? TRUE : FALSE);
		}

		if (ImGui::Button("Build Navigation"))
		{
			RequestBuildNavigation();
		}

		ImGui::TreePop();
	}

	void PNavMeshComponent::EditorTick(FLOAT InDeltaTime)
	{
		PSceneComponent::EditorTick(InDeltaTime);

		if ((!bDebugDrawEnabled) || (!ENavigationManager::GetManagerSingleton()->IsDebugDrawEnabled()))
		{
			return;
		}
		if ((BakeVolumeExtent.x <= 0.0f) || (BakeVolumeExtent.y <= 0.0f) || (BakeVolumeExtent.z <= 0.0f))
		{
			// A whole world volume has no box worth drawing.
			return;
		}

		RDebugWireframePrimitiveManager* WireframeManager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		WireframeManager->DrawCuboid(GetComponentWorldLocation(), Quaternion::Identity(),
			BakeVolumeExtent.x * 2.0f, BakeVolumeExtent.y * 2.0f, BakeVolumeExtent.z * 2.0f, Color4::Green());
	}
#endif

}  // namespace PigeonEngine
