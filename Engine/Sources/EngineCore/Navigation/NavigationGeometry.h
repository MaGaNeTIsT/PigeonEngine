#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Navigation/NavigationTypes.h>

namespace PigeonEngine
{

	class PWorld;
	class PSceneComponent;

	// One world space triangle handed to a navigation backend. Vertices are in engine units
	// (centimeters) so a backend never has to know about component or actor transforms.
	struct ENavigationSourceTriangle
	{
	public:
		Vector3	Vertex0 = Vector3::Zero();
		Vector3	Vertex1 = Vector3::Zero();
		Vector3	Vertex2 = Vector3::Zero();
		// Class of the surface this triangle belongs to, taken from the component that owns
		// it. The bake stamps it on the polygons the triangle produces, and the query filter
		// selects polygons by it.
		ENavigationAreaFlagType	Area = ENavigationAreaFlagType::NAVIGATION_AREA_GROUND;
	};

	struct ENavigationSourceGeometry
	{
	public:
		TArray<ENavigationSourceTriangle>	Triangles;
		// Authored off-mesh connections. They are not surface geometry: the backend stores
		// them next to the polygons and links each end to the closest polygon it finds.
		TArray<ENavigationLink>				Links;
		/*
		* Coverage volumes the backend should stay inside. Empty means "no restriction": the
		* triangles are all there is. A tiled backend skips every tile no volume covers, which
		* is what keeps the gaps between disjoint volumes out of the bake; the single tile
		* path cannot clip and rasterizes the union instead.
		*/
		TArray<EBoundAABB>					Volumes;
		// Union of Volumes, or invalid when there are none. Every backend needs the grid box
		// even when it ignores the per-volume list, so the union is carried next to it.
		EBoundAABB	Bounds = EBoundAABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), FALSE);
		// Static mesh components whose cold data was missing, or that produced no
		// triangles. Reported to the caller instead of being silently dropped.
		UINT32		SkippedMeshCount = 0;
	};

	/*
	* Collects world space triangles from the world.
	*
	* Only static mesh components take part. Skinned meshes are excluded because their cold
	* data is the bind pose, and the procedural components (grass, water) because their
	* geometry is generated for rendering, not for collision. Components that are not
	* rendered are excluded as well: hiding a helper mesh in the editor is the intended way
	* to keep it out of the navmesh. Every emitted triangle carries the owning component's
	* NavigationArea, so one mesh can mark a surface as water, a door or a jump.
	*
	* A component takes part when its world bound intersects any of InVolumes. An empty list
	* means "no restriction" and collects the whole world, which is what a level without
	* bounds volumes bakes.
	*
	* InFlipTriangleWinding reverses every emitted triangle. Recast decides walkability from
	* the normal of (v1 - v0) x (v2 - v0), which has to point up. The engine renders with
	* Direct3D's left handed clockwise front faces and imports through assimp with
	* aiProcess_MakeLeftHanded plus aiProcess_FlipWindingOrder, so whether an imported mesh
	* needs the flip is a property of that chain, measured once against the demo level. It is
	* deliberately a whole mesh switch: flipping only the triangles that face down would
	* make ceilings walkable.
	*/
	class ENavigationGeometryCollector
	{
	public:
		// Component local bound transformed by the component's world transform, expressed as a
		// world space axis aligned box. Rotation and scale are folded in, so a rotated mesh
		// reports the box its geometry actually occupies. Shared with the bounds volume
		// component, which describes its region the same way.
		static EBoundAABB MakeComponentWorldBound(const PSceneComponent& InComponent, const EBoundAABB& InLocalBound);

		// Appends every enabled bounds volume of the given navmesh group. An empty name is the
		// level's default navmesh. Volumes with any non-positive extent are skipped.
		static void CollectVolumeBounds(const PWorld& InWorld, const EString& InNavMeshName, TArray<EBoundAABB>& OutVolumes);

		static void CollectStaticMeshTriangles(const PWorld& InWorld, const TArray<EBoundAABB>& InVolumes, const BOOL32& InFlipTriangleWinding, ENavigationSourceGeometry& OutGeometry);
	};

}  // namespace PigeonEngine
