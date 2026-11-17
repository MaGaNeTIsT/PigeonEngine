#include <RecastNavigation/Headers/RecastNavigationProvider.h>

#include <Editor/EditorLogManager.h>
#include <Development/Profiler.h>
#include <Navigation/NavigationGeometry.h>
#include <Navigation/NavigationProviderRegistry.h>

#include <Recast/Include/Recast.h>
#include <Detour/Include/DetourNavMesh.h>
#include <Detour/Include/DetourNavMeshBuilder.h>
#include <Detour/Include/DetourNavMeshQuery.h>
#include <Detour/Include/DetourStatus.h>

namespace PigeonEngine
{

	// Sanity limit on the voxel grid. A cell size typed one digit too small can ask for a
	// heightfield that would exhaust memory before any diagnostic is printed, so the build
	// refuses it instead of trying.
	PE_CONSTEXPR_STATIC UINT32 NavigationMaxHeightfieldCells = 4u * 1024u * 1024u;
	PE_CONSTEXPR_STATIC INT32 NavigationMaxPathPolygons = 256;
	PE_CONSTEXPR_STATIC INT32 NavigationMaxPathPoints = 256;
	PE_CONSTEXPR_STATIC INT32 NavigationMaxVisitedPolygons = 32;
	// Area id every walkable polygon gets. Zero is RC_NULL_AREA in Recast and a legal area id
	// in Detour; the reference sample assigns ground area 0 as well, and the query filter
	// selects polygons by flags, not by area.
	PE_CONSTEXPR_STATIC UINT8 NavigationGroundAreaId = 0u;

	// Routes Recast's own diagnostics into the engine log. Progress lines are file only: the
	// bake reports one summary line, and a per step transcript in the editor window would
	// bury it.
	class ERecastBuildLogContext final : public rcContext
	{
	public:
		ERecastBuildLogContext() : rcContext(true) {}
		virtual ~ERecastBuildLogContext() override {}
	protected:
		virtual void doLog(const rcLogCategory InCategory, const char* InMessage, const int InLength) override
		{
			rcIgnoreUnused(InLength);
			if (!InMessage)
			{
				return;
			}
			if (InCategory == RC_LOG_ERROR)
			{
				PE_LOG_ERROR(EString(PE_TEXT("Navigation: Recast error: ")) + EString(InMessage));
			}
			else if (InCategory == RC_LOG_WARNING)
			{
				PE_LOG_WARN(EString(PE_TEXT("Navigation: Recast warning: ")) + EString(InMessage));
			}
			else
			{
				PE_LOG_FILE_ONLY(EString(PE_TEXT("Navigation: Recast: ")) + EString(InMessage));
			}
		}
	};

	// Owns every intermediate structure of one bake. Recast hands out raw allocations and
	// every step can fail, so the releases live in a destructor rather than in a chain of
	// early returns that would each have to remember them.
	struct FRecastBuildStructures
	{
		rcHeightfield*			Heightfield = nullptr;
		rcCompactHeightfield*	CompactHeightfield = nullptr;
		rcContourSet*			ContourSet = nullptr;
		rcPolyMesh*				PolyMesh = nullptr;
		rcPolyMeshDetail*		DetailMesh = nullptr;

		~FRecastBuildStructures()
		{
			if (DetailMesh)
			{
				rcFreePolyMeshDetail(DetailMesh);
			}
			if (PolyMesh)
			{
				rcFreePolyMesh(PolyMesh);
			}
			if (ContourSet)
			{
				rcFreeContourSet(ContourSet);
			}
			if (CompactHeightfield)
			{
				rcFreeCompactHeightfield(CompactHeightfield);
			}
			if (Heightfield)
			{
				rcFreeHeightField(Heightfield);
			}
		}
	};

	static void ApplyQueryFilter(const ENavigationQueryFilter& InFilter, dtQueryFilter& OutFilter)
	{
		OutFilter.setIncludeFlags(InFilter.IncludeAreaFlags);
		OutFilter.setExcludeFlags(InFilter.ExcludeAreaFlags);
		const UINT32 CostNum = InFilter.AreaCosts.Num<UINT32>();
		const UINT32 CostLimit = EMath::Min<UINT32>(CostNum, static_cast<UINT32>(DT_MAX_AREAS));
		for (UINT32 Index = 0u; Index < CostLimit; Index++)
		{
			OutFilter.setAreaCost(static_cast<int>(Index), InFilter.AreaCosts[Index]);
		}
	}

	// The search box a projection uses when the caller does not supply one. Two agent radii
	// across and one agent height up is the smallest box that reliably contains the polygon
	// an agent is standing on.
	static void ComputeSearchHalfExtents(const ENavMeshBuildSettings& InSettings, float* OutHalfExtents)
	{
		OutHalfExtents[0] = InSettings.AgentRadius * 2.0f;
		OutHalfExtents[1] = InSettings.AgentHeight;
		OutHalfExtents[2] = InSettings.AgentRadius * 2.0f;
	}

	static void RegisterNavigationProvider()
	{
		ENavigationProviderRegistry::Register(ERecastNavigationProvider::GetProviderName(),
			[]() -> ENavigationProviderInterface* { return new ERecastNavigationProvider(); });
	}

	PE_REGISTER_CLASS_TYPE(&RegisterNavigationProvider);

	const EName& ERecastNavigationProvider::GetProviderName()
	{
		static const EName ProviderName(PE_TEXT("RecastNavMesh"));
		return ProviderName;
	}

	ERecastNavigationProvider::ERecastNavigationProvider()
	{
	}

	ERecastNavigationProvider::~ERecastNavigationProvider()
	{
		ReleaseNavigationData();
	}

	const ENavMeshBuildSettings& ERecastNavigationProvider::GetSettings() const
	{
		return Settings;
	}

	void ERecastNavigationProvider::SetSettings(const ENavMeshBuildSettings& InSettings)
	{
		Settings = InSettings;
	}

	BOOL32 ERecastNavigationProvider::IsReady() const
	{
		return bReady;
	}

	const EBoundAABB& ERecastNavigationProvider::GetBounds() const
	{
		return Bounds;
	}

	BOOL32 ERecastNavigationProvider::Build(const ENavigationSourceGeometry& InGeometry, ENavigationBuildStats& OutStats)
	{
		PE_PROFILE_SCOPE(PROFILER_SCOPE_NAVIGATION);

		OutStats = ENavigationBuildStats();
		OutStats.SkippedMeshCount = InGeometry.SkippedMeshCount;
		OutStats.TriangleCount = InGeometry.Triangles.Num<UINT32>();

		const DOUBLE StartSeconds = EBaseTimer::GetClockSeconds();

		if (OutStats.TriangleCount == 0u)
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: build skipped, no source triangles.")));
			return FALSE;
		}

		// A triangle soup is expanded to one vertex per triangle corner. Recast does not need
		// shared vertices, and building them here keeps the collector free of any index
		// remapping.
		TArray<FLOAT> Vertices;
		TArray<INT32> Triangles;
		Vertices.SetNum(OutStats.TriangleCount * 9u);
		Triangles.SetNum(OutStats.TriangleCount * 3u);
		for (UINT32 Index = 0u; Index < OutStats.TriangleCount; Index++)
		{
			const ENavigationSourceTriangle& Triangle = InGeometry.Triangles[Index];
			const UINT32 VertexBase = Index * 9u;
			Vertices[VertexBase + 0u] = Triangle.Vertex0.x;
			Vertices[VertexBase + 1u] = Triangle.Vertex0.y;
			Vertices[VertexBase + 2u] = Triangle.Vertex0.z;
			Vertices[VertexBase + 3u] = Triangle.Vertex1.x;
			Vertices[VertexBase + 4u] = Triangle.Vertex1.y;
			Vertices[VertexBase + 5u] = Triangle.Vertex1.z;
			Vertices[VertexBase + 6u] = Triangle.Vertex2.x;
			Vertices[VertexBase + 7u] = Triangle.Vertex2.y;
			Vertices[VertexBase + 8u] = Triangle.Vertex2.z;

			const UINT32 IndexBase = Index * 3u;
			Triangles[IndexBase + 0u] = static_cast<INT32>(IndexBase);
			Triangles[IndexBase + 1u] = static_cast<INT32>(IndexBase + 1u);
			Triangles[IndexBase + 2u] = static_cast<INT32>(IndexBase + 2u);
		}

		const INT32 VertexNum = static_cast<INT32>(OutStats.TriangleCount * 3u);
		const INT32 TriangleNum = static_cast<INT32>(OutStats.TriangleCount);

		rcConfig Config;
		memset(&Config, 0, sizeof(Config));
		Config.cs = Settings.CellSize;
		Config.ch = Settings.CellHeight;

		if (InGeometry.Bounds.IsValid
			&& (InGeometry.Bounds.AABBMax.x > InGeometry.Bounds.AABBMin.x)
			&& (InGeometry.Bounds.AABBMax.y > InGeometry.Bounds.AABBMin.y)
			&& (InGeometry.Bounds.AABBMax.z > InGeometry.Bounds.AABBMin.z))
		{
			// The caller pinned the volume, so the voxel grid covers exactly that box and the
			// same scene always produces the same cell layout.
			rcVcopy(Config.bmin, &InGeometry.Bounds.AABBMin.x);
			rcVcopy(Config.bmax, &InGeometry.Bounds.AABBMax.x);
		}
		else
		{
			rcCalcBounds(Vertices.GetData(), VertexNum, Config.bmin, Config.bmax);
		}

		rcCalcGridSize(Config.bmin, Config.bmax, Config.cs, &Config.width, &Config.height);
		if ((Config.width <= 0) || (Config.height <= 0)
			|| (static_cast<UINT64>(Config.width) * static_cast<UINT64>(Config.height) > NavigationMaxHeightfieldCells))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, cell size ")) + EString::FromFloat(Settings.CellSize)
				+ EString(PE_TEXT(" cm asks for a ")) + EString::FromInt(static_cast<UINT32>(Config.width))
				+ EString(PE_TEXT("x")) + EString::FromInt(static_cast<UINT32>(Config.height))
				+ EString(PE_TEXT(" cell grid.")));
			return FALSE;
		}

		// Every length below is in centimeters, except the fields Recast defines in voxels:
		// walkableHeight, walkableClimb, walkableRadius and maxEdgeLen.
		Config.borderSize = 0;
		Config.tileSize = 0;
		Config.walkableSlopeAngle = Settings.AgentMaxSlopeAngle;
		Config.walkableHeight = static_cast<int>(ceilf(Settings.AgentHeight / Config.ch));
		Config.walkableClimb = static_cast<int>(floorf(Settings.AgentMaxClimb / Config.ch));
		Config.walkableRadius = static_cast<int>(ceilf(Settings.AgentRadius / Config.cs));
		Config.maxEdgeLen = static_cast<int>((Settings.MaxEdgeLength > 0.0f) ? (Settings.MaxEdgeLength / Config.cs) : 12.0f);
		Config.maxSimplificationError = (Settings.MaxSimplificationError > 0.0f) ? Settings.MaxSimplificationError : (1.3f * Config.cs);
		Config.minRegionArea = static_cast<int>(rcSqr(Settings.MinRegionArea));
		Config.mergeRegionArea = static_cast<int>(rcSqr(Settings.MergeRegionArea));
		Config.maxVertsPerPoly = static_cast<int>(EMath::Clamp<UINT32>(Settings.MaxVertsPerPoly, 3u, static_cast<UINT32>(DT_VERTS_PER_POLYGON)));
		Config.detailSampleDist = (Settings.DetailSampleDistance > 0.0f) ? Settings.DetailSampleDistance : (6.0f * Config.ch);
		Config.detailSampleMaxError = (Settings.DetailSampleMaxError > 0.0f) ? Settings.DetailSampleMaxError : Config.ch;

		ERecastBuildLogContext Context;
		FRecastBuildStructures Build;

		Build.Heightfield = rcAllocHeightfield();
		if (!Build.Heightfield)
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, out of memory for the heightfield.")));
			return FALSE;
		}
		if (!rcCreateHeightfield(&Context, *Build.Heightfield, Config.width, Config.height, Config.bmin, Config.bmax, Config.cs, Config.ch))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not create the heightfield.")));
			return FALSE;
		}

		TArray<UINT8> TriangleAreas;
		TriangleAreas.SetNum(OutStats.TriangleCount);
		memset(TriangleAreas.GetData(), RC_NULL_AREA, OutStats.TriangleCount);
		rcMarkWalkableTriangles(&Context, Config.walkableSlopeAngle, Vertices.GetData(), VertexNum, Triangles.GetData(), TriangleNum, TriangleAreas.GetData());

		for (UINT32 Index = 0u; Index < OutStats.TriangleCount; Index++)
		{
			if (TriangleAreas[Index] != RC_NULL_AREA)
			{
				OutStats.WalkableTriangleCount++;
			}
		}

		if (!rcRasterizeTriangles(&Context, Vertices.GetData(), VertexNum, Triangles.GetData(), TriangleAreas.GetData(), TriangleNum, *Build.Heightfield, Config.walkableClimb))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not rasterize the triangles.")));
			return FALSE;
		}
		TriangleAreas.Empty(0u);

		rcFilterLowHangingWalkableObstacles(&Context, Config.walkableClimb, *Build.Heightfield);
		rcFilterLedgeSpans(&Context, Config.walkableHeight, Config.walkableClimb, *Build.Heightfield);
		rcFilterWalkableLowHeightSpans(&Context, Config.walkableHeight, *Build.Heightfield);

		Build.CompactHeightfield = rcAllocCompactHeightfield();
		if (!Build.CompactHeightfield)
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, out of memory for the compact heightfield.")));
			return FALSE;
		}
		if (!rcBuildCompactHeightfield(&Context, Config.walkableHeight, Config.walkableClimb, *Build.Heightfield, *Build.CompactHeightfield))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not build the compact heightfield.")));
			return FALSE;
		}
		if (!rcErodeWalkableArea(&Context, Config.walkableRadius, *Build.CompactHeightfield))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not erode the walkable area.")));
			return FALSE;
		}
		if (!rcBuildDistanceField(&Context, *Build.CompactHeightfield))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not build the distance field.")));
			return FALSE;
		}
		if (!rcBuildRegions(&Context, *Build.CompactHeightfield, 0, Config.minRegionArea, Config.mergeRegionArea))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not build the regions.")));
			return FALSE;
		}

		Build.ContourSet = rcAllocContourSet();
		if (!Build.ContourSet)
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, out of memory for the contour set.")));
			return FALSE;
		}
		if (!rcBuildContours(&Context, *Build.CompactHeightfield, Config.maxSimplificationError, Config.maxEdgeLen, *Build.ContourSet, RC_CONTOUR_TESS_WALL_EDGES))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not build the contours.")));
			return FALSE;
		}

		Build.PolyMesh = rcAllocPolyMesh();
		if (!Build.PolyMesh)
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, out of memory for the polygon mesh.")));
			return FALSE;
		}
		if (!rcBuildPolyMesh(&Context, *Build.ContourSet, Config.maxVertsPerPoly, *Build.PolyMesh))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not build the polygon mesh.")));
			return FALSE;
		}

		Build.DetailMesh = rcAllocPolyMeshDetail();
		if (!Build.DetailMesh)
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, out of memory for the detail mesh.")));
			return FALSE;
		}
		if (!rcBuildPolyMeshDetail(&Context, *Build.PolyMesh, *Build.CompactHeightfield, Config.detailSampleDist, Config.detailSampleMaxError, *Build.DetailMesh))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not build the detail mesh.")));
			return FALSE;
		}

		// Recast marks walkable polygons with RC_WALKABLE_AREA; Detour stores an area id and a
		// separate flag word, and the query filter selects on the flags.
		for (int PolygonIndex = 0; PolygonIndex < Build.PolyMesh->npolys; PolygonIndex++)
		{
			if (Build.PolyMesh->areas[PolygonIndex] == RC_WALKABLE_AREA)
			{
				Build.PolyMesh->areas[PolygonIndex] = NavigationGroundAreaId;
				Build.PolyMesh->flags[PolygonIndex] = ENavigationAreaFlagType::NAVIGATION_AREA_GROUND;
			}
			else
			{
				Build.PolyMesh->flags[PolygonIndex] = 0;
			}
		}

		dtNavMeshCreateParams Params;
		memset(&Params, 0, sizeof(Params));
		Params.verts = Build.PolyMesh->verts;
		Params.vertCount = Build.PolyMesh->nverts;
		Params.polys = Build.PolyMesh->polys;
		Params.polyAreas = Build.PolyMesh->areas;
		Params.polyFlags = Build.PolyMesh->flags;
		Params.polyCount = Build.PolyMesh->npolys;
		Params.nvp = Build.PolyMesh->nvp;
		Params.detailMeshes = Build.DetailMesh->meshes;
		Params.detailVerts = Build.DetailMesh->verts;
		Params.detailVertsCount = Build.DetailMesh->nverts;
		Params.detailTris = Build.DetailMesh->tris;
		Params.detailTriCount = Build.DetailMesh->ntris;
		// Off-mesh connections (jump, ladder) are not authored yet. The arrays stay null and
		// empty, which dtCreateNavMeshData accepts.
		Params.offMeshConVerts = nullptr;
		Params.offMeshConRad = nullptr;
		Params.offMeshConDir = nullptr;
		Params.offMeshConAreas = nullptr;
		Params.offMeshConFlags = nullptr;
		Params.offMeshConUserID = nullptr;
		Params.offMeshConCount = 0;
		Params.walkableHeight = Settings.AgentHeight;
		Params.walkableRadius = Settings.AgentRadius;
		Params.walkableClimb = Settings.AgentMaxClimb;
		Params.cs = Config.cs;
		Params.ch = Config.ch;
		Params.buildBvTree = true;
		rcVcopy(Params.bmin, Build.PolyMesh->bmin);
		rcVcopy(Params.bmax, Build.PolyMesh->bmax);

		unsigned char* NavData = nullptr;
		int NavDataSize = 0;
		if (!dtCreateNavMeshData(&Params, &NavData, &NavDataSize))
		{
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not create the Detour navmesh data (no walkable polygons?).")));
			return FALSE;
		}

		// From here on the new data replaces the old one, so a failed allocation leaves the
		// previous navmesh usable instead of leaving nothing behind.
		dtNavMesh* NewNavMesh = dtAllocNavMesh();
		if (!NewNavMesh)
		{
			dtFree(NavData);
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, out of memory for the Detour navmesh.")));
			return FALSE;
		}
		dtStatus Status = NewNavMesh->init(NavData, NavDataSize, DT_TILE_FREE_DATA);
		if (dtStatusFailed(Status))
		{
			dtFree(NavData);
			dtFreeNavMesh(NewNavMesh);
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not initialize the Detour navmesh.")));
			return FALSE;
		}

		dtNavMeshQuery* NewNavMeshQuery = dtAllocNavMeshQuery();
		if (!NewNavMeshQuery)
		{
			dtFreeNavMesh(NewNavMesh);
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, out of memory for the Detour navmesh query.")));
			return FALSE;
		}
		Status = NewNavMeshQuery->init(NewNavMesh, NavigationMaxPathPolygons * 8);
		if (dtStatusFailed(Status))
		{
			dtFreeNavMeshQuery(NewNavMeshQuery);
			dtFreeNavMesh(NewNavMesh);
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: build failed, could not initialize the Detour navmesh query.")));
			return FALSE;
		}

		ReleaseNavigationData();
		NavMesh = NewNavMesh;
		NavMeshQuery = NewNavMeshQuery;
		OutStats.PolygonCount = static_cast<UINT32>(Build.PolyMesh->npolys);
		OutStats.TileCount = 1u;
		Bounds = EBoundAABB(Vector3(Config.bmin[0], Config.bmin[1], Config.bmin[2]), Vector3(Config.bmax[0], Config.bmax[1], Config.bmax[2]), TRUE);
		bReady = TRUE;
		OutStats.Milliseconds = static_cast<FLOAT>((EBaseTimer::GetClockSeconds() - StartSeconds) * 1000.0);

		PE_LOG_LOG(EString(PE_TEXT("Navigation: built "))
			+ EString::FromInt(OutStats.TriangleCount) + EString(PE_TEXT(" triangles ("))
			+ EString::FromInt(OutStats.WalkableTriangleCount) + EString(PE_TEXT(" walkable), "))
			+ EString::FromInt(OutStats.PolygonCount) + EString(PE_TEXT(" polygons in "))
			+ EString::FromFloat(OutStats.Milliseconds) + EString(PE_TEXT(" ms.")));
		if (OutStats.SkippedMeshCount > 0u)
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: "))
				+ EString::FromInt(OutStats.SkippedMeshCount)
				+ EString(PE_TEXT(" static mesh components were skipped because their data was not ready. Rebuild once the assets finish loading.")));
		}
		return TRUE;
	}

	void ERecastNavigationProvider::ReleaseNavigationData()
	{
		if (NavMeshQuery)
		{
			dtFreeNavMeshQuery(NavMeshQuery);
			NavMeshQuery = nullptr;
		}
		if (NavMesh)
		{
			// The tile data was handed over with DT_TILE_FREE_DATA, so freeing the navmesh
			// frees it as well.
			dtFreeNavMesh(NavMesh);
			NavMesh = nullptr;
		}
		bReady = FALSE;
	}

	BOOL32 ERecastNavigationProvider::FindNearestPoint(const Vector3& InLocation, const Vector3& InHalfExtents, const ENavigationQueryFilter& InFilter, ENavigationProjectionResult& OutResult) const
	{
		OutResult = ENavigationProjectionResult();
		if ((!bReady) || (!NavMeshQuery))
		{
			return FALSE;
		}

		dtQueryFilter Filter;
		ApplyQueryFilter(InFilter, Filter);

		dtPolyRef PolygonRef = 0;
		float NearestPoint[3];
		const dtStatus Status = NavMeshQuery->findNearestPoly(InLocation.v, InHalfExtents.v, &Filter, &PolygonRef, NearestPoint);
		if ((dtStatusFailed(Status)) || (PolygonRef == 0))
		{
			return FALSE;
		}

		OutResult.Point = Vector3(NearestPoint[0], NearestPoint[1], NearestPoint[2]);
		OutResult.bFound = TRUE;
		bool bOverPolygon = false;
		float ClosestPoint[3];
		if (dtStatusSucceed(NavMeshQuery->closestPointOnPoly(PolygonRef, InLocation.v, ClosestPoint, &bOverPolygon)))
		{
			OutResult.Point = Vector3(ClosestPoint[0], ClosestPoint[1], ClosestPoint[2]);
			OutResult.bOverPolygon = (bOverPolygon ? TRUE : FALSE);
		}
		return TRUE;
	}

	BOOL32 ERecastNavigationProvider::ProjectPoint(const Vector3& InLocation, const ENavigationQueryFilter& InFilter, ENavigationProjectionResult& OutResult) const
	{
		float HalfExtents[3];
		ComputeSearchHalfExtents(Settings, HalfExtents);
		return FindNearestPoint(InLocation, Vector3(HalfExtents[0], HalfExtents[1], HalfExtents[2]), InFilter, OutResult);
	}

	BOOL32 ERecastNavigationProvider::FindPath(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationPath& OutPath) const
	{
		if ((!bReady) || (!NavMeshQuery))
		{
			return FALSE;
		}

		dtQueryFilter Filter;
		ApplyQueryFilter(InFilter, Filter);

		float HalfExtents[3];
		ComputeSearchHalfExtents(Settings, HalfExtents);

		dtPolyRef StartRef = 0;
		dtPolyRef EndRef = 0;
		float StartPoint[3];
		float EndPoint[3];
		if (dtStatusFailed(NavMeshQuery->findNearestPoly(InStart.v, HalfExtents, &Filter, &StartRef, StartPoint)) || (StartRef == 0))
		{
			return FALSE;
		}
		if (dtStatusFailed(NavMeshQuery->findNearestPoly(InEnd.v, HalfExtents, &Filter, &EndRef, EndPoint)) || (EndRef == 0))
		{
			return FALSE;
		}

		dtPolyRef PolygonPath[NavigationMaxPathPolygons];
		int PolygonPathNum = 0;
		dtStatus Status = NavMeshQuery->findPath(StartRef, EndRef, StartPoint, EndPoint, &Filter, PolygonPath, &PolygonPathNum, NavigationMaxPathPolygons);
		if ((dtStatusFailed(Status)) || (PolygonPathNum == 0))
		{
			return FALSE;
		}

		// A partial polygon path means the destination is not reachable. It is still worth
		// walking: the agent moves to the closest reachable point and reports it as partial.
		const BOOL32 bPolygonPathPartial = (dtStatusDetail(Status, DT_PARTIAL_RESULT) != 0)
			|| (PolygonPath[PolygonPathNum - 1] != EndRef);

		float StraightPathPoints[NavigationMaxPathPoints * 3];
		unsigned char StraightPathFlags[NavigationMaxPathPoints];
		dtPolyRef StraightPathRefs[NavigationMaxPathPoints];
		int StraightPathNum = 0;
		Status = NavMeshQuery->findStraightPath(StartPoint, EndPoint, PolygonPath, PolygonPathNum,
			StraightPathPoints, StraightPathFlags, StraightPathRefs, &StraightPathNum, NavigationMaxPathPoints);
		if ((dtStatusFailed(Status)) || (StraightPathNum == 0))
		{
			return FALSE;
		}

		OutPath.Reset();
		OutPath.Points.SetNum(static_cast<UINT32>(StraightPathNum));
		OutPath.PointTypes.SetNum(static_cast<UINT32>(StraightPathNum));
		for (int Index = 0; Index < StraightPathNum; Index++)
		{
			const float* Point = &StraightPathPoints[Index * 3];
			OutPath.Points[Index] = Vector3(Point[0], Point[1], Point[2]);
			const BOOL32 bIsLink = ((StraightPathFlags[Index] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) != 0) ? TRUE : FALSE;
			OutPath.PointTypes[Index] = (bIsLink ? ENavigationPathPointType::NAVIGATION_PATH_POINT_LINK : ENavigationPathPointType::NAVIGATION_PATH_POINT_WALK);
		}
		OutPath.Status = (bPolygonPathPartial ? ENavigationPathStatusType::NAVIGATION_PATH_PARTIAL : ENavigationPathStatusType::NAVIGATION_PATH_COMPLETE);
		return TRUE;
	}

	BOOL32 ERecastNavigationProvider::Raycast(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationRaycastResult& OutResult) const
	{
		OutResult = ENavigationRaycastResult();
		if ((!bReady) || (!NavMeshQuery))
		{
			return FALSE;
		}

		dtQueryFilter Filter;
		ApplyQueryFilter(InFilter, Filter);

		float HalfExtents[3];
		ComputeSearchHalfExtents(Settings, HalfExtents);

		dtPolyRef StartRef = 0;
		float StartPoint[3];
		if (dtStatusFailed(NavMeshQuery->findNearestPoly(InStart.v, HalfExtents, &Filter, &StartRef, StartPoint)) || (StartRef == 0))
		{
			return FALSE;
		}

		float HitT = 0.0f;
		float HitNormal[3];
		dtPolyRef Path[NavigationMaxPathPolygons];
		int PathNum = 0;
		const dtStatus Status = NavMeshQuery->raycast(StartRef, StartPoint, InEnd.v, &Filter, &HitT, HitNormal, Path, &PathNum, NavigationMaxPathPolygons);
		if (dtStatusFailed(Status))
		{
			return FALSE;
		}

		const FLOAT DirectionX = InEnd.x - StartPoint[0];
		const FLOAT DirectionY = InEnd.y - StartPoint[1];
		const FLOAT DirectionZ = InEnd.z - StartPoint[2];
		OutResult.HitPoint = Vector3(StartPoint[0] + (DirectionX * HitT), StartPoint[1] + (DirectionY * HitT), StartPoint[2] + (DirectionZ * HitT));
		OutResult.HitNormal = Vector3(HitNormal[0], HitNormal[1], HitNormal[2]);
		OutResult.Distance = EMath::Sqrt((DirectionX * DirectionX) + (DirectionY * DirectionY) + (DirectionZ * DirectionZ)) * HitT;
		// t reaches 1 when nothing blocked the ray, so a shorter segment means a wall.
		OutResult.bHit = (HitT < 1.0f) ? TRUE : FALSE;
		return TRUE;
	}

	BOOL32 ERecastNavigationProvider::MoveAlongSurface(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, Vector3& OutLocation) const
	{
		if ((!bReady) || (!NavMeshQuery))
		{
			return FALSE;
		}

		dtQueryFilter Filter;
		ApplyQueryFilter(InFilter, Filter);

		float HalfExtents[3];
		ComputeSearchHalfExtents(Settings, HalfExtents);

		dtPolyRef StartRef = 0;
		float StartPoint[3];
		if (dtStatusFailed(NavMeshQuery->findNearestPoly(InStart.v, HalfExtents, &Filter, &StartRef, StartPoint)) || (StartRef == 0))
		{
			return FALSE;
		}

		dtPolyRef Visited[NavigationMaxVisitedPolygons];
		int VisitedNum = 0;
		float ResultPoint[3];
		const dtStatus Status = NavMeshQuery->moveAlongSurface(StartRef, StartPoint, InEnd.v, &Filter, ResultPoint, Visited, &VisitedNum, NavigationMaxVisitedPolygons);
		if (dtStatusFailed(Status))
		{
			return FALSE;
		}

		OutLocation = Vector3(ResultPoint[0], ResultPoint[1], ResultPoint[2]);
		return TRUE;
	}

	void ERecastNavigationProvider::CollectDebugLines(ENavigationDebugModeType InMode, TArray<Vector3>& OutLineSegments) const
	{
		if ((!bReady) || (!NavMesh))
		{
			return;
		}

		// The mutating getTile overload is private, so the read only one has to be selected
		// through a const pointer.
		const dtNavMesh* ReadOnlyNavMesh = NavMesh;
		const int MaxTiles = NavMesh->getMaxTiles();
		for (int TileIndex = 0; TileIndex < MaxTiles; TileIndex++)
		{
			const dtMeshTile* Tile = ReadOnlyNavMesh->getTile(TileIndex);
			if ((!Tile) || (!Tile->header) || (!Tile->polys) || (!Tile->verts))
			{
				continue;
			}

			for (int PolygonIndex = 0; PolygonIndex < Tile->header->polyCount; PolygonIndex++)
			{
				const dtPoly* Polygon = &Tile->polys[PolygonIndex];
				if (Polygon->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
				{
					// A link is a single segment whatever the view mode is; it is never part of
					// the walkable surface.
					if (Polygon->vertCount < 2u)
					{
						continue;
					}
					const float* LinkStart = &Tile->verts[Polygon->verts[0] * 3u];
					const float* LinkEnd = &Tile->verts[Polygon->verts[1] * 3u];
					OutLineSegments.Add(Vector3(LinkStart[0], LinkStart[1], LinkStart[2]));
					OutLineSegments.Add(Vector3(LinkEnd[0], LinkEnd[1], LinkEnd[2]));
					continue;
				}

				const UINT32 VertexCount = Polygon->vertCount;
				if (VertexCount < 3u)
				{
					continue;
				}

				for (UINT32 EdgeIndex = 0u; EdgeIndex < VertexCount; EdgeIndex++)
				{
					// An interior edge has a neighbour ref without the external flag. Everything
					// else - no neighbour, or a link to another tile - is part of the boundary.
					const unsigned short Neighbour = Polygon->neis[EdgeIndex];
					const BOOL32 bIsBoundary = (((Neighbour & DT_EXT_LINK) != 0u) || (Neighbour == 0u)) ? TRUE : FALSE;
					if ((InMode == ENavigationDebugModeType::NAVIGATION_DEBUG_BOUNDARY) && (!bIsBoundary))
					{
						continue;
					}

					const UINT32 NextIndex = (EdgeIndex + 1u) % VertexCount;
					const float* EdgeStart = &Tile->verts[Polygon->verts[EdgeIndex] * 3u];
					const float* EdgeEnd = &Tile->verts[Polygon->verts[NextIndex] * 3u];
					OutLineSegments.Add(Vector3(EdgeStart[0], EdgeStart[1], EdgeStart[2]));
					OutLineSegments.Add(Vector3(EdgeEnd[0], EdgeEnd[1], EdgeEnd[2]));
				}
			}
		}
	}

}  // namespace PigeonEngine
