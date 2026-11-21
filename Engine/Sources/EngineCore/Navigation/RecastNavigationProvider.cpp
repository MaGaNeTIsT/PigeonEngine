#include <Navigation/RecastNavigationProvider.h>

#include <Base/Memory/Memory.h>
#include <Development/EditorLogManager.h>
#include <Development/Profiler.h>
#include <Navigation/NavigationGeometry.h>
#include <Navigation/NavigationProviderRegistry.h>

#include <Recast/Include/Recast.h>
#include <Recast/Include/RecastAlloc.h>
#include <Detour/Include/DetourCommon.h>
#include <Detour/Include/DetourNavMesh.h>
#include <Detour/Include/DetourNavMeshBuilder.h>
#include <Detour/Include/DetourNavMeshQuery.h>
#include <Detour/Include/DetourStatus.h>
#include <Detour/Include/DetourAlloc.h>
#include <DetourCrowd/Include/DetourCrowd.h>
#include <DetourTileCache/Include/DetourTileCache.h>
#include <DetourTileCache/Include/DetourTileCacheBuilder.h>
#include <FastLZ/fastlz.h>

namespace PigeonEngine
{
	namespace
	{
		// Route Recast/Detour allocations through the engine allocator so navmesh memory sits
		// on the same bookkeeping as everything else. The hook shapes match (size plus a
		// lifetime hint this allocator ignores) but the hint enums are distinct types, so one
		// shim per library; installed once from a static initializer, before any bake can run.
		// The libraries store plain function pointers, so the install is permanent by design.
		void* NavigationRcAllocate(size_t InSize, rcAllocHint /*InHint*/)
		{
			return EMemory::Allocate(InSize);
		}

		void* NavigationDtAllocate(size_t InSize, dtAllocHint /*InHint*/)
		{
			return EMemory::Allocate(InSize);
		}

		void NavigationLibraryDeallocate(void* InPointer)
		{
			EMemory::Deallocate(InPointer);
		}

		const BOOL32 bNavigationAllocatorsInstalled = []() -> BOOL32
		{
			rcAllocSetCustom(NavigationRcAllocate, NavigationLibraryDeallocate);
			dtAllocSetCustom(NavigationDtAllocate, NavigationLibraryDeallocate);
			return TRUE;
		}();
	}


	// Sanity limit on the voxel grid. A cell size typed one digit too small can ask for a
	// heightfield that would exhaust memory before any diagnostic is printed, so the build
	// refuses it instead of trying.
	PE_CONSTEXPR_STATIC UINT32 NavigationMaxHeightfieldCells = 4u * 1024u * 1024u;
	PE_CONSTEXPR_STATIC INT32 NavigationMaxPathPolygons = 256;
	PE_CONSTEXPR_STATIC INT32 NavigationMaxPathPoints = 256;
	PE_CONSTEXPR_STATIC INT32 NavigationMaxVisitedPolygons = 32;
	// Detour area ids the engine classes are stored as. Ground is zero, and the remaining
	// classes follow in the same order as their bits in ENavigationAreaFlagType, because a
	// query filter's cost array is indexed by class order and mapped straight onto these ids.
	PE_CONSTEXPR_STATIC UINT8 NavigationGroundAreaId = 0u;
	PE_CONSTEXPR_STATIC UINT8 NavigationWaterAreaId = 1u;
	PE_CONSTEXPR_STATIC UINT8 NavigationDoorAreaId = 2u;
	PE_CONSTEXPR_STATIC UINT8 NavigationJumpAreaId = 3u;

	// Recast stamps one area id per triangle and carries it through to the polygon. Its ids
	// are UINT8 and RC_WALKABLE_AREA (63) is the largest legal value, so the classes are
	// stamped descending from it - ground first, which leaves the common case untouched -
	// and translated to the compact Detour ids above when polygons are written.
	static UINT8 NavigationAreaBuildId(const UINT16 InAreaFlags)
	{
		if (InAreaFlags & ENavigationAreaFlagType::NAVIGATION_AREA_GROUND)
		{
			return RC_WALKABLE_AREA;
		}
		if (InAreaFlags & ENavigationAreaFlagType::NAVIGATION_AREA_WATER)
		{
			return RC_WALKABLE_AREA - 1;
		}
		if (InAreaFlags & ENavigationAreaFlagType::NAVIGATION_AREA_DOOR)
		{
			return RC_WALKABLE_AREA - 2;
		}
		if (InAreaFlags & ENavigationAreaFlagType::NAVIGATION_AREA_JUMP)
		{
			return RC_WALKABLE_AREA - 3;
		}
		// An unmarked surface is plain ground.
		return RC_WALKABLE_AREA;
	}

	// Rewrites one polygon's build area id into the Detour area id it is queried by and the
	// flags the query filter selects on. Ids outside the mapping belong to no class and end up
	// with flags 0, which no filter matches.
	static void NavigationAreaDetourStamp(const UINT8 InBuildArea, UINT8& OutDetourArea, UINT16& OutFlags)
	{
		switch (InBuildArea)
		{
		case RC_WALKABLE_AREA:
			OutDetourArea = NavigationGroundAreaId;
			OutFlags = ENavigationAreaFlagType::NAVIGATION_AREA_GROUND;
			return;
		case RC_WALKABLE_AREA - 1:
			OutDetourArea = NavigationWaterAreaId;
			OutFlags = ENavigationAreaFlagType::NAVIGATION_AREA_WATER;
			return;
		case RC_WALKABLE_AREA - 2:
			OutDetourArea = NavigationDoorAreaId;
			OutFlags = ENavigationAreaFlagType::NAVIGATION_AREA_DOOR;
			return;
		case RC_WALKABLE_AREA - 3:
			OutDetourArea = NavigationJumpAreaId;
			OutFlags = ENavigationAreaFlagType::NAVIGATION_AREA_JUMP;
			return;
		default:
			OutDetourArea = 0u;
			OutFlags = 0u;
			return;
		}
	}
	// Upper bound Detour reserves per grid cell. A tile that would need more polygons fails to add
	// and is reported, which is the same failure the reference sample has.
	PE_CONSTEXPR_STATIC INT32 NavigationMaxPolysPerTile = 2048;
	// Obstacles a tile cache can hold at once.
	PE_CONSTEXPR_STATIC INT32 NavigationMaxObstacles = 128;

	// Flat arrays dtNavMeshCreateParams expects for the authored connections. Both bake paths
	// fill one of these from the engine links, so the single tile build and the tile cache's
	// mesh process cannot drift apart.
	struct FNavigationLinkArrays
	{
		// (start.x, start.y, start.z, end.x, end.y, end.z) per link.
		TArray<FLOAT>	Vertices;
		TArray<FLOAT>	Radii;
		TArray<UINT16>	ConFlags;
		TArray<UINT8>	ConAreas;
		// One per link. Detour reads this byte as "bidirectional", despite the name.
		TArray<UINT8>	ConDirections;

		void Fill(const TArray<ENavigationLink>& InLinks)
		{
			for (UINT32 Index = 0u, Num = InLinks.Num<UINT32>(); Index < Num; Index++)
			{
				const ENavigationLink& Link = InLinks[Index];
				Vertices.Add(Link.Start.x);
				Vertices.Add(Link.Start.y);
				Vertices.Add(Link.Start.z);
				Vertices.Add(Link.End.x);
				Vertices.Add(Link.End.y);
				Vertices.Add(Link.End.z);
				// A link is never smaller than a voxel in radius, and a zero radius would leave
				// Detour unable to find the polygon under the endpoint.
				Radii.Add(EMath::Max(Link.Radius, 1.0f));

				UINT8 DetourArea = 0u;
				UINT16 Flags = 0u;
				NavigationAreaDetourStamp(NavigationAreaBuildId(static_cast<UINT16>(Link.Area)), DetourArea, Flags);
				ConAreas.Add(DetourArea);
				ConFlags.Add(Flags);
				ConDirections.Add(Link.bDirectional ? 0u : 1u);
			}
		}

		PE_NODISCARD INT32 Count() const
		{
			return static_cast<INT32>(Radii.Num<UINT32>());
		}

		void ApplyTo(dtNavMeshCreateParams& OutParams) const
		{
			if (Count() <= 0)
			{
				return;
			}
			OutParams.offMeshConVerts = Vertices.GetData();
			OutParams.offMeshConRad = Radii.GetData();
			OutParams.offMeshConFlags = ConFlags.GetData();
			OutParams.offMeshConAreas = ConAreas.GetData();
			OutParams.offMeshConDir = ConDirections.GetData();
			OutParams.offMeshConUserID = nullptr;
			OutParams.offMeshConCount = Count();
		}
	};

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
		rcHeightfieldLayerSet*	LayerSet = nullptr;

		~FRecastBuildStructures()
		{
			if (LayerSet)
			{
				rcFreeHeightfieldLayerSet(LayerSet);
			}
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

	// --- Tile cache support -------------------------------------------------

	// Frees every block the tile cache asks for when the cache is reset. Detour rebuilds a tile by
	// allocating a batch of temporaries and releasing them together, so tracking blocks is enough
	// and needs no real heap.
	class ENavTileCacheAllocator final : public dtTileCacheAlloc
	{
	public:
		virtual ~ENavTileCacheAllocator() override
		{
			reset();
		}
		virtual void reset() override
		{
			for (UINT32 Index = 0u, Num = Blocks.Num<UINT32>(); Index < Num; Index++)
			{
				dtFree(Blocks[Index]);
			}
			Blocks.Empty(0u);
		}
		virtual void* alloc(const size_t size) override
		{
			void* Block = dtAlloc(size, DT_ALLOC_PERM);
			if (!!Block)
			{
				Blocks.Add(Block);
			}
			return Block;
		}
		virtual void free(void* ptr) override
		{
			// The cache frees through reset(). An individual free is still honoured, because the
			// interface allows it.
			if (!ptr)
			{
				return;
			}
			const INT32 Index = Blocks.Find(static_cast<void*>(ptr));
			if (Index >= 0)
			{
				Blocks.RemoveAt(static_cast<UINT32>(Index));
				dtFree(ptr);
			}
		}
	private:
		TArray<void*> Blocks;
	};

	// Compresses the layer bytes with FastLZ - the same compression the upstream demo's tile
	// cache uses - so obstacle-rebuild layers stay small in memory. The exported navmesh file
	// never sees this: the asset format stores the built tile bytes, not the layers, so a file
	// written by an uncompressed build reads back identically.
	class ENavTileCacheCompressor final : public dtTileCacheCompressor
	{
	public:
		virtual int maxCompressedSize(const int bufferSize) override
		{
			// FastLZ's contract: the output buffer must be at least 5% larger than the input
			// and never smaller than 66 bytes, and incompressible data can come out slightly
			// larger than the input. 1/8 plus 128 covers all of it with room to spare.
			return bufferSize + (bufferSize / 8) + 128;
		}
		virtual dtStatus compress(const unsigned char* buffer, const int bufferSize
			, unsigned char* compressed, const int maxCompressedSize, int* compressedSize) override
		{
			const int CompressedSize = (bufferSize > 0)
				? fastlz_compress_level(1, buffer, bufferSize, compressed)
				: 0;
			if ((bufferSize > 0) && (CompressedSize <= 0))
			{
				return DT_FAILURE;
			}
			if (CompressedSize > maxCompressedSize)
			{
				return DT_FAILURE | DT_BUFFER_TOO_SMALL;
			}
			*compressedSize = CompressedSize;
			return DT_SUCCESS;
		}
		virtual dtStatus decompress(const unsigned char* compressed, const int compressedSize
			, unsigned char* buffer, const int maxBufferSize, int* bufferSize) override
		{
			// A zero return means the data is corrupt or the output buffer is too small;
			// FastLZ is memory safe either way.
			const int DecompressedSize = (compressedSize > 0)
				? fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize)
				: 0;
			if ((compressedSize > 0) && (DecompressedSize <= 0))
			{
				return DT_FAILURE;
			}
			*bufferSize = DecompressedSize;
			return DT_SUCCESS;
		}
	};

	// Stamps the same area id and flags on a rebuilt polygon that a fresh bake writes, so a tile
	// rebuilt around an obstacle is indistinguishable from a baked one. The layer keeps the area
	// ids the bake stamped, which is why the mapping is the same function the single tile path
	// uses.
	class ENavTileCacheMeshProcess final : public dtTileCacheMeshProcess
	{
	public:
		// Copies the authored connections for the tile cache to hand to every tile build. The
		// cache calls process for each tile, including obstacle rebuilds long after the bake,
		// so the arrays live here for the cache's lifetime.
		void SetLinks(const TArray<ENavigationLink>& InLinks)
		{
			LinkArrays.Fill(InLinks);
		}

		virtual void process(dtNavMeshCreateParams* params, unsigned char* polyAreas, unsigned short* polyFlags) override
		{
			if ((!params) || (!polyAreas) || (!polyFlags))
			{
				return;
			}
			for (int Index = 0; Index < params->polyCount; Index++)
			{
				UINT8 DetourArea = 0u;
				UINT16 Flags = 0u;
				NavigationAreaDetourStamp(polyAreas[Index], DetourArea, Flags);
				polyAreas[Index] = DetourArea;
				polyFlags[Index] = Flags;
			}
			// Every tile gets the whole connection list: dtCreateNavMeshData stores a link in
			// the tile its start point falls into and links the other end across the seam.
			LinkArrays.ApplyTo(*params);
		}

	private:
		FNavigationLinkArrays LinkArrays;
	};

	// Owns the tile cache and the three interfaces it borrows. Declared in the provider header as
	// an opaque pointer, defined here because everything in it derives from Detour types.
	struct FRecastNavigationTileCache
	{
		ENavTileCacheAllocator		Allocator;
		ENavTileCacheCompressor		Compressor;
		ENavTileCacheMeshProcess	MeshProcess;
		dtTileCache*				TileCache = nullptr;
		dtTileCacheParams			Params;

		~FRecastNavigationTileCache()
		{
			// The cache has to go before the allocator it borrowed, which is why the release lives
			// in this body rather than in a member's destructor.
			if (TileCache)
			{
				dtFreeTileCache(TileCache);
				TileCache = nullptr;
			}
		}
	};

	// Everything the tiled bake needs, in engine types only, so the bake can stay a file local
	// function instead of a provider member that would drag rcConfig into the header.
	struct FRecastBakeInputs
	{
		const TArray<FLOAT>*	Vertices = nullptr;
		const TArray<INT32>*	Triangles = nullptr;
		// Area class per triangle, parallel to Triangles (one entry per triangle). The bake
		// turns each into the Recast area id it rasterizes the triangle with.
		const TArray<UINT16>*	TriangleAreaFlags = nullptr;
		// Authored off-mesh connections. The tiled path hands them to the tile cache's mesh
		// process; the single tile path writes them into its create params directly.
		const TArray<ENavigationLink>*	Links = nullptr;
		FLOAT					BMin[3] = { 0.0f, 0.0f, 0.0f };
		FLOAT					BMax[3] = { 0.0f, 0.0f, 0.0f };
		FLOAT					CellSize = 20.0f;
		FLOAT					CellHeight = 10.0f;
		FLOAT					WalkableSlopeAngle = 45.0f;
		FLOAT					AgentHeight = 150.0f;
		FLOAT					AgentRadius = 35.0f;
		FLOAT					AgentMaxClimb = 40.0f;
		INT32					WalkableHeight = 0;
		INT32					WalkableClimb = 0;
		INT32					WalkableRadius = 0;
		INT32					MaxEdgeLen = 12;
		FLOAT					MaxSimplificationError = 26.0f;
		INT32					MinRegionArea = 0;
		INT32					MergeRegionArea = 0;
		INT32					MaxVertsPerPoly = 6;
		FLOAT					DetailSampleDistance = 60.0f;
		FLOAT					DetailSampleMaxError = 10.0f;
		INT32					WholeWidth = 0;
		INT32					WholeHeight = 0;
		INT32					TileSizeCells = 0;
		UINT32					MaxLayersPerTile = 16u;
	};

	// Rasterizes one tile and stores its layers in the cache. Returns the layer count, or -1 when
	// the tile could not be built.
	// InTileIndexNum is the number of indices in InTileTriangles (three per triangle); the Recast
	// calls below take a triangle count, which is why it is divided once and used from there.
	static INT32 RasterizeTileLayers(rcContext& Context, const FRecastBakeInputs& InInputs
		, const INT32* InTileTriangles, const UINT16* InTileAreaFlags, const INT32 InTileIndexNum
		, const INT32 InTileX, const INT32 InTileY, dtTileCache& InTileCache
		, INT32& OutWalkableTriangleNum)
	{
		const INT32 BorderSize = InInputs.WalkableRadius + 3;
		const FLOAT TileWorldSize = static_cast<FLOAT>(InInputs.TileSizeCells) * InInputs.CellSize;
		const FLOAT BorderWorld = static_cast<FLOAT>(BorderSize) * InInputs.CellSize;

		rcConfig Config;
		memset(&Config, 0, sizeof(Config));
		Config.cs = InInputs.CellSize;
		Config.ch = InInputs.CellHeight;
		Config.walkableSlopeAngle = InInputs.WalkableSlopeAngle;
		Config.walkableHeight = InInputs.WalkableHeight;
		Config.walkableClimb = InInputs.WalkableClimb;
		Config.walkableRadius = InInputs.WalkableRadius;
		Config.maxEdgeLen = InInputs.MaxEdgeLen;
		Config.maxSimplificationError = InInputs.MaxSimplificationError;
		Config.minRegionArea = InInputs.MinRegionArea;
		Config.mergeRegionArea = InInputs.MergeRegionArea;
		Config.maxVertsPerPoly = InInputs.MaxVertsPerPoly;
		Config.detailSampleDist = InInputs.DetailSampleDistance;
		Config.detailSampleMaxError = InInputs.DetailSampleMaxError;
		// A tile is built on its own grid, with a border ring so neighbours agree along the seam.
		Config.tileSize = InInputs.TileSizeCells;
		Config.borderSize = BorderSize;
		Config.width = InInputs.TileSizeCells + (BorderSize * 2);
		Config.height = InInputs.TileSizeCells + (BorderSize * 2);
		Config.bmin[0] = InInputs.BMin[0] + (static_cast<FLOAT>(InTileX) * TileWorldSize) - BorderWorld;
		Config.bmin[1] = InInputs.BMin[1];
		Config.bmin[2] = InInputs.BMin[2] + (static_cast<FLOAT>(InTileY) * TileWorldSize) - BorderWorld;
		Config.bmax[0] = InInputs.BMin[0] + (static_cast<FLOAT>(InTileX + 1) * TileWorldSize) + BorderWorld;
		Config.bmax[1] = InInputs.BMax[1];
		Config.bmax[2] = InInputs.BMin[2] + (static_cast<FLOAT>(InTileY + 1) * TileWorldSize) + BorderWorld;

		if ((Config.width <= 0) || (Config.height <= 0))
		{
			return -1;
		}

		FRecastBuildStructures Build;
		Build.Heightfield = rcAllocHeightfield();
		if (!Build.Heightfield)
		{
			return -1;
		}
		if (!rcCreateHeightfield(&Context, *Build.Heightfield, Config.width, Config.height, Config.bmin, Config.bmax, Config.cs, Config.ch))
		{
			return -1;
		}

		const INT32 TileTriangleNum = InTileIndexNum / 3;
		if (TileTriangleNum > 0)
		{
			TArray<UINT8> TriangleAreas;
			TriangleAreas.SetNum(static_cast<UINT32>(TileTriangleNum));
			memset(TriangleAreas.GetData(), RC_NULL_AREA, static_cast<size_t>(TileTriangleNum));
			rcMarkWalkableTriangles(&Context, Config.walkableSlopeAngle, InInputs.Vertices->GetData()
				, InInputs.Vertices->Num<INT32>(), InTileTriangles, TileTriangleNum, TriangleAreas.GetData());
			for (INT32 TriangleIndex = 0; TriangleIndex < TileTriangleNum; TriangleIndex++)
			{
				if (TriangleAreas.GetRef(TriangleIndex) != RC_NULL_AREA)
				{
					OutWalkableTriangleNum++;
					// rcMarkWalkableTriangles stamped every walkable triangle with the same
					// class; the source triangle's mark replaces it.
					const UINT16 AreaFlags = InTileAreaFlags ? InTileAreaFlags[TriangleIndex] : 0u;
					TriangleAreas.GetRef(TriangleIndex) = NavigationAreaBuildId(AreaFlags);
				}
			}
			if (!rcRasterizeTriangles(&Context, InInputs.Vertices->GetData(), InInputs.Vertices->Num<INT32>()
				, InTileTriangles, TriangleAreas.GetData(), TileTriangleNum, *Build.Heightfield, Config.walkableClimb))
			{
				return -1;
			}
		}

		rcFilterLowHangingWalkableObstacles(&Context, Config.walkableClimb, *Build.Heightfield);
		rcFilterLedgeSpans(&Context, Config.walkableHeight, Config.walkableClimb, *Build.Heightfield);
		rcFilterWalkableLowHeightSpans(&Context, Config.walkableHeight, *Build.Heightfield);

		Build.CompactHeightfield = rcAllocCompactHeightfield();
		if (!Build.CompactHeightfield)
		{
			return -1;
		}
		if (!rcBuildCompactHeightfield(&Context, Config.walkableHeight, Config.walkableClimb, *Build.Heightfield, *Build.CompactHeightfield))
		{
			return -1;
		}
		if (!rcErodeWalkableArea(&Context, Config.walkableRadius, *Build.CompactHeightfield))
		{
			return -1;
		}

		Build.LayerSet = rcAllocHeightfieldLayerSet();
		if (!Build.LayerSet)
		{
			return -1;
		}
		if (!rcBuildHeightfieldLayers(&Context, *Build.CompactHeightfield, Config.borderSize, Config.walkableHeight, *Build.LayerSet))
		{
			return -1;
		}

		INT32 LayerNum = 0;
		for (INT32 LayerIndex = 0; LayerIndex < Build.LayerSet->nlayers; LayerIndex++)
		{
			const rcHeightfieldLayer* Layer = &Build.LayerSet->layers[LayerIndex];

			dtTileCacheLayerHeader Header;
			memset(&Header, 0, sizeof(Header));
			Header.magic = DT_TILECACHE_MAGIC;
			Header.version = DT_TILECACHE_VERSION;
			Header.tx = InTileX;
			Header.ty = InTileY;
			Header.tlayer = LayerIndex;
			rcVcopy(Header.bmin, Layer->bmin);
			rcVcopy(Header.bmax, Layer->bmax);
			Header.width = static_cast<unsigned char>(Layer->width);
			Header.height = static_cast<unsigned char>(Layer->height);
			Header.minx = static_cast<unsigned char>(Layer->minx);
			Header.maxx = static_cast<unsigned char>(Layer->maxx);
			Header.miny = static_cast<unsigned char>(Layer->miny);
			Header.maxy = static_cast<unsigned char>(Layer->maxy);
			Header.hmin = static_cast<unsigned short>(Layer->hmin);
			Header.hmax = static_cast<unsigned short>(Layer->hmax);

			unsigned char* LayerData = nullptr;
			int LayerDataSize = 0;
			if (dtStatusFailed(dtBuildTileCacheLayer(InTileCache.getCompressor(), &Header
				, Layer->heights, Layer->areas, Layer->cons, &LayerData, &LayerDataSize)))
			{
				return -1;
			}
			if (dtStatusFailed(InTileCache.addTile(LayerData, LayerDataSize, DT_COMPRESSEDTILE_FREE_DATA, 0)))
			{
				dtFree(LayerData);
				continue;
			}
			LayerNum++;
		}
		return LayerNum;
	}

	// Tile grid index for a world coordinate, clamped to the grid. Triangles outside the bake
	// bounds still have to land in some tile: Recast clips them during rasterization.
	static INT32 TileIndexFromCoordinate(const FLOAT InCoordinate, const FLOAT InOrigin, const FLOAT InTileWorldSize, const INT32 InTileNum)
	{
		const INT32 Index = static_cast<INT32>(floorf((InCoordinate - InOrigin) / InTileWorldSize));
		return EMath::Clamp(Index, 0, InTileNum - 1);
	}

	// Builds the whole tile grid, the tile cache that mirrors it and the navmesh they feed.
	// On failure nothing is handed back; the caller publishes the results only on success.
	static BOOL32 BakeTiled(const FRecastBakeInputs& InInputs, FRecastNavigationTileCache** OutTileCache
		, dtNavMesh** OutNavMesh, dtNavMeshQuery** OutNavQuery, ENavigationBuildStats& OutStats, EBoundAABB& OutBounds)
	{
		if ((InInputs.TileSizeCells <= 0) || (!InInputs.Vertices) || (!InInputs.Triangles))
		{
			return FALSE;
		}
		const INT32 TileNumX = (InInputs.WholeWidth + InInputs.TileSizeCells - 1) / InInputs.TileSizeCells;
		const INT32 TileNumY = (InInputs.WholeHeight + InInputs.TileSizeCells - 1) / InInputs.TileSizeCells;
		if ((TileNumX <= 0) || (TileNumY <= 0))
		{
			return FALSE;
		}

		const INT32 TileSize = InInputs.TileSizeCells;
		const UINT32 TileNum = static_cast<UINT32>(TileNumX * TileNumY);
		const FLOAT TileWorldSize = static_cast<FLOAT>(TileSize) * InInputs.CellSize;
		const FLOAT BorderWorld = static_cast<FLOAT>(InInputs.WalkableRadius + 3) * InInputs.CellSize;
		const UINT32 TriangleNum = InInputs.Triangles->Num<UINT32>() / 3u;
		const INT32* SourceTriangles = InInputs.Triangles->GetData();

		// Bucket the triangles per tile. A tile owns its cells plus a border ring, so the test is
		// against the expanded tile box and a triangle straddling a seam lands in both tiles.
		// Two passes: count per tile, prefix sum into offsets, then fill.
		TArray<UINT32> TileCounts;
		TileCounts.SetNum(TileNum);
		TArray<UINT32> TileOffsets;
		TileOffsets.SetNum(TileNum + 1u);
		TArray<UINT32> TileCursor;
		TileCursor.SetNum(TileNum);
		TArray<INT32> BucketTriangles;
		// Area class per bucket triangle, parallel to BucketTriangles. A triangle straddling a
		// seam lands in several tiles, and every copy has to keep its class.
		TArray<UINT16> BucketAreaFlags;

		for (UINT32 Pass = 0u; Pass < 2u; Pass++)
		{
			if (Pass == 1u)
			{
				UINT32 Running = 0u;
				for (UINT32 TileIndex = 0u; TileIndex < TileNum; TileIndex++)
				{
					TileOffsets[TileIndex] = Running;
					TileCursor[TileIndex] = Running;
					Running += TileCounts[TileIndex];
				}
				TileOffsets[TileNum] = Running;
				BucketTriangles.SetNum(Running * 3u);
				BucketAreaFlags.SetNum(Running);
			}

			for (UINT32 TriangleIndex = 0u; TriangleIndex < TriangleNum; TriangleIndex++)
			{
				const FLOAT* Positions = InInputs.Vertices->GetData() + (TriangleIndex * 9u);
				FLOAT MinX = Positions[0];
				FLOAT MinZ = Positions[2];
				FLOAT MaxX = Positions[0];
				FLOAT MaxZ = Positions[2];
				for (UINT32 Corner = 1u; Corner < 3u; Corner++)
				{
					const FLOAT* Vertex = Positions + (Corner * 3u);
					MinX = EMath::Min(MinX, Vertex[0]);
					MaxX = EMath::Max(MaxX, Vertex[0]);
					MinZ = EMath::Min(MinZ, Vertex[2]);
					MaxZ = EMath::Max(MaxZ, Vertex[2]);
				}

				const INT32 TileX0 = TileIndexFromCoordinate(MinX - BorderWorld, InInputs.BMin[0], TileWorldSize, TileNumX);
				const INT32 TileX1 = TileIndexFromCoordinate(MaxX + BorderWorld, InInputs.BMin[0], TileWorldSize, TileNumX);
				const INT32 TileY0 = TileIndexFromCoordinate(MinZ - BorderWorld, InInputs.BMin[2], TileWorldSize, TileNumY);
				const INT32 TileY1 = TileIndexFromCoordinate(MaxZ + BorderWorld, InInputs.BMin[2], TileWorldSize, TileNumY);
				const INT32* Source = SourceTriangles + (TriangleIndex * 3u);
				const UINT16 AreaFlags = InInputs.TriangleAreaFlags ? (*InInputs.TriangleAreaFlags)[TriangleIndex] : 0u;

				for (INT32 TileY = TileY0; TileY <= TileY1; TileY++)
				{
					for (INT32 TileX = TileX0; TileX <= TileX1; TileX++)
					{
						const UINT32 TileIndex = (static_cast<UINT32>(TileY) * static_cast<UINT32>(TileNumX)) + static_cast<UINT32>(TileX);
						if (Pass == 0u)
						{
							TileCounts[TileIndex]++;
						}
						else
						{
							const UINT32 Target = TileCursor[TileIndex]++;
							BucketTriangles[Target * 3u + 0u] = Source[0];
							BucketTriangles[Target * 3u + 1u] = Source[1];
							BucketTriangles[Target * 3u + 2u] = Source[2];
							BucketAreaFlags[Target] = AreaFlags;
						}
					}
				}
			}
		}

		FRecastNavigationTileCache* State = new FRecastNavigationTileCache();
		if (InInputs.Links)
		{
			State->MeshProcess.SetLinks(*InInputs.Links);
		}
		memset(&State->Params, 0, sizeof(State->Params));
		rcVcopy(State->Params.orig, InInputs.BMin);
		State->Params.cs = InInputs.CellSize;
		State->Params.ch = InInputs.CellHeight;
		State->Params.width = TileSize;
		State->Params.height = TileSize;
		// The tile cache works in world units, like the navmesh creation params.
		State->Params.walkableHeight = InInputs.AgentHeight;
		State->Params.walkableRadius = InInputs.AgentRadius;
		State->Params.walkableClimb = InInputs.AgentMaxClimb;
		State->Params.maxSimplificationError = InInputs.MaxSimplificationError;
		State->Params.maxTiles = static_cast<int>(TileNum * EMath::Max(InInputs.MaxLayersPerTile, 1u));
		State->Params.maxObstacles = NavigationMaxObstacles;

		State->TileCache = dtAllocTileCache();
		if (!State->TileCache)
		{
			delete State;
			return FALSE;
		}
		if (dtStatusFailed(State->TileCache->init(&State->Params, &State->Allocator, &State->Compressor, &State->MeshProcess)))
		{
			delete State;
			return FALSE;
		}

		dtNavMeshParams NavParams;
		memset(&NavParams, 0, sizeof(NavParams));
		rcVcopy(NavParams.orig, InInputs.BMin);
		NavParams.tileWidth = TileWorldSize;
		NavParams.tileHeight = TileWorldSize;
		NavParams.maxTiles = State->Params.maxTiles;
		NavParams.maxPolys = NavigationMaxPolysPerTile;

		dtNavMesh* NavMesh = dtAllocNavMesh();
		if (!NavMesh)
		{
			delete State;
			return FALSE;
		}
		if (dtStatusFailed(NavMesh->init(&NavParams)))
		{
			dtFreeNavMesh(NavMesh);
			delete State;
			return FALSE;
		}

		ERecastBuildLogContext Context;
		OutStats.LayerCount = 0u;
		INT32 OutWalkableTriangleNum = 0;
		for (INT32 TileY = 0; TileY < TileNumY; TileY++)
		{
			for (INT32 TileX = 0; TileX < TileNumX; TileX++)
			{
				const UINT32 TileIndex = (static_cast<UINT32>(TileY) * static_cast<UINT32>(TileNumX)) + static_cast<UINT32>(TileX);
				const UINT32 First = TileOffsets[TileIndex];
				const UINT32 Count = TileOffsets[TileIndex + 1u] - First;
				const INT32 LayerNum = RasterizeTileLayers(Context, InInputs
					, (Count > 0u) ? (BucketTriangles.GetData() + (First * 3u)) : nullptr
					, (Count > 0u) ? (BucketAreaFlags.GetData() + First) : nullptr
					, static_cast<INT32>(Count * 3u), TileX, TileY, *State->TileCache
					, OutWalkableTriangleNum);
				if (LayerNum < 0)
				{
					PE_LOG_ERROR(EString(PE_TEXT("Navigation: tiled bake failed while building tile ("))
						+ EString::FromInt(static_cast<UINT32>(TileX)) + EString(PE_TEXT(","))
						+ EString::FromInt(static_cast<UINT32>(TileY)) + EString(PE_TEXT(").")));
					dtFreeNavMesh(NavMesh);
					delete State;
					return FALSE;
				}
				OutStats.LayerCount += static_cast<UINT32>(LayerNum);
			}
		}
		OutStats.TileCount = OutStats.LayerCount;
		OutStats.WalkableTriangleCount = static_cast<UINT32>(OutWalkableTriangleNum);

		// Every layer is stored before any tile is built: a tile needs its neighbours in place so
		// the seam between them gets linked.
		for (INT32 TileY = 0; TileY < TileNumY; TileY++)
		{
			for (INT32 TileX = 0; TileX < TileNumX; TileX++)
			{
				State->TileCache->buildNavMeshTilesAt(TileX, TileY, NavMesh);
			}
		}

		// The bytes every tile allocated; the tile cache's layers are scratch and are not part
		// of what a query needs in memory. The read only getTile is selected through a const
		// pointer, the mutating overload is private.
		const dtNavMesh* TileDataNavMesh = NavMesh;
		for (int TileIndex = 0; TileIndex < TileDataNavMesh->getMaxTiles(); TileIndex++)
		{
			const dtMeshTile* Tile = TileDataNavMesh->getTile(TileIndex);
			if ((Tile) && (Tile->dataSize > 0))
			{
				OutStats.NavMeshBytes += static_cast<UINT32>(Tile->dataSize);
			}
		}

		dtNavMeshQuery* NavQuery = dtAllocNavMeshQuery();
		if (!NavQuery)
		{
			dtFreeNavMesh(NavMesh);
			delete State;
			return FALSE;
		}
		if (dtStatusFailed(NavQuery->init(NavMesh, NavigationMaxPathPolygons * 8)))
		{
			dtFreeNavMeshQuery(NavQuery);
			dtFreeNavMesh(NavMesh);
			delete State;
			return FALSE;
		}

		// The polygon count comes from the tiles the build produced; the tiled path has no single
		// poly mesh to read it from.
		const dtNavMesh* ReadOnlyNavMesh = NavMesh;
		UINT32 PolygonNum = 0u;
		for (int TileIndex = 0; TileIndex < ReadOnlyNavMesh->getMaxTiles(); TileIndex++)
		{
			const dtMeshTile* Tile = ReadOnlyNavMesh->getTile(TileIndex);
			if ((!!Tile) && (!!Tile->header))
			{
				PolygonNum += static_cast<UINT32>(Tile->header->polyCount);
			}
		}
		OutStats.PolygonCount = PolygonNum;
		OutBounds = EBoundAABB(
			Vector3(InInputs.BMin[0], InInputs.BMin[1], InInputs.BMin[2]),
			Vector3(InInputs.BMax[0], InInputs.BMax[1], InInputs.BMax[2]),
			TRUE);

		*OutTileCache = State;
		*OutNavMesh = NavMesh;
		*OutNavQuery = NavQuery;
		return TRUE;
	}

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

	// Signature and version of the bytes ExportData produces. The asset treats them as opaque, so
	// the backend owns both ends and validates them on import.
	PE_CONSTEXPR_STATIC UINT32 NavigationDataSignature = 0x4352564Eu;	// 'N','V','R','C'
	PE_CONSTEXPR_STATIC UINT32 NavigationDataVersion = 1u;
	// Upper bound on tiles or layers accepted from a file, so a corrupt header cannot make the
	// importer allocate its way out of memory.
	PE_CONSTEXPR_STATIC UINT32 NavigationDataMaxSections = 1u << 16;

	// Appends one fixed size value in native byte order. The container is written and read by the
	// same build, and the engine's other assets already store raw values, so no byte order
	// conversion is attempted.
	template<typename TValueType>
	static void AppendValue(TArray<UINT8>& InOutData, const TValueType& InValue)
	{
		const UINT32 Offset = InOutData.Num<UINT32>();
		InOutData.SetNum(Offset + static_cast<UINT32>(sizeof(TValueType)));
		::memcpy(InOutData.GetData() + Offset, &InValue, sizeof(TValueType));
	}

	static void AppendBytes(TArray<UINT8>& InOutData, const void* InBytes, UINT32 InSize)
	{
		const UINT32 Offset = InOutData.Num<UINT32>();
		InOutData.SetNum(Offset + InSize);
		::memcpy(InOutData.GetData() + Offset, InBytes, InSize);
	}

	// Bounds checked cursor over imported bytes. Every read either succeeds whole or leaves the
	// cursor untouched, so a truncated file cannot produce a half parsed structure.
	struct FNavigationDataReader
	{
		const UINT8*	Cursor = nullptr;
		UINT32			Remaining = 0u;

		BOOL32 ReadUInt32(UINT32& OutValue)
		{
			const UINT8* Bytes = nullptr;
			if (!ReadBytes(Bytes, static_cast<UINT32>(sizeof(UINT32))))
			{
				return FALSE;
			}
			::memcpy(&OutValue, Bytes, sizeof(UINT32));
			return TRUE;
		}

		BOOL32 ReadFloat(FLOAT& OutValue)
		{
			const UINT8* Bytes = nullptr;
			if (!ReadBytes(Bytes, static_cast<UINT32>(sizeof(FLOAT))))
			{
				return FALSE;
			}
			::memcpy(&OutValue, Bytes, sizeof(FLOAT));
			return TRUE;
		}

		BOOL32 ReadBytes(const UINT8*& OutBytes, UINT32 InSize)
		{
			if (Remaining < InSize)
			{
				return FALSE;
			}
			OutBytes = Cursor;
			Cursor += InSize;
			Remaining -= InSize;
			return TRUE;
		}
	};

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

	UINT32 ERecastNavigationProvider::GetCapabilities() const
	{
		UINT32 Capabilities = ENavigationCapabilityType::NAVIGATION_CAPABILITY_PERSISTENCE;
		if (!!TileCache)
		{
			Capabilities |= ENavigationCapabilityType::NAVIGATION_CAPABILITY_OBSTACLES;
			Capabilities |= ENavigationCapabilityType::NAVIGATION_CAPABILITY_TILES;
		}
		if (Settings.MaxCrowdAgents > 0u)
		{
			Capabilities |= ENavigationCapabilityType::NAVIGATION_CAPABILITY_CROWD;
		}
		return Capabilities;
	}

	UINT32 ERecastNavigationProvider::GetContentRevision() const
	{
		return ContentRevision;
	}

	void ERecastNavigationProvider::Tick(FLOAT InDeltaTime)
	{
		if ((!!Crowd) && (!!NavMesh))
		{
			// Steering, avoidance and the crowd's own path queue advance here. Agents then read their
			// pose back, which is why this runs before the world ticks.
			Crowd->update(InDeltaTime, nullptr);
		}

		if ((!TileCache) || (!TileCache->TileCache) || (!NavMesh))
		{
			return;
		}
		if (PendingObstacleChanges > 0u)
		{
			// Any obstacle add or remove may have rebuilt tiles, so the cached debug lines and paths
			// have to be considered stale. The update itself reports "up to date" once its queue is
			// drained, which is true again by the end of the call that did the rebuild.
			PendingObstacleChanges = 0u;
			ContentRevision++;
		}

		// Rebuilds whatever tiles the obstacle queue touched. Detour consumes a bounded amount of
		// work per call, so a burst of obstacles spreads over a few frames instead of stalling one.
		bool bUpToDate = false;
		if (dtStatusFailed(TileCache->TileCache->update(InDeltaTime, NavMesh, &bUpToDate)))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the tile cache could not update; dynamic obstacles may be stale.")));
			return;
		}
	}

	ENavigationObstacleHandle ERecastNavigationProvider::AddCylinderObstacle(const Vector3& InCenter, const FLOAT& InRadius, const FLOAT& InHeight)
	{
		if ((!TileCache) || (!TileCache->TileCache) || (InRadius <= 0.0f) || (InHeight <= 0.0f))
		{
			return NavigationInvalidObstacleHandle;
		}
		dtObstacleRef ObstacleRef = 0;
		if (dtStatusFailed(TileCache->TileCache->addObstacle(InCenter.v, InRadius, InHeight, &ObstacleRef)))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the tile cache refused a cylinder obstacle.")));
			return NavigationInvalidObstacleHandle;
		}
		PendingObstacleChanges++;
		return static_cast<ENavigationObstacleHandle>(ObstacleRef);
	}

	ENavigationObstacleHandle ERecastNavigationProvider::AddBoxObstacle(const Vector3& InCenter, const Vector3& InHalfExtents, const FLOAT& InYawRadians)
	{
		if ((!TileCache) || (!TileCache->TileCache) || (InHalfExtents.x <= 0.0f) || (InHalfExtents.z <= 0.0f))
		{
			return NavigationInvalidObstacleHandle;
		}
		dtObstacleRef ObstacleRef = 0;
		if (dtStatusFailed(TileCache->TileCache->addBoxObstacle(InCenter.v, InHalfExtents.v, InYawRadians, &ObstacleRef)))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the tile cache refused a box obstacle.")));
			return NavigationInvalidObstacleHandle;
		}
		PendingObstacleChanges++;
		return static_cast<ENavigationObstacleHandle>(ObstacleRef);
	}

	void ERecastNavigationProvider::RemoveObstacle(ENavigationObstacleHandle InHandle)
	{
		if ((!TileCache) || (!TileCache->TileCache) || (InHandle == NavigationInvalidObstacleHandle))
		{
			return;
		}
		TileCache->TileCache->removeObstacle(static_cast<dtObstacleRef>(InHandle));
		PendingObstacleChanges++;
	}

	INT32 ERecastNavigationProvider::FindCrowdAgentIndex(ENavigationCrowdAgentHandle InHandle) const
	{
		if ((!Crowd) || (InHandle == NavigationInvalidCrowdAgentHandle))
		{
			return -1;
		}
		for (int AgentIndex = 0; AgentIndex < Crowd->getAgentCount(); AgentIndex++)
		{
			const dtCrowdAgent* Agent = Crowd->getAgent(AgentIndex);
			// Inactive slots hold no agent, and their user data is zero, so they never match.
			if ((!!Agent) && (Agent->active) && (reinterpret_cast<uintptr_t>(Agent->params.userData) == static_cast<uintptr_t>(InHandle)))
			{
				return AgentIndex;
			}
		}
		return -1;
	}

	ENavigationCrowdAgentHandle ERecastNavigationProvider::AddCrowdAgent(const Vector3& InPosition, const ENavigationAgentSettings& InSettings)
	{
		if ((!bReady) || (!NavMesh) || (Settings.MaxCrowdAgents == 0u))
		{
			return NavigationInvalidCrowdAgentHandle;
		}

		if (!Crowd)
		{
			Crowd = dtAllocCrowd();
			if (!Crowd)
			{
				return NavigationInvalidCrowdAgentHandle;
			}
			// The radius the crowd reserves around an agent for avoidance. Twice the largest agent
			// gives the local boundary solver room to place neighbours.
			const FLOAT MaxAgentRadius = EMath::Max(InSettings.Radius, Settings.AgentRadius) * 2.0f;
			if (dtStatusFailed(Crowd->init(static_cast<int>(Settings.MaxCrowdAgents), MaxAgentRadius, NavMesh)))
			{
				dtFreeCrowd(Crowd);
				Crowd = nullptr;
				PE_LOG_WARN(EString(PE_TEXT("Navigation: the crowd could not be initialized.")));
				return NavigationInvalidCrowdAgentHandle;
			}
		}

		dtCrowdAgentParams Params;
		memset(&Params, 0, sizeof(Params));
		Params.radius = InSettings.Radius;
		Params.height = InSettings.Height;
		Params.maxAcceleration = InSettings.MaxAcceleration;
		Params.maxSpeed = InSettings.MoveSpeed;
		Params.collisionQueryRange = InSettings.CollisionQueryRange;
		Params.pathOptimizationRange = InSettings.PathOptimizationRange;
		Params.separationWeight = InSettings.SeparationWeight;
		Params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION
			| DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO;
		Params.obstacleAvoidanceType = InSettings.ObstacleAvoidanceType;
		Params.queryFilterType = 0;

		// The handle is the agent's user data, so it survives the slot shuffling that happens when
		// another agent leaves.
		const UINT32 AgentId = NextCrowdAgentId++;
		Params.userData = reinterpret_cast<void*>(static_cast<uintptr_t>(AgentId));

		if (Crowd->addAgent(InPosition.v, &Params) < 0)
		{
			return NavigationInvalidCrowdAgentHandle;
		}
		return static_cast<ENavigationCrowdAgentHandle>(AgentId);
	}

	void ERecastNavigationProvider::RemoveCrowdAgent(ENavigationCrowdAgentHandle InHandle)
	{
		const INT32 AgentIndex = FindCrowdAgentIndex(InHandle);
		if (AgentIndex >= 0)
		{
			Crowd->removeAgent(AgentIndex);
		}
	}

	BOOL32 ERecastNavigationProvider::RequestCrowdAgentMove(ENavigationCrowdAgentHandle InHandle, const Vector3& InTarget)
	{
		const INT32 AgentIndex = FindCrowdAgentIndex(InHandle);
		if ((AgentIndex < 0) || (!NavMeshQuery))
		{
			return FALSE;
		}

		// The crowd wants a polygon reference, so the target is projected onto the mesh first. A
		// target outside the mesh still yields the closest polygon, which is what makes an
		// unreachable destination a partial move instead of a failure.
		const dtQueryFilter* Filter = Crowd->getFilter(0);
		const float HalfExtents[3] = { Settings.AgentRadius * 2.0f, Settings.AgentHeight, Settings.AgentRadius * 2.0f };
		dtPolyRef TargetRef = 0;
		float TargetPoint[3];
		const dtStatus PolyStatus = NavMeshQuery->findNearestPoly(InTarget.v, HalfExtents, Filter, &TargetRef, TargetPoint);
		if (dtStatusFailed(PolyStatus) || (TargetRef == 0))
		{
			return FALSE;
		}
		return (Crowd->requestMoveTarget(AgentIndex, TargetRef, TargetPoint)) ? TRUE : FALSE;
	}

	void ERecastNavigationProvider::StopCrowdAgent(ENavigationCrowdAgentHandle InHandle)
	{
		const INT32 AgentIndex = FindCrowdAgentIndex(InHandle);
		if (AgentIndex >= 0)
		{
			Crowd->resetMoveTarget(AgentIndex);
		}
	}

	BOOL32 ERecastNavigationProvider::GetCrowdAgentState(ENavigationCrowdAgentHandle InHandle, ENavigationCrowdAgentState& OutState) const
	{
		const INT32 AgentIndex = FindCrowdAgentIndex(InHandle);
		if (AgentIndex < 0)
		{
			return FALSE;
		}
		const dtCrowdAgent* Agent = Crowd->getAgent(AgentIndex);
		OutState.Position = Vector3(Agent->npos[0], Agent->npos[1], Agent->npos[2]);
		OutState.Velocity = Vector3(Agent->vel[0], Agent->vel[1], Agent->vel[2]);
		OutState.bMoving = (Agent->state == DT_CROWDAGENT_STATE_WALKING) && (Agent->targetState != DT_CROWDAGENT_TARGET_NONE);
		// Detour keeps a reached target until it is reset, so arrival is measured here: the corridor
		// has collapsed to the target polygon and the agent stands inside its own radius of it.
		const FLOAT DistanceToTarget = dtVdist(Agent->npos, Agent->targetPos);
		OutState.bTargetReached = ((Agent->targetState == DT_CROWDAGENT_TARGET_FAILED)
			|| ((Agent->targetState == DT_CROWDAGENT_TARGET_VALID)
				&& (Agent->corridor.getPathCount() == 1)
				&& (DistanceToTarget <= (Agent->params.radius + 5.0f)))) ? TRUE : FALSE;
		OutState.bPartial = (Agent->partial != 0) ? TRUE : FALSE;
		return TRUE;
	}

	BOOL32 ERecastNavigationProvider::ExportData(TArray<UINT8>& OutData) const
	{
		OutData.Empty();
		if ((!bReady) || (!NavMesh))
		{
			return FALSE;
		}

		const dtNavMeshParams* Params = NavMesh->getParams();
		if (!Params)
		{
			return FALSE;
		}

		// The read only overload is the public one; the mutating getTile is private.
		const dtNavMesh* ReadOnlyNavMesh = NavMesh;
		const int MaxTiles = ReadOnlyNavMesh->getMaxTiles();
		UINT32 TileNum = 0u;
		UINT32 TileBytes = 0u;
		for (int TileIndex = 0; TileIndex < MaxTiles; TileIndex++)
		{
			const dtMeshTile* Tile = ReadOnlyNavMesh->getTile(TileIndex);
			if ((!!Tile) && (!!Tile->header) && (!!Tile->data) && (Tile->dataSize > 0))
			{
				TileNum++;
				TileBytes += static_cast<UINT32>(Tile->dataSize);
			}
		}
		if (TileNum == 0u)
		{
			return FALSE;
		}

		OutData.Reserve(static_cast<UINT32>(sizeof(dtNavMeshParams)) + (TileNum * 4u * sizeof(UINT32)) + TileBytes + 64u);
		AppendValue(OutData, NavigationDataSignature);
		AppendValue(OutData, NavigationDataVersion);
		AppendValue(OutData, static_cast<UINT32>(sizeof(dtNavMeshParams)));
		AppendBytes(OutData, Params, static_cast<UINT32>(sizeof(dtNavMeshParams)));

		AppendValue(OutData, Bounds.AABBMin.x);
		AppendValue(OutData, Bounds.AABBMin.y);
		AppendValue(OutData, Bounds.AABBMin.z);
		AppendValue(OutData, Bounds.AABBMax.x);
		AppendValue(OutData, Bounds.AABBMax.y);
		AppendValue(OutData, Bounds.AABBMax.z);
		AppendValue(OutData, static_cast<UINT32>(Bounds.IsValid ? 1u : 0u));

		AppendValue(OutData, TileNum);
		for (int TileIndex = 0; TileIndex < MaxTiles; TileIndex++)
		{
			const dtMeshTile* Tile = ReadOnlyNavMesh->getTile(TileIndex);
			if ((!Tile) || (!Tile->header) || (!Tile->data) || (Tile->dataSize <= 0))
			{
				continue;
			}
			AppendValue(OutData, static_cast<INT32>(Tile->header->x));
			AppendValue(OutData, static_cast<INT32>(Tile->header->y));
			AppendValue(OutData, static_cast<INT32>(Tile->header->layer));
			AppendValue(OutData, static_cast<UINT32>(Tile->dataSize));
			AppendBytes(OutData, Tile->data, static_cast<UINT32>(Tile->dataSize));
		}

		// Tile cache section. Its presence is what lets a loaded asset answer to obstacles: the
		// navmesh tiles above are the current result, and the layers are how they are rebuilt.
		if ((!!TileCache) && (!!TileCache->TileCache))
		{
			AppendValue(OutData, static_cast<UINT32>(sizeof(dtTileCacheParams)));
			AppendBytes(OutData, &TileCache->Params, static_cast<UINT32>(sizeof(dtTileCacheParams)));

			// Count first, so the reader can size its loops from the container itself.
			UINT32 LayerNum = 0u;
			const int CachedTileNum = TileCache->TileCache->getTileCount();
			for (int TileIndex = 0; TileIndex < CachedTileNum; TileIndex++)
			{
				const dtCompressedTile* Tile = TileCache->TileCache->getTile(TileIndex);
				if ((!!Tile) && (Tile->header) && (Tile->dataSize > 0))
				{
					LayerNum++;
				}
			}
			AppendValue(OutData, LayerNum);
			for (int TileIndex = 0; TileIndex < CachedTileNum; TileIndex++)
			{
				const dtCompressedTile* Tile = TileCache->TileCache->getTile(TileIndex);
				if ((!Tile) || (!Tile->header) || (Tile->dataSize <= 0))
				{
					continue;
				}
				AppendValue(OutData, static_cast<UINT32>(Tile->dataSize));
				AppendBytes(OutData, Tile->data, static_cast<UINT32>(Tile->dataSize));
			}
		}
		else
		{
			AppendValue(OutData, static_cast<UINT32>(0u));
			AppendValue(OutData, static_cast<UINT32>(0u));
		}
		return TRUE;
	}

	BOOL32 ERecastNavigationProvider::ImportData(const UINT8* InData, UINT32 InSize)
	{
		if ((!InData) || (InSize < (7u * sizeof(UINT32))))
		{
			return FALSE;
		}

		FNavigationDataReader Reader;
		Reader.Cursor = InData;
		Reader.Remaining = InSize;

		UINT32 Signature = 0u;
		UINT32 Version = 0u;
		UINT32 ParamsSize = 0u;
		if ((!Reader.ReadUInt32(Signature)) || (!Reader.ReadUInt32(Version)) || (!Reader.ReadUInt32(ParamsSize)))
		{
			return FALSE;
		}
		if ((Signature != NavigationDataSignature) || (Version != NavigationDataVersion) || (ParamsSize != sizeof(dtNavMeshParams)))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: imported data has an unknown signature or version; the asset was baked by a different build.")));
			return FALSE;
		}

		const UINT8* ParamsBytes = nullptr;
		if (!Reader.ReadBytes(ParamsBytes, ParamsSize))
		{
			return FALSE;
		}
		dtNavMeshParams Params;
		::memcpy(&Params, ParamsBytes, ParamsSize);

		FLOAT BoundsMin[3] = { 0.0f, 0.0f, 0.0f };
		FLOAT BoundsMax[3] = { 0.0f, 0.0f, 0.0f };
		UINT32 BoundsValid = 0u;
		for (UINT32 Axis = 0u; Axis < 3u; Axis++)
		{
			if (!Reader.ReadFloat(BoundsMin[Axis]))
			{
				return FALSE;
			}
		}
		for (UINT32 Axis = 0u; Axis < 3u; Axis++)
		{
			if (!Reader.ReadFloat(BoundsMax[Axis]))
			{
				return FALSE;
			}
		}
		if (!Reader.ReadUInt32(BoundsValid))
		{
			return FALSE;
		}

		UINT32 TileNum = 0u;
		if ((!Reader.ReadUInt32(TileNum)) || (TileNum == 0u) || (TileNum > NavigationDataMaxSections))
		{
			return FALSE;
		}

		// Everything allocated below is freed by this guard unless it is handed over at the end,
		// so a truncated or malformed file leaks nothing.
		struct FImportedNavigationData
		{
			dtNavMesh*					NavMesh = nullptr;
			dtNavMeshQuery*				NavMeshQuery = nullptr;
			FRecastNavigationTileCache*	TileCache = nullptr;
			~FImportedNavigationData()
			{
				if (TileCache)
				{
					delete TileCache;
				}
				if (NavMeshQuery)
				{
					dtFreeNavMeshQuery(NavMeshQuery);
				}
				if (NavMesh)
				{
					dtFreeNavMesh(NavMesh);
				}
			}
		} Imported;

		Imported.NavMesh = dtAllocNavMesh();
		if (!Imported.NavMesh)
		{
			return FALSE;
		}
		if (dtStatusFailed(Imported.NavMesh->init(&Params)))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: imported data has navmesh parameters this build cannot use.")));
			return FALSE;
		}

		for (UINT32 TileIndex = 0u; TileIndex < TileNum; TileIndex++)
		{
			UINT32 TileX = 0u;
			UINT32 TileY = 0u;
			UINT32 TileLayer = 0u;
			UINT32 DataSize = 0u;
			if ((!Reader.ReadUInt32(TileX)) || (!Reader.ReadUInt32(TileY)) || (!Reader.ReadUInt32(TileLayer)) || (!Reader.ReadUInt32(DataSize)))
			{
				return FALSE;
			}
			const UINT8* TileBytes = nullptr;
			if (!Reader.ReadBytes(TileBytes, DataSize))
			{
				return FALSE;
			}

			// Detour takes ownership of the buffer with DT_TILE_FREE_DATA, so the tile data has to
			// live in Detour memory rather than point into the asset (which the editor can replace).
			unsigned char* TileCopy = static_cast<unsigned char*>(dtAlloc(DataSize, DT_ALLOC_PERM));
			if (!TileCopy)
			{
				return FALSE;
			}
			::memcpy(TileCopy, TileBytes, DataSize);
			dtTileRef TileRef = 0;
			if (dtStatusFailed(Imported.NavMesh->addTile(TileCopy, static_cast<int>(DataSize), DT_TILE_FREE_DATA, 0, &TileRef)))
			{
				dtFree(TileCopy);
				PE_LOG_WARN(EString(PE_TEXT("Navigation: imported tile data was rejected by Detour.")));
				return FALSE;
			}
		}

		// Tile cache section, in the order ExportData writes it: parameter size, the parameters
		// themselves, then the layer count and the layers.
		UINT32 TileCacheParamsSize = 0u;
		if (!Reader.ReadUInt32(TileCacheParamsSize))
		{
			return FALSE;
		}
		if ((TileCacheParamsSize > 0u) && (TileCacheParamsSize != sizeof(dtTileCacheParams)))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the asset's tile cache parameters do not match this build.")));
			return FALSE;
		}
		const UINT8* TileCacheParamsBytes = nullptr;
		if (!Reader.ReadBytes(TileCacheParamsBytes, TileCacheParamsSize))
		{
			return FALSE;
		}
		UINT32 LayerNum = 0u;
		if (!Reader.ReadUInt32(LayerNum))
		{
			return FALSE;
		}
		if (LayerNum > NavigationDataMaxSections)
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the asset's layer count is out of range.")));
			return FALSE;
		}

		if (TileCacheParamsSize > 0u)
		{
			Imported.TileCache = new FRecastNavigationTileCache();
			::memcpy(&Imported.TileCache->Params, TileCacheParamsBytes, TileCacheParamsSize);
			Imported.TileCache->TileCache = dtAllocTileCache();
			if ((!Imported.TileCache->TileCache)
				|| (dtStatusFailed(Imported.TileCache->TileCache->init(&Imported.TileCache->Params
					, &Imported.TileCache->Allocator, &Imported.TileCache->Compressor, &Imported.TileCache->MeshProcess))))
			{
				PE_LOG_WARN(EString(PE_TEXT("Navigation: the asset's tile cache could not be created.")));
				return FALSE;
			}

			for (UINT32 LayerIndex = 0u; LayerIndex < LayerNum; LayerIndex++)
			{
				UINT32 LayerSize = 0u;
				if (!Reader.ReadUInt32(LayerSize))
				{
					return FALSE;
				}
				const UINT8* LayerBytes = nullptr;
				if (!Reader.ReadBytes(LayerBytes, LayerSize))
				{
					return FALSE;
				}
				// The cache keeps the buffer and frees it itself, so the bytes have to live in Detour
				// memory rather than pointing into the asset.
				unsigned char* LayerCopy = static_cast<unsigned char*>(dtAlloc(LayerSize, DT_ALLOC_PERM));
				if (!LayerCopy)
				{
					return FALSE;
				}
				::memcpy(LayerCopy, LayerBytes, LayerSize);
				if (dtStatusFailed(Imported.TileCache->TileCache->addTile(LayerCopy, static_cast<int>(LayerSize), DT_COMPRESSEDTILE_FREE_DATA, 0)))
				{
					dtFree(LayerCopy);
					PE_LOG_WARN(EString(PE_TEXT("Navigation: the asset's tile cache rejected a layer.")));
					return FALSE;
				}
			}
		}
		else
		{
			// A single tile asset still has the two counts; consume the (empty) layer list.
			for (UINT32 LayerIndex = 0u; LayerIndex < LayerNum; LayerIndex++)
			{
				UINT32 LayerSize = 0u;
				const UINT8* LayerBytes = nullptr;
				if ((!Reader.ReadUInt32(LayerSize)) || (!Reader.ReadBytes(LayerBytes, LayerSize)))
				{
					return FALSE;
				}
			}
		}

		Imported.NavMeshQuery = dtAllocNavMeshQuery();
		if (!Imported.NavMeshQuery)
		{
			return FALSE;
		}
		if (dtStatusFailed(Imported.NavMeshQuery->init(Imported.NavMesh, NavigationMaxPathPolygons * 8)))
		{
			return FALSE;
		}

		ReleaseNavigationData();
		NavMesh = Imported.NavMesh;
		NavMeshQuery = Imported.NavMeshQuery;
		TileCache = Imported.TileCache;
		Imported.NavMesh = nullptr;
		Imported.NavMeshQuery = nullptr;
		Imported.TileCache = nullptr;
		ContentRevision++;

		Bounds = EBoundAABB(Vector3(BoundsMin[0], BoundsMin[1], BoundsMin[2]), Vector3(BoundsMax[0], BoundsMax[1], BoundsMax[2]), (BoundsValid != 0u) ? TRUE : FALSE);
		bReady = TRUE;
		return TRUE;
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
		// Area class per triangle, kept parallel to Triangles through the whole bake.
		TArray<UINT16> TriangleAreaFlags;
		Vertices.SetNum(OutStats.TriangleCount * 9u);
		Triangles.SetNum(OutStats.TriangleCount * 3u);
		TriangleAreaFlags.SetNum(OutStats.TriangleCount);
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
			TriangleAreaFlags[Index] = static_cast<UINT16>(Triangle.Area);
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
		// walkableHeight, walkableClimb, walkableRadius, maxEdgeLen and maxSimplificationError.
		// Passing a centimeter value where voxels are expected makes the contour simplification
		// devour whole cell ranges: corners vanish and interior holes (a pool, a courtyard) are
		// bridged over, which is exactly what a navmesh looks like when its polygons no longer
		// follow the walkable cells.
		Config.borderSize = 0;
		Config.tileSize = 0;
		Config.walkableSlopeAngle = Settings.AgentMaxSlopeAngle;
		Config.walkableHeight = static_cast<int>(ceilf(Settings.AgentHeight / Config.ch));
		Config.walkableClimb = static_cast<int>(floorf(Settings.AgentMaxClimb / Config.ch));
		Config.walkableRadius = static_cast<int>(ceilf(Settings.AgentRadius / Config.cs));
		Config.maxEdgeLen = static_cast<int>((Settings.MaxEdgeLength > 0.0f) ? (Settings.MaxEdgeLength / Config.cs) : 12.0f);
		Config.maxSimplificationError = (Settings.MaxSimplificationError > 0.0f) ? (Settings.MaxSimplificationError / Config.cs) : 1.3f;
		Config.minRegionArea = static_cast<int>(rcSqr(Settings.MinRegionArea));
		Config.mergeRegionArea = static_cast<int>(rcSqr(Settings.MergeRegionArea));
		Config.maxVertsPerPoly = static_cast<int>(EMath::Clamp<UINT32>(Settings.MaxVertsPerPoly, 3u, static_cast<UINT32>(DT_VERTS_PER_POLYGON)));
		Config.detailSampleDist = (Settings.DetailSampleDistance > 0.0f) ? Settings.DetailSampleDistance : (6.0f * Config.ch);
		Config.detailSampleMaxError = (Settings.DetailSampleMaxError > 0.0f) ? Settings.DetailSampleMaxError : Config.ch;

		// A tile size means the tiled path, which always keeps heightfield layers for a tile cache:
		// that is what lets a dynamic obstacle rebuild only the tiles it overlaps.
		if (Settings.TileSizeCells > 0u)
		{
			FRecastBakeInputs Inputs;
			Inputs.Vertices = &Vertices;
			Inputs.Triangles = &Triangles;
			Inputs.TriangleAreaFlags = &TriangleAreaFlags;
			Inputs.Links = &InGeometry.Links;
			rcVcopy(Inputs.BMin, Config.bmin);
			rcVcopy(Inputs.BMax, Config.bmax);
			Inputs.CellSize = Config.cs;
			Inputs.CellHeight = Config.ch;
			Inputs.WalkableSlopeAngle = Config.walkableSlopeAngle;
			Inputs.AgentHeight = Settings.AgentHeight;
			Inputs.AgentRadius = Settings.AgentRadius;
			Inputs.AgentMaxClimb = Settings.AgentMaxClimb;
			Inputs.WalkableHeight = Config.walkableHeight;
			Inputs.WalkableClimb = Config.walkableClimb;
			Inputs.WalkableRadius = Config.walkableRadius;
			Inputs.MaxEdgeLen = Config.maxEdgeLen;
			Inputs.MaxSimplificationError = Config.maxSimplificationError;
			Inputs.MinRegionArea = Config.minRegionArea;
			Inputs.MergeRegionArea = Config.mergeRegionArea;
			Inputs.MaxVertsPerPoly = Config.maxVertsPerPoly;
			Inputs.DetailSampleDistance = Config.detailSampleDist;
			Inputs.DetailSampleMaxError = Config.detailSampleMaxError;
			Inputs.WholeWidth = Config.width;
			Inputs.WholeHeight = Config.height;
			Inputs.TileSizeCells = static_cast<INT32>(Settings.TileSizeCells);
			Inputs.MaxLayersPerTile = EMath::Max(Settings.MaxLayersPerTile, 1u);

			FRecastNavigationTileCache* NewTileCache = nullptr;
			dtNavMesh* NewNavMesh = nullptr;
			dtNavMeshQuery* NewNavQuery = nullptr;
			EBoundAABB NewBounds;
			if (!BakeTiled(Inputs, &NewTileCache, &NewNavMesh, &NewNavQuery, OutStats, NewBounds))
			{
				PE_LOG_ERROR(EString(PE_TEXT("Navigation: tiled bake failed.")));
				return FALSE;
			}

			ReleaseNavigationData();
			TileCache = NewTileCache;
			NavMesh = NewNavMesh;
			NavMeshQuery = NewNavQuery;
			Bounds = NewBounds;
			bReady = TRUE;
			ContentRevision++;
			OutStats.Milliseconds = static_cast<FLOAT>((EBaseTimer::GetClockSeconds() - StartSeconds) * 1000.0);

			PE_LOG_LOG(EString(PE_TEXT("Navigation: built "))
				+ EString::FromInt(OutStats.TriangleCount) + EString(PE_TEXT(" triangles ("))
				+ EString::FromInt(OutStats.WalkableTriangleCount) + EString(PE_TEXT(" walkable), "))
				+ EString::FromInt(OutStats.PolygonCount) + EString(PE_TEXT(" polygons in "))
				+ EString::FromInt(OutStats.TileCount) + EString(PE_TEXT(" tiles, "))
				+ EString::FromInt(OutStats.LayerCount) + EString(PE_TEXT(" layers, "))
				+ EString::FromFloat(OutStats.Milliseconds) + EString(PE_TEXT(" ms.")));
			if (OutStats.SkippedMeshCount > 0u)
			{
				PE_LOG_WARN(EString(PE_TEXT("Navigation: "))
					+ EString::FromInt(OutStats.SkippedMeshCount)
					+ EString(PE_TEXT(" static mesh components were skipped because their data was not ready. Rebuild once the assets finish loading.")));
			}
			return TRUE;
		}

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
				// rcMarkWalkableTriangles stamped every walkable triangle with the same class;
				// the source triangle's mark replaces it.
				TriangleAreas[Index] = NavigationAreaBuildId(TriangleAreaFlags[Index]);
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

		// Recast keeps the per-triangle class on the polygons it built; Detour stores a compact
		// area id and a separate flag word, and the query filter selects on the flags.
		for (int PolygonIndex = 0; PolygonIndex < Build.PolyMesh->npolys; PolygonIndex++)
		{
			UINT8 DetourArea = 0u;
			UINT16 Flags = 0u;
			NavigationAreaDetourStamp(Build.PolyMesh->areas[PolygonIndex], DetourArea, Flags);
			Build.PolyMesh->areas[PolygonIndex] = DetourArea;
			Build.PolyMesh->flags[PolygonIndex] = Flags;
		}

		// Authored off-mesh connections. The arrays have to outlive the create call below,
		// which is why they live in this scope.
		FNavigationLinkArrays LinkArrays;
		LinkArrays.Fill(InGeometry.Links);

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
		LinkArrays.ApplyTo(Params);
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
		OutStats.NavMeshBytes = static_cast<UINT32>(NavDataSize);
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
		if (Crowd)
		{
			// The crowd borrows the navmesh, so it goes first.
			dtFreeCrowd(Crowd);
			Crowd = nullptr;
		}
		if (TileCache)
		{
			delete TileCache;
			TileCache = nullptr;
		}
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
	void ERecastNavigationProvider::CollectDebugTriangles(TArray<Vector3>& OutLineSegments) const
	{
		if ((!bReady) || (!NavMesh))
		{
			return;
		}

		// Same read only access as CollectDebugLines: the mutating getTile overload is private.
		const dtNavMesh* ReadOnlyNavMesh = NavMesh;
		const int MaxTiles = NavMesh->getMaxTiles();
		for (int TileIndex = 0; TileIndex < MaxTiles; TileIndex++)
		{
			const dtMeshTile* Tile = ReadOnlyNavMesh->getTile(TileIndex);
			if ((!Tile) || (!Tile->header) || (!Tile->polys) || (!Tile->verts)
				|| (!Tile->detailMeshes) || (!Tile->detailTris) || (!Tile->detailVerts))
			{
				continue;
			}

			for (int PolygonIndex = 0; PolygonIndex < Tile->header->polyCount; PolygonIndex++)
			{
				const dtPoly* Polygon = &Tile->polys[PolygonIndex];
				if (Polygon->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
				{
					// A link has no surface to triangulate.
					continue;
				}

				const dtPolyDetail* Detail = &Tile->detailMeshes[PolygonIndex];
				for (int TriangleIndex = 0; TriangleIndex < Detail->triCount; TriangleIndex++)
				{
					const unsigned char* Triangle = &Tile->detailTris[(Detail->triBase + TriangleIndex) * 4];
					Vector3 Corners[3];
					for (UINT32 CornerIndex = 0u; CornerIndex < 3u; CornerIndex++)
					{
						// A corner is either one of the polygon's own vertices or one of the
						// extra detail vertices stored after them in the same tile.
						const UINT32 Corner = Triangle[CornerIndex];
						if (Corner < Polygon->vertCount)
						{
							const float* Position = &Tile->verts[Polygon->verts[Corner] * 3u];
							Corners[CornerIndex] = Vector3(Position[0], Position[1], Position[2]);
						}
						else
						{
							const float* Position = &Tile->detailVerts[(Detail->vertBase + Corner - Polygon->vertCount) * 3u];
							Corners[CornerIndex] = Vector3(Position[0], Position[1], Position[2]);
						}
					}

					OutLineSegments.Add(Corners[0]);
					OutLineSegments.Add(Corners[1]);
					OutLineSegments.Add(Corners[1]);
					OutLineSegments.Add(Corners[2]);
					OutLineSegments.Add(Corners[2]);
					OutLineSegments.Add(Corners[0]);
				}
			}
		}
	}

}  // namespace PigeonEngine
