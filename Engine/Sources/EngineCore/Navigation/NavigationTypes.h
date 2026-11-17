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
	struct ENavigationBuildStats
	{
	public:
		UINT32	TriangleCount		= 0;
		UINT32	WalkableTriangleCount = 0;
		UINT32	PolygonCount		= 0;
		UINT32	TileCount			= 0;
		// Static mesh components inside the bake bounds whose cold data was not ready, or
		// whose asset produced no triangles. Non zero means the mesh is incomplete.
		UINT32	SkippedMeshCount	= 0;
		FLOAT	Milliseconds		= 0.0f;
	};

}  // namespace PigeonEngine
