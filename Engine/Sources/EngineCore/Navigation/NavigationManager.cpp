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
	// whole level instead of one per component.
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
		// Authored inputs changed: one debounced pass for the whole level.
		TickWorldBuildInputs(InDeltaTime);
#endif

		// Per provider work: dynamic obstacles rebuild tiles here, and later local avoidance steps
		// its agents. It runs after publishing so a provider that became ready this frame is already
		// driving, and before the world tick so agents see the result.
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			ENavigationProviderInterface* Provider = Providers[Index].Provider;
			if ((!!Provider) && (Provider->IsReady()))
			{
				Provider->Tick(InDeltaTime);
			}
		}
	}

	void ENavigationManager::RegisterProvider(const ObjectIdentityType& InOwnerId, const EString& InNavMeshName, ENavigationProviderInterface* InProvider, const ENavigationBuildStats& InStats)
	{
		if (!InProvider)
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: registering a null provider was ignored.")));
			return;
		}

		UINT32 Serial = 1u;
		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		if (ExistingIndex >= 0)
		{
			// A replacement releases the old provider here rather than at the call site so
			// ownership never depends on who built what. Republishing the same object is a
			// state refresh, not a replacement: deleting it would leave the entry dangling.
			FNavigationProviderEntry& Entry = Providers[static_cast<UINT32>(ExistingIndex)];
			if (Entry.Provider != InProvider)
			{
				delete Entry.Provider;
			}
			Entry.Provider = InProvider;
			Entry.GroupName = InNavMeshName;
			Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_READY;
			Entry.Stats = InStats;
			// A bake queued before this publish is older than what is being installed now.
			Entry.Serial++;
			Serial = Entry.Serial;
		}
		else
		{
			FNavigationProviderEntry Entry;
			Entry.OwnerId = InOwnerId;
			Entry.GroupName = InNavMeshName;
			Entry.Provider = InProvider;
			Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_READY;
			Entry.Serial = 1u;
			Entry.Stats = InStats;
			Serial = Entry.Serial;
			Providers.Add(Entry);
		}
		// The replaced provider took its crowd agents and obstacles with it, so both are
		// restored on the new one before any query can reach it.
		InvalidateCrowdBindings(InOwnerId);
		ReplayObstacles(InOwnerId, InNavMeshName, Serial, *InProvider);
		InvalidateDebugLines();
	}

	BOOL32 ENavigationManager::RequestBuild(const ObjectIdentityType& InOwnerId, const EString& InNavMeshName, ENavigationProviderInterface* InProvider, ENavigationSourceGeometry&& InGeometry)
	{
		return RequestBuildInternal(InOwnerId, InNavMeshName, InProvider, static_cast<ENavigationSourceGeometry&&>(InGeometry), 0u);
	}

	BOOL32 ENavigationManager::RequestBuildInternal(const ObjectIdentityType& InOwnerId, const EString& InNavMeshName, ENavigationProviderInterface* InProvider, ENavigationSourceGeometry&& InGeometry, const UINT32& InBatchId)
	{
		if (!InProvider)
		{
			return FALSE;
		}

		INT32 EntryIndex = FindProviderIndex(InOwnerId);
		if (EntryIndex < 0)
		{
			FNavigationProviderEntry Entry;
			Entry.OwnerId = InOwnerId;
			Entry.GroupName = InNavMeshName;
			Providers.Add(Entry);
			EntryIndex = static_cast<INT32>(Providers.Num<UINT32>() - 1u);
		}
		else
		{
			// The owner may have been renamed between requests; the next publish has to answer
			// for the profile the caller asked for now.
			Providers[static_cast<UINT32>(EntryIndex)].GroupName = InNavMeshName;
		}

		FNavigationProviderEntry& Entry = Providers[static_cast<UINT32>(EntryIndex)];
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
			Job.OwnerId = InOwnerId;
			Job.NavMeshName = InNavMeshName;
			Job.Serial = Entry.Serial;
			Job.BatchId = InBatchId;
			Job.Provider = InProvider;
			Job.Geometry = static_cast<ENavigationSourceGeometry&&>(InGeometry);
			BakeJobs.Add(Job);
		}
		BakeWakeEvent.Signal();
		return TRUE;
	}

	void ENavigationManager::UnregisterProvider(const ObjectIdentityType& InOwnerId)
	{
		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		if (ExistingIndex < 0)
		{
			return;
		}
		// Dropping the entry means a bake still running for it is discarded when it reports back.
		delete Providers[static_cast<UINT32>(ExistingIndex)].Provider;
		Providers[static_cast<UINT32>(ExistingIndex)].Provider = nullptr;
		Providers.RemoveAt(static_cast<UINT32>(ExistingIndex));

		// Obstacles and crowd handles are namespaces of the dead provider; keeping them would
		// let a later provider interpret a handle it never issued. Obstacles stay retained and
		// unowned so a replacement provider of the same group can adopt them.
		for (UINT32 Index = 0u, Num = Obstacles.Num<UINT32>(); Index < Num; Index++)
		{
			if (Obstacles[Index].ProviderOwnerId == InOwnerId)
			{
				Obstacles[Index].ProviderHandle = NavigationInvalidObstacleHandle;
				Obstacles[Index].ProviderOwnerId = 0;
				Obstacles[Index].ProviderSerial = 0u;
			}
		}
		InvalidateCrowdBindings(InOwnerId);
		InvalidateDebugLines();
	}

	void ENavigationManager::SetProviderDebugDrawEnabled(const ObjectIdentityType& InOwnerId, const BOOL32& InEnabled)
	{
		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		if (ExistingIndex < 0)
		{
			return;
		}
		if (Providers[static_cast<UINT32>(ExistingIndex)].bDebugDrawEnabled == InEnabled)
		{
			return;
		}
		Providers[static_cast<UINT32>(ExistingIndex)].bDebugDrawEnabled = InEnabled;
		InvalidateDebugLines();
	}

	ENavigationProviderInterface* ENavigationManager::GetProvider(const ObjectIdentityType& InOwnerId) const
	{
		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		return (ExistingIndex >= 0) ? Providers[static_cast<UINT32>(ExistingIndex)].Provider : nullptr;
	}

	ENavigationProviderInterface* ENavigationManager::GetDefaultProvider(const EString& InNavMeshName) const
	{
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			const FNavigationProviderEntry& Entry = Providers[Index];
			if ((Entry.GroupName != InNavMeshName) || (!Entry.Provider) || (!Entry.Provider->IsReady()))
			{
				continue;
			}
			return Entry.Provider;
		}
		return nullptr;
	}

	INT32 ENavigationManager::FindProviderIndexForLocation(const Vector3& InLocation, const EString& InNavMeshName) const
	{
		INT32 FirstReady = -1;
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			const FNavigationProviderEntry& Entry = Providers[Index];
			if ((Entry.GroupName != InNavMeshName) || (!Entry.Provider) || (!Entry.Provider->IsReady()))
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

	ENavigationProviderInterface* ENavigationManager::FindProviderForLocation(const Vector3& InLocation, const EString& InNavMeshName) const
	{
		// A group never falls back to another group's mesh: an agent configured for one navmesh
		// must not silently walk on a different one. Inside the group the covering provider
		// wins, and outside every volume of it the group's first ready provider projects the
		// stray point onto its own mesh.
		const INT32 Index = FindProviderIndexForLocation(InLocation, InNavMeshName);
		return (Index >= 0) ? Providers[static_cast<UINT32>(Index)].Provider : nullptr;
	}

	ENavigationProviderInterface* ENavigationManager::GetProviderForOwner(const ObjectIdentityType& InOwnerId, const UINT32& InSerial) const
	{
		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		if (ExistingIndex < 0)
		{
			return nullptr;
		}
		const FNavigationProviderEntry& Entry = Providers[static_cast<UINT32>(ExistingIndex)];
		if ((Entry.Serial != InSerial) || (!Entry.Provider) || (!Entry.Provider->IsReady()))
		{
			// A rebuild replaced the provider the caller's handle belonged to.
			return nullptr;
		}
		return Entry.Provider;
	}

	UINT32 ENavigationManager::GetProviderNum() const
	{
		return Providers.Num<UINT32>();
	}

	ENavigationBuildStateType ENavigationManager::GetBuildState(const ObjectIdentityType& InOwnerId) const
	{
		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		if (ExistingIndex < 0)
		{
			return ENavigationBuildStateType::NAVIGATION_BUILD_IDLE;
		}
		return Providers[static_cast<UINT32>(ExistingIndex)].State;
	}

	const ENavigationBuildStats& ENavigationManager::GetBuildStats(const ObjectIdentityType& InOwnerId) const
	{
		static const ENavigationBuildStats EmptyStats;
		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		if (ExistingIndex < 0)
		{
			return EmptyStats;
		}
		return Providers[static_cast<UINT32>(ExistingIndex)].Stats;
	}

	void ENavigationManager::BumpNavigationInputRevision(const PWorld* InWorld)
	{
		for (UINT32 Index = 0u, Num = WorldInputRevisions.Num<UINT32>(); Index < Num; Index++)
		{
			if (WorldInputRevisions[Index].World == InWorld)
			{
				WorldInputRevisions[Index].Revision++;
				return;
			}
		}
		FNavigationWorldRevision Entry;
		Entry.World = InWorld;
		Entry.Revision = 2u;
		WorldInputRevisions.Add(Entry);
	}

	UINT32 ENavigationManager::GetNavigationInputRevision(const PWorld* InWorld) const
	{
		for (UINT32 Index = 0u, Num = WorldInputRevisions.Num<UINT32>(); Index < Num; Index++)
		{
			if (WorldInputRevisions[Index].World == InWorld)
			{
				return WorldInputRevisions[Index].Revision;
			}
		}
		return 1u;
	}

	ENavigationManager::FNavigationWorldBuildState* ENavigationManager::FindWorldBuild(const PWorld* InWorld)
	{
		for (UINT32 Index = 0u, Num = WorldBuilds.Num<UINT32>(); Index < Num; Index++)
		{
			if (WorldBuilds[Index].World == InWorld)
			{
				return &WorldBuilds[Index];
			}
		}
		return nullptr;
	}

	const ENavigationManager::FNavigationWorldBuildState* ENavigationManager::FindWorldBuild(const PWorld* InWorld) const
	{
		for (UINT32 Index = 0u, Num = WorldBuilds.Num<UINT32>(); Index < Num; Index++)
		{
			if (WorldBuilds[Index].World == InWorld)
			{
				return &WorldBuilds[Index];
			}
		}
		return nullptr;
	}

	BOOL32 ENavigationManager::IsWorldBuildRunning(PWorld& InWorld) const
	{
		const FNavigationWorldBuildState* State = FindWorldBuild(&InWorld);
		return ((State) && (State->Pending > 0u)) ? TRUE : FALSE;
	}

	const ENavigationManager::FNavigationWorldBuildReport& ENavigationManager::GetWorldBuildReport(PWorld& InWorld) const
	{
		static const FNavigationWorldBuildReport EmptyReport;
		const FNavigationWorldBuildState* State = FindWorldBuild(&InWorld);
		return (!!State) ? State->Report : EmptyReport;
	}

	void ENavigationManager::NotifyWorldDestroyed(PWorld* InWorld)
	{
		for (UINT32 Index = WorldBuilds.Num<UINT32>(); Index > 0u; Index--)
		{
			if (WorldBuilds[Index - 1u].World == InWorld)
			{
				WorldBuilds.RemoveAt(Index - 1u);
			}
		}
		for (UINT32 Index = PendingWorldBuilds.Num<UINT32>(); Index > 0u; Index--)
		{
			if (PendingWorldBuilds[Index - 1u].World == InWorld)
			{
				PendingWorldBuilds.RemoveAt(Index - 1u);
			}
		}
		for (UINT32 Index = WorldInputRevisions.Num<UINT32>(); Index > 0u; Index--)
		{
			if (WorldInputRevisions[Index - 1u].World == InWorld)
			{
				WorldInputRevisions.RemoveAt(Index - 1u);
			}
		}
	}

	BOOL32 ENavigationManager::RequestWorldBuild(PWorld& InWorld, ENavigationWorldBuildReason InReason, const EString& InLevelStem)
	{
		if (FNavigationWorldBuildState* State = FindWorldBuild(&InWorld))
		{
			if (State->Pending > 0u)
			{
				// A pass is running; repeat once with the latest world state when it finishes.
				State->bDirtyDuringBuild = TRUE;
				return TRUE;
			}
		}

		// An explicit bake cancels a pending load wait: it deliberately bakes from the world and
		// has no reason to wait for the asset queue.
		for (UINT32 Index = 0u, Num = PendingWorldBuilds.Num<UINT32>(); Index < Num; Index++)
		{
			if (PendingWorldBuilds[Index].World != (&InWorld))
			{
				continue;
			}
			if ((InReason == ENavigationWorldBuildReason::EDITOR) || (InReason == ENavigationWorldBuildReason::SAVE))
			{
				PendingWorldBuilds.RemoveAt(Index);
			}
			else
			{
				PendingWorldBuilds[Index].Reason = InReason;
				PendingWorldBuilds[Index].LevelStem = InLevelStem;
				return TRUE;
			}
			break;
		}

		if (((InReason == ENavigationWorldBuildReason::LOAD)
				&& (EAssetLoadManager::GetManagerSingleton()->GetPendingRequestNum() > 0u))
			|| ((InReason == ENavigationWorldBuildReason::RUNTIME) && (HasOutstandingProfileAssets(InWorld) != FALSE)))
		{
			// One wait for the whole level; the manager tick starts the pass once it is resolved.
			FPendingWorldBuild Pending;
			Pending.World = &InWorld;
			Pending.Reason = InReason;
			Pending.LevelStem = InLevelStem;
			PendingWorldBuilds.Add(Pending);
			return TRUE;
		}

		return StartWorldBuild(InWorld, InReason, InLevelStem);
	}

	void ENavigationManager::TickPendingWorldBuilds(FLOAT InDeltaTime)
	{
		for (UINT32 Index = 0u; Index < PendingWorldBuilds.Num<UINT32>(); )
		{
			FPendingWorldBuild& Pending = PendingWorldBuilds[Index];
			Pending.WaitedSeconds += InDeltaTime;
			BOOL32 bReady = FALSE;
			if (Pending.Reason == ENavigationWorldBuildReason::LOAD)
			{
				const BOOL32 bQueueDrained = (EAssetLoadManager::GetManagerSingleton()->GetPendingRequestNum() == 0u) ? TRUE : FALSE;
				// The deferred asset apply can run a frame after the queue drains; wait a moment
				// past the drain so an assigned asset is visible before the import decision.
				bReady = ((bQueueDrained) && (Pending.WaitedSeconds >= 0.2f)) ? TRUE : FALSE;
			}
			else
			{
				// Runtime: only the profiles' own navmesh assets gate the pass.
				bReady = (HasOutstandingProfileAssets(*Pending.World) == FALSE) ? TRUE : FALSE;
			}
			if ((!bReady) && (Pending.WaitedSeconds < NavigationWorldBuildAssetWaitSeconds))
			{
				Index++;
				continue;
			}
			PWorld* World = Pending.World;
			const ENavigationWorldBuildReason Reason = Pending.Reason;
			const EString Stem = Pending.LevelStem;
			PendingWorldBuilds.RemoveAt(Index);
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

		FNavigationWorldBuildState* Existing = FindWorldBuild(&InWorld);
		if (!Existing)
		{
			FNavigationWorldBuildState NewState;
			NewState.World = &InWorld;
			WorldBuilds.Add(NewState);
			Existing = &WorldBuilds[WorldBuilds.Num<UINT32>() - 1u];
		}
		FNavigationWorldBuildState& State = *Existing;
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
			Result.OwnerId = Owner->GetUniqueID();
			Result.NavMeshName = Owner->GetNavMeshName();

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
			if ((Volumes.Num<UINT32>() == 0u) && (Result.NavMeshName.Length() > 0u))
			{
				Result.Error = EString(PE_TEXT("no bounds volume supports this navmesh"));
				State.Report.Profiles.Add(Result);
				PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh '")) + Result.NavMeshName
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
			if (!RequestBuildInternal(Result.OwnerId, Result.NavMeshName, Provider, static_cast<ENavigationSourceGeometry&&>(Geometry), BatchId))
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
			FinishWorldBuild(State);
		}
		return TRUE;
	}

	void ENavigationManager::CompleteWorldBuildJob(const FBakeResult& InResult, BOOL32 bSucceeded, BOOL32 bDiscarded)
	{
		if (InResult.BatchId == 0u)
		{
			return;
		}
		FNavigationWorldBuildState* State = nullptr;
		for (UINT32 Index = 0u, Num = WorldBuilds.Num<UINT32>(); Index < Num; Index++)
		{
			if (WorldBuilds[Index].BatchId == InResult.BatchId)
			{
				State = &WorldBuilds[Index];
				break;
			}
		}
		if (!State)
		{
			return;
		}

		FNavigationProfileBuildResult* Profile = nullptr;
		for (UINT32 Index = 0u, Num = State->Report.Profiles.Num<UINT32>(); Index < Num; Index++)
		{
			if (State->Report.Profiles[Index].OwnerId == InResult.OwnerId)
			{
				Profile = &State->Report.Profiles[Index];
				break;
			}
		}
		if (!Profile)
		{
			FNavigationProfileBuildResult NewProfile;
			NewProfile.OwnerId = InResult.OwnerId;
			NewProfile.NavMeshName = InResult.NavMeshName;
			State->Report.Profiles.Add(NewProfile);
			Profile = &State->Report.Profiles[State->Report.Profiles.Num<UINT32>() - 1u];
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

		if (State->Pending > 0u)
		{
			State->Pending--;
		}
		if (State->Pending == 0u)
		{
			FinishWorldBuild(*State);
		}
	}

	void ENavigationManager::FinishWorldBuild(FNavigationWorldBuildState& InOutState)
	{
		FNavigationWorldBuildReport& Report = InOutState.Report;
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
						+ ((Profile.NavMeshName.Length() > 0u) ? Profile.NavMeshName : EString(PE_TEXT("Default")))
						+ EString(PE_TEXT("' was built with "))
						+ EString::FromInt(Profile.Stats.SkippedMeshCount)
						+ EString(PE_TEXT(" static mesh components skipped; build again once they load.")));
				}
				continue;
			}
			Report.bAllSucceeded = FALSE;
			PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh profile '"))
				+ ((Profile.NavMeshName.Length() > 0u) ? Profile.NavMeshName : EString(PE_TEXT("Default")))
				+ EString(PE_TEXT("' was not built: ")) + Profile.Error);
		}
		PE_LOG_LOG(EString(PE_TEXT("Navigation: level build finished: "))
			+ EString::FromInt(Succeeded) + EString(PE_TEXT("/")) + EString::FromInt(Report.Profiles.Num<UINT32>())
			+ EString(PE_TEXT(" navmesh profiles built.")));

		const BOOL32 bRestart = InOutState.bDirtyDuringBuild;
		PWorld* World = InOutState.World;
		const ENavigationWorldBuildReason Reason = InOutState.Reason;
		const EString Stem = InOutState.LevelStem;
		InOutState.bDirtyDuringBuild = FALSE;
		InOutState.LastSeenInputRevision = GetNavigationInputRevision(World);
		InOutState.RebuildCountdown = 0.0f;
		InOutState.bWatchingInput = TRUE;
		if (bRestart)
		{
			// Inputs changed while this pass baked; run one more for the latest world state.
			RequestWorldBuild(*World, (Reason == ENavigationWorldBuildReason::SAVE) ? ENavigationWorldBuildReason::EDITOR : Reason, Stem);
		}
	}

	void ENavigationManager::TickWorldBuildInputs(FLOAT InDeltaTime)
	{
		for (UINT32 Index = 0u; Index < WorldBuilds.Num<UINT32>(); Index++)
		{
			FNavigationWorldBuildState& State = WorldBuilds[Index];
			if ((!State.bWatchingInput) || (!State.World) || (State.Pending > 0u))
			{
				continue;
			}
			const UINT32 Revision = GetNavigationInputRevision(State.World);
			if (Revision != State.LastSeenInputRevision)
			{
				State.LastSeenInputRevision = Revision;
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
			PWorld* World = State.World;
			RequestWorldBuild(*World, ENavigationWorldBuildReason::EDITOR);
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
			Result.OwnerId = Owner->GetUniqueID();
			Result.NavMeshName = Owner->GetNavMeshName();

			const TArray<EBoundAABB> Volumes = Owner->CollectBakeVolumesInWorld();
			if ((Volumes.Num<UINT32>() == 0u) && (Result.NavMeshName.Length() > 0u))
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
				AssetName = PNavMeshComponent::MakeNavMeshAssetName(InLevelStem, Result.NavMeshName);
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
			RegisterProvider(Result.OwnerId, Result.NavMeshName, Provider, Stats);
			Result.bSucceeded = TRUE;
			Result.Stats = Stats;
			OutReport.Profiles.Add(Result);
		}

		// Save builds do not need a batch record, but the panel and the input watch should see the
		// result, and the level is about to be serialized right after this.
		FNavigationWorldBuildState* State = FindWorldBuild(&InWorld);
		if (!State)
		{
			FNavigationWorldBuildState NewState;
			NewState.World = &InWorld;
			WorldBuilds.Add(NewState);
			State = &WorldBuilds[WorldBuilds.Num<UINT32>() - 1u];
		}
		State->Report = OutReport;
		State->Pending = 0u;
		State->bDirtyDuringBuild = FALSE;
		State->LastSeenInputRevision = GetNavigationInputRevision(&InWorld);
		State->RebuildCountdown = 0.0f;
		State->bWatchingInput = TRUE;

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
					+ ((OutReport.Profiles[Index].NavMeshName.Length() > 0u) ? OutReport.Profiles[Index].NavMeshName : EString(PE_TEXT("Default")))
					+ EString(PE_TEXT("' was not built for save: ")) + OutReport.Profiles[Index].Error);
			}
		}
		PE_LOG_LOG(EString(PE_TEXT("Navigation: save build finished: "))
			+ EString::FromInt(Succeeded) + EString(PE_TEXT("/")) + EString::FromInt(OutReport.Profiles.Num<UINT32>())
			+ EString(PE_TEXT(" navmesh profiles written.")));
		return OutReport.bAllSucceeded;
	}
#endif

	BOOL32 ENavigationManager::FindPath(const Vector3& InStart, const Vector3& InEnd, ENavigationPath& OutPath, const EString& InNavMeshName) const
	{
		const ENavigationQueryFilter DefaultFilter;
		return FindPath(InStart, InEnd, DefaultFilter, OutPath, InNavMeshName);
	}

	BOOL32 ENavigationManager::FindPath(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationPath& OutPath, const EString& InNavMeshName) const
	{
		ENavigationProviderInterface* Provider = FindProviderForLocation(InStart, InNavMeshName);
		if (!Provider)
		{
			return FALSE;
		}
		return Provider->FindPath(InStart, InEnd, InFilter, OutPath);
	}

	BOOL32 ENavigationManager::ProjectPoint(const Vector3& InLocation, Vector3& OutLocation, const EString& InNavMeshName) const
	{
		ENavigationProviderInterface* Provider = FindProviderForLocation(InLocation, InNavMeshName);
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

	ENavigationObstacleHandle ENavigationManager::AttachObstacle(FNavigationObstacleDesc& InOutDesc, const ObjectIdentityType& InOwnerId, const UINT32& InSerial, ENavigationProviderInterface& InProvider)
	{
		const ENavigationObstacleHandle Handle = InOutDesc.bBox
			? InProvider.AddBoxObstacle(InOutDesc.Center, InOutDesc.HalfExtents, InOutDesc.YawRadians)
			: InProvider.AddCylinderObstacle(InOutDesc.Center, InOutDesc.Radius, InOutDesc.Height);
		if (Handle == NavigationInvalidObstacleHandle)
		{
			// The provider cannot take obstacles at all (a single tile bake has no tile cache).
			// The description stays unowned so a later tiled provider of the group can adopt it.
			InOutDesc.ProviderHandle = NavigationInvalidObstacleHandle;
			InOutDesc.ProviderOwnerId = 0;
			InOutDesc.ProviderSerial = 0u;
			return NavigationInvalidObstacleHandle;
		}
		InOutDesc.ProviderHandle = Handle;
		InOutDesc.ProviderOwnerId = InOwnerId;
		InOutDesc.ProviderSerial = InSerial;
		return Handle;
	}

	ENavigationObstacleHandle ENavigationManager::PublishObstacle(FNavigationObstacleDesc& InOutDesc)
	{
		const INT32 ProviderIndex = FindProviderIndexForLocation(InOutDesc.Center, InOutDesc.GroupName);
		if (ProviderIndex < 0)
		{
			// No volume of the group covers the obstacle yet. The description stays retained and
			// is replayed when one is published, which is what makes an obstacle added before
			// the bake survive it.
			InOutDesc.ProviderHandle = NavigationInvalidObstacleHandle;
			InOutDesc.ProviderOwnerId = 0;
			InOutDesc.ProviderSerial = 0u;
			return InOutDesc.Id;
		}
		const FNavigationProviderEntry& Entry = Providers[static_cast<UINT32>(ProviderIndex)];
		if (!Entry.Provider)
		{
			return InOutDesc.Id;
		}
		if (AttachObstacle(InOutDesc, Entry.OwnerId, Entry.Serial, *Entry.Provider) == NavigationInvalidObstacleHandle)
		{
			// A provider exists but refused the obstacle: report it instead of handing back an
			// id for something that is not live.
			return NavigationInvalidObstacleHandle;
		}
		return InOutDesc.Id;
	}

	void ENavigationManager::ReplayObstacles(const ObjectIdentityType& InOwnerId, const EString& InNavMeshName, const UINT32& InSerial, ENavigationProviderInterface& InProvider)
	{
		for (UINT32 Index = 0u, Num = Obstacles.Num<UINT32>(); Index < Num; Index++)
		{
			FNavigationObstacleDesc& Desc = Obstacles[Index];
			const BOOL32 bOwnedHere = (Desc.ProviderOwnerId == InOwnerId) ? TRUE : FALSE;
			if ((!bOwnedHere) && (Desc.ProviderOwnerId != 0))
			{
				// Belongs to another live provider; touching it here would duplicate it there.
				continue;
			}
			if ((!bOwnedHere) && (Desc.GroupName != InNavMeshName))
			{
				continue;
			}
			if ((!bOwnedHere) && (!BoundsContainLocation(InProvider.GetBounds(), Desc.Center)))
			{
				// Still outside this provider's coverage; the provider that covers it adopts it.
				continue;
			}
			AttachObstacle(Desc, InOwnerId, InSerial, InProvider);
		}
	}

	ENavigationObstacleHandle ENavigationManager::AddCylinderObstacle(const ObjectIdentityType& InRequesterId, const EString& InNavMeshName, const Vector3& InCenter, const FLOAT& InRadius, const FLOAT& InHeight)
	{
		FNavigationObstacleDesc Desc;
		Desc.Id = NextObstacleId++;
		Desc.RequesterId = InRequesterId;
		Desc.GroupName = InNavMeshName;
		Desc.bBox = FALSE;
		Desc.Center = InCenter;
		Desc.Radius = InRadius;
		Desc.Height = InHeight;
		// Retained even when no volume covers it yet: the replay after the next build picks it up.
		if (PublishObstacle(Desc) == NavigationInvalidObstacleHandle)
		{
			return NavigationInvalidObstacleHandle;
		}
		Obstacles.Add(Desc);
		return Desc.Id;
	}

	ENavigationObstacleHandle ENavigationManager::AddBoxObstacle(const ObjectIdentityType& InRequesterId, const EString& InNavMeshName, const Vector3& InCenter, const Vector3& InHalfExtents, const FLOAT& InYawRadians)
	{
		FNavigationObstacleDesc Desc;
		Desc.Id = NextObstacleId++;
		Desc.RequesterId = InRequesterId;
		Desc.GroupName = InNavMeshName;
		Desc.bBox = TRUE;
		Desc.Center = InCenter;
		Desc.HalfExtents = InHalfExtents;
		Desc.YawRadians = InYawRadians;
		if (PublishObstacle(Desc) == NavigationInvalidObstacleHandle)
		{
			return NavigationInvalidObstacleHandle;
		}
		Obstacles.Add(Desc);
		return Desc.Id;
	}

	void ENavigationManager::RemoveObstacle(const ObjectIdentityType& InRequesterId, ENavigationObstacleHandle InHandle)
	{
		if (InHandle == NavigationInvalidObstacleHandle)
		{
			return;
		}
		for (UINT32 Index = 0u, Num = Obstacles.Num<UINT32>(); Index < Num; Index++)
		{
			if ((Obstacles[Index].RequesterId != InRequesterId) || (Obstacles[Index].Id != InHandle))
			{
				continue;
			}
			// Remove through the provider that accepted the obstacle. Resolving by position
			// could hand the handle to a different provider after a rebuild.
			if (Obstacles[Index].ProviderHandle != NavigationInvalidObstacleHandle)
			{
				ENavigationProviderInterface* Provider = GetProviderForOwner(Obstacles[Index].ProviderOwnerId, Obstacles[Index].ProviderSerial);
				if (!!Provider)
				{
					Provider->RemoveObstacle(Obstacles[Index].ProviderHandle);
				}
			}
			Obstacles.RemoveAt(Index);
			return;
		}
	}

	ENavigationManager::FCrowdBinding* ENavigationManager::FindCrowdBinding(const ObjectIdentityType& InRequesterId)
	{
		for (UINT32 Index = 0u, Num = CrowdBindings.Num<UINT32>(); Index < Num; Index++)
		{
			if (CrowdBindings[Index].RequesterId == InRequesterId)
			{
				return &CrowdBindings[Index];
			}
		}
		return nullptr;
	}

	const ENavigationManager::FCrowdBinding* ENavigationManager::FindCrowdBinding(const ObjectIdentityType& InRequesterId) const
	{
		for (UINT32 Index = 0u, Num = CrowdBindings.Num<UINT32>(); Index < Num; Index++)
		{
			if (CrowdBindings[Index].RequesterId == InRequesterId)
			{
				return &CrowdBindings[Index];
			}
		}
		return nullptr;
	}

	void ENavigationManager::InvalidateCrowdBindings(const ObjectIdentityType& InOwnerId)
	{
		// Only the bindings of the provider that was replaced or removed lose their handle;
		// agents living on other providers keep steering in place.
		for (UINT32 Index = 0u, Num = CrowdBindings.Num<UINT32>(); Index < Num; Index++)
		{
			if (CrowdBindings[Index].ProviderOwnerId != InOwnerId)
			{
				continue;
			}
			CrowdBindings[Index].ProviderHandle = NavigationInvalidCrowdAgentHandle;
			CrowdBindings[Index].ProviderOwnerId = 0;
			CrowdBindings[Index].ProviderSerial = 0u;
		}
	}

	BOOL32 ENavigationManager::RegisterCrowdAgent(const ObjectIdentityType& InRequesterId, const EString& InNavMeshName, const Vector3& InPosition, const ENavigationAgentSettings& InSettings)
	{
		FCrowdBinding* Binding = FindCrowdBinding(InRequesterId);
		if (!Binding)
		{
			FCrowdBinding NewBinding;
			NewBinding.RequesterId = InRequesterId;
			CrowdBindings.Add(NewBinding);
			Binding = &CrowdBindings[CrowdBindings.Num<UINT32>() - 1u];
		}
		Binding->GroupName = InNavMeshName;
		Binding->Settings = InSettings;
		Binding->LastPosition = InPosition;

		if (Binding->ProviderHandle != NavigationInvalidCrowdAgentHandle)
		{
			// Already registered, but only while the provider is still the build that issued it.
			if (!!GetProviderForOwner(Binding->ProviderOwnerId, Binding->ProviderSerial))
			{
				return TRUE;
			}
			Binding->ProviderHandle = NavigationInvalidCrowdAgentHandle;
			Binding->ProviderOwnerId = 0;
			Binding->ProviderSerial = 0u;
		}

		const INT32 ProviderIndex = FindProviderIndexForLocation(InPosition, Binding->GroupName);
		if (ProviderIndex < 0)
		{
			return FALSE;
		}
		const FNavigationProviderEntry& Entry = Providers[static_cast<UINT32>(ProviderIndex)];
		if (!Entry.Provider)
		{
			return FALSE;
		}
		Binding->ProviderHandle = Entry.Provider->AddCrowdAgent(InPosition, InSettings);
		if (Binding->ProviderHandle == NavigationInvalidCrowdAgentHandle)
		{
			return FALSE;
		}
		Binding->ProviderOwnerId = Entry.OwnerId;
		Binding->ProviderSerial = Entry.Serial;
		if (Binding->bHasTarget)
		{
			// The agent was re-created, so the target has to be handed over again.
			Entry.Provider->RequestCrowdAgentMove(Binding->ProviderHandle, Binding->Target);
		}
		return TRUE;
	}

	BOOL32 ENavigationManager::RequestCrowdAgentMove(const ObjectIdentityType& InRequesterId, const Vector3& InTarget)
	{
		FCrowdBinding* Binding = FindCrowdBinding(InRequesterId);
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
		ENavigationProviderInterface* Provider = GetProviderForOwner(Binding->ProviderOwnerId, Binding->ProviderSerial);
		if (!Provider)
		{
			Binding->ProviderHandle = NavigationInvalidCrowdAgentHandle;
			Binding->ProviderOwnerId = 0;
			Binding->ProviderSerial = 0u;
			return FALSE;
		}
		return Provider->RequestCrowdAgentMove(Binding->ProviderHandle, InTarget);
	}

	void ENavigationManager::StopCrowdAgent(const ObjectIdentityType& InRequesterId)
	{
		FCrowdBinding* Binding = FindCrowdBinding(InRequesterId);
		if (!Binding)
		{
			return;
		}
		Binding->bHasTarget = FALSE;
		if (Binding->ProviderHandle == NavigationInvalidCrowdAgentHandle)
		{
			return;
		}
		ENavigationProviderInterface* Provider = GetProviderForOwner(Binding->ProviderOwnerId, Binding->ProviderSerial);
		if (!!Provider)
		{
			Provider->StopCrowdAgent(Binding->ProviderHandle);
		}
		else
		{
			Binding->ProviderHandle = NavigationInvalidCrowdAgentHandle;
			Binding->ProviderOwnerId = 0;
			Binding->ProviderSerial = 0u;
		}
	}

	void ENavigationManager::UnregisterCrowdAgent(const ObjectIdentityType& InRequesterId)
	{
		for (UINT32 Index = 0u, Num = CrowdBindings.Num<UINT32>(); Index < Num; Index++)
		{
			if (CrowdBindings[Index].RequesterId != InRequesterId)
			{
				continue;
			}
			if (CrowdBindings[Index].ProviderHandle != NavigationInvalidCrowdAgentHandle)
			{
				ENavigationProviderInterface* Provider = GetProviderForOwner(CrowdBindings[Index].ProviderOwnerId, CrowdBindings[Index].ProviderSerial);
				if (!!Provider)
				{
					Provider->RemoveCrowdAgent(CrowdBindings[Index].ProviderHandle);
				}
			}
			CrowdBindings.RemoveAt(Index);
			return;
		}
	}

	BOOL32 ENavigationManager::GetCrowdAgentState(const ObjectIdentityType& InRequesterId, ENavigationCrowdAgentState& OutState) const
	{
		const FCrowdBinding* Binding = FindCrowdBinding(InRequesterId);
		if ((!Binding) || (Binding->ProviderHandle == NavigationInvalidCrowdAgentHandle))
		{
			return FALSE;
		}
		ENavigationProviderInterface* Provider = GetProviderForOwner(Binding->ProviderOwnerId, Binding->ProviderSerial);
		if (!Provider)
		{
			return FALSE;
		}
		return Provider->GetCrowdAgentState(Binding->ProviderHandle, OutState);
	}

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
			for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
			{
				ENavigationProviderInterface* Provider = Providers[Index].Provider;
				if ((!!Provider) && (Provider->IsReady()) && (Provider->GetContentRevision() != Providers[Index].DebugRevision))
				{
					bDebugLinesDirty = TRUE;
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
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			ENavigationProviderInterface* Provider = Providers[Index].Provider;
			if ((!!Provider) && (Provider->IsReady()))
			{
				if (Providers[Index].bDebugDrawEnabled)
				{
					Provider->CollectDebugLines(DebugDrawMode, DebugPolygonSegments);
					if (bDebugTrianglesEnabled)
					{
						Provider->CollectDebugTriangles(DebugTriangleSegments);
					}
				}
				// Tracked even while this provider is hidden: otherwise its content revision would
				// keep the whole cache dirty and every request would re-collect every provider.
				Providers[Index].DebugRevision = Provider->GetContentRevision();
			}
		}
		bDebugLinesDirty = FALSE;
		// The editor compares this against the revision it last uploaded, so every rebuild
		// has to produce a value the previous one did not.
		DebugSegmentsRevision++;
	}

	INT32 ENavigationManager::FindProviderIndex(const ObjectIdentityType& InOwnerId) const
	{
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			if (Providers[Index].OwnerId == InOwnerId)
			{
				return static_cast<INT32>(Index);
			}
		}
		return -1;
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
		Obstacles.Empty(0u);
		CrowdBindings.Empty(0u);
		WorldInputRevisions.Empty(0u);
		WorldBuilds.Empty(0u);
		PendingWorldBuilds.Empty(0u);
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			delete Providers[Index].Provider;
			Providers[Index].Provider = nullptr;
		}
		Providers.Empty(0u);
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
			const INT32 EntryIndex = FindProviderIndex(Result.OwnerId);
			if ((EntryIndex < 0) || (Providers[static_cast<UINT32>(EntryIndex)].Serial != Result.Serial))
			{
				// The component left the world or asked again while this was baking.
				delete Result.Provider;
				Result.Provider = nullptr;
				CompleteWorldBuildJob(Result, FALSE, TRUE);
				continue;
			}

			FNavigationProviderEntry& Entry = Providers[static_cast<UINT32>(EntryIndex)];
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
			Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_READY;
			Entry.Stats = Result.Stats;
			Result.Provider = nullptr;
			// The old provider's tile cache and crowd died with it, so the obstacles have to be
			// re-registered and the crowd bindings rebuilt by their owners. Only this provider's
			// records are touched; other profiles keep their agents and obstacles.
			InvalidateCrowdBindings(Entry.OwnerId);
			ReplayObstacles(Entry.OwnerId, Entry.GroupName, Entry.Serial, *Entry.Provider);
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
				Result.OwnerId = Job.OwnerId;
				Result.NavMeshName = Job.NavMeshName;
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
