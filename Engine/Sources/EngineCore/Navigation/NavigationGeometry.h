#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Navigation/NavigationTypes.h>

namespace PigeonEngine
{

	class PWorld;

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
		// Volume the backend should rasterize. An invalid bound means "derive it from the
		// triangles". A valid bound pins the voxel grid to the bake volume, so rebuilding
		// the same scene keeps the same cell layout even when geometry changes slightly.
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
		static void CollectStaticMeshTriangles(const PWorld& InWorld, const EBoundAABB& InBakeBounds, const BOOL32& InFlipTriangleWinding, ENavigationSourceGeometry& OutGeometry);
	};

}  // namespace PigeonEngine
