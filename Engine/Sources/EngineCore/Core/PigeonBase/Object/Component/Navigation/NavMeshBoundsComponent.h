#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

	/*
	* One region a navmesh is baked for - the engine's NavMeshBoundsVolume.
	*
	* The component carries no settings and owns no navigation data: it only tells the
	* PNavMeshComponent of the same NavMeshName which parts of the world its bake covers.
	* Every volume of one name is baked into a single navmesh, so overlapping or touching
	* volumes are the authored way to make one mesh span several areas.
	*
	* The box is a half extent in component local space; the world box is the axis aligned
	* bound of that box under the component's world transform, so rotation and scale are
	* honored. Any axis left at zero makes the volume invalid: it contributes nothing and
	* the editor draws it red, which is the signal that its authoring is incomplete.
	*/
	class PNavMeshBoundsComponent final : public PSceneComponent
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PNavMeshBoundsComponent)
	public:
		// Group of the navmesh this volume feeds. Empty is the level's default navmesh.
		PE_NODISCARD const EString& GetNavMeshName() const;
		void SetNavMeshName(const EString& InName);

		// Half size of the box in component local space. Each axis is clamped to >= 0.
		PE_NODISCARD const Vector3& GetExtent() const;
		void SetExtent(const Vector3& InExtent);

		// World space box this volume contributes. IsValid is FALSE when any extent axis is 0.
		PE_NODISCARD EBoundAABB MakeWorldBound() const;

#if ENGINE_EDITOR_ONLY
	public:
		virtual void EditorTick(FLOAT InDeltaTime) override;
#endif

	private:
		EString	NavMeshName;
		Vector3	Extent = Vector3(500.0f, 250.0f, 500.0f);
	};

}  // namespace PigeonEngine
