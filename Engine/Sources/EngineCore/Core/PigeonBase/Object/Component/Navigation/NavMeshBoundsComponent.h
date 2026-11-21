#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

	/*
	* One region a navmesh is baked for - the engine's NavMeshBoundsVolume.
	*
	* The component carries no settings and owns no navigation data: it tells every navmesh
	* profile whose name it lists (or all of them, when the list is empty) that this part of
	* the world is walkable. One volume therefore feeds the default agent and a large agent
	* alike; the profiles differ in their bake settings, not in the region data.
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
		// Navmesh profiles this region feeds. Empty means every profile: the usual case, one
		// volume set for all agent sizes.
		PE_NODISCARD const TArray<EString>& GetSupportedNavMeshes() const;
		void SetSupportedNavMeshes(const TArray<EString>& InProfiles);
		// TRUE when this volume contributes to the profile (empty list = all profiles).
		PE_NODISCARD BOOL32 SupportsNavMesh(const EString& InNavMeshName) const;

		// Half size of the box in component local space. Each axis is clamped to >= 0.
		PE_NODISCARD const Vector3& GetExtent() const;
		void SetExtent(const Vector3& InExtent);

		// World space box this volume contributes. IsValid is FALSE when any extent axis is 0.
		PE_NODISCARD EBoundAABB MakeWorldBound() const;

	protected:
		virtual void	BeginAddedToScene(PWorld* InWorld) override;
		virtual void	RemovedFromScene() override;

#if ENGINE_EDITOR_ONLY
	public:
		virtual void EditorTick(FLOAT InDeltaTime) override;
#endif

	private:
		TArray<EString>	SupportedNavMeshes;
		Vector3	Extent = Vector3(500.0f, 250.0f, 500.0f);
		// Last transform the editor saw. A move has no other notification hook, so the tick
		// compares against it and bumps the manager's input revision when it changed.
		BOOL32		bHasLastTickTransform = FALSE;
		Vector3		LastTickLocation = Vector3::Zero();
		Quaternion	LastTickRotation;
		Vector3		LastTickScale = Vector3(1.0f, 1.0f, 1.0f);
	};

}  // namespace PigeonEngine
