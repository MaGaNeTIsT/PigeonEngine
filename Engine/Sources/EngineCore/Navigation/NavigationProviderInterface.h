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

		// Bit set of ENavigationCapabilityType. Read by gameplay and the editor before offering
		// an operation the backend may not implement.
		virtual UINT32 GetCapabilities() const = 0;

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
		/*
		* Appends the walkable surface's triangulation, two entries per segment: three edges for
		* every triangle the backend draws it with. A backend without a triangulated
		* representation leaves the list empty, which simply draws no triangles.
		*/
		virtual void CollectDebugTriangles(TArray<Vector3>& OutLineSegments) const { (void)OutLineSegments; }

		/*
		* Per frame work, driven by ENavigationManager::Tick before the world ticks. Local avoidance
		* and dynamic obstacle rebuilds happen here; a backend with neither does nothing.
		*/
		virtual void Tick(FLOAT InDeltaTime) { (void)InDeltaTime; }
		/*
		* Changes whenever the polygons change without a new build: a rebuilt tile, for example.
		* The editor caches its debug lines against this, so an obstacle that bent the mesh shows up
		* without rebuilding the overlay every frame.
		*/
		virtual UINT32 GetContentRevision() const { return 0u; }

		/*
		* Runtime obstacles. A backend without NAVIGATION_CAPABILITY_OBSTACLES returns
		* NavigationInvalidObstacleHandle. The handles are only valid for the provider that created
		* them; a rebuilt provider starts with none.
		*/
		virtual ENavigationObstacleHandle AddCylinderObstacle(const Vector3& InCenter, const FLOAT& InRadius, const FLOAT& InHeight)
		{
			(void)InCenter; (void)InRadius; (void)InHeight;
			return NavigationInvalidObstacleHandle;
		}
		virtual ENavigationObstacleHandle AddBoxObstacle(const Vector3& InCenter, const Vector3& InHalfExtents, const FLOAT& InYawRadians)
		{
			(void)InCenter; (void)InHalfExtents; (void)InYawRadians;
			return NavigationInvalidObstacleHandle;
		}
		virtual void RemoveObstacle(ENavigationObstacleHandle InHandle) { (void)InHandle; }

		/*
		* Local avoidance. A backend with NAVIGATION_CAPABILITY_CROWD registers agents here and
		* steers them itself; the caller only reads back the resulting pose. Detour's crowd is the
		* reference implementation, which is why the settings type is the same one a directly moved
		* agent uses.
		*/
		virtual ENavigationCrowdAgentHandle AddCrowdAgent(const Vector3& InPosition, const ENavigationAgentSettings& InSettings)
		{
			(void)InPosition; (void)InSettings;
			return NavigationInvalidCrowdAgentHandle;
		}
		virtual void RemoveCrowdAgent(ENavigationCrowdAgentHandle InHandle) { (void)InHandle; }
		virtual BOOL32 RequestCrowdAgentMove(ENavigationCrowdAgentHandle InHandle, const Vector3& InTarget)
		{
			(void)InHandle; (void)InTarget;
			return FALSE;
		}
		virtual void StopCrowdAgent(ENavigationCrowdAgentHandle InHandle) { (void)InHandle; }
		virtual BOOL32 GetCrowdAgentState(ENavigationCrowdAgentHandle InHandle, ENavigationCrowdAgentState& OutState) const
		{
			(void)InHandle; (void)OutState;
			return FALSE;
		}

		/*
		* Persistence. The bytes are the backend's own container: the asset stores them verbatim
		* next to the provider name, so a backend swap only changes which name the asset names.
		* ExportData fails when there is nothing ready to export; ImportData may be called on a
		* fresh provider and must leave it unready when the data does not validate.
		*/
		virtual BOOL32 ExportData(TArray<UINT8>& OutData) const = 0;
		virtual BOOL32 ImportData(const UINT8* InData, UINT32 InSize) = 0;
	};

}  // namespace PigeonEngine
