#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <Navigation/NavigationTypes.h>

namespace PigeonEngine
{

	struct ENavigationSourceGeometry;

	/*
	* Contract between the engine and a navigation backend.
	*
	* Everything crossing this interface is an engine type: no Recast, Detour or other
	* library type appears here, which is what lets gameplay and editor code stay unaware of
	* which backend is behind a navmesh. A second backend only has to implement this
	* interface, register a factory under its name in ENavigationProviderRegistry, and derive
	* a component from PNavigationComponent.
	*
	* Thread affinity: a provider is built and queried on the game thread. Bake is expected
	* to be synchronous; a provider that moves work to a worker reports IsReady() == FALSE
	* until the result is in place.
	*/
	class ENavigationProviderInterface
	{
	public:
		virtual ~ENavigationProviderInterface() {}

		// Replaces every existing polygon with the ones generated from InGeometry. A failed
		// build leaves the provider in whatever state it was in before the call, so a
		// caller can retry without losing the previous mesh.
		virtual BOOL32 Build(const ENavigationSourceGeometry& InGeometry, ENavigationBuildStats& OutStats) = 0;
		// TRUE once a build succeeded and queries can be answered.
		virtual BOOL32 IsReady() const = 0;
		// World space volume the provider covers. Used to pick a provider for a location.
		virtual const EBoundAABB& GetBounds() const = 0;

		// Projects a point onto the closest polygon inside the search box. InHalfExtents is
		// the search box half size, in engine units.
		virtual BOOL32 FindNearestPoint(const Vector3& InLocation, const Vector3& InHalfExtents, const ENavigationQueryFilter& InFilter, ENavigationProjectionResult& OutResult) const = 0;
		// Start and End are world positions; the provider projects them onto the mesh. A
		// reachable destination produces points from the projected start to the projected
		// destination, a blocked one a partial path up to the closest reachable polygon.
		virtual BOOL32 FindPath(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationPath& OutPath) const = 0;
		virtual BOOL32 ProjectPoint(const Vector3& InLocation, const ENavigationQueryFilter& InFilter, ENavigationProjectionResult& OutResult) const = 0;
		// Walks the mesh surface between two points and stops at the first wall.
		virtual BOOL32 Raycast(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationRaycastResult& OutResult) const = 0;
		virtual BOOL32 MoveAlongSurface(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, Vector3& OutLocation) const = 0;

		// Appends world space line segments, two entries per segment. The backend writes
		// positions only; the editor decides colors and how to draw them.
		virtual void CollectDebugLines(ENavigationDebugModeType InMode, TArray<Vector3>& OutLineSegments) const = 0;
	};

}  // namespace PigeonEngine
