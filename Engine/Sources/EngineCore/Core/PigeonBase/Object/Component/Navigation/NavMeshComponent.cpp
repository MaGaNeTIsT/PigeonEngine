#include "NavMeshComponent.h"

#include <AssetLoadManager.h>
#include <Editor/EditorLogManager.h>
#include <Navigation/NavigationGeometry.h>
#include <Navigation/NavigationManager.h>
#include <Navigation/NavigationProviderInterface.h>
#include <Navigation/NavigationProviderRegistry.h>
#include <Reflection/Property.h>

#if ENGINE_EDITOR_ONLY
#include <AssetManager.h>
#include <RenderProxy/RenderSingletonObject.h>
#include <imgui.h>
#endif

namespace PigeonEngine
{

	static void SerializeNavMeshBuildSettings(EArchive& InArchive, const CHAR* InName, ENavMeshBuildSettings& InOutValue)
	{
		InArchive.BeginObject(InName);
		InArchive.SerializeFloat(PE_TEXT("CellSize"), InOutValue.CellSize);
		InArchive.SerializeFloat(PE_TEXT("CellHeight"), InOutValue.CellHeight);
		InArchive.SerializeFloat(PE_TEXT("AgentRadius"), InOutValue.AgentRadius);
		InArchive.SerializeFloat(PE_TEXT("AgentHeight"), InOutValue.AgentHeight);
		InArchive.SerializeFloat(PE_TEXT("AgentMaxClimb"), InOutValue.AgentMaxClimb);
		InArchive.SerializeFloat(PE_TEXT("AgentMaxSlopeAngle"), InOutValue.AgentMaxSlopeAngle);
		InArchive.SerializeFloat(PE_TEXT("MaxEdgeLength"), InOutValue.MaxEdgeLength);
		InArchive.SerializeFloat(PE_TEXT("MaxSimplificationError"), InOutValue.MaxSimplificationError);
		InArchive.SerializeFloat(PE_TEXT("DetailSampleDistance"), InOutValue.DetailSampleDistance);
		InArchive.SerializeFloat(PE_TEXT("DetailSampleMaxError"), InOutValue.DetailSampleMaxError);
		InArchive.SerializeUInt32(PE_TEXT("MinRegionArea"), InOutValue.MinRegionArea);
		InArchive.SerializeUInt32(PE_TEXT("MergeRegionArea"), InOutValue.MergeRegionArea);
		InArchive.SerializeUInt32(PE_TEXT("MaxVertsPerPoly"), InOutValue.MaxVertsPerPoly);
		InArchive.SerializeUInt32(PE_TEXT("TileSizeCells"), InOutValue.TileSizeCells);
		InArchive.SerializeUInt32(PE_TEXT("MaxLayersPerTile"), InOutValue.MaxLayersPerTile);
		InArchive.SerializeUInt32(PE_TEXT("MaxCrowdAgents"), InOutValue.MaxCrowdAgents);
		InArchive.SerializeBool(PE_TEXT("FlipTriangleWinding"), InOutValue.bFlipTriangleWinding);
		InArchive.EndObject();
	}

	static void RegisterClassProperties()
	{
		const size_t ClassHash = GetClassHashCode<PNavMeshComponent>();
		AddClassAssetPropertyAccessors<PNavMeshComponent>(ClassHash, PE_TEXT("NavMeshAsset"),
			[](const PNavMeshComponent* Component) -> EString { return Component->GetNavMeshAssetName(); },
			[](PNavMeshComponent* Component, const EString& Value) -> void
			{
				if (Value.Length() > 0u)
				{
					Component->RequestNavMeshAsset(Value);
				}
				else
				{
					Component->ClearNavMeshAsset();
				}
			},
			EPropertyAssetType::NAV_MESH,
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PNavMeshComponent, Vector3>(ClassHash, PE_TEXT("BakeVolumeExtent"),
			[](const PNavMeshComponent* Component) -> Vector3 { return Component->GetBakeVolumeExtent(); },
			[](PNavMeshComponent* Component, const Vector3& Value) -> void { Component->SetBakeVolumeExtent(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassBoolPropertyAccessors<PNavMeshComponent>(ClassHash, PE_TEXT("BuildOnBeginPlay"),
			[](const PNavMeshComponent* Component) -> BOOL32 { return Component->GetBuildOnBeginPlay(); },
			[](PNavMeshComponent* Component, const BOOL32& Value) -> void { Component->SetBuildOnBeginPlay(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyCustom<PNavMeshComponent, ENavMeshBuildSettings>(ClassHash, PE_TEXT("BuildSettings"),
			[](const PNavMeshComponent* Component) -> ENavMeshBuildSettings { return Component->GetBuildSettings(); },
			[](PNavMeshComponent* Component, const ENavMeshBuildSettings& Value) -> void { Component->SetBuildSettings(Value); },
			SerializeNavMeshBuildSettings,
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
	}

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
		RegisterClassProperties();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PNavMeshComponent::PNavMeshComponent()
	{
		POBJ_DEBUGNAME_SET(this, "NavMeshComponent");
		// The component has per frame work of its own: the automatic publish polls the asset
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

	const ENavMeshAsset* PNavMeshComponent::GetNavMeshAsset() const
	{
		return NavMeshAsset;
	}
	const EString& PNavMeshComponent::GetNavMeshAssetName() const
	{
		if (NavMeshAsset)
		{
			return NavMeshAsset->GetAssetName();
		}
		return NavMeshAssetName;
	}

	void PNavMeshComponent::SetNavMeshAsset(const ENavMeshAsset* InAsset)
	{
		if (NavMeshAsset == InAsset)
		{
			return;
		}
		NavMeshAsset = InAsset;
		// The provider in the manager was built from the previous source, so the next tick has to
		// publish again. Until then the old provider keeps answering queries.
		bHasBuilt = FALSE;
		if (!!InAsset)
		{
			PE_LOG_LOG(EString(PE_TEXT("Navigation: using navmesh asset '")) + InAsset->GetAssetFullPathName() + EString(PE_TEXT("'.")));
		}
	}

	void PNavMeshComponent::RequestNavMeshAsset(const EString& InAssetName)
	{
		NavMeshAssetName = InAssetName;
		bAssetRequestIssued = FALSE;
		bAssetLoadFailed = FALSE;
		if (InAssetName.Length() <= 0u)
		{
			return;
		}

		const EString AssetPath(EBaseSettings::ENGINE_NAV_MESH_PATH);
		const EString FullPathName = MakeAssetFullPathName(AssetPath, InAssetName);
		const EString RequestKey = MakeAssetLoadRequestKey(PE_TEXT("NavMesh"), FullPathName);
		PNavMeshComponent* Component = this;
		TFunction<BOOL32()> Step = [Component, AssetPath, FullPathName, RequestKey]() -> BOOL32
		{
			// Cache only, no disk access: this runs every frame until the request settles.
			const ENavMeshAsset* CachedAsset = ENavMeshAssetManager::GetManagerSingleton()->FindNavMeshAsset(AssetPath, Component->NavMeshAssetName);
			if (!!CachedAsset)
			{
				Component->SetNavMeshAsset(CachedAsset);
				return TRUE;
			}
			// One request, issued once. Re-requesting while the first one is still queued would make
			// a missing file retry and log forever.
			if (!Component->bAssetRequestIssued)
			{
				Component->bAssetRequestIssued = TRUE;
				const ENavMeshAsset* LoadedAsset = nullptr;
				ENavMeshAssetManager::GetManagerSingleton()->RequireLoadNavMeshAsset(AssetPath, Component->NavMeshAssetName, LoadedAsset);
				return FALSE;
			}
			if (EAssetLoadManager::GetManagerSingleton()->IsRequestPending(RequestKey))
			{
				return FALSE;
			}
			// The request finished without producing an asset: the file is missing or was rejected.
			// Report it once and let the bake path take over.
			PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh asset '")) + FullPathName
				+ EString(PE_TEXT("' is missing or invalid; the component will bake from the world instead.")));
			Component->bAssetLoadFailed = TRUE;
			return TRUE;
		};
		// Tried once here as well, so an asset that is already loaded is applied before this call
		// returns rather than a frame later.
		if (Step() == FALSE)
		{
			AddDeferredApply(static_cast<TFunction<BOOL32()>&&>(Step));
		}
	}

	void PNavMeshComponent::ClearNavMeshAsset()
	{
		NavMeshAssetName = EString();
		bAssetRequestIssued = FALSE;
		bAssetLoadFailed = FALSE;
		SetNavMeshAsset(nullptr);
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

	void PNavMeshComponent::PublishProvider(ENavigationProviderInterface* InProvider, const ENavigationBuildStats& InStats)
	{
		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		Manager->RegisterProvider(GetUniqueID(), InProvider);
		Manager->SetProviderDebugDrawEnabled(GetUniqueID(), bDebugDrawEnabled);
		LastBuildStats = InStats;
	}

	ERecastNavigationProvider* PNavMeshComponent::CreateProvider()
	{
		ENavigationProviderInterface* CreatedProvider = ENavigationProviderRegistry::Create(GetProviderName());
		if (!CreatedProvider)
		{
			return nullptr;
		}
		// This component is the Recast backend's author facing half, so the settings it holds are
		// that provider's settings. The cast is the single place where the coupling is visible,
		// and it cannot fail: the provider name comes from the same class the cast targets.
		ERecastNavigationProvider* Provider = dynamic_cast<ERecastNavigationProvider*>(CreatedProvider);
		if (!Provider)
		{
			delete CreatedProvider;
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: the provider registered under the navmesh name is not a Recast provider.")));
			return nullptr;
		}
		Provider->SetSettings(BuildSettings);
		return Provider;
	}

	BOOL32 PNavMeshComponent::StartBakeFromWorld()
	{
		PWorld* World = GetWorld();
		if (!World)
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: build requested for a navmesh component that is not in a world.")));
			return FALSE;
		}

		ERecastNavigationProvider* Provider = CreateProvider();
		if (!Provider)
		{
			return FALSE;
		}

		// Collection reads the world, so it stays here; only the Recast part runs on the worker.
		ENavigationSourceGeometry Geometry;
		ENavigationGeometryCollector::CollectStaticMeshTriangles(*World, MakeBakeBoundsInWorld(), BuildSettings.bFlipTriangleWinding, Geometry);

		if (!ENavigationManager::GetManagerSingleton()->RequestBuild(GetUniqueID(), Provider, static_cast<ENavigationSourceGeometry&&>(Geometry)))
		{
			// Either a bake is already running for this volume (nothing to do) or the provider was
			// refused; in both cases this one is not needed.
			delete Provider;
			return FALSE;
		}
		bBakeFailed = FALSE;
		return TRUE;
	}

	BOOL32 PNavMeshComponent::BuildFromAsset(ENavigationBuildStats& OutStats)
	{
		OutStats = ENavigationBuildStats();

		const ENavMeshData* Data = (!!NavMeshAsset) ? NavMeshAsset->GetStoragedResource() : nullptr;
		if ((!Data) || (!Data->IsResourceValid()))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the assigned navmesh asset has no usable data; baking from the world instead.")));
			return FALSE;
		}
		if (Data->GetSettings().ProviderName != GetProviderName().ToString())
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the assigned navmesh asset was baked by '"))
				+ Data->GetSettings().ProviderName + EString(PE_TEXT("', which this component cannot import; baking from the world instead.")));
			return FALSE;
		}

		ENavigationProviderInterface* CreatedProvider = ENavigationProviderRegistry::Create(GetProviderName());
		if (!CreatedProvider)
		{
			return FALSE;
		}
		ERecastNavigationProvider* Provider = dynamic_cast<ERecastNavigationProvider*>(CreatedProvider);
		if (!Provider)
		{
			delete CreatedProvider;
			return FALSE;
		}

		const FNavMeshAssetSettings& AssetSettings = Data->GetSettings();
		Provider->SetSettings(AssetSettings.BuildSettings);

		const TArray<UINT8>& Blob = Data->GetBlob();
		if (!Provider->ImportData(Blob.GetData(), Blob.Num<UINT32>()))
		{
			delete Provider;
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the assigned navmesh asset could not be imported; baking from the world instead.")));
			return FALSE;
		}

		OutStats.PolygonCount = AssetSettings.PolygonCount;
		OutStats.TileCount = AssetSettings.TileCount;
		PublishProvider(Provider, OutStats);
		return TRUE;
	}

	BOOL32 PNavMeshComponent::BuildNavigation(ENavigationBuildStats& OutStats)
	{
		// The shipped path first: a level that carries a baked mesh never pays for a bake, and a
		// mesh that fails to import falls through to the bake instead of leaving the level without
		// navigation.
		if ((!!NavMeshAsset) && (BuildFromAsset(OutStats)))
		{
			return TRUE;
		}

		// The world path is asynchronous: this only queues it. Returns FALSE because no provider is
		// live yet; the manager publishes it and Tick reports it.
		OutStats = ENavigationBuildStats();
		StartBakeFromWorld();
		return FALSE;
	}

	void PNavMeshComponent::Tick(FLOAT InDeltaTime)
	{
		PSceneComponent::Tick(InDeltaTime);

		// The manager publishes a finished bake before the world ticks, so a provider that became
		// ready this frame is already answering queries.
		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		const ENavigationBuildStateType BuildState = Manager->GetBuildState(GetUniqueID());
		if (BuildState == ENavigationBuildStateType::NAVIGATION_BUILD_READY)
		{
			if (!bHasBuilt)
			{
				bHasBuilt = TRUE;
				LastBuildStats = Manager->GetBuildStats(GetUniqueID());
			}
			bBuildRequested = FALSE;
			bBakeFailed = FALSE;
		}
		else if (BuildState == ENavigationBuildStateType::NAVIGATION_BUILD_FAILED)
		{
			bBuildRequested = FALSE;
			bBakeFailed = TRUE;
		}

		if (bBuildRequested)
		{
			// An explicit rebuild always comes from the world: the button exists to pick up geometry
			// changes, which the asset cannot know about. It is queued, not executed here.
			bBuildRequested = FALSE;
			StartBakeFromWorld();
			return;
		}

		if ((bBuildOnBeginPlay) && (!bHasBuilt))
		{
			TryBuildOnBeginPlay(InDeltaTime);
		}
	}

	void PNavMeshComponent::TryBuildOnBeginPlay(FLOAT InDeltaTime)
	{
		// A requested asset is part of the load queue, but its completion callback (the deferred
		// apply that assigns it) can run a frame after the queue drains. Waiting for that frame
		// prevents baking a mesh the asset was about to replace.
		const BOOL32 bAssetOutstanding = ((NavMeshAssetName.Length() > 0u) && (!NavMeshAsset) && (!bAssetLoadFailed)) ? TRUE : FALSE;
		if (bAssetOutstanding)
		{
			return;
		}

		WaitedSeconds += InDeltaTime;
		if (WaitedSeconds < NextAttemptSeconds)
		{
			return;
		}

		// Baking before the static mesh assets are loaded produces a navmesh with holes, and the
		// cold data of a mesh component is null until its request finishes. The queue is the only
		// signal available, so it gates the first bake.
		const UINT32 PendingNum = EAssetLoadManager::GetManagerSingleton()->GetPendingRequestNum();
		const BOOL32 bQueueDrained = (PendingNum == 0u);
		const BOOL32 bWaitExpired = (WaitedSeconds >= NavigationAutoBuildWaitSeconds);
		if ((!bQueueDrained) && (!bWaitExpired))
		{
			return;
		}

		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		if (Manager->GetBuildState(GetUniqueID()) == ENavigationBuildStateType::NAVIGATION_BUILD_RUNNING)
		{
			// A queued bake is in flight; Tick reports the result.
			return;
		}

		if (!!NavMeshAsset)
		{
			// Importing an asset is a memory copy and finishes the publish right here.
			ENavigationBuildStats AssetStats;
			if (BuildFromAsset(AssetStats))
			{
				bHasBuilt = TRUE;
				return;
			}
		}

		if (bBakeFailed)
		{
			// The last queued bake produced nothing. Retrying only helps while the asset queue is
			// still busy; otherwise the level has nothing rasterizable and a manual rebuild is the
			// only way forward.
			if (PendingNum == 0u)
			{
				bHasBuilt = TRUE;
				PE_LOG_WARN(EString(PE_TEXT("Navigation: no navmesh could be built from the current world; use Rebuild From World after adding walkable geometry.")));
				return;
			}
			NextAttemptSeconds = WaitedSeconds + NavigationAutoBuildRetrySeconds;
			return;
		}

		ENavigationBuildStats Stats;
		if (!BuildNavigation(Stats))
		{
			// Either the bake was queued (the usual case) or nothing could be collected. Both are
			// observed through the build state on the following ticks.
			return;
		}
		if (Stats.SkippedMeshCount == 0u)
		{
			bHasBuilt = TRUE;
			return;
		}
		if (PendingNum == 0u)
		{
			// Nothing is loading any more, so the components that were skipped are not going to
			// recover by themselves. Keep the incomplete mesh and let the warning say so; a manual
			// rebuild picks up whatever was fixed.
			bHasBuilt = TRUE;
			PE_LOG_WARN(EString(PE_TEXT("Navigation: automatic bake kept a mesh with "))
				+ EString::FromInt(Stats.SkippedMeshCount)
				+ EString(PE_TEXT(" skipped static mesh components; rebuild manually once they load.")));
			return;
		}
		// Something is still loading: bake again shortly instead of freezing the holes into the
		// navmesh for the rest of the session.
		NextAttemptSeconds = WaitedSeconds + NavigationAutoBuildRetrySeconds;
	}

#if ENGINE_EDITOR_ONLY
	ERecastNavigationProvider* PNavMeshComponent::BakeProviderSynchronously(ENavigationBuildStats& OutStats)
	{
		OutStats = ENavigationBuildStats();

		PWorld* World = GetWorld();
		if (!World)
		{
			return nullptr;
		}
		ERecastNavigationProvider* Provider = CreateProvider();
		if (!Provider)
		{
			return nullptr;
		}
		ENavigationSourceGeometry Geometry;
		ENavigationGeometryCollector::CollectStaticMeshTriangles(*World, MakeBakeBoundsInWorld(), BuildSettings.bFlipTriangleWinding, Geometry);
		if (!Provider->Build(Geometry, OutStats))
		{
			delete Provider;
			return nullptr;
		}
		return Provider;
	}

	BOOL32 PNavMeshComponent::SaveBakeAsAsset(const EString& InAssetName, EString& OutError)
	{
		OutError = EString();
		if (InAssetName.Length() <= 0u)
		{
			OutError = EString(PE_TEXT("Enter a name for the navmesh asset first."));
			return FALSE;
		}

		// Deliberately synchronous: saving is an explicit editor action, and it has to publish the
		// same bytes the file holds.
		ENavigationBuildStats Stats;
		ERecastNavigationProvider* Provider = BakeProviderSynchronously(Stats);
		if (!Provider)
		{
			OutError = EString(PE_TEXT("Bake produced no navmesh; nothing to save."));
			return FALSE;
		}

		TArray<UINT8> Blob;
		if (!Provider->ExportData(Blob))
		{
			delete Provider;
			OutError = EString(PE_TEXT("Baked navmesh could not be exported."));
			return FALSE;
		}

		FNavMeshAssetSettings Settings;
		Settings.ProviderName = GetProviderName().ToString();
		Settings.BuildSettings = BuildSettings;
		Settings.Bounds = Provider->GetBounds();
		Settings.PolygonCount = Stats.PolygonCount;
		Settings.TileCount = Stats.TileCount;

		const ENavMeshAsset* SavedAsset = nullptr;
		if (!ENavMeshAssetManager::GetManagerSingleton()->CreateNavMeshAsset(
			InAssetName, Settings, Blob, EString(EBaseSettings::ENGINE_NAV_MESH_PATH), SavedAsset, OutError))
		{
			delete Provider;
			return FALSE;
		}

		// Publish the provider that was written, then point the component at the file so the next
		// load of the level takes the asset path. The flag is set after SetNavMeshAsset because
		// assigning an asset deliberately clears it to force a republish.
		PublishProvider(Provider, Stats);
		SetNavMeshAsset(SavedAsset);
		NavMeshAssetName = InAssetName;
		bHasBuilt = TRUE;
		// The browser only rescans after its own create/rename/delete actions, so a bake has to
		// ask for one explicitly.
		EAssetManager::GetManagerSingleton()->RequestContentRefresh();
		return TRUE;
	}

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
		bSettingsChanged |= ImGui::DragFloat("Detail Sample Dist (cm, 0=auto)", &Settings.DetailSampleDistance, 0.5f, 0.0f, 500.0f, "%.1f");
		bSettingsChanged |= ImGui::DragFloat("Detail Sample Error (cm, 0=auto)", &Settings.DetailSampleMaxError, 0.1f, 0.0f, 100.0f, "%.2f");
		bSettingsChanged |= ImGui::DragScalar("Min Region Area (cells)", ImGuiDataType_U32, &Settings.MinRegionArea, 1.0f);
		bSettingsChanged |= ImGui::DragScalar("Merge Region Area (cells)", ImGuiDataType_U32, &Settings.MergeRegionArea, 1.0f);
		bSettingsChanged |= ImGui::DragScalar("Tile Size (cells, 0=single)", ImGuiDataType_U32, &Settings.TileSizeCells, 1.0f);
		bSettingsChanged |= ImGui::DragScalar("Max Layers Per Tile", ImGuiDataType_U32, &Settings.MaxLayersPerTile, 1.0f);

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

		ImGui::Separator();
		const ENavigationBuildStateType BuildState = ENavigationManager::GetManagerSingleton()->GetBuildState(GetUniqueID());
		if (BuildState == ENavigationBuildStateType::NAVIGATION_BUILD_RUNNING)
		{
			ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "Baking on the worker...");
		}
		else if (BuildState == ENavigationBuildStateType::NAVIGATION_BUILD_FAILED)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Last bake produced no navmesh.");
		}
		if (ImGui::Button("Rebuild From World"))
		{
			RequestBuildNavigation();
		}

		ImGui::InputText("Asset Name", AssetNameBuffer, sizeof(AssetNameBuffer));
		const BOOL32 bCanSave = (AssetNameBuffer[0] != '\0') ? TRUE : FALSE;
		if (!bCanSave)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Bake & Save As Asset"))
		{
			LastAssetError = EString();
			EString SaveError;
			if (!SaveBakeAsAsset(EString(AssetNameBuffer), SaveError))
			{
				LastAssetError = SaveError;
			}
		}
		if (!bCanSave)
		{
			ImGui::EndDisabled();
		}
		ImGui::SameLine();
		if (ImGui::Button("Load Asset"))
		{
			LastAssetError = EString();
			if (AssetNameBuffer[0] != '\0')
			{
				RequestNavMeshAsset(EString(AssetNameBuffer));
			}
		}
		if (LastAssetError.Length() > 0u)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", *LastAssetError);
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
