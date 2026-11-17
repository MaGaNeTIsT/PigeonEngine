#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	/*
	* Navigation data types shared by every navigation backend.
	*
	* Nothing in this header mentions Recast or Detour. Gameplay code, the runtime manager
	* and the editor all speak these types only, so adding a second backend (a waypoint
	* graph, a grid) is additive: it implements ENavigationProviderInterface and exposes the
	* same path and projection results.
	*/

	// What a polygon is, as far as a query is concerned. The filter includes or excludes
	// whole classes, so gameplay asks for "ground only" or "no water" without knowing how a
	// backend stores area identifiers.
	//
	// Today the bake stamps every walkable polygon with GROUND, so only GROUND is reachable and
	// the other bits exist for a future area authoring pass. A filter that excludes GROUND
	// therefore reaches nothing.
	enum ENavigationAreaFlagType : UINT16
	{
		NAVIGATION_AREA_GROUND	= (1 << 0),
		NAVIGATION_AREA_WATER	= (1 << 1),
		NAVIGATION_AREA_DOOR	= (1 << 2),
		NAVIGATION_AREA_JUMP	= (1 << 3),
		NAVIGATION_AREA_ALL		= 0xFFFF
	};

	enum ENavigationPathStatusType : UINT8
	{
		// The query failed, nothing usable in the result.
		NAVIGATION_PATH_INVALID	= 0,
		// Every requested point was reached.
		NAVIGATION_PATH_COMPLETE,
		// The destination is not reachable; the path ends at the closest reachable point.
		NAVIGATION_PATH_PARTIAL
	};

	enum ENavigationPathPointType : UINT8
	{
		NAVIGATION_PATH_POINT_WALK = 0,
		// Crossing an authored link (jump, ladder). Off-mesh connections are not built yet;
		// the value exists so the agent and the debug drawing do not have to change when
		// they are.
		NAVIGATION_PATH_POINT_LINK
	};

	// State of one volume's build, as the manager reports it to the component that asked for it.
	enum ENavigationBuildStateType : UINT8
	{
		// Nothing requested yet, or the last request was consumed.
		NAVIGATION_BUILD_IDLE = 0,
		// A bake is running on the worker; the previous provider, if any, still answers queries.
		NAVIGATION_BUILD_RUNNING,
		// The provider was published and is answering queries.
		NAVIGATION_BUILD_READY,
		// The bake produced nothing; the previous provider, if any, still answers queries.
		NAVIGATION_BUILD_FAILED
	};

	// What a backend can do beyond building and querying. Gameplay and the editor check these
	// before offering an operation, so a minimal backend does not have to fake the rest.
	enum ENavigationCapabilityType : UINT32
	{
		// ExportData / ImportData can round trip the baked mesh.
		NAVIGATION_CAPABILITY_PERSISTENCE	= (1 << 0),
		// Runtime obstacles can be added and removed.
		NAVIGATION_CAPABILITY_OBSTACLES		= (1 << 1),
		// Agents can be registered for local steering and avoidance.
		NAVIGATION_CAPABILITY_CROWD			= (1 << 2),
		// The mesh is built from tiles, so a local change only rebuilds the affected ones.
		NAVIGATION_CAPABILITY_TILES			= (1 << 3)
	};

	// Line segments are emitted two per pair, (0,1), (2,3), ... so the debug drawing code
	// stays a plain loop over consecutive entries.
	enum ENavigationDebugModeType : UINT8
	{
		// Outer boundary only: far fewer segments, and the shape is still readable.
		NAVIGATION_DEBUG_BOUNDARY = 0,
		// Every polygon edge, including the internal ones.
		NAVIGATION_DEBUG_FULL
	};

	struct ENavigationPath
	{
	public:
		TArray<Vector3>					Points;
		TArray<ENavigationPathPointType>	PointTypes;
		ENavigationPathStatusType		Status = ENavigationPathStatusType::NAVIGATION_PATH_INVALID;
	public:
		PE_NODISCARD BOOL32 IsValid() const;
		PE_NODISCARD UINT32 Num() const;
		// Clears the points and drops the status back to invalid.
		void Reset();
	};

	// Filters are copied per query, so a caller can build one on the stack. AreaCosts is
	// indexed by area id as the backend numbers them; an empty array means "all costs 1".
	struct ENavigationQueryFilter
	{
	public:
		UINT16			IncludeAreaFlags = ENavigationAreaFlagType::NAVIGATION_AREA_ALL;
		UINT16			ExcludeAreaFlags = 0;
		TArray<FLOAT>	AreaCosts;
	};

	struct ENavigationProjectionResult
	{
	public:
		Vector3	Point = Vector3::Zero();
		BOOL32	bFound = FALSE;
		// TRUE when the input point was already inside a polygon, FALSE when it was
		// projected onto the nearest edge. Callers use it to tell "standing on the mesh"
		// from "snapped onto the mesh".
		BOOL32	bOverPolygon = FALSE;
	};

	struct ENavigationRaycastResult
	{
	public:
		Vector3	HitPoint = Vector3::Zero();
		Vector3	HitNormal = Vector3::Zero();
		FLOAT	Distance = 0.0f;
		BOOL32	bHit = FALSE;
	};

	// Reported by a backend after a build. The triangle and polygon counts are what a
	// caller checks first: zero walkable triangles while TriangleCount is not zero means the
	// geometry was fed with the opposite winding (see the winding note in the collector).
	/*
	* Movement settings for one navmesh agent. All lengths are in engine units (centimeters)
	* and speeds in centimeters per second.
	*
	* Radius and Height describe the agent to the local avoidance solver, so they only matter in
	* crowd mode. A directly moved agent is projected onto the mesh with the bake settings'
	* AgentRadius/AgentHeight instead, because the mesh was already eroded for those.
	*/
	struct ENavigationAgentSettings
	{
	public:
		FLOAT	Radius = 35.0f;
		FLOAT	Height = 150.0f;
		FLOAT	MoveSpeed = 300.0f;
		FLOAT	TurnSpeedDegreesPerSecond = 240.0f;
		// Distance at which a corner counts as reached. Too small and the agent jitters around
		// a corner it can never exactly hit; too large and it cuts the path short.
		FLOAT	AcceptanceRadius = 20.0f;
		// Minimum time between two path queries. A target that moves every frame would
		// otherwise run a graph search every frame per agent.
		FLOAT	RepathInterval = 0.5f;

		// Crowd only: how hard the agent accelerates towards its desired velocity, and how far it
		// looks for neighbours and obstacles to avoid.
		FLOAT	MaxAcceleration = 600.0f;
		FLOAT	CollisionQueryRange = 360.0f;
		FLOAT	PathOptimizationRange = 900.0f;
		// How strongly the agent pushes away from its neighbours. Zero ignores them.
		FLOAT	SeparationWeight = 2.0f;
		// Index into the backend's avoidance quality presets.
		UINT8	ObstacleAvoidanceType = 3u;
	};

	// Runtime obstacle handle. Zero means "not created"; the value is opaque to gameplay and only
	// the backend that created it may interpret it.
	typedef UINT64	ENavigationObstacleHandle;
	PE_CONSTEXPR_STATIC ENavigationObstacleHandle NavigationInvalidObstacleHandle = 0u;

	// Handle for one agent registered with a backend's local avoidance. Zero means "not registered".
	typedef UINT64	ENavigationCrowdAgentHandle;
	PE_CONSTEXPR_STATIC ENavigationCrowdAgentHandle NavigationInvalidCrowdAgentHandle = 0u;

	// What a crowd driven agent reports back each frame.
	struct ENavigationCrowdAgentState
	{
	public:
		Vector3	Position = Vector3::Zero();
		Vector3	Velocity = Vector3::Zero();
		BOOL32	bMoving = FALSE;
		// The crowd consumed the requested target: either reached it, or gave up on it.
		BOOL32	bTargetReached = FALSE;
		// The path ended early, so the agent stopped short of the destination.
		BOOL32	bPartial = FALSE;
	};

	struct ENavigationBuildStats
	{
	public:
		UINT32	TriangleCount		= 0;
		UINT32	WalkableTriangleCount = 0;
		UINT32	PolygonCount		= 0;
		UINT32	TileCount			= 0;
		// Heightfield layers stored for the tile cache; zero when the volume was baked as a single
		// tile without dynamic obstacle support.
		UINT32	LayerCount			= 0;
		// Static mesh components inside the bake bounds whose cold data was not ready, or
		// whose asset produced no triangles. Non zero means the mesh is incomplete.
		UINT32	SkippedMeshCount	= 0;
		FLOAT	Milliseconds		= 0.0f;
	};

}  // namespace PigeonEngine
