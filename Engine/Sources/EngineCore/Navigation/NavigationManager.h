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

	class ENavigationProviderInterface;

	/*
	* Runtime owner of every navigation provider in the process.
	*
	* A provider is registered by the component that built it (one provider per component,
	* keyed by the component's unique id), and gameplay asks this manager instead of holding a
	* provider pointer. That keeps the queries backend agnostic: the agent component calls
	* FindPath and never learns whether the answer came from Detour or from a future grid
	* backend.
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
		// Publishes a provider immediately. Used by the asset import path, which is a memory copy,
		// and by any provider that does not need a bake.
		void	RegisterProvider(const ObjectIdentityType& InOwnerId, ENavigationProviderInterface* InProvider);
		// Queues a bake of InProvider with InGeometry on the worker. Ownership of the provider
		// transfers when this returns TRUE; on FALSE the caller still owns it. A second request
		// while one is running is refused, so a caller spamming rebuild is harmless.
		BOOL32	RequestBuild(const ObjectIdentityType& InOwnerId, ENavigationProviderInterface* InProvider, ENavigationSourceGeometry&& InGeometry);
		void	UnregisterProvider(const ObjectIdentityType& InOwnerId);
		// Per provider overlay switch. A level can hold several navigation volumes and only
		// the one under construction needs to be drawn.
		void	SetProviderDebugDrawEnabled(const ObjectIdentityType& InOwnerId, const BOOL32& InEnabled);

		PE_NODISCARD ENavigationProviderInterface* GetProvider(const ObjectIdentityType& InOwnerId) const;
		// First provider that finished a build. Null while nothing has been baked yet.
		PE_NODISCARD ENavigationProviderInterface* GetDefaultProvider() const;
		// Provider whose bounds contain InLocation, or the default one when none does.
		PE_NODISCARD ENavigationProviderInterface* FindProviderForLocation(const Vector3& InLocation) const;
		PE_NODISCARD UINT32 GetProviderNum() const;
		// Build state of one volume, for the component that requested it.
		PE_NODISCARD ENavigationBuildStateType		GetBuildState(const ObjectIdentityType& InOwnerId) const;
		PE_NODISCARD const ENavigationBuildStats&	GetBuildStats(const ObjectIdentityType& InOwnerId) const;

		/*
		* Runtime obstacles. The description is retained, so a provider that is rebuilt or reloaded
		* gets its obstacles back on its own: gameplay adds an obstacle once and then only moves it.
		* The volume is the one whose bounds contain the obstacle.
		*
		* A cylinder sits with its base at InCenter; a box is centred on InCenter. Detour has no
		* "move", so a moving obstacle is removed and re-added, which is what the obstacle component
		* does when its threshold is crossed.
		*/
		ENavigationObstacleHandle	AddCylinderObstacle(const ObjectIdentityType& InRequesterId, const Vector3& InCenter, const FLOAT& InRadius, const FLOAT& InHeight);
		ENavigationObstacleHandle	AddBoxObstacle(const ObjectIdentityType& InRequesterId, const Vector3& InCenter, const Vector3& InHalfExtents, const FLOAT& InYawRadians);
		void						RemoveObstacle(const ObjectIdentityType& InRequesterId, ENavigationObstacleHandle InHandle);

		/*
		* Local avoidance, keyed by the component that owns the agent. RegisterCrowdAgent is
		* idempotent and is meant to be called every frame with the agent's current position: it
		* (re)creates the backend agent when there is none, which is how an agent survives a provider
		* rebuild. The requester drives its own transform from GetCrowdAgentState; the manager never
		* writes to game objects.
		*/
		BOOL32	RegisterCrowdAgent(const ObjectIdentityType& InRequesterId, const Vector3& InPosition, const ENavigationAgentSettings& InSettings);
		BOOL32	RequestCrowdAgentMove(const ObjectIdentityType& InRequesterId, const Vector3& InTarget);
		void	StopCrowdAgent(const ObjectIdentityType& InRequesterId);
		void	UnregisterCrowdAgent(const ObjectIdentityType& InRequesterId);
		PE_NODISCARD BOOL32	GetCrowdAgentState(const ObjectIdentityType& InRequesterId, ENavigationCrowdAgentState& OutState) const;

	public:
		// Shortcuts used by gameplay: they resolve a provider and answer with engine types.
		// Both return FALSE when no provider is ready; OutPath keeps its previous contents.
		BOOL32	FindPath(const Vector3& InStart, const Vector3& InEnd, ENavigationPath& OutPath) const;
		BOOL32	FindPath(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationPath& OutPath) const;
		BOOL32	ProjectPoint(const Vector3& InLocation, Vector3& OutLocation) const;

	public:
		void						SetDebugDrawEnabled(const BOOL32& InEnabled);
		PE_NODISCARD BOOL32			IsDebugDrawEnabled() const;
		void						SetDebugDrawMode(ENavigationDebugModeType InMode);
		PE_NODISCARD ENavigationDebugModeType	GetDebugDrawMode() const;
		/*
		* Cached line segments, two entries per segment. The list is rebuilt on the first
		* request after a provider changed or the mode changed: walking every polygon each
		* frame would cost more than drawing them.
		*/
		const TArray<Vector3>&		GetDebugLineSegments();

	private:
		struct FNavigationProviderEntry
		{
			ObjectIdentityType				OwnerId = 0;
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
			ObjectIdentityType				OwnerId = 0;
			UINT32							Serial = 0;
			ENavigationProviderInterface*	Provider = nullptr;
			ENavigationSourceGeometry		Geometry;
		};
		/*
		* What gameplay asked for, kept so a rebuilt provider can be restored.
		*
		* Id is what gameplay holds and never changes. ProviderHandle belongs to whichever provider
		* currently holds the obstacle, and is reassigned when a rebuild replaces that provider; that
		* is what keeps a caller's handle valid across a rebuild.
		*/
		struct FNavigationObstacleDesc
		{
			ENavigationObstacleHandle	Id = NavigationInvalidObstacleHandle;
			ObjectIdentityType			RequesterId = 0;
			BOOL32						bBox = FALSE;
			Vector3						Center = Vector3::Zero();
			Vector3						HalfExtents = Vector3::Zero();
			FLOAT						Radius = 0.0f;
			FLOAT						Height = 0.0f;
			FLOAT						YawRadians = 0.0f;
			ENavigationObstacleHandle	ProviderHandle = NavigationInvalidObstacleHandle;
		};
		// One agent registered for local avoidance, kept so a rebuilt provider gets it back.
		struct FCrowdBinding
		{
			ObjectIdentityType			RequesterId = 0;
			ENavigationCrowdAgentHandle	ProviderHandle = NavigationInvalidCrowdAgentHandle;
			ENavigationAgentSettings	Settings;
			Vector3						Target = Vector3::Zero();
			BOOL32						bHasTarget = FALSE;
			Vector3						LastPosition = Vector3::Zero();
		};
		struct FBakeResult
		{
			ObjectIdentityType				OwnerId = 0;
			UINT32							Serial = 0;
			ENavigationProviderInterface*	Provider = nullptr;
			BOOL32							bSucceeded = FALSE;
			ENavigationBuildStats			Stats;
		};

	private:
		PE_NODISCARD INT32	FindProviderIndex(const ObjectIdentityType& InOwnerId) const;
		void				InvalidateDebugLines();
		void				ReleaseAllProviders();
		void				PublishFinishedBakes();
		ENavigationObstacleHandle	PublishObstacle(FNavigationObstacleDesc& InOutDesc);
		void						ReplayObstacles();
		PE_NODISCARD FCrowdBinding*			FindCrowdBinding(const ObjectIdentityType& InRequesterId);
		PE_NODISCARD const FCrowdBinding*	FindCrowdBinding(const ObjectIdentityType& InRequesterId) const;
		// The crowd died with the provider that owned it, so every binding needs a new agent.
		void						InvalidateCrowdBindings();
		void				BakeWorkerLoop();
		void				StopBakeWorker();

	private:
		TArray<FNavigationProviderEntry>	Providers;
		TArray<FNavigationObstacleDesc>		Obstacles;
		TArray<FCrowdBinding>				CrowdBindings;
		// Handed out to gameplay so its handle survives a provider rebuild.
		ENavigationObstacleHandle			NextObstacleId = 1;

		EThread					BakeThread;
		EEvent					BakeWakeEvent;
		EMutex					BakeMutex;
		TArray<FBakeJob>		BakeJobs;
		TArray<FBakeResult>		BakeResults;
		BOOL32					bBakeWorkerStarted = FALSE;

		BOOL32								bDebugDrawEnabled = TRUE;
		ENavigationDebugModeType			DebugDrawMode = ENavigationDebugModeType::NAVIGATION_DEBUG_BOUNDARY;
		TArray<Vector3>						DebugLineSegments;
		BOOL32								bDebugLinesDirty = TRUE;
	};

}  // namespace PigeonEngine
