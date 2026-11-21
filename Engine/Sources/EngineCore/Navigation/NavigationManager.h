#pragma once

#include <Base/DataStructure/ObjectBase.h>
#include <EngineCommon.h>
#include <MultiThread/Event.h>
#include <MultiThread/Mutex.h>
#include <MultiThread/Thread.h>
#include <Navigation/NavigationGeometry.h>
#include <Navigation/NavigationTypes.h>

namespace PigeonEngine
{

	class ENavMeshAsset;
	class ENavigationProviderInterface;
	class PWorld;

	/*
	* Runtime owner of every navigation provider - the engine's navigation system.
	*
	* One FNavigationWorld exists per PWorld: its providers, obstacles, crowd bindings, build
	* state and authored input revision. Queries resolve inside one world and one agent profile
	* only, so two worlds (or two agent sizes) never answer for each other. The world manager
	* creates the state when a world is created and drops it before the world is destroyed.
	*
	* Baking runs on a worker thread the manager owns. Collecting the source geometry stays on
	* the game thread (it reads the world), the provider is created on the game thread, and only
	* the Recast/Detour part runs on the worker. The result is published by Tick, which the frame
	* calls before the world ticks, so a bake that finished is usable in the same frame.
	*
	* Thread affinity: game thread for every public method; the worker only touches the job and
	* result lists, both under BakeMutex.
	*/
	class ENavigationManager final : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(ENavigationManager)
	public:
		void	Initialize() override;
		void	ShutDown() override;
		// Called once per frame, before the world ticks: publishes finished bakes and advances the
		// per provider work (local avoidance, dynamic obstacle rebuilds).
		void	Tick(FLOAT InDeltaTime);

	public:
		// Creates/resets the navigation state of a world. Called by the world manager after a
		// world is created; on-demand lookups also create it, so a caller never has to order this.
		void	NotifyWorldCreated(PWorld* InWorld);
		// Drops every provider, obstacle, crowd binding and build record of a world. Called by the
		// world manager before the world is destroyed.
		void	NotifyWorldDestroyed(PWorld* InWorld);

	public:
		/*
		* Publishes a provider immediately. Used by the asset import path, which is a memory copy,
		* and by any provider that does not need a bake. InProfileName is the agent profile the
		* provider answers for; an empty name is the level's default agent. InSourceId is the navmesh
		* actor that authored the profile, used to clean the providers up when that object leaves the
		* world.
		*/
		void	RegisterProvider(PWorld* InWorld, const ObjectIdentityType& InSourceId, const EString& InProfileName, ENavigationProviderInterface* InProvider, const ENavigationBuildStats& InStats = ENavigationBuildStats());
		// Queues a bake of InProvider with InGeometry on the worker. Ownership of the provider
		// transfers when this returns TRUE; on FALSE the caller still owns it. A second request
		// for the same profile while one is running is refused.
		BOOL32	RequestBuild(PWorld* InWorld, const ObjectIdentityType& InSourceId, const EString& InProfileName, ENavigationProviderInterface* InProvider, ENavigationSourceGeometry&& InGeometry);
		// Removes the provider of one profile in one world.
		void	UnregisterProvider(PWorld* InWorld, const EString& InProfileName);
		// Removes every provider authored by InSourceId. A null world searches all worlds, which is
		// what an object needs right after it left the scene.
		void	UnregisterProfilesBySource(PWorld* InWorld, const ObjectIdentityType& InSourceId);
		// Per provider overlay switch.
		void	SetProviderDebugDrawEnabled(PWorld* InWorld, const EString& InProfileName, const BOOL32& InEnabled);

		PE_NODISCARD ENavigationProviderInterface* GetProvider(PWorld* InWorld, const EString& InProfileName) const;
		// First provider of the profile that finished a build. Null while the profile has nothing.
		PE_NODISCARD ENavigationProviderInterface* GetDefaultProvider(PWorld* InWorld, const EString& InProfileName = EString()) const;
		/*
		* Provider of the profile whose bounds contain InLocation, or that profile's first ready
		* provider when none does. Profiles never answer for each other, and neither do worlds.
		*/
		PE_NODISCARD ENavigationProviderInterface* FindProviderForLocation(PWorld* InWorld, const Vector3& InLocation, const EString& InProfileName = EString()) const;
		PE_NODISCARD UINT32 GetProviderNum(PWorld* InWorld) const;
		// Build state/stats of one profile, for the panel and the authoring object.
		PE_NODISCARD ENavigationBuildStateType		GetBuildState(PWorld* InWorld, const EString& InProfileName) const;
		PE_NODISCARD const ENavigationBuildStats&	GetBuildStats(PWorld* InWorld, const EString& InProfileName) const;

		/*
		* Authored navigation input changed: a bounds volume was added, moved or resized, or a
		* profile edited its bake settings or links. The revision is per world, so a preview world
		* editing its volumes never schedules a rebuild in the level next to it. The world build
		* pass watches it and rebuilds in the editor; runtime builds stay explicit.
		*/
		void				BumpNavigationInputRevision(PWorld* InWorld);
		PE_NODISCARD UINT32	GetNavigationInputRevision(const PWorld* InWorld) const;

	public:
		// Why a world build pass runs. It selects the source of each profile's mesh.
		enum class ENavigationWorldBuildReason : UINT8
		{
			// Load a level: import an assigned asset when there is one, bake the agents that
			// have none. One wait for the asset queue covers the whole level.
			LOAD,
			// Shipped game start: like LOAD, but agents with BuildOnBeginPlay off are skipped.
			RUNTIME,
			// Editor Build Navigation: bake every agent from the world, ignoring assets.
			EDITOR,
			// Level save: bake every agent from the world and write each agent's asset, so
			// the level and the files it references describe the same world.
			SAVE
		};
		// One agent profile's outcome inside a world build pass.
		struct FNavigationProfileBuildResult
		{
			// Navmesh actor behind the profile; identifies who authored the agent in this pass.
			ObjectIdentityType	SourceId = 0;
			EString				ProfileName;
			BOOL32				bSucceeded = FALSE;
			ENavigationBuildStats	Stats;
			EString				Error;
		};
		// Outcome of the last world build pass. One entry per agent profile of the level.
		struct FNavigationWorldBuildReport
		{
			BOOL32								bHasRun = FALSE;
			UINT32								BatchId = 0;
			BOOL32								bAllSucceeded = FALSE;
			TArray<FNavigationProfileBuildResult>	Profiles;
		};
		/*
		* One pass over every navigation profile of InWorld: LOAD/RUNTIME import an assigned asset
		* or bake the profiles that have none; EDITOR and SAVE bake all of them from the world.
		* SAVE additionally writes each profile's asset (the level-save path uses the synchronous
		* variant, so the files exist before the level is serialized).
		*
		* Returns TRUE when a pass was started, was already running (the running pass is marked
		* dirty and repeats once), or was queued behind the asset load. False only when the world
		* has no navigation profile at all.
		*/
		BOOL32 RequestWorldBuild(PWorld& InWorld, ENavigationWorldBuildReason InReason, const EString& InLevelStem = EString());
#if ENGINE_EDITOR_ONLY
		// Synchronous SAVE-style pass: bakes and writes every profile on this thread and publishes
		// the results. Used by the level save path.
		BOOL32 BuildWorldSynchronously(PWorld& InWorld, const EString& InLevelStem, FNavigationWorldBuildReport& OutReport);
#endif
		PE_NODISCARD BOOL32 IsWorldBuildRunning(PWorld& InWorld) const;
		PE_NODISCARD const FNavigationWorldBuildReport& GetWorldBuildReport(PWorld& InWorld) const;

		/*
		* Runtime obstacles. The description is retained, so a provider that is rebuilt or reloaded
		* gets its obstacles back on its own: gameplay adds an obstacle once and then only moves it.
		* InProfileName selects the agent profile; the obstacle lands on the provider of that
		* profile whose bounds contain the obstacle.
		*
		* A cylinder sits with its base at InCenter; a box is centred on InCenter. Detour has no
		* "move", so a moving obstacle is removed and re-added, which is what the obstacle component
		* does when its threshold is crossed.
		*/
		ENavigationObstacleHandle	AddCylinderObstacle(PWorld* InWorld, const ObjectIdentityType& InRequesterId, const EString& InProfileName, const Vector3& InCenter, const FLOAT& InRadius, const FLOAT& InHeight);
		ENavigationObstacleHandle	AddBoxObstacle(PWorld* InWorld, const ObjectIdentityType& InRequesterId, const EString& InProfileName, const Vector3& InCenter, const Vector3& InHalfExtents, const FLOAT& InYawRadians);
		void						RemoveObstacle(PWorld* InWorld, const ObjectIdentityType& InRequesterId, ENavigationObstacleHandle InHandle);

		/*
		* Local avoidance, keyed by the component that owns the agent. RegisterCrowdAgent is
		* idempotent and is meant to be called every frame with the agent's current position: it
		* (re)creates the backend agent when there is none, which is how an agent survives a provider
		* rebuild. The requester drives its own transform from GetCrowdAgentState; the manager never
		* writes to game objects.
		*/
		BOOL32	RegisterCrowdAgent(PWorld* InWorld, const ObjectIdentityType& InRequesterId, const EString& InProfileName, const Vector3& InPosition, const ENavigationAgentSettings& InSettings);
		BOOL32	RequestCrowdAgentMove(PWorld* InWorld, const ObjectIdentityType& InRequesterId, const Vector3& InTarget);
		void	StopCrowdAgent(PWorld* InWorld, const ObjectIdentityType& InRequesterId);
		// A null world searches every world, for the scene-exit path where the world is already gone.
		void	UnregisterCrowdAgent(PWorld* InWorld, const ObjectIdentityType& InRequesterId);
		PE_NODISCARD BOOL32	GetCrowdAgentState(PWorld* InWorld, const ObjectIdentityType& InRequesterId, ENavigationCrowdAgentState& OutState) const;

	public:
		// Shortcuts used by gameplay: they resolve a provider of InProfileName in InWorld and answer
		// with engine types. Both return FALSE when that profile has no provider ready; OutPath
		// keeps its previous contents.
		BOOL32	FindPath(PWorld* InWorld, const Vector3& InStart, const Vector3& InEnd, ENavigationPath& OutPath, const EString& InProfileName = EString()) const;
		BOOL32	FindPath(PWorld* InWorld, const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationPath& OutPath, const EString& InProfileName = EString()) const;
		BOOL32	ProjectPoint(PWorld* InWorld, const Vector3& InLocation, Vector3& OutLocation, const EString& InProfileName = EString()) const;

	public:
		void						SetDebugDrawEnabled(const BOOL32& InEnabled);
		PE_NODISCARD BOOL32			IsDebugDrawEnabled() const;
		void						SetDebugDrawMode(ENavigationDebugModeType InMode);
		PE_NODISCARD ENavigationDebugModeType	GetDebugDrawMode() const;
		// Draws the walkable surface's triangulation on top of the polygon edges. The editor
		// owns the colors, so the manager only decides whether the list is collected.
		void						SetDebugTrianglesEnabled(const BOOL32& InEnabled);
		PE_NODISCARD BOOL32			IsDebugTrianglesEnabled() const;
		/*
		* Cached line segments, two entries per segment. The lists are rebuilt on the first
		* request after a provider changed or the mode changed: walking every polygon each
		* frame would cost more than drawing them.
		*/
		const TArray<Vector3>&		GetDebugLineSegments();
		const TArray<Vector3>&		GetDebugTriangleSegments();
		/*
		* Bumped whenever the cached lists were rebuilt. The editor uploads the whole list to
		* the debug primitive only when this changed, instead of every frame.
		*/
		PE_NODISCARD UINT32			GetDebugSegmentsRevision() const;

	private:
		struct FNavigationProviderEntry
		{
			// Agent profile this provider answers for; empty is the level's default agent.
			EString							ProfileName;
			// Object that authored the profile, used to unregister the provider on scene exit.
			ObjectIdentityType				SourceId = 0;
			ENavigationProviderInterface*	Provider = nullptr;
			BOOL32							bDebugDrawEnabled = TRUE;
			ENavigationBuildStateType		State = ENavigationBuildStateType::NAVIGATION_BUILD_IDLE;
			// Bumped whenever a build is requested, so a result from an older request can be
			// dropped instead of replacing a newer provider.
			UINT32							Serial = 0;
			// The provider revision the cached debug lines were built from.
			UINT32							DebugRevision = 0u;
			ENavigationBuildStats			Stats;
		};
		// One bake, owned by the worker until it reports back.
		struct FBakeJob
		{
			PWorld*							World = nullptr;
			ObjectIdentityType				SourceId = 0;
			// Profile name, carried to the report of a world build pass.
			EString							ProfileName;
			UINT32							Serial = 0;
			// Non zero when this job belongs to a world build pass.
			UINT32							BatchId = 0;
			ENavigationProviderInterface*	Provider = nullptr;
			ENavigationSourceGeometry		Geometry;
		};
		/*
		* What gameplay asked for, kept so a rebuilt provider can be restored.
		*
		* Id is what gameplay holds and never changes. ProviderHandle belongs to the provider
		* recorded by ProviderProfile/ProviderSerial; storing the identity (not just the handle)
		* is what keeps a handle from being interpreted by a provider that did not create it.
		*/
		struct FNavigationObstacleDesc
		{
			ENavigationObstacleHandle	Id = NavigationInvalidObstacleHandle;
			ObjectIdentityType			RequesterId = 0;
			EString						ProfileName;
			BOOL32						bBox = FALSE;
			Vector3						Center = Vector3::Zero();
			Vector3						HalfExtents = Vector3::Zero();
			FLOAT						Radius = 0.0f;
			FLOAT						Height = 0.0f;
			FLOAT						YawRadians = 0.0f;
			ENavigationObstacleHandle	ProviderHandle = NavigationInvalidObstacleHandle;
			EString						ProviderProfile;
			UINT32						ProviderSerial = 0u;
		};
		// One agent registered for local avoidance, kept so a rebuilt provider gets it back.
		struct FCrowdBinding
		{
			ObjectIdentityType			RequesterId = 0;
			EString						ProfileName;
			ENavigationCrowdAgentHandle	ProviderHandle = NavigationInvalidCrowdAgentHandle;
			EString						ProviderProfile;
			UINT32						ProviderSerial = 0u;
			ENavigationAgentSettings	Settings;
			Vector3						Target = Vector3::Zero();
			BOOL32						bHasTarget = FALSE;
			Vector3						LastPosition = Vector3::Zero();
		};
		struct FBakeResult
		{
			PWorld*							World = nullptr;
			ObjectIdentityType				SourceId = 0;
			EString							ProfileName;
			UINT32							Serial = 0;
			UINT32							BatchId = 0;
			ENavigationProviderInterface*	Provider = nullptr;
			BOOL32							bSucceeded = FALSE;
			ENavigationBuildStats			Stats;
		};

		// --- World build pass -------------------------------------------------
		struct FNavigationWorldBuildState
		{
			UINT32							BatchId = 0;
			UINT32							Pending = 0;
			BOOL32							bDirtyDuringBuild = FALSE;
			EString							LevelStem;
			ENavigationWorldBuildReason		Reason = ENavigationWorldBuildReason::LOAD;
			FNavigationWorldBuildReport		Report;
			// Editor input watch: the revision the last completed pass saw and the debounce left.
			UINT32							LastSeenInputRevision = 0;
			FLOAT							RebuildCountdown = 0.0f;
			BOOL32							bWatchingInput = FALSE;
		};
		// A LOAD/RUNTIME pass waiting for the asset queue to drain (once for the whole level).
		struct FPendingWorldBuild
		{
			ENavigationWorldBuildReason		Reason = ENavigationWorldBuildReason::LOAD;
			EString							LevelStem;
			FLOAT							WaitedSeconds = 0.0f;
		};
		// Runtime state of one agent's assigned navmesh asset. LOAD/RUNTIME request these once and
		// wait for them; a profile whose asset failed falls back to a world bake.
		struct FNavigationProfileAsset
		{
			// Agent name the state belongs to (the world keys assets by agent, not by actor).
			EString					AgentName;
			EString					RequestedAssetName;
			const ENavMeshAsset*	Asset = nullptr;
			BOOL32					bRequested = FALSE;
			BOOL32					bFailed = FALSE;
		};
		struct FNavigationWorld
		{
			PWorld*								World = nullptr;
			TArray<FNavigationProviderEntry>	Providers;
			TArray<FNavigationObstacleDesc>		Obstacles;
			TArray<FCrowdBinding>				CrowdBindings;
			// Parallel to the world's agent names, not to the actors: duplicates share one entry.
			TArray<FNavigationProfileAsset>		ProfileAssets;
			// Authored bake input revision; see BumpNavigationInputRevision.
			UINT32								InputRevision = 1u;
			FNavigationWorldBuildState			Build;
			BOOL32								bHasPendingBuild = FALSE;
			FPendingWorldBuild					Pending;
		};

	private:
		PE_NODISCARD FNavigationWorld*	FindWorldState(const PWorld* InWorld);
		PE_NODISCARD const FNavigationWorld* FindWorldState(const PWorld* InWorld) const;
		PE_NODISCARD FNavigationWorld&	FindOrCreateWorldState(PWorld* InWorld);
		PE_NODISCARD INT32	FindProviderIndex(const FNavigationWorld& InWorld, const EString& InProfileName) const;
		// Index of the profile's provider covering InLocation, or of the profile's first ready one.
		// -1 when the profile has no ready provider at all.
		PE_NODISCARD INT32	FindProviderIndexForLocation(const FNavigationWorld& InWorld, const Vector3& InLocation, const EString& InProfileName) const;
		// Provider of InProfileName, but only while it is still the build recorded by InSerial.
		PE_NODISCARD ENavigationProviderInterface* GetProviderForProfile(const FNavigationWorld& InWorld, const EString& InProfileName, const UINT32& InSerial) const;
		void				InvalidateDebugLines();
		// Refills both cached debug lists from every ready provider when anything changed.
		void				RebuildDebugSegments();
		void				ReleaseAllProviders();
		void				PublishFinishedBakes();
		// Adds the description to InProvider and records the profile and serial, so removal goes
		// back to the provider that accepted it. Returns the gameplay handle, or invalid when the
		// provider cannot take obstacles.
		ENavigationObstacleHandle	AttachObstacle(FNavigationWorld& InWorld, FNavigationObstacleDesc& InOutDesc, const EString& InProfileName, const UINT32& InSerial, ENavigationProviderInterface& InProvider);
		ENavigationObstacleHandle	PublishObstacle(FNavigationWorld& InWorld, FNavigationObstacleDesc& InOutDesc);
		// Re-adds the obstacles that belonged to InProfileName, and adopts unowned ones of the
		// profile that this provider now covers.
		void						ReplayObstacles(FNavigationWorld& InWorld, const EString& InProfileName, const UINT32& InSerial, ENavigationProviderInterface& InProvider);
		PE_NODISCARD FCrowdBinding*			FindCrowdBinding(FNavigationWorld& InWorld, const ObjectIdentityType& InRequesterId);
		PE_NODISCARD const FCrowdBinding*	FindCrowdBinding(const FNavigationWorld& InWorld, const ObjectIdentityType& InRequesterId) const;
		// The provider died with the old build, so its bindings need a new agent.
		void						InvalidateCrowdBindings(FNavigationWorld& InWorld, const EString& InProfileName);

		// Refreshes the per-agent asset states from the world's navmesh actors (an asset name may
		// have changed, appeared or disappeared since the last pass).
		void							SyncProfileAssets(FNavigationWorld& InWorld);
		// Requests/resolves the assets and reports whether any request is still outstanding.
		BOOL32							TickProfileAssets(FNavigationWorld& InWorld);
		PE_NODISCARD FNavigationProfileAsset*	FindProfileAsset(FNavigationWorld& InWorld, const EString& InAgentName);

		// Queues a bake; a non zero batch id ties it to a world build pass.
		BOOL32	RequestBuildInternal(PWorld* InWorld, const ObjectIdentityType& InSourceId, const EString& InProfileName, ENavigationProviderInterface* InProvider, ENavigationSourceGeometry&& InGeometry, const UINT32& InBatchId);
		// Enumerates the world's primary authors and submits one import or bake per profile.
		BOOL32	StartWorldBuild(PWorld& InWorld, ENavigationWorldBuildReason InReason, const EString& InLevelStem);
		void	TickPendingWorldBuilds(FLOAT InDeltaTime);
		// Editor only: turns a changed input revision into one debounced world build.
		void	TickWorldBuildInputs(FLOAT InDeltaTime);
		// Accounts one finished (or discarded) batch job and completes the pass at zero.
		void	CompleteWorldBuildJob(const FBakeResult& InResult, BOOL32 bSucceeded, BOOL32 bDiscarded);
		void	FinishWorldBuild(FNavigationWorld& InWorld);
		void				BakeWorkerLoop();
		void				StopBakeWorker();

	private:
		TArray<FNavigationWorld>	Worlds;
		// Handed out to gameplay so its handle survives a provider rebuild.
		ENavigationObstacleHandle	NextObstacleId = 1;
		UINT32						NextWorldBuildBatchId = 1;

		EThread					BakeThread;
		EEvent					BakeWakeEvent;
		EMutex					BakeMutex;
		TArray<FBakeJob>		BakeJobs;
		TArray<FBakeResult>		BakeResults;
		BOOL32					bBakeWorkerStarted = FALSE;

		BOOL32								bDebugDrawEnabled = TRUE;
		ENavigationDebugModeType			DebugDrawMode = ENavigationDebugModeType::NAVIGATION_DEBUG_BOUNDARY;
		BOOL32								bDebugTrianglesEnabled = TRUE;
		TArray<Vector3>						DebugPolygonSegments;
		TArray<Vector3>						DebugTriangleSegments;
		BOOL32								bDebugLinesDirty = TRUE;
		UINT32								DebugSegmentsRevision = 1u;
	};

}  // namespace PigeonEngine
