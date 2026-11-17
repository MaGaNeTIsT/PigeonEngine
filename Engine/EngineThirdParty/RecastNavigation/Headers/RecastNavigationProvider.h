#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Navigation/NavigationProviderInterface.h>

// Detour types this header stores pointers to. They are declared at global scope because
// that is where the vendored library declares them; declaring them here rather than
// including a Detour header is what keeps the third party type out of gameplay code.
class dtCrowd;
class dtNavMesh;
class dtNavMeshQuery;

namespace PigeonEngine
{

	/*
	* Bake settings for the Recast/Detour backend.
	*
	* Every length is in engine units (centimeters), the same units the geometry arrives in,
	* so no conversion layer exists between the world and the voxel grid. Recast only cares
	* about the ratio between CellSize, CellHeight and the agent dimensions, so these are
	* resolutions rather than absolute sizes.
	*
	* Zero on a derived field means "use Recast's recommended factor", which is why the
	* defaults can stay zero while the resulting values still follow the cell size.
	*/
	struct ENavMeshBuildSettings
	{
	public:
		// Voxel footprint. Smaller cells hug walls more closely and grow memory with the
		// square of the resolution; 20 cm resolves a doorway without a huge heightfield.
		FLOAT	CellSize = 20.0f;
		FLOAT	CellHeight = 10.0f;
		// Agent footprint. Radius is eroded off every wall in whole cells, Height is the
		// clearance kept under an overhang, MaxClimb the tallest step that stays walkable.
		FLOAT	AgentRadius = 35.0f;
		FLOAT	AgentHeight = 150.0f;
		FLOAT	AgentMaxClimb = 40.0f;
		FLOAT	AgentMaxSlopeAngle = 45.0f;
		// 0 = 12 * CellSize.
		FLOAT	MaxEdgeLength = 0.0f;
		// 0 = 1.3 * CellSize.
		FLOAT	MaxSimplificationError = 0.0f;
		// 0 = 6 * CellHeight and 1 * CellHeight.
		FLOAT	DetailSampleDistance = 0.0f;
		FLOAT	DetailSampleMaxError = 0.0f;
		// Region culling, as a linear size in cells: regions smaller than MinRegionArea are
		// dropped, regions smaller than MergeRegionArea are merged into a neighbour. Zero
		// keeps everything, which is what a hand built test level wants.
		UINT32	MinRegionArea = 0u;
		UINT32	MergeRegionArea = 0u;
		// Polygon vertex limit. Detour stores at most DT_VERTS_PER_POLYGON (6) per polygon,
		// so values above that are clamped, not rejected.
		UINT32	MaxVertsPerPoly = 6u;
		/*
		* Cells along one tile edge. Zero builds a single tile: cheaper, simpler, and enough for a
		* static level. Anything else builds a tile grid together with the tile cache behind it,
		* which is what lets a dynamic obstacle rebuild only the tiles it overlaps.
		*/
		UINT32	TileSizeCells = 0u;
		// Upper bound on the layers one grid cell may produce. A room with several stacked floors
		// has more than one layer per cell.
		UINT32	MaxLayersPerTile = 16u;
		// Agents the crowd can steer at once. Zero disables the crowd, and agents then have to be
		// moved by the caller.
		UINT32	MaxCrowdAgents = 64u;
		/*
		* Reverses the winding of every source triangle before rasterization.
		*
		* Recast marks a triangle walkable when the normal of (v1 - v0) x (v2 - v0) points up.
		* The engine renders with Direct3D's left handed clockwise front faces and imports
		* through assimp with aiProcess_MakeLeftHanded plus aiProcess_FlipWindingOrder, so
		* whether an imported mesh needs the flip depends on that chain rather than on the
		* mesh. Measure it once with the demo level: a build that reports zero walkable
		* triangles needs this turned on. It is deliberately a whole mesh switch - flipping
		* only the triangles that face down would make ceilings walkable.
		*/
		BOOL32	bFlipTriangleWinding = FALSE;
	};

	/*
	* Detour backed navigation provider.
	*
	* This is the only translation unit allowed to include Recast and Detour headers. The
	* class builds a single tile navmesh from a world space triangle soup and answers queries
	* through dtNavMeshQuery, translating every result back into engine types.
	*/
	// Tile cache state, defined in the .cpp because everything in it derives from Detour
	// interfaces. The header only holds the pointer.
	struct FRecastNavigationTileCache;

	class ERecastNavigationProvider final : public ENavigationProviderInterface
	{
	public:
		// Name the provider is registered under, and the name a component asks the registry for.
		static const EName& GetProviderName();

	public:
		ERecastNavigationProvider();
		virtual ~ERecastNavigationProvider() override;

	public:
		virtual BOOL32	Build(const ENavigationSourceGeometry& InGeometry, ENavigationBuildStats& OutStats) override;
		virtual BOOL32	IsReady() const override;
		virtual const EBoundAABB& GetBounds() const override;
		virtual UINT32	GetCapabilities() const override;

		virtual BOOL32	FindNearestPoint(const Vector3& InLocation, const Vector3& InHalfExtents, const ENavigationQueryFilter& InFilter, ENavigationProjectionResult& OutResult) const override;
		virtual BOOL32	FindPath(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationPath& OutPath) const override;
		virtual BOOL32	ProjectPoint(const Vector3& InLocation, const ENavigationQueryFilter& InFilter, ENavigationProjectionResult& OutResult) const override;
		virtual BOOL32	Raycast(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationRaycastResult& OutResult) const override;
		virtual BOOL32	MoveAlongSurface(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, Vector3& OutLocation) const override;

		virtual void	CollectDebugLines(ENavigationDebugModeType InMode, TArray<Vector3>& OutLineSegments) const override;

		virtual void	Tick(FLOAT InDeltaTime) override;
		virtual UINT32	GetContentRevision() const override;

		virtual ENavigationObstacleHandle	AddCylinderObstacle(const Vector3& InCenter, const FLOAT& InRadius, const FLOAT& InHeight) override;
		virtual ENavigationObstacleHandle	AddBoxObstacle(const Vector3& InCenter, const Vector3& InHalfExtents, const FLOAT& InYawRadians) override;
		virtual void						RemoveObstacle(ENavigationObstacleHandle InHandle) override;

		virtual ENavigationCrowdAgentHandle	AddCrowdAgent(const Vector3& InPosition, const ENavigationAgentSettings& InSettings) override;
		virtual void						RemoveCrowdAgent(ENavigationCrowdAgentHandle InHandle) override;
		virtual BOOL32						RequestCrowdAgentMove(ENavigationCrowdAgentHandle InHandle, const Vector3& InTarget) override;
		virtual void						StopCrowdAgent(ENavigationCrowdAgentHandle InHandle) override;
		virtual BOOL32						GetCrowdAgentState(ENavigationCrowdAgentHandle InHandle, ENavigationCrowdAgentState& OutState) const override;

		// Container: signature, version, dtNavMeshParams, bounds, one entry per tile, then the
		// tile cache section (empty until dynamic obstacles are built). ImportData copies the
		// bytes into Detour memory, so the caller keeps ownership of InData.
		virtual BOOL32	ExportData(TArray<UINT8>& OutData) const override;
		virtual BOOL32	ImportData(const UINT8* InData, UINT32 InSize) override;

	public:
		PE_NODISCARD const ENavMeshBuildSettings&	GetSettings() const;
		void										SetSettings(const ENavMeshBuildSettings& InSettings);

	private:
		// Releases the navmesh and the query. Called before every rebuild and on destruction.
		void	ReleaseNavigationData();
		// Crowd slot for a handle, or -1. Slots shuffle when an agent leaves, so the handle is the
		// agent's user data rather than its index.
		PE_NODISCARD INT32	FindCrowdAgentIndex(ENavigationCrowdAgentHandle InHandle) const;

	private:
		ENavMeshBuildSettings	Settings;
		dtNavMesh*				NavMesh = nullptr;
		dtNavMeshQuery*			NavMeshQuery = nullptr;
		dtCrowd*				Crowd = nullptr;
		UINT32					NextCrowdAgentId = 1u;
		FRecastNavigationTileCache*	TileCache = nullptr;
		EBoundAABB				Bounds = EBoundAABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), FALSE);
		BOOL32					bReady = FALSE;
		// Bumped whenever the polygons change without a new build (a rebuilt tile).
		UINT32					ContentRevision = 0u;
		// Obstacle adds and removes since the last tick. Each one may rebuild tiles, which is what
		// invalidates cached debug lines and paths.
		UINT32					PendingObstacleChanges = 0u;
	};

}  // namespace PigeonEngine
