#include <Navigation/NavigationGeometry.h>

#include <PigeonBase/Object/World/World.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <PigeonBase/Object/Component/Navigation/NavMeshBoundsComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <MeshAsset/MeshAsset.h>

namespace PigeonEngine
{

	// Triangles below this area (0.01 cm^2, one square millimeter) cannot cover a voxel and
	// only feed Recast near degenerate normals, so they are dropped before rasterization.
	// The value is compared against a squared cross product, which is twice the area.
	PE_CONSTEXPR_STATIC FLOAT NavigationMinCrossLengthSquared = 4.0e-4f;

	EBoundAABB ENavigationGeometryCollector::MakeComponentWorldBound(const PSceneComponent& InComponent, const EBoundAABB& InLocalBound)
	{
		// CalculateBoundWithSpace is a non-const member that writes into the caller's corner
		// array, so the local copy acts as the input.
		EBoundAABB LocalBound = InLocalBound;
		TArray<Vector3> Corners;
		LocalBound.CalculateBoundWithSpace(InComponent.GetComponentWorldLocation(), InComponent.GetComponentWorldRotation(), InComponent.GetComponentWorldScale(), Corners);

		if (Corners.Num<UINT32>() < 8u)
		{
			return EBoundAABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), FALSE);
		}

		Vector3 Minimum = Corners[0];
		Vector3 Maximum = Corners[0];
		for (UINT32 Index = 1u; Index < 8u; Index++)
		{
			Minimum.x = EMath::Min(Minimum.x, Corners[Index].x);
			Minimum.y = EMath::Min(Minimum.y, Corners[Index].y);
			Minimum.z = EMath::Min(Minimum.z, Corners[Index].z);
			Maximum.x = EMath::Max(Maximum.x, Corners[Index].x);
			Maximum.y = EMath::Max(Maximum.y, Corners[Index].y);
			Maximum.z = EMath::Max(Maximum.z, Corners[Index].z);
		}
		return EBoundAABB(Minimum, Maximum, TRUE);
	}

	// An invalid bound on either side means "unbounded", which keeps a bake volume that the
	// caller left open from silently rejecting geometry.
	static BOOL32 BoundsIntersect(const EBoundAABB& InA, const EBoundAABB& InB)
	{
		if ((!InA.IsValid) || (!InB.IsValid))
		{
			return TRUE;
		}
		return ((InA.AABBMin.x <= InB.AABBMax.x) && (InA.AABBMax.x >= InB.AABBMin.x)
			&& (InA.AABBMin.y <= InB.AABBMax.y) && (InA.AABBMax.y >= InB.AABBMin.y)
			&& (InA.AABBMin.z <= InB.AABBMax.z) && (InA.AABBMax.z >= InB.AABBMin.z));
	}

	// No volumes means "no restriction": with no authored coverage every mesh in the world is
	// a candidate, which is the legacy whole-world bake.
	static BOOL32 IntersectsAnyVolume(const EBoundAABB& InBound, const TArray<EBoundAABB>& InVolumes)
	{
		if (InVolumes.Num<UINT32>() == 0u)
		{
			return TRUE;
		}
		for (UINT32 Index = 0u, Num = InVolumes.Num<UINT32>(); Index < Num; Index++)
		{
			if (BoundsIntersect(InBound, InVolumes[Index]))
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	// x, y, z of one vertex out of an interleaved float stream. The importer stores positions
	// with a 16 byte stride and a padding float, so the stride has to be honored.
	static Vector3 ReadVertexPosition(const EVertexData& InPositions, UINT32 InVertexIndex)
	{
		const UINT8* BasePointer = reinterpret_cast<const UINT8*>(InPositions.Datas);
		const FLOAT* ValuePointer = reinterpret_cast<const FLOAT*>(BasePointer + (static_cast<SIZE_T>(InVertexIndex) * InPositions.Stride));
		return Vector3(ValuePointer[0], ValuePointer[1], ValuePointer[2]);
	}

	static BOOL32 IsDegenerateTriangle(const Vector3& InVertex0, const Vector3& InVertex1, const Vector3& InVertex2)
	{
		const FLOAT Edge0X = InVertex1.x - InVertex0.x;
		const FLOAT Edge0Y = InVertex1.y - InVertex0.y;
		const FLOAT Edge0Z = InVertex1.z - InVertex0.z;
		const FLOAT Edge1X = InVertex2.x - InVertex0.x;
		const FLOAT Edge1Y = InVertex2.y - InVertex0.y;
		const FLOAT Edge1Z = InVertex2.z - InVertex0.z;

		const FLOAT CrossX = (Edge0Y * Edge1Z) - (Edge0Z * Edge1Y);
		const FLOAT CrossY = (Edge0Z * Edge1X) - (Edge0X * Edge1Z);
		const FLOAT CrossZ = (Edge0X * Edge1Y) - (Edge0Y * Edge1X);
		const FLOAT CrossLengthSquared = (CrossX * CrossX) + (CrossY * CrossY) + (CrossZ * CrossZ);
		return (CrossLengthSquared < NavigationMinCrossLengthSquared);
	}

	static void CollectComponentTriangles(const PStaticMeshComponent& InComponent, const TArray<EBoundAABB>& InVolumes, const BOOL32& InFlipTriangleWinding, ENavigationSourceGeometry& InOutGeometry)
	{
		const EStaticMeshAsset* MeshAsset = InComponent.GetMeshAsset();
		const EStaticMesh* Mesh = (!!MeshAsset) ? MeshAsset->GetStoragedResource() : nullptr;
		if (!Mesh)
		{
			// The asset is still loading, or the request failed. The caller retries the build,
			// so this is a skip now rather than a hole in the baked mesh.
			InOutGeometry.SkippedMeshCount++;
			return;
		}

		const EBoundAABB ComponentWorldBound = ENavigationGeometryCollector::MakeComponentWorldBound(InComponent, InComponent.GetLocalBound());
		if (!IntersectsAnyVolume(ComponentWorldBound, InVolumes))
		{
			return;
		}

		const EIndexData* IndexData = nullptr;
		const EVertexData* PositionData = nullptr;
		if ((!Mesh->GetIndexElement(IndexData)) || (!Mesh->GetVertexElement(EVertexLayoutType::MESH_VERTEX, 0u, PositionData)))
		{
			InOutGeometry.SkippedMeshCount++;
			return;
		}
		if ((!!IndexData) && (!!PositionData))
		{
			const UINT32 VertexNum = PositionData->ElementNum;
			const UINT32 IndexNum = IndexData->ElementNum;
			if ((VertexNum == 0u) || (IndexNum < 3u) || (PositionData->Stride < (3u * sizeof(FLOAT))) || (!IndexData->Indices) || (!PositionData->Datas))
			{
				InOutGeometry.SkippedMeshCount++;
				return;
			}

			const Matrix4x4 LocalToWorld = MakeMatrix4x4(InComponent.GetComponentWorldLocation(), InComponent.GetComponentWorldRotation(), InComponent.GetComponentWorldScale());
			const EMesh::ESubmeshPart& Submeshes = Mesh->GetSubmeshPart();
			for (UINT32 SubmeshIndex = 0u, SubmeshNum = Submeshes.Num<UINT32>(); SubmeshIndex < SubmeshNum; SubmeshIndex++)
			{
				const ESubmeshData& Submesh = Submeshes[SubmeshIndex];
				const UINT32 SubmeshEndIndex = Submesh.StartIndex + Submesh.IndexNum;
				for (UINT32 Index = Submesh.StartIndex; (Index + 2u) < SubmeshEndIndex; Index += 3u)
				{
					const UINT32 VertexIndex0 = IndexData->Indices[Index];
					const UINT32 VertexIndex1 = IndexData->Indices[Index + 1u];
					const UINT32 VertexIndex2 = IndexData->Indices[Index + 2u];
					if ((VertexIndex0 >= VertexNum) || (VertexIndex1 >= VertexNum) || (VertexIndex2 >= VertexNum))
					{
						continue;
					}

					const Vector3 LocalVertex0 = ReadVertexPosition(*PositionData, VertexIndex0);
					const Vector3 LocalVertex1 = ReadVertexPosition(*PositionData, VertexIndex1);
					const Vector3 LocalVertex2 = ReadVertexPosition(*PositionData, VertexIndex2);

					ENavigationSourceTriangle Triangle;
					Triangle.Vertex0 = Matrix4x4TransformPosition(LocalToWorld, LocalVertex0);
					Triangle.Vertex1 = Matrix4x4TransformPosition(LocalToWorld, LocalVertex1);
					Triangle.Vertex2 = Matrix4x4TransformPosition(LocalToWorld, LocalVertex2);
					Triangle.Area = InComponent.GetNavigationArea();
					if (IsDegenerateTriangle(Triangle.Vertex0, Triangle.Vertex1, Triangle.Vertex2))
					{
						continue;
					}
					// The whole mesh shares one winding convention, so the flip is applied here
					// rather than per triangle; see ENavigationGeometryCollector.
					if (InFlipTriangleWinding)
					{
						const Vector3 TempVertex = Triangle.Vertex1;
						Triangle.Vertex1 = Triangle.Vertex2;
						Triangle.Vertex2 = TempVertex;
					}
					InOutGeometry.Triangles.Add(Triangle);
				}
			}
		}
	}

	// Walks one actor's component tree. The recursion stops at a component owned by another
	// actor: an attached child actor's root component is a child of the parent's root
	// component, and that child actor is visited on its own, so descending would collect its
	// geometry twice.
	static void CollectActorTriangles(const PActor& InActor, const PSceneComponent& InComponent, const TArray<EBoundAABB>& InVolumes, const BOOL32& InFlipTriangleWinding, ENavigationSourceGeometry& InOutGeometry)
	{
		if (InComponent.GetOwnerActor() != (&InActor))
		{
			return;
		}

		const PStaticMeshComponent* MeshComponent = dynamic_cast<const PStaticMeshComponent*>(&InComponent);
		if ((!!MeshComponent) && (MeshComponent->ShouldRender()) && (MeshComponent->IsAffectsNavigation()))
		{
			CollectComponentTriangles(*MeshComponent, InVolumes, InFlipTriangleWinding, InOutGeometry);
		}

		const TArray<PSceneComponent*> Children = InComponent.GetChildrenComponents();
		for (UINT32 ChildIndex = 0u, ChildNum = Children.Num<UINT32>(); ChildIndex < ChildNum; ChildIndex++)
		{
			PSceneComponent* ChildComponent = Children[ChildIndex];
			if (!!ChildComponent)
			{
				CollectActorTriangles(InActor, *ChildComponent, InVolumes, InFlipTriangleWinding, InOutGeometry);
			}
		}
	}

	void ENavigationGeometryCollector::CollectStaticMeshTriangles(const PWorld& InWorld, const TArray<EBoundAABB>& InVolumes, const BOOL32& InFlipTriangleWinding, ENavigationSourceGeometry& OutGeometry)
	{
		OutGeometry.Triangles.Empty();
		OutGeometry.SkippedMeshCount = 0u;
		OutGeometry.Volumes = InVolumes;
		// The union is what a backend uses to size its grid even when it ignores the list, so
		// it is derived here instead of at every call site.
		OutGeometry.Bounds = EBoundAABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), FALSE);
		for (UINT32 VolumeIndex = 0u, VolumeNum = InVolumes.Num<UINT32>(); VolumeIndex < VolumeNum; VolumeIndex++)
		{
			if (InVolumes[VolumeIndex].IsValid)
			{
				OutGeometry.Bounds += InVolumes[VolumeIndex];
			}
		}

		const TArray<PActor*> Actors = InWorld.GetAllActors();
		for (UINT32 ActorIndex = 0u, ActorNum = Actors.Num<UINT32>(); ActorIndex < ActorNum; ActorIndex++)
		{
			const PActor* Actor = Actors[ActorIndex];
			if (!Actor)
			{
				continue;
			}
			const PSceneComponent* RootComponent = Actor->GetRootComponent();
			if (!RootComponent)
			{
				continue;
			}
			CollectActorTriangles(*Actor, *RootComponent, InVolumes, InFlipTriangleWinding, OutGeometry);
		}
	}

	// Same walk as the triangle collection: a bounds volume may sit anywhere in an actor's
	// scene component tree, and a child actor is visited on its own so its components are not
	// counted twice.
	static void CollectActorVolumeBounds(const PActor& InActor, const PSceneComponent& InComponent, const EString& InNavMeshName, TArray<EBoundAABB>& OutVolumes)
	{
		if (InComponent.GetOwnerActor() != (&InActor))
		{
			return;
		}

		const PNavMeshBoundsComponent* VolumeComponent = dynamic_cast<const PNavMeshBoundsComponent*>(&InComponent);
		if ((!!VolumeComponent) && (VolumeComponent->SupportsNavMesh(InNavMeshName)))
		{
			const EBoundAABB WorldBound = VolumeComponent->MakeWorldBound();
			if (WorldBound.IsValid)
			{
				OutVolumes.Add(WorldBound);
			}
		}

		const TArray<PSceneComponent*> Children = InComponent.GetChildrenComponents();
		for (UINT32 ChildIndex = 0u, ChildNum = Children.Num<UINT32>(); ChildIndex < ChildNum; ChildIndex++)
		{
			PSceneComponent* ChildComponent = Children[ChildIndex];
			if (!!ChildComponent)
			{
				CollectActorVolumeBounds(InActor, *ChildComponent, InNavMeshName, OutVolumes);
			}
		}
	}

	void ENavigationGeometryCollector::CollectVolumeBounds(const PWorld& InWorld, const EString& InNavMeshName, TArray<EBoundAABB>& OutVolumes)
	{
		const TArray<PActor*> Actors = InWorld.GetAllActors();
		for (UINT32 ActorIndex = 0u, ActorNum = Actors.Num<UINT32>(); ActorIndex < ActorNum; ActorIndex++)
		{
			const PActor* Actor = Actors[ActorIndex];
			if (!Actor)
			{
				continue;
			}
			const PSceneComponent* RootComponent = Actor->GetRootComponent();
			if (!RootComponent)
			{
				continue;
			}
			CollectActorVolumeBounds(*Actor, *RootComponent, InNavMeshName, OutVolumes);
		}
	}

}  // namespace PigeonEngine
