#include <Navigation/NavigationManager.h>

#include <Development/EditorLogManager.h>
#include <Navigation/NavigationProviderInterface.h>

namespace PigeonEngine
{

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
		PublishFinishedBakes();

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

	void ENavigationManager::RegisterProvider(const ObjectIdentityType& InOwnerId, ENavigationProviderInterface* InProvider)
	{
		if (!InProvider)
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: registering a null provider was ignored.")));
			return;
		}

		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		if (ExistingIndex >= 0)
		{
			// A replacement releases the old provider here rather than at the call site so
			// ownership never depends on who built what.
			FNavigationProviderEntry& Entry = Providers[static_cast<UINT32>(ExistingIndex)];
			delete Entry.Provider;
			Entry.Provider = InProvider;
			Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_READY;
			// A bake queued before this publish is older than what is being installed now.
			Entry.Serial++;
		}
		else
		{
			FNavigationProviderEntry Entry;
			Entry.OwnerId = InOwnerId;
			Entry.Provider = InProvider;
			Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_READY;
			Entry.Serial = 1u;
			Providers.Add(Entry);
		}
		ReplayObstacles();
		InvalidateCrowdBindings();
		InvalidateDebugLines();
	}

	BOOL32 ENavigationManager::RequestBuild(const ObjectIdentityType& InOwnerId, ENavigationProviderInterface* InProvider, ENavigationSourceGeometry&& InGeometry)
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
			Providers.Add(Entry);
			EntryIndex = static_cast<INT32>(Providers.Num<UINT32>() - 1u);
		}

		FNavigationProviderEntry& Entry = Providers[static_cast<UINT32>(EntryIndex)];
		if (Entry.State == ENavigationBuildStateType::NAVIGATION_BUILD_RUNNING)
		{
			// Already baking this volume: the caller keeps its provider and the running job wins.
			return FALSE;
		}

		Entry.Serial++;
		Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_RUNNING;

		{
			EScopedLock Lock(BakeMutex);
			FBakeJob Job;
			Job.OwnerId = InOwnerId;
			Job.Serial = Entry.Serial;
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

	ENavigationProviderInterface* ENavigationManager::GetDefaultProvider() const
	{
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			ENavigationProviderInterface* Provider = Providers[Index].Provider;
			if ((!!Provider) && (Provider->IsReady()))
			{
				return Provider;
			}
		}
		return nullptr;
	}

	ENavigationProviderInterface* ENavigationManager::FindProviderForLocation(const Vector3& InLocation) const
	{
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			ENavigationProviderInterface* Provider = Providers[Index].Provider;
			if ((!!Provider) && (Provider->IsReady()) && (BoundsContainLocation(Provider->GetBounds(), InLocation)))
			{
				return Provider;
			}
		}
		// Outside every baked volume the default provider is still the best answer: it
		// projects the point onto its own mesh, which is what a caller with a stray start
		// position expects.
		return GetDefaultProvider();
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

	BOOL32 ENavigationManager::FindPath(const Vector3& InStart, const Vector3& InEnd, ENavigationPath& OutPath) const
	{
		const ENavigationQueryFilter DefaultFilter;
		return FindPath(InStart, InEnd, DefaultFilter, OutPath);
	}

	BOOL32 ENavigationManager::FindPath(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationPath& OutPath) const
	{
		ENavigationProviderInterface* Provider = FindProviderForLocation(InStart);
		if (!Provider)
		{
			return FALSE;
		}
		return Provider->FindPath(InStart, InEnd, InFilter, OutPath);
	}

	BOOL32 ENavigationManager::ProjectPoint(const Vector3& InLocation, Vector3& OutLocation) const
	{
		ENavigationProviderInterface* Provider = FindProviderForLocation(InLocation);
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

	ENavigationObstacleHandle ENavigationManager::PublishObstacle(FNavigationObstacleDesc& InOutDesc)
	{
		ENavigationProviderInterface* Provider = FindProviderForLocation(InOutDesc.Center);
		if (!Provider)
		{
			// No volume covers the obstacle yet. The description stays retained and is replayed when
			// one is published, which is what makes an obstacle added before the bake survive it.
			InOutDesc.ProviderHandle = NavigationInvalidObstacleHandle;
			return InOutDesc.Id;
		}
		InOutDesc.ProviderHandle = InOutDesc.bBox
			? Provider->AddBoxObstacle(InOutDesc.Center, InOutDesc.HalfExtents, InOutDesc.YawRadians)
			: Provider->AddCylinderObstacle(InOutDesc.Center, InOutDesc.Radius, InOutDesc.Height);
		return InOutDesc.Id;
	}

	void ENavigationManager::ReplayObstacles()
	{
		for (UINT32 Index = 0u, Num = Obstacles.Num<UINT32>(); Index < Num; Index++)
		{
			PublishObstacle(Obstacles[Index]);
		}
	}

	ENavigationObstacleHandle ENavigationManager::AddCylinderObstacle(const ObjectIdentityType& InRequesterId, const Vector3& InCenter, const FLOAT& InRadius, const FLOAT& InHeight)
	{
		FNavigationObstacleDesc Desc;
		Desc.Id = NextObstacleId++;
		Desc.RequesterId = InRequesterId;
		Desc.bBox = FALSE;
		Desc.Center = InCenter;
		Desc.Radius = InRadius;
		Desc.Height = InHeight;
		// Retained even when no volume covers it yet: the replay after the next build picks it up.
		PublishObstacle(Desc);
		Obstacles.Add(Desc);
		return Desc.Id;
	}

	ENavigationObstacleHandle ENavigationManager::AddBoxObstacle(const ObjectIdentityType& InRequesterId, const Vector3& InCenter, const Vector3& InHalfExtents, const FLOAT& InYawRadians)
	{
		FNavigationObstacleDesc Desc;
		Desc.Id = NextObstacleId++;
		Desc.RequesterId = InRequesterId;
		Desc.bBox = TRUE;
		Desc.Center = InCenter;
		Desc.HalfExtents = InHalfExtents;
		Desc.YawRadians = InYawRadians;
		PublishObstacle(Desc);
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
			if (Obstacles[Index].ProviderHandle != NavigationInvalidObstacleHandle)
			{
				ENavigationProviderInterface* Provider = FindProviderForLocation(Obstacles[Index].Center);
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

	void ENavigationManager::InvalidateCrowdBindings()
	{
		for (UINT32 Index = 0u, Num = CrowdBindings.Num<UINT32>(); Index < Num; Index++)
		{
			CrowdBindings[Index].ProviderHandle = NavigationInvalidCrowdAgentHandle;
		}
	}

	BOOL32 ENavigationManager::RegisterCrowdAgent(const ObjectIdentityType& InRequesterId, const Vector3& InPosition, const ENavigationAgentSettings& InSettings)
	{
		FCrowdBinding* Binding = FindCrowdBinding(InRequesterId);
		if (!Binding)
		{
			FCrowdBinding NewBinding;
			NewBinding.RequesterId = InRequesterId;
			CrowdBindings.Add(NewBinding);
			Binding = &CrowdBindings[CrowdBindings.Num<UINT32>() - 1u];
		}
		Binding->Settings = InSettings;
		Binding->LastPosition = InPosition;

		if (Binding->ProviderHandle != NavigationInvalidCrowdAgentHandle)
		{
			// Already registered with a live provider.
			return TRUE;
		}

		ENavigationProviderInterface* Provider = FindProviderForLocation(InPosition);
		if (!Provider)
		{
			return FALSE;
		}
		Binding->ProviderHandle = Provider->AddCrowdAgent(InPosition, InSettings);
		if (Binding->ProviderHandle == NavigationInvalidCrowdAgentHandle)
		{
			return FALSE;
		}
		if (Binding->bHasTarget)
		{
			// The agent was re-created, so the target has to be handed over again.
			Provider->RequestCrowdAgentMove(Binding->ProviderHandle, Binding->Target);
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
		ENavigationProviderInterface* Provider = FindProviderForLocation(Binding->LastPosition);
		if (!Provider)
		{
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
		ENavigationProviderInterface* Provider = FindProviderForLocation(Binding->LastPosition);
		if (!!Provider)
		{
			Provider->StopCrowdAgent(Binding->ProviderHandle);
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
				ENavigationProviderInterface* Provider = FindProviderForLocation(CrowdBindings[Index].LastPosition);
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
		ENavigationProviderInterface* Provider = FindProviderForLocation(Binding->LastPosition);
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
				continue;
			}

			FNavigationProviderEntry& Entry = Providers[static_cast<UINT32>(EntryIndex)];
			if (!Result.bSucceeded)
			{
				Entry.State = ENavigationBuildStateType::NAVIGATION_BUILD_FAILED;
				delete Result.Provider;
				Result.Provider = nullptr;
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
			// re-registered and the crowd bindings rebuilt by their owners.
			ReplayObstacles();
			InvalidateCrowdBindings();
			InvalidateDebugLines();
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
				Result.Serial = Job.Serial;
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
