#include <Navigation/NavigationManager.h>

#include <AssetLoadManager.h>
#include <Development/EditorLogManager.h>
#include <Navigation/NavigationProviderInterface.h>
#include <PigeonBase/Object/Component/Navigation/NavMeshComponent.h>
#include <PigeonBase/Object/World/World.h>

namespace PigeonEngine
{

	// The asset queue gates the first bake: cold mesh data is null until its request finishes, and a
	// LOAD pass must not import/bake before the level's assets settled. One wait now covers the
	// whole level.
	PE_CONSTEXPR_STATIC FLOAT NavigationWorldBuildAssetWaitSeconds = 30.0f;
	// Editor input changes (a volume drag bumps the revision every frame) rebuild once they settle.
	PE_CONSTEXPR_STATIC FLOAT NavigationWorldBuildDebounceSeconds = 0.5f;

	// A runtime pass waits only for the profiles' own navmesh assets, not for the whole asset
	// queue: a level's textures may still be streaming in while the navmesh is ready to import.
	static BOOL32 HasOutstandingProfileAssets(const PWorld& InWorld)
	{
		TArray<PNavMeshComponent*> Owners;
		PNavMeshComponent::CollectPrimaryOwnersInWorld(InWorld, Owners);
		for (UINT32 Index = 0u, Num = Owners.Num<UINT32>(); Index < Num; Index++)
		{
			if ((!!Owners[Index]) && (Owners[Index]->HasOutstandingAssetRequest() != FALSE))
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<ENavigationManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	// An invalid bound on either side means "unbounded", so a provider whose build did not
	// pin a volume still answers for any location.
	static BOOL32 BoundsContainLocation(const EBoundAABB& InBounds, const Vector3& InLocation)
	{
		if (!InBounds.IsValid)
		{
			return TRUE;
		}
		return ((InLocation.x >= InBounds.AABBMin.x) && (InLocation.x <= InBounds.AABBMax.x)
			&& (InLocation.y >= InBounds.AABBMin.y) && (InLocation.y <= InBounds.AABBMax.y)
			&& (InLocation.z >= InBounds.AABBMin.z) && (InLocation.z <= InBounds.AABBMax.z));
	}

	ENavigationManager::ENavigationManager()
		: BakeThread(PE_TEXT("NavigationBake"))
		, BakeWakeEvent(FALSE)
	{
	}

	ENavigationManager::~ENavigationManager()
	{
		ShutDown();
	}

	void ENavigationManager::Initialize()
	{
		if (bBakeWorkerStarted)
		{
			return;
		}
		bBakeWorkerStarted = TRUE;
		ENavigationManager* Manager = this;
		BakeThread.Start([Manager]() -> void
		{
			Manager->BakeWorkerLoop();
		});
	}

	void ENavigationManager::ShutDown()
	{
		StopBakeWorker();
		ReleaseAllProviders();
		InvalidateDebugLines();
	}

	void ENavigationManager::Tick(FLOAT InDeltaTime)
	{
		// A LOAD/RUNTIME pass waiting for the asset queue: this is the one wait for the whole level.
		TickPendingWorldBuilds(InDeltaTime);

		PublishFinishedBakes();

#if ENGINE_EDITOR_ONLY
		// Authored inputs changed: one debounced pass per world.
		TickWorldBuildInputs(InDeltaTime);
#endif

		// Per provider work: dynamic obstacles rebuild tiles here, and later local avoidance steps
		// its agents. It runs after publishing so a provider that became ready this frame is already
		// driving, and before the world tick so agents see the result.
		for (UINT32 WorldIndex = 0u, WorldNum = Worlds.Num<UINT32>(); WorldIndex < WorldNum; WorldIndex++)
		{
			FNavigationWorld& WorldState = Worlds[WorldIndex];
			for (UINT32 Index = 0u, Num = WorldState.Providers.Num<UINT32>(); Index < Num; Index++)
			{
				ENavigationProviderInterface* Provider = WorldState.Providers[Index].Provider;
				if ((!!Provider) && (Provider->IsReady()))
				{
					Provider->Tick(InDeltaTime);
				}
			}
		}
	}

	// --- World state ---------------------------------------------------------

	ENavigationManager::FNavigationWorld* ENavigationManager::FindWorldState(const PWorld* InWorld)
	{
		for (UINT32 Index = 0u, Num = Worlds.Num<UINT32>(); Index < Num; Index++)
		{
			if (Worlds[Index].World == InWorld)
			{
				return &Worlds[Index];
			}
		}
		return nullptr;
	}

	const ENavigationManager::FNavigationWorld* ENavigationManager::FindWorldState(const PWorld* InWorld) const
	{
		for (UINT32 Index = 0u, Num = Worlds.Num<UINT32>(); Index < Num; Index++)
		{
			if (Worlds[Index].World == InWorld)
			{
				return &Worlds[Index];
			}
		}
		return nullptr;
	}

	ENavigationManager::FNavigationWorld& ENavigationManager::FindOrCreateWorldState(PWorld* InWorld)
	{
		if (FNavigationWorld* Existing = FindWorldState(InWorld))
		{
			return *Existing;
		}
		FNavigationWorld NewState;
		NewState.World = InWorld;
		Worlds.Add(NewState);
		return Worlds[Worlds.Num<UINT32>() - 1u];
	}

	void ENavigationManager::NotifyWorldCreated(PWorld* InWorld)
	{
		(void)FindOrCreateWorldState(InWorld);
	}

	void ENavigationManager::NotifyWorldDestroyed(PWorld* InWorld)
	{
		for (UINT32 Index = Worlds.Num<UINT32>(); Index > 0u; Index--)
		{
			FNavigationWorld& WorldState = Worlds[Index - 1u];
			if (WorldState.World != InWorld)
			{
				continue;
			}
			for (UINT32 ProviderIndex = 0u, ProviderNum = WorldState.Providers.Num<UINT32>(); ProviderIndex < ProviderNum; ProviderIndex++)
			{
				delete WorldState.Providers[ProviderIndex].Provider;
				WorldState.Providers[ProviderIndex].Provider = nullptr;
			}
			Worlds.RemoveAt(Index - 1u);
		}
		InvalidateDebugLines();
	}

	// --- Providers -----------------------------------------------------------

	INT32 ENavigationManager::FindProviderIndex(const FNavigationWorld& InWorld, const EString& InProfileName) const
	{
		for (UINT32 Index = 0u, Num = InWorld.Providers.Num<UINT32>(); Index < Num; Index++)
		{
			if (InWorld.Providers[Index].ProfileName == InProfileName)
			{
				return static_cast<INT32>(Index);
			}
		}
		return -1;
	}

	void ENavigationManager::RegisterProvider(PWorld* InWorld, const ObjectIdentityType& InSourceId, const EString& InProfileName, ENavigationProviderInterface* InProvider, const ENavigationBuildStats& InStats)
	{
		if ((!InWorld) || (!InProvider))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: registering a null provider or world was ignored.")));
			return;
		}
		FNavigationWorld& WorldState = FindOrCreateWorldState(InWorld);

		UINT32 Serial = 1u;
		const INT32 ExistingIndex = FindProviderIndex(WorldState, InProfileName);
		if (ExistingIndex >= 0)
		{
			// A replacement releases the old provider here rather than at the call site so
			// ownership never depends on who built what. Republishing the same object is a
			// state refresh, not a replacement: deleting it would leave the entry dangling.
			FNavigationProviderEntry& Entry = WorldState.Providers[static_cast<UINT32>(ExistingIndex)];
			if (Entry.Provider != InProvider)
			{
				delete Entry.Provider;
			}
			Entry.Provider = InProvider;
			Entry.SourceId = InSourceId;
			Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_READY;
			Entry.Stats = InStats;
			// A bake queued before this publish is older than what is being installed now.
			Entry.Serial++;
			Serial = Entry.Serial;
		}
		else
		{
			FNavigationProviderEntry Entry;
			Entry.ProfileName = InProfileName;
			Entry.SourceId = InSourceId;
			Entry.Provider = InProvider;
			Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_READY;
			Entry.Serial = 1u;
			Entry.Stats = InStats;
			Serial = Entry.Serial;
			WorldState.Providers.Add(Entry);
		}
		// The replaced provider took its crowd agents and obstacles with it, so both are
		// restored on the new one before any query can reach it.
		InvalidateCrowdBindings(WorldState, InProfileName);
		ReplayObstacles(WorldState, InProfileName, Serial, *InProvider);
		InvalidateDebugLines();
	}

	BOOL32 ENavigationManager::RequestBuild(PWorld* InWorld, const ObjectIdentityType& InSourceId, const EString& InProfileName, ENavigationProviderInterface* InProvider, ENavigationSourceGeometry&& InGeometry)
	{
		return RequestBuildInternal(InWorld, InSourceId, InProfileName, InProvider, static_cast<ENavigationSourceGeometry&&>(InGeometry), 0u);
	}

	BOOL32 ENavigationManager::RequestBuildInternal(PWorld* InWorld, const ObjectIdentityType& InSourceId, const EString& InProfileName, ENavigationProviderInterface* InProvider, ENavigationSourceGeometry&& InGeometry, const UINT32& InBatchId)
	{
		if ((!InWorld) || (!InProvider))
		{
			return FALSE;
		}
		FNavigationWorld& WorldState = FindOrCreateWorldState(InWorld);

		INT32 EntryIndex = FindProviderIndex(WorldState, InProfileName);
		if (EntryIndex < 0)
		{
			FNavigationProviderEntry Entry;
			Entry.ProfileName = InProfileName;
			Entry.SourceId = InSourceId;
			WorldState.Providers.Add(Entry);
			EntryIndex = static_cast<INT32>(WorldState.Providers.Num<UINT32>() - 1u);
		}
		else
		{
			WorldState.Providers[static_cast<UINT32>(EntryIndex)].SourceId = InSourceId;
		}

		FNavigationProviderEntry& Entry = WorldState.Providers[static_cast<UINT32>(EntryIndex)];
		if (Entry.State == ENavigationBuildStateType::NAVIGATION_BUILD_RUNNING)
		{
			// Already baking this profile: the caller keeps its provider and the running job wins.
			return FALSE;
		}

		Entry.Serial++;
		Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_RUNNING;

		{
			EScopedLock Lock(BakeMutex);
			FBakeJob Job;
			Job.World = InWorld;
			Job.SourceId = InSourceId;
			Job.ProfileName = InProfileName;
			Job.Serial = Entry.Serial;
			Job.BatchId = InBatchId;
			Job.Provider = InProvider;
			Job.Geometry = static_cast<ENavigationSourceGeometry&&>(InGeometry);
			BakeJobs.Add(Job);
		}
		BakeWakeEvent.Signal();
		return TRUE;
	}

	void ENavigationManager::UnregisterProvider(PWorld* InWorld, const EString& InProfileName)
	{
		FNavigationWorld* WorldState = FindWorldState(InWorld);
		if (!WorldState)
		{
			return;
		}
		const INT32 ExistingIndex = FindProviderIndex(*WorldState, InProfileName);
		if (ExistingIndex < 0)
		{
			return;
		}
		// Dropping the entry means a bake still running for it is discarded when it reports back.
		delete WorldState->Providers[static_cast<UINT32>(ExistingIndex)].Provider;
		WorldState->Providers[static_cast<UINT32>(ExistingIndex)].Provider = nullptr;
		WorldState->Providers.RemoveAt(static_cast<UINT32>(ExistingIndex));

		// Obstacles and crowd handles are namespaces of the dead provider; keeping them would
		// let a later provider interpret a handle it never issued. Obstacles stay retained and
		// unowned so a replacement provider of the same profile can adopt them.
		for (UINT32 Index = 0u, Num = WorldState->Obstacles.Num<UINT32>(); Index < Num; Index++)
		{
			if (WorldState->Obstacles[Index].ProviderProfile == InProfileName)
			{
				WorldState->Obstacles[Index].ProviderHandle = NavigationInvalidObstacleHandle;
				WorldState->Obstacles[Index].ProviderProfile = EString();
				WorldState->Obstacles[Index].ProviderSerial = 0u;
			}
		}
		InvalidateCrowdBindings(*WorldState, InProfileName);
		InvalidateDebugLines();
	}

	void ENavigationManager::UnregisterProfilesBySource(PWorld* InWorld, const ObjectIdentityType& InSourceId)
	{
		TArray<EString> RemovedProfiles;
		for (UINT32 WorldIndex = 0u, WorldNum = Worlds.Num<UINT32>(); WorldIndex < WorldNum; WorldIndex++)
		{
			FNavigationWorld& WorldState = Worlds[WorldIndex];
			if ((InWorld) && (WorldState.World != InWorld))
			{
				continue;
			}
			for (UINT32 Index = WorldState.Providers.Num<UINT32>(); Index > 0u; Index--)
			{
				FNavigationProviderEntry& Entry = WorldState.Providers[Index - 1u];
				if (Entry.SourceId != InSourceId)
				{
					continue;
				}
				RemovedProfiles.Add(Entry.ProfileName);
				delete Entry.Provider;
				Entry.Provider = nullptr;
				WorldState.Providers.RemoveAt(Index - 1u);
			}
			for (UINT32 ProfileIndex = 0u; ProfileIndex < RemovedProfiles.Num<UINT32>(); ProfileIndex++)
			{
				for (UINT32 Index = 0u, Num = WorldState.Obstacles.Num<UINT32>(); Index < Num; Index++)
				{
					if (WorldState.Obstacles[Index].ProviderProfile == RemovedProfiles[ProfileIndex])
					{
						WorldState.Obstacles[Index].ProviderHandle = NavigationInvalidObstacleHandle;
						WorldState.Obstacles[Index].ProviderProfile = EString();
						WorldState.Obstacles[Index].ProviderSerial = 0u;
					}
				}
				InvalidateCrowdBindings(WorldState, RemovedProfiles[ProfileIndex]);
			}
			RemovedProfiles.Empty(0u);
		}
		InvalidateDebugLines();
	}

	void ENavigationManager::SetProviderDebugDrawEnabled(PWorld* InWorld, const EString& InProfileName, const BOOL32& InEnabled)
	{
		FNavigationWorld* WorldState = FindWorldState(InWorld);
		if (!WorldState)
		{
			return;
		}
		const INT32 ExistingIndex = FindProviderIndex(*WorldState, InProfileName);
		if (ExistingIndex < 0)
		{
			return;
		}
		if (WorldState->Providers[static_cast<UINT32>(ExistingIndex)].bDebugDrawEnabled == InEnabled)
		{
			return;
		}
		WorldState->Providers[static_cast<UINT32>(ExistingIndex)].bDebugDrawEnabled = InEnabled;
		InvalidateDebugLines();
	}

	ENavigationProviderInterface* ENavigationManager::GetProvider(PWorld* InWorld, const EString& InProfileName) const
	{
		const FNavigationWorld* WorldState = FindWorldState(InWorld);
		if (!WorldState)
		{
			return nullptr;
		}
		const INT32 ExistingIndex = FindProviderIndex(*WorldState, InProfileName);
		return (ExistingIndex >= 0) ? WorldState->Providers[static_cast<UINT32>(ExistingIndex)].Provider : nullptr;
	}

	ENavigationProviderInterface* ENavigationManager::GetDefaultProvider(PWorld* InWorld, const EString& InProfileName) const
	{
		const FNavigationWorld* WorldState = FindWorldState(InWorld);
		if (!WorldState)
		{
			return nullptr;
		}
		for (UINT32 Index = 0u, Num = WorldState->Providers.Num<UINT32>(); Index < Num; Index++)
		{
			const FNavigationProviderEntry& Entry = WorldState->Providers[Index];
			if ((Entry.ProfileName != InProfileName) || (!Entry.Provider) || (!Entry.Provider->IsReady()))
			{
				continue;
			}
			return Entry.Provider;
		}
		return nullptr;
	}

	INT32 ENavigationManager::FindProviderIndexForLocation(const FNavigationWorld& InWorld, const Vector3& InLocation, const EString& InProfileName) const
	{
		INT32 FirstReady = -1;
		for (UINT32 Index = 0u, Num = InWorld.Providers.Num<UINT32>(); Index < Num; Index++)
		{
			const FNavigationProviderEntry& Entry = InWorld.Providers[Index];
			if ((Entry.ProfileName != InProfileName) || (!Entry.Provider) || (!Entry.Provider->IsReady()))
			{
				continue;
			}
			if (FirstReady < 0)
			{
				FirstReady = static_cast<INT32>(Index);
			}
			if (BoundsContainLocation(Entry.Provider->GetBounds(), InLocation))
			{
				return static_cast<INT32>(Index);
			}
		}
		return FirstReady;
	}

	ENavigationProviderInterface* ENavigationManager::FindProviderForLocation(PWorld* InWorld, const Vector3& InLocation, const EString& InProfileName) const
	{
		// A profile never falls back to another profile's mesh: an agent configured for one agent
		// size must not silently walk on a different one. Inside the profile the covering provider
		// wins, and outside every volume of it the profile's first ready provider projects the
		// stray point onto its own mesh.
		const FNavigationWorld* WorldState = FindWorldState(InWorld);
		if (!WorldState)
		{
			return nullptr;
		}
		const INT32 Index = FindProviderIndexForLocation(*WorldState, InLocation, InProfileName);
		return (Index >= 0) ? WorldState->Providers[static_cast<UINT32>(Index)].Provider : nullptr;
	}

	ENavigationProviderInterface* ENavigationManager::GetProviderForProfile(const FNavigationWorld& InWorld, const EString& InProfileName, const UINT32& InSerial) const
	{
		const INT32 ExistingIndex = FindProviderIndex(InWorld, InProfileName);
		if (ExistingIndex < 0)
		{
			return nullptr;
		}
		const FNavigationProviderEntry& Entry = InWorld.Providers[static_cast<UINT32>(ExistingIndex)];
		if ((Entry.Serial != InSerial) || (!Entry.Provider) || (!Entry.Provider->IsReady()))
		{
			// A rebuild replaced the provider the caller's handle belonged to.
			return nullptr;
		}
		return Entry.Provider;
	}

	UINT32 ENavigationManager::GetProviderNum(PWorld* InWorld) const
	{
		const FNavigationWorld* WorldState = FindWorldState(InWorld);
		return WorldState ? WorldState->Providers.Num<UINT32>() : 0u;
	}

	ENavigationBuildStateType ENavigationManager::GetBuildState(PWorld* InWorld, const EString& InProfileName) const
	{
		const FNavigationWorld* WorldState = FindWorldState(InWorld);
		if (!WorldState)
		{
			return ENavigationBuildStateType::NAVIGATION_BUILD_IDLE;
		}
		const INT32 ExistingIndex = FindProviderIndex(*WorldState, InProfileName);
		if (ExistingIndex < 0)
		{
			return ENavigationBuildStateType::NAVIGATION_BUILD_IDLE;
		}
		return WorldState->Providers[static_cast<UINT32>(ExistingIndex)].State;
	}

	const ENavigationBuildStats& ENavigationManager::GetBuildStats(PWorld* InWorld, const EString& InProfileName) const
	{
		static const ENavigationBuildStats EmptyStats;
		const FNavigationWorld* WorldState = FindWorldState(InWorld);
		if (!WorldState)
		{
			return EmptyStats;
		}
		const INT32 ExistingIndex = FindProviderIndex(*WorldState, InProfileName);
		if (ExistingIndex < 0)
		{
			return EmptyStats;
		}
		return WorldState->Providers[static_cast<UINT32>(ExistingIndex)].Stats;
	}

	void ENavigationManager::BumpNavigationInputRevision(PWorld* InWorld)
	{
		if (!InWorld)
		{
			return;
		}
		FindOrCreateWorldState(InWorld).InputRevision++;
	}

	UINT32 ENavigationManager::GetNavigationInputRevision(const PWorld* InWorld) const
	{
		const FNavigationWorld* WorldState = FindWorldState(InWorld);
		return WorldState ? WorldState->InputRevision : 1u;
	}

	// --- World build pass ----------------------------------------------------

	BOOL32 ENavigationManager::IsWorldBuildRunning(PWorld& InWorld) const
	{
		const FNavigationWorld* WorldState = FindWorldState(&InWorld);
		return ((WorldState) && (WorldState->Build.Pending > 0u)) ? TRUE : FALSE;
	}

	const ENavigationManager::FNavigationWorldBuildReport& ENavigationManager::GetWorldBuildReport(PWorld& InWorld) const
	{
		static const FNavigationWorldBuildReport EmptyReport;
		const FNavigationWorld* WorldState = FindWorldState(&InWorld);
		return (!!WorldState) ? WorldState->Build.Report : EmptyReport;
	}

	BOOL32 ENavigationManager::RequestWorldBuild(PWorld& InWorld, ENavigationWorldBuildReason InReason, const EString& InLevelStem)
	{
		FNavigationWorld& WorldState = FindOrCreateWorldState(&InWorld);
		if (WorldState.Build.Pending > 0u)
		{
			// A pass is running; repeat once with the latest world state when it finishes.
			WorldState.Build.bDirtyDuringBuild = TRUE;
			return TRUE;
		}

		// An explicit bake cancels a pending load wait: it deliberately bakes from the world and
		// has no reason to wait for the asset queue.
		if (WorldState.bHasPendingBuild)
		{
			if ((InReason == ENavigationWorldBuildReason::EDITOR) || (InReason == ENavigationWorldBuildReason::SAVE))
			{
				WorldState.bHasPendingBuild = FALSE;
			}
			else
			{
				WorldState.Pending.Reason = InReason;
				WorldState.Pending.LevelStem = InLevelStem;
				return TRUE;
			}
		}

		if (((InReason == ENavigationWorldBuildReason::LOAD)
				&& (EAssetLoadManager::GetManagerSingleton()->GetPendingRequestNum() > 0u))
			|| ((InReason == ENavigationWorldBuildReason::RUNTIME) && (HasOutstandingProfileAssets(InWorld) != FALSE)))
		{
			// One wait for the whole level; the manager tick starts the pass once it is resolved.
			WorldState.bHasPendingBuild = TRUE;
			WorldState.Pending.Reason = InReason;
			WorldState.Pending.LevelStem = InLevelStem;
			WorldState.Pending.WaitedSeconds = 0.0f;
			return TRUE;
		}

		return StartWorldBuild(InWorld, InReason, InLevelStem);
	}

	void ENavigationManager::TickPendingWorldBuilds(FLOAT InDeltaTime)
	{
		for (UINT32 WorldIndex = 0u; WorldIndex < Worlds.Num<UINT32>(); WorldIndex++)
		{
			FNavigationWorld& WorldState = Worlds[WorldIndex];
			if ((!WorldState.bHasPendingBuild) || (!WorldState.World))
			{
				continue;
			}
			WorldState.Pending.WaitedSeconds += InDeltaTime;
			BOOL32 bReady = FALSE;
			if (WorldState.Pending.Reason == ENavigationWorldBuildReason::LOAD)
			{
				const BOOL32 bQueueDrained = (EAssetLoadManager::GetManagerSingleton()->GetPendingRequestNum() == 0u) ? TRUE : FALSE;
				// The deferred asset apply can run a frame after the queue drains; wait a moment
				// past the drain so an assigned asset is visible before the import decision.
				bReady = ((bQueueDrained) && (WorldState.Pending.WaitedSeconds >= 0.2f)) ? TRUE : FALSE;
			}
			else
			{
				// Runtime: only the profiles' own navmesh assets gate the pass.
				bReady = (HasOutstandingProfileAssets(*WorldState.World) == FALSE) ? TRUE : FALSE;
			}
			if ((!bReady) && (WorldState.Pending.WaitedSeconds < NavigationWorldBuildAssetWaitSeconds))
			{
				continue;
			}
			PWorld* World = WorldState.World;
			const ENavigationWorldBuildReason Reason = WorldState.Pending.Reason;
			const EString Stem = WorldState.Pending.LevelStem;
			WorldState.bHasPendingBuild = FALSE;
			StartWorldBuild(*World, Reason, Stem);
		}
	}

	BOOL32 ENavigationManager::StartWorldBuild(PWorld& InWorld, ENavigationWorldBuildReason InReason, const EString& InLevelStem)
	{
		TArray<PNavMeshComponent*> Owners;
		PNavMeshComponent::CollectPrimaryOwnersInWorld(InWorld, Owners);
		if (Owners.Num<UINT32>() == 0u)
		{
			return FALSE;
		}

		FNavigationWorld& WorldState = FindOrCreateWorldState(&InWorld);
		FNavigationWorldBuildState& State = WorldState.Build;
		const UINT32 BatchId = NextWorldBuildBatchId++;
		State.BatchId = BatchId;
		State.Pending = 0u;
		State.bDirtyDuringBuild = FALSE;
		State.LevelStem = InLevelStem;
		State.Reason = InReason;
		State.Report = FNavigationWorldBuildReport();
		State.Report.BatchId = BatchId;
		State.RebuildCountdown = 0.0f;

		PE_LOG_LOG(EString(PE_TEXT("Navigation: building ")) + EString::FromInt(Owners.Num<UINT32>())
			+ EString(PE_TEXT(" navmesh profile(s) for the level.")));

		UINT32 Considered = 0u;
		for (UINT32 OwnerIndex = 0u, OwnerNum = Owners.Num<UINT32>(); OwnerIndex < OwnerNum; OwnerIndex++)
		{
			PNavMeshComponent* Owner = Owners[OwnerIndex];
			if (!Owner)
			{
				continue;
			}
			if ((InReason == ENavigationWorldBuildReason::RUNTIME) && (!Owner->GetBuildOnBeginPlay()))
			{
				continue;
			}
			Considered++;

			FNavigationProfileBuildResult Result;
			Result.SourceId = Owner->GetUniqueID();
			Result.ProfileName = Owner->GetNavMeshName();

			// A loaded level takes the shipped path first: an assigned asset is a memory copy. A
			// rejected asset falls through to a world bake instead of leaving the profile empty.
			if (((InReason == ENavigationWorldBuildReason::LOAD) || (InReason == ENavigationWorldBuildReason::RUNTIME))
				&& (!!Owner->GetNavMeshAsset()))
			{
				ENavigationBuildStats ImportStats;
				if (Owner->ImportAssignedAsset(ImportStats))
				{
					Result.bSucceeded = TRUE;
					Result.Stats = ImportStats;
					State.Report.Profiles.Add(Result);
					continue;
				}
			}

			const TArray<EBoundAABB> Volumes = Owner->CollectBakeVolumesInWorld();
			if ((Volumes.Num<UINT32>() == 0u) && (Result.ProfileName.Length() > 0u))
			{
				Result.Error = EString(PE_TEXT("no bounds volume supports this navmesh"));
				State.Report.Profiles.Add(Result);
				PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh '")) + Result.ProfileName
					+ EString(PE_TEXT("' has no bounds volume that supports it; the profile was skipped.")));
				continue;
			}

			ENavigationSourceGeometry Geometry;
			ENavigationGeometryCollector::CollectStaticMeshTriangles(InWorld, Volumes, Owner->GetBuildSettings().bFlipTriangleWinding, Geometry);
			Geometry.Links = Owner->GetNavigationLinks();

			ERecastNavigationProvider* Provider = Owner->CreateConfiguredProvider();
			if (!Provider)
			{
				Result.Error = EString(PE_TEXT("the provider registry refused the navigation backend"));
				State.Report.Profiles.Add(Result);
				continue;
			}
			if (!RequestBuildInternal(&InWorld, Result.SourceId, Result.ProfileName, Provider, static_cast<ENavigationSourceGeometry&&>(Geometry), BatchId))
			{
				delete Provider;
				Result.Error = EString(PE_TEXT("a bake for this profile is already running"));
				State.Report.Profiles.Add(Result);
				continue;
			}
			State.Pending++;
			// Success and stats are filled in when the job reports back.
			State.Report.Profiles.Add(Result);
		}

		if (Considered == 0u)
		{
			// Every profile opted out of a runtime build; leave no half-filled report behind.
			State.Report = FNavigationWorldBuildReport();
			return FALSE;
		}

		if (State.Pending == 0u)
		{
			FinishWorldBuild(WorldState);
		}
		return TRUE;
	}

	void ENavigationManager::CompleteWorldBuildJob(const FBakeResult& InResult, BOOL32 bSucceeded, BOOL32 bDiscarded)
	{
		if (InResult.BatchId == 0u)
		{
			return;
		}
		FNavigationWorld* WorldState = FindWorldState(InResult.World);
		if ((!WorldState) || (WorldState->Build.BatchId != InResult.BatchId))
		{
			return;
		}
		FNavigationWorldBuildState& State = WorldState->Build;

		FNavigationProfileBuildResult* Profile = nullptr;
		for (UINT32 Index = 0u, Num = State.Report.Profiles.Num<UINT32>(); Index < Num; Index++)
		{
			if (State.Report.Profiles[Index].SourceId == InResult.SourceId)
			{
				Profile = &State.Report.Profiles[Index];
				break;
			}
		}
		if (!Profile)
		{
			FNavigationProfileBuildResult NewProfile;
			NewProfile.SourceId = InResult.SourceId;
			NewProfile.ProfileName = InResult.ProfileName;
			State.Report.Profiles.Add(NewProfile);
			Profile = &State.Report.Profiles[State.Report.Profiles.Num<UINT32>() - 1u];
		}

		if (bSucceeded != FALSE)
		{
			Profile->bSucceeded = TRUE;
			Profile->Stats = InResult.Stats;
			Profile->Error = EString();
		}
		else
		{
			Profile->bSucceeded = FALSE;
			Profile->Error = (bDiscarded != FALSE)
				? EString(PE_TEXT("superseded by a newer build"))
				: EString(PE_TEXT("the bake produced no navmesh"));
		}

		if (State.Pending > 0u)
		{
			State.Pending--;
		}
		if (State.Pending == 0u)
		{
			FinishWorldBuild(*WorldState);
		}
	}

	void ENavigationManager::FinishWorldBuild(FNavigationWorld& InWorld)
	{
		FNavigationWorldBuildState& State = InWorld.Build;
		FNavigationWorldBuildReport& Report = State.Report;
		Report.bHasRun = TRUE;
		Report.bAllSucceeded = TRUE;
		UINT32 Succeeded = 0u;
		for (UINT32 Index = 0u, Num = Report.Profiles.Num<UINT32>(); Index < Num; Index++)
		{
			const FNavigationProfileBuildResult& Profile = Report.Profiles[Index];
			if (Profile.bSucceeded)
			{
				Succeeded++;
				if (Profile.Stats.SkippedMeshCount > 0u)
				{
					PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh profile '"))
						+ ((Profile.ProfileName.Length() > 0u) ? Profile.ProfileName : EString(PE_TEXT("Default")))
						+ EString(PE_TEXT("' was built with "))
						+ EString::FromInt(Profile.Stats.SkippedMeshCount)
						+ EString(PE_TEXT(" static mesh components skipped; build again once they load.")));
				}
				continue;
			}
			Report.bAllSucceeded = FALSE;
			PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh profile '"))
				+ ((Profile.ProfileName.Length() > 0u) ? Profile.ProfileName : EString(PE_TEXT("Default")))
				+ EString(PE_TEXT("' was not built: ")) + Profile.Error);
		}
		PE_LOG_LOG(EString(PE_TEXT("Navigation: level build finished: "))
			+ EString::FromInt(Succeeded) + EString(PE_TEXT("/")) + EString::FromInt(Report.Profiles.Num<UINT32>())
			+ EString(PE_TEXT(" navmesh profiles built.")));

		const BOOL32 bRestart = State.bDirtyDuringBuild;
		PWorld* World = InWorld.World;
		const ENavigationWorldBuildReason Reason = State.Reason;
		const EString Stem = State.LevelStem;
		State.bDirtyDuringBuild = FALSE;
		State.LastSeenInputRevision = InWorld.InputRevision;
		State.RebuildCountdown = 0.0f;
		State.bWatchingInput = TRUE;
		if (bRestart)
		{
			// Inputs changed while this pass baked; run one more for the latest world state.
			RequestWorldBuild(*World, (Reason == ENavigationWorldBuildReason::SAVE) ? ENavigationWorldBuildReason::EDITOR : Reason, Stem);
		}
	}

	void ENavigationManager::TickWorldBuildInputs(FLOAT InDeltaTime)
	{
		for (UINT32 WorldIndex = 0u; WorldIndex < Worlds.Num<UINT32>(); WorldIndex++)
		{
			FNavigationWorld& WorldState = Worlds[WorldIndex];
			FNavigationWorldBuildState& State = WorldState.Build;
			if ((!State.bWatchingInput) || (!WorldState.World) || (State.Pending > 0u))
			{
				continue;
			}
			if (WorldState.InputRevision != State.LastSeenInputRevision)
			{
				State.LastSeenInputRevision = WorldState.InputRevision;
				// Restart the countdown on every change, so a volume drag bakes once at its end.
				State.RebuildCountdown = NavigationWorldBuildDebounceSeconds;
				continue;
			}
			if (State.RebuildCountdown <= 0.0f)
			{
				continue;
			}
			State.RebuildCountdown -= InDeltaTime;
			if (State.RebuildCountdown > 0.0f)
			{
				continue;
			}
			State.RebuildCountdown = 0.0f;
			RequestWorldBuild(*WorldState.World, ENavigationWorldBuildReason::EDITOR);
		}
	}

#if ENGINE_EDITOR_ONLY
	BOOL32 ENavigationManager::BuildWorldSynchronously(PWorld& InWorld, const EString& InLevelStem, FNavigationWorldBuildReport& OutReport)
	{
		OutReport = FNavigationWorldBuildReport();
		OutReport.bHasRun = TRUE;
		OutReport.BatchId = NextWorldBuildBatchId++;

		TArray<PNavMeshComponent*> Owners;
		PNavMeshComponent::CollectPrimaryOwnersInWorld(InWorld, Owners);
		OutReport.bAllSucceeded = TRUE;
		for (UINT32 OwnerIndex = 0u; OwnerIndex < Owners.Num<UINT32>(); OwnerIndex++)
		{
			PNavMeshComponent* Owner = Owners[OwnerIndex];
			if (!Owner)
			{
				continue;
			}
			FNavigationProfileBuildResult Result;
			Result.SourceId = Owner->GetUniqueID();
			Result.ProfileName = Owner->GetNavMeshName();

			const TArray<EBoundAABB> Volumes = Owner->CollectBakeVolumesInWorld();
			if ((Volumes.Num<UINT32>() == 0u) && (Result.ProfileName.Length() > 0u))
			{
				Result.Error = EString(PE_TEXT("no bounds volume supports this navmesh"));
				OutReport.Profiles.Add(Result);
				OutReport.bAllSucceeded = FALSE;
				continue;
			}

			ERecastNavigationProvider* Provider = Owner->CreateConfiguredProvider();
			if (!Provider)
			{
				Result.Error = EString(PE_TEXT("the provider registry refused the navigation backend"));
				OutReport.Profiles.Add(Result);
				OutReport.bAllSucceeded = FALSE;
				continue;
			}

			ENavigationSourceGeometry Geometry;
			ENavigationGeometryCollector::CollectStaticMeshTriangles(InWorld, Volumes, Owner->GetBuildSettings().bFlipTriangleWinding, Geometry);
			Geometry.Links = Owner->GetNavigationLinks();

			ENavigationBuildStats Stats;
			if (!Provider->Build(Geometry, Stats))
			{
				delete Provider;
				Result.Error = EString(PE_TEXT("the bake produced no navmesh"));
				OutReport.Profiles.Add(Result);
				OutReport.bAllSucceeded = FALSE;
				continue;
			}
			if (Stats.SkippedMeshCount > 0u)
			{
				// Saving while static meshes are still loading would bake holes and overwrite a
				// good asset with them. Keep the file and report; a save after loading fixes it.
				delete Provider;
				Result.Error = EString(PE_TEXT("static mesh data was still loading; the existing asset was kept"));
				OutReport.Profiles.Add(Result);
				OutReport.bAllSucceeded = FALSE;
				continue;
			}

			// Named assets keep their name; unnamed profiles get the level convention name,
			// including the profile so two agent sizes cannot overwrite each other's file.
			EString AssetName = Owner->GetNavMeshAssetName();
			if (AssetName.Length() == 0u)
			{
				AssetName = PNavMeshComponent::MakeNavMeshAssetName(InLevelStem, Result.ProfileName);
			}
			EString SaveError;
			if (!Owner->SaveProviderAsAsset(*Provider, AssetName, Stats, SaveError, TRUE))
			{
				delete Provider;
				Result.Error = SaveError;
				OutReport.Profiles.Add(Result);
				OutReport.bAllSucceeded = FALSE;
				continue;
			}

			// The provider is published only after its asset was written, so a failed write does
			// not leave the level referencing a file that does not exist.
			RegisterProvider(&InWorld, Result.SourceId, Result.ProfileName, Provider, Stats);
			Result.bSucceeded = TRUE;
			Result.Stats = Stats;
			OutReport.Profiles.Add(Result);
		}

		// Save builds do not need a batch record, but the panel and the input watch should see the
		// result, and the level is about to be serialized right after this.
		FNavigationWorld& WorldState = FindOrCreateWorldState(&InWorld);
		WorldState.Build.Report = OutReport;
		WorldState.Build.Pending = 0u;
		WorldState.Build.bDirtyDuringBuild = FALSE;
		WorldState.Build.LastSeenInputRevision = WorldState.InputRevision;
		WorldState.Build.RebuildCountdown = 0.0f;
		WorldState.Build.bWatchingInput = TRUE;

		UINT32 Succeeded = 0u;
		for (UINT32 Index = 0u, Num = OutReport.Profiles.Num<UINT32>(); Index < Num; Index++)
		{
			if (OutReport.Profiles[Index].bSucceeded)
			{
				Succeeded++;
			}
			else
			{
				PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh profile '"))
					+ ((OutReport.Profiles[Index].ProfileName.Length() > 0u) ? OutReport.Profiles[Index].ProfileName : EString(PE_TEXT("Default")))
					+ EString(PE_TEXT("' was not built for save: ")) + OutReport.Profiles[Index].Error);
			}
		}
		PE_LOG_LOG(EString(PE_TEXT("Navigation: save build finished: "))
			+ EString::FromInt(Succeeded) + EString(PE_TEXT("/")) + EString::FromInt(OutReport.Profiles.Num<UINT32>())
			+ EString(PE_TEXT(" navmesh profiles written.")));
		return OutReport.bAllSucceeded;
	}
#endif

	BOOL32 ENavigationManager::FindPath(PWorld* InWorld, const Vector3& InStart, const Vector3& InEnd, ENavigationPath& OutPath, const EString& InProfileName) const
	{
		const ENavigationQueryFilter DefaultFilter;
		return FindPath(InWorld, InStart, InEnd, DefaultFilter, OutPath, InProfileName);
	}

	BOOL32 ENavigationManager::FindPath(PWorld* InWorld, const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationPath& OutPath, const EString& InProfileName) const
	{
		ENavigationProviderInterface* Provider = FindProviderForLocation(InWorld, InStart, InProfileName);
		if (!Provider)
		{
			return FALSE;
		}
		return Provider->FindPath(InStart, InEnd, InFilter, OutPath);
	}

	BOOL32 ENavigationManager::ProjectPoint(PWorld* InWorld, const Vector3& InLocation, Vector3& OutLocation, const EString& InProfileName) const
	{
		ENavigationProviderInterface* Provider = FindProviderForLocation(InWorld, InLocation, InProfileName);
		if (!Provider)
		{
			return FALSE;
		}
		const ENavigationQueryFilter DefaultFilter;
		ENavigationProjectionResult Projection;
		if (!Provider->ProjectPoint(InLocation, DefaultFilter, Projection))
		{
			return FALSE;
		}
		OutLocation = Projection.Point;
		return TRUE;
	}

	// --- Obstacles -----------------------------------------------------------

	ENavigationObstacleHandle ENavigationManager::AttachObstacle(FNavigationWorld& InWorld, FNavigationObstacleDesc& InOutDesc, const EString& InProfileName, const UINT32& InSerial, ENavigationProviderInterface& InProvider)
	{
		const ENavigationObstacleHandle Handle = InOutDesc.bBox
			? InProvider.AddBoxObstacle(InOutDesc.Center, InOutDesc.HalfExtents, InOutDesc.YawRadians)
			: InProvider.AddCylinderObstacle(InOutDesc.Center, InOutDesc.Radius, InOutDesc.Height);
		if (Handle == NavigationInvalidObstacleHandle)
		{
			// The provider cannot take obstacles at all (a single tile bake has no tile cache).
			// The description stays unowned so a later tiled provider of the profile can adopt it.
			InOutDesc.ProviderHandle = NavigationInvalidObstacleHandle;
			InOutDesc.ProviderProfile = EString();
			InOutDesc.ProviderSerial = 0u;
			return NavigationInvalidObstacleHandle;
		}
		InOutDesc.ProviderHandle = Handle;
		InOutDesc.ProviderProfile = InProfileName;
		InOutDesc.ProviderSerial = InSerial;
		return Handle;
	}

	ENavigationObstacleHandle ENavigationManager::PublishObstacle(FNavigationWorld& InWorld, FNavigationObstacleDesc& InOutDesc)
	{
		const INT32 ProviderIndex = FindProviderIndexForLocation(InWorld, InOutDesc.Center, InOutDesc.ProfileName);
		if (ProviderIndex < 0)
		{
			// No volume of the profile covers the obstacle yet. The description stays retained and
			// is replayed when one is published, which is what makes an obstacle added before
			// the bake survive it.
			InOutDesc.ProviderHandle = NavigationInvalidObstacleHandle;
			InOutDesc.ProviderProfile = EString();
			InOutDesc.ProviderSerial = 0u;
			return InOutDesc.Id;
		}
		const FNavigationProviderEntry& Entry = InWorld.Providers[static_cast<UINT32>(ProviderIndex)];
		if (!Entry.Provider)
		{
			return InOutDesc.Id;
		}
		if (AttachObstacle(InWorld, InOutDesc, Entry.ProfileName, Entry.Serial, *Entry.Provider) == NavigationInvalidObstacleHandle)
		{
			// A provider exists but refused the obstacle: report it instead of handing back an
			// id for something that is not live.
			return NavigationInvalidObstacleHandle;
		}
		return InOutDesc.Id;
	}

	void ENavigationManager::ReplayObstacles(FNavigationWorld& InWorld, const EString& InProfileName, const UINT32& InSerial, ENavigationProviderInterface& InProvider)
	{
		for (UINT32 Index = 0u, Num = InWorld.Obstacles.Num<UINT32>(); Index < Num; Index++)
		{
			FNavigationObstacleDesc& Desc = InWorld.Obstacles[Index];
			const BOOL32 bOwnedHere = (Desc.ProviderProfile == InProfileName) ? TRUE : FALSE;
			if ((!bOwnedHere) && (Desc.ProviderProfile.Length() > 0u))
			{
				// Belongs to another live provider; touching it here would duplicate it there.
				continue;
			}
			if ((!bOwnedHere) && (Desc.ProfileName != InProfileName))
			{
				continue;
			}
			if ((!bOwnedHere) && (!BoundsContainLocation(InProvider.GetBounds(), Desc.Center)))
			{
				// Still outside this provider's coverage; the provider that covers it adopts it.
				continue;
			}
			AttachObstacle(InWorld, Desc, InProfileName, InSerial, InProvider);
		}
	}

	ENavigationObstacleHandle ENavigationManager::AddCylinderObstacle(PWorld* InWorld, const ObjectIdentityType& InRequesterId, const EString& InProfileName, const Vector3& InCenter, const FLOAT& InRadius, const FLOAT& InHeight)
	{
		if (!InWorld)
		{
			return NavigationInvalidObstacleHandle;
		}
		FNavigationWorld& WorldState = FindOrCreateWorldState(InWorld);
		FNavigationObstacleDesc Desc;
		Desc.Id = NextObstacleId++;
		Desc.RequesterId = InRequesterId;
		Desc.ProfileName = InProfileName;
		Desc.bBox = FALSE;
		Desc.Center = InCenter;
		Desc.Radius = InRadius;
		Desc.Height = InHeight;
		// Retained even when no volume covers it yet: the replay after the next build picks it up.
		if (PublishObstacle(WorldState, Desc) == NavigationInvalidObstacleHandle)
		{
			return NavigationInvalidObstacleHandle;
		}
		WorldState.Obstacles.Add(Desc);
		return Desc.Id;
	}

	ENavigationObstacleHandle ENavigationManager::AddBoxObstacle(PWorld* InWorld, const ObjectIdentityType& InRequesterId, const EString& InProfileName, const Vector3& InCenter, const Vector3& InHalfExtents, const FLOAT& InYawRadians)
	{
		if (!InWorld)
		{
			return NavigationInvalidObstacleHandle;
		}
		FNavigationWorld& WorldState = FindOrCreateWorldState(InWorld);
		FNavigationObstacleDesc Desc;
		Desc.Id = NextObstacleId++;
		Desc.RequesterId = InRequesterId;
		Desc.ProfileName = InProfileName;
		Desc.bBox = TRUE;
		Desc.Center = InCenter;
		Desc.HalfExtents = InHalfExtents;
		Desc.YawRadians = InYawRadians;
		if (PublishObstacle(WorldState, Desc) == NavigationInvalidObstacleHandle)
		{
			return NavigationInvalidObstacleHandle;
		}
		WorldState.Obstacles.Add(Desc);
		return Desc.Id;
	}

	void ENavigationManager::RemoveObstacle(PWorld* InWorld, const ObjectIdentityType& InRequesterId, ENavigationObstacleHandle InHandle)
	{
		if (InHandle == NavigationInvalidObstacleHandle)
		{
			return;
		}
		FNavigationWorld* WorldState = FindWorldState(InWorld);
		if (!WorldState)
		{
			return;
		}
		for (UINT32 Index = 0u, Num = WorldState->Obstacles.Num<UINT32>(); Index < Num; Index++)
		{
			FNavigationObstacleDesc& Desc = WorldState->Obstacles[Index];
			if ((Desc.RequesterId != InRequesterId) || (Desc.Id != InHandle))
			{
				continue;
			}
			// Remove through the provider that accepted the obstacle. Resolving by position
			// could hand the handle to a different provider after a rebuild.
			if (Desc.ProviderHandle != NavigationInvalidObstacleHandle)
			{
				ENavigationProviderInterface* Provider = GetProviderForProfile(*WorldState, Desc.ProviderProfile, Desc.ProviderSerial);
				if (!!Provider)
				{
					Provider->RemoveObstacle(Desc.ProviderHandle);
				}
			}
			WorldState->Obstacles.RemoveAt(Index);
			return;
		}
	}

	// --- Crowd ---------------------------------------------------------------

	ENavigationManager::FCrowdBinding* ENavigationManager::FindCrowdBinding(FNavigationWorld& InWorld, const ObjectIdentityType& InRequesterId)
	{
		for (UINT32 Index = 0u, Num = InWorld.CrowdBindings.Num<UINT32>(); Index < Num; Index++)
		{
			if (InWorld.CrowdBindings[Index].RequesterId == InRequesterId)
			{
				return &InWorld.CrowdBindings[Index];
			}
		}
		return nullptr;
	}

	const ENavigationManager::FCrowdBinding* ENavigationManager::FindCrowdBinding(const FNavigationWorld& InWorld, const ObjectIdentityType& InRequesterId) const
	{
		for (UINT32 Index = 0u, Num = InWorld.CrowdBindings.Num<UINT32>(); Index < Num; Index++)
		{
			if (InWorld.CrowdBindings[Index].RequesterId == InRequesterId)
			{
				return &InWorld.CrowdBindings[Index];
			}
		}
		return nullptr;
	}

	void ENavigationManager::InvalidateCrowdBindings(FNavigationWorld& InWorld, const EString& InProfileName)
	{
		// Only the bindings of the provider that was replaced or removed lose their handle;
		// agents living on other providers keep steering in place.
		for (UINT32 Index = 0u, Num = InWorld.CrowdBindings.Num<UINT32>(); Index < Num; Index++)
		{
			if (InWorld.CrowdBindings[Index].ProviderProfile != InProfileName)
			{
				continue;
			}
			InWorld.CrowdBindings[Index].ProviderHandle = NavigationInvalidCrowdAgentHandle;
			InWorld.CrowdBindings[Index].ProviderProfile = EString();
			InWorld.CrowdBindings[Index].ProviderSerial = 0u;
		}
	}

	BOOL32 ENavigationManager::RegisterCrowdAgent(PWorld* InWorld, const ObjectIdentityType& InRequesterId, const EString& InProfileName, const Vector3& InPosition, const ENavigationAgentSettings& InSettings)
	{
		if (!InWorld)
		{
			return FALSE;
		}
		FNavigationWorld& WorldState = FindOrCreateWorldState(InWorld);
		FCrowdBinding* Binding = FindCrowdBinding(WorldState, InRequesterId);
		if (!Binding)
		{
			FCrowdBinding NewBinding;
			NewBinding.RequesterId = InRequesterId;
			WorldState.CrowdBindings.Add(NewBinding);
			Binding = &WorldState.CrowdBindings[WorldState.CrowdBindings.Num<UINT32>() - 1u];
		}
		Binding->ProfileName = InProfileName;
		Binding->Settings = InSettings;
		Binding->LastPosition = InPosition;

		if (Binding->ProviderHandle != NavigationInvalidCrowdAgentHandle)
		{
			// Already registered, but only while the provider is still the build that issued it.
			if (!!GetProviderForProfile(WorldState, Binding->ProviderProfile, Binding->ProviderSerial))
			{
				return TRUE;
			}
			Binding->ProviderHandle = NavigationInvalidCrowdAgentHandle;
			Binding->ProviderProfile = EString();
			Binding->ProviderSerial = 0u;
		}

		const INT32 ProviderIndex = FindProviderIndexForLocation(WorldState, InPosition, Binding->ProfileName);
		if (ProviderIndex < 0)
		{
			return FALSE;
		}
		const FNavigationProviderEntry& Entry = WorldState.Providers[static_cast<UINT32>(ProviderIndex)];
		if (!Entry.Provider)
		{
			return FALSE;
		}
		Binding->ProviderHandle = Entry.Provider->AddCrowdAgent(InPosition, InSettings);
		if (Binding->ProviderHandle == NavigationInvalidCrowdAgentHandle)
		{
			return FALSE;
		}
		Binding->ProviderProfile = Entry.ProfileName;
		Binding->ProviderSerial = Entry.Serial;
		if (Binding->bHasTarget)
		{
			// The agent was re-created, so the target has to be handed over again.
			Entry.Provider->RequestCrowdAgentMove(Binding->ProviderHandle, Binding->Target);
		}
		return TRUE;
	}

	BOOL32 ENavigationManager::RequestCrowdAgentMove(PWorld* InWorld, const ObjectIdentityType& InRequesterId, const Vector3& InTarget)
	{
		FNavigationWorld* WorldState = FindWorldState(InWorld);
		if (!WorldState)
		{
			return FALSE;
		}
		FCrowdBinding* Binding = FindCrowdBinding(*WorldState, InRequesterId);
		if (!Binding)
		{
			return FALSE;
		}
		Binding->Target = InTarget;
		Binding->bHasTarget = TRUE;
		if (Binding->ProviderHandle == NavigationInvalidCrowdAgentHandle)
		{
			// No agent yet; RegisterCrowdAgent hands the retained target over once there is one.
			return FALSE;
		}
		ENavigationProviderInterface* Provider = GetProviderForProfile(*WorldState, Binding->ProviderProfile, Binding->ProviderSerial);
		if (!Provider)
		{
			Binding->ProviderHandle = NavigationInvalidCrowdAgentHandle;
			Binding->ProviderProfile = EString();
			Binding->ProviderSerial = 0u;
			return FALSE;
		}
		return Provider->RequestCrowdAgentMove(Binding->ProviderHandle, InTarget);
	}

	void ENavigationManager::StopCrowdAgent(PWorld* InWorld, const ObjectIdentityType& InRequesterId)
	{
		FNavigationWorld* WorldState = FindWorldState(InWorld);
		if (!WorldState)
		{
			return;
		}
		FCrowdBinding* Binding = FindCrowdBinding(*WorldState, InRequesterId);
		if (!Binding)
		{
			return;
		}
		Binding->bHasTarget = FALSE;
		if (Binding->ProviderHandle == NavigationInvalidCrowdAgentHandle)
		{
			return;
		}
		ENavigationProviderInterface* Provider = GetProviderForProfile(*WorldState, Binding->ProviderProfile, Binding->ProviderSerial);
		if (!!Provider)
		{
			Provider->StopCrowdAgent(Binding->ProviderHandle);
		}
		else
		{
			Binding->ProviderHandle = NavigationInvalidCrowdAgentHandle;
			Binding->ProviderProfile = EString();
			Binding->ProviderSerial = 0u;
		}
	}

	void ENavigationManager::UnregisterCrowdAgent(PWorld* InWorld, const ObjectIdentityType& InRequesterId)
	{
		for (UINT32 WorldIndex = 0u; WorldIndex < Worlds.Num<UINT32>(); WorldIndex++)
		{
			FNavigationWorld& WorldState = Worlds[WorldIndex];
			if ((InWorld) && (WorldState.World != InWorld))
			{
				continue;
			}
			for (UINT32 Index = 0u, Num = WorldState.CrowdBindings.Num<UINT32>(); Index < Num; Index++)
			{
				FCrowdBinding& Binding = WorldState.CrowdBindings[Index];
				if (Binding.RequesterId != InRequesterId)
				{
					continue;
				}
				if (Binding.ProviderHandle != NavigationInvalidCrowdAgentHandle)
				{
					ENavigationProviderInterface* Provider = GetProviderForProfile(WorldState, Binding.ProviderProfile, Binding.ProviderSerial);
					if (!!Provider)
					{
						Provider->RemoveCrowdAgent(Binding.ProviderHandle);
					}
				}
				WorldState.CrowdBindings.RemoveAt(Index);
				return;
			}
		}
	}

	BOOL32 ENavigationManager::GetCrowdAgentState(PWorld* InWorld, const ObjectIdentityType& InRequesterId, ENavigationCrowdAgentState& OutState) const
	{
		const FNavigationWorld* WorldState = FindWorldState(InWorld);
		if (!WorldState)
		{
			return FALSE;
		}
		const FCrowdBinding* Binding = FindCrowdBinding(*WorldState, InRequesterId);
		if ((!Binding) || (Binding->ProviderHandle == NavigationInvalidCrowdAgentHandle))
		{
			return FALSE;
		}
		ENavigationProviderInterface* Provider = GetProviderForProfile(*WorldState, Binding->ProviderProfile, Binding->ProviderSerial);
		if (!Provider)
		{
			return FALSE;
		}
		return Provider->GetCrowdAgentState(Binding->ProviderHandle, OutState);
	}

	// --- Debug ---------------------------------------------------------------

	void ENavigationManager::SetDebugDrawEnabled(const BOOL32& InEnabled)
	{
		if (bDebugDrawEnabled == InEnabled)
		{
			return;
		}
		bDebugDrawEnabled = InEnabled;
	}

	BOOL32 ENavigationManager::IsDebugDrawEnabled() const
	{
		return bDebugDrawEnabled;
	}

	void ENavigationManager::SetDebugDrawMode(ENavigationDebugModeType InMode)
	{
		if (DebugDrawMode == InMode)
		{
			return;
		}
		DebugDrawMode = InMode;
		InvalidateDebugLines();
	}

	ENavigationDebugModeType ENavigationManager::GetDebugDrawMode() const
	{
		return DebugDrawMode;
	}

	void ENavigationManager::SetDebugTrianglesEnabled(const BOOL32& InEnabled)
	{
		if (bDebugTrianglesEnabled == InEnabled)
		{
			return;
		}
		bDebugTrianglesEnabled = InEnabled;
		InvalidateDebugLines();
	}

	BOOL32 ENavigationManager::IsDebugTrianglesEnabled() const
	{
		return bDebugTrianglesEnabled;
	}

	UINT32 ENavigationManager::GetDebugSegmentsRevision() const
	{
		return DebugSegmentsRevision;
	}

	const TArray<Vector3>& ENavigationManager::GetDebugLineSegments()
	{
		RebuildDebugSegments();
		return DebugPolygonSegments;
	}

	const TArray<Vector3>& ENavigationManager::GetDebugTriangleSegments()
	{
		RebuildDebugSegments();
		return DebugTriangleSegments;
	}

	void ENavigationManager::RebuildDebugSegments()
	{
		// A rebuilt tile changes the polygons without a new build, so the cache follows the
		// provider's own content revision as well.
		if (!bDebugLinesDirty)
		{
			for (UINT32 WorldIndex = 0u, WorldNum = Worlds.Num<UINT32>(); WorldIndex < WorldNum; WorldIndex++)
			{
				const FNavigationWorld& WorldState = Worlds[WorldIndex];
				for (UINT32 Index = 0u, Num = WorldState.Providers.Num<UINT32>(); Index < Num; Index++)
				{
					ENavigationProviderInterface* Provider = WorldState.Providers[Index].Provider;
					if ((!!Provider) && (Provider->IsReady()) && (Provider->GetContentRevision() != WorldState.Providers[Index].DebugRevision))
					{
						bDebugLinesDirty = TRUE;
						break;
					}
				}
				if (bDebugLinesDirty)
				{
					break;
				}
			}
		}

		if (!bDebugLinesDirty)
		{
			return;
		}

		DebugPolygonSegments.Empty();
		DebugTriangleSegments.Empty();
		for (UINT32 WorldIndex = 0u, WorldNum = Worlds.Num<UINT32>(); WorldIndex < WorldNum; WorldIndex++)
		{
			FNavigationWorld& WorldState = Worlds[WorldIndex];
			for (UINT32 Index = 0u, Num = WorldState.Providers.Num<UINT32>(); Index < Num; Index++)
			{
				FNavigationProviderEntry& Entry = WorldState.Providers[Index];
				ENavigationProviderInterface* Provider = Entry.Provider;
				if ((!!Provider) && (Provider->IsReady()))
				{
					if (Entry.bDebugDrawEnabled)
					{
						Provider->CollectDebugLines(DebugDrawMode, DebugPolygonSegments);
						if (bDebugTrianglesEnabled)
						{
							Provider->CollectDebugTriangles(DebugTriangleSegments);
						}
					}
					// Tracked even while this provider is hidden: otherwise its content revision would
					// keep the whole cache dirty and every request would re-collect every provider.
					Entry.DebugRevision = Provider->GetContentRevision();
				}
			}
		}
		bDebugLinesDirty = FALSE;
		// The editor compares this against the revision it last uploaded, so every rebuild
		// has to produce a value the previous one did not.
		DebugSegmentsRevision++;
	}

	void ENavigationManager::InvalidateDebugLines()
	{
		bDebugLinesDirty = TRUE;
		DebugPolygonSegments.Empty();
		DebugTriangleSegments.Empty();
		DebugSegmentsRevision++;
	}

	void ENavigationManager::ReleaseAllProviders()
	{
		for (UINT32 WorldIndex = 0u, WorldNum = Worlds.Num<UINT32>(); WorldIndex < WorldNum; WorldIndex++)
		{
			FNavigationWorld& WorldState = Worlds[WorldIndex];
			for (UINT32 Index = 0u, Num = WorldState.Providers.Num<UINT32>(); Index < Num; Index++)
			{
				delete WorldState.Providers[Index].Provider;
				WorldState.Providers[Index].Provider = nullptr;
			}
		}
		Worlds.Empty(0u);
	}

	void ENavigationManager::PublishFinishedBakes()
	{
		TArray<FBakeResult> Results;
		{
			EScopedLock Lock(BakeMutex);
			if (BakeResults.Num<UINT32>() == 0u)
			{
				return;
			}
			Results.MoveFrom(static_cast<TArray<FBakeResult>&&>(BakeResults));
		}

		for (UINT32 Index = 0u, Num = Results.Num<UINT32>(); Index < Num; Index++)
		{
			FBakeResult& Result = Results[Index];
			FNavigationWorld* WorldState = FindWorldState(Result.World);
			const INT32 EntryIndex = WorldState ? FindProviderIndex(*WorldState, Result.ProfileName) : -1;
			if ((!WorldState) || (EntryIndex < 0) || (WorldState->Providers[static_cast<UINT32>(EntryIndex)].Serial != Result.Serial))
			{
				// The profile left the world or asked again while this was baking.
				delete Result.Provider;
				Result.Provider = nullptr;
				CompleteWorldBuildJob(Result, FALSE, TRUE);
				continue;
			}

			FNavigationProviderEntry& Entry = WorldState->Providers[static_cast<UINT32>(EntryIndex)];
			if (!Result.bSucceeded)
			{
				Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_FAILED;
				delete Result.Provider;
				Result.Provider = nullptr;
				CompleteWorldBuildJob(Result, FALSE, FALSE);
				continue;
			}

			// A successful bake replaces the previous provider. Until this point the old one kept
			// answering queries, so a rebuild never leaves the world without navigation.
			delete Entry.Provider;
			Entry.Provider = Result.Provider;
			Entry.SourceId = Result.SourceId;
			Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_READY;
			Entry.Stats = Result.Stats;
			Result.Provider = nullptr;
			// The old provider's tile cache and crowd died with it, so the obstacles have to be
			// re-registered and the crowd bindings rebuilt by their owners. Only this provider's
			// records are touched; other profiles keep their agents and obstacles.
			InvalidateCrowdBindings(*WorldState, Entry.ProfileName);
			ReplayObstacles(*WorldState, Entry.ProfileName, Entry.Serial, *Entry.Provider);
			InvalidateDebugLines();
			CompleteWorldBuildJob(Result, TRUE, FALSE);
		}
	}

	void ENavigationManager::BakeWorkerLoop()
	{
		for (;;)
		{
			BakeWakeEvent.WaitAndReset();
			if (BakeThread.IsStopRequested())
			{
				return;
			}

			for (;;)
			{
				FBakeJob Job;
				{
					EScopedLock Lock(BakeMutex);
					if (BakeJobs.Num<UINT32>() == 0u)
					{
						break;
					}
					Job = BakeJobs[0];
					BakeJobs.RemoveAt(0u);
				}

				// Only the backend runs here. It owns its own Recast/Detour state and logs through
				// the mutex guarded logger; it never touches the world.
				FBakeResult Result;
				Result.World = Job.World;
				Result.SourceId = Job.SourceId;
				Result.ProfileName = Job.ProfileName;
				Result.Serial = Job.Serial;
				Result.BatchId = Job.BatchId;
				Result.Provider = Job.Provider;
				Result.bSucceeded = (!!Job.Provider) ? Job.Provider->Build(Job.Geometry, Result.Stats) : FALSE;

				EScopedLock Lock(BakeMutex);
				BakeResults.Add(Result);
			}
		}
	}

	void ENavigationManager::StopBakeWorker()
	{
		if (!bBakeWorkerStarted)
		{
			return;
		}
		BakeThread.RequestStop();
		BakeWakeEvent.Signal();
		// Joins the worker; a bake already running is allowed to finish rather than being killed
		// in the middle of its allocations.
		BakeThread.Join();
		bBakeWorkerStarted = FALSE;

		// Jobs that were never started and results that were never published are unowned now.
		for (UINT32 Index = 0u, Num = BakeJobs.Num<UINT32>(); Index < Num; Index++)
		{
			delete BakeJobs[Index].Provider;
		}
		BakeJobs.Empty(0u);
		for (UINT32 Index = 0u, Num = BakeResults.Num<UINT32>(); Index < Num; Index++)
		{
			delete BakeResults[Index].Provider;
		}
		BakeResults.Empty(0u);
	}

}  // namespace PigeonEngine
