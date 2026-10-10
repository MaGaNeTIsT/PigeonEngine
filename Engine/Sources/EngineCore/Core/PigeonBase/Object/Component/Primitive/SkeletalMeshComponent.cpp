#include "SkeletalMeshComponent.h"
#include <Asset/AssetLoadManager.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PSkeletalMeshComponent, PMeshComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PSkeletalMeshComponent::PSkeletalMeshComponent()
		: MeshAsset(nullptr), SkeletonAsset(nullptr)
	{
		POBJ_DEBUGNAME_SET(this, "SkeletalMeshComponent");
	}
	PSkeletalMeshComponent::~PSkeletalMeshComponent()
	{
		// Nothing to release here: the proxy belongs to the render scene and is deleted by the
		// remove command, so the component never holds a pointer to it.
	}
	const ESkinnedMeshAsset* PSkeletalMeshComponent::GetMeshAsset()const
	{
		return MeshAsset;
	}
	const EName& PSkeletalMeshComponent::GetRenderProxyTypeName() const
	{
		// Has to match the name RSkeletalMeshSceneProxy registers itself with, because that is
		// what the render side looks the proxy class up by.
		static const EName ProxyTypeName(PE_TEXT("SkeletalMeshPrimitive"));
		return ProxyTypeName;
	}
	void PSkeletalMeshComponent::CollectRenderAssets(FPrimitiveRenderAssetReport& InOutReport) const
	{
		// The mesh base reports the mesh and the material slots; a skinned primitive adds its
		// skeleton, which is what its skin streams are indexed against.
		PMeshComponent::CollectRenderAssets(InOutReport);
		InOutReport.SetSkeletonAsset(SkeletonAsset);
	}
	BOOL32 PSkeletalMeshComponent::BuildRenderSnapshot(FPrimitiveRenderSnapshot& OutSnapshot) const
	{
		if (!PMeshComponent::BuildRenderSnapshot(OutSnapshot))
		{
			return FALSE;
		}
		// The bone matrices are animation state, so they are re-reported with every snapshot
		// rather than cached: the pool is what the animation system writes into.
		const auto& BoneToRootTransforms = BoneMemoryPool.GetBoneToRootTransforms();
		const UINT32 MatrixNum = BoneToRootTransforms.Num<UINT32>();
		if (MatrixNum > 0u)
		{
			OutSnapshot.BoneToRootMatrices.SetNum(MatrixNum);
			for (UINT32 MatrixIndex = 0u; MatrixIndex < MatrixNum; MatrixIndex++)
			{
#if (_USE_MATRIX_FOR_BONE_TO_ROOT)
				OutSnapshot.BoneToRootMatrices[MatrixIndex] = BoneToRootTransforms[MatrixIndex];
#else
				OutSnapshot.BoneToRootMatrices[MatrixIndex] = BoneToRootTransforms[MatrixIndex].ToMatrix4x4();
#endif
			}
		}
		return TRUE;
	}
	void PSkeletalMeshComponent::SetMeshAsset(const ESkinnedMeshAsset* InMeshAsset)
	{
		MeshAsset = InMeshAsset;
		SetMeshAssetBase(InMeshAsset);
		if (!!InMeshAsset)
		{
			const ESkinnedMesh* Mesh = InMeshAsset->GetStoragedResource();
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Skinned mesh asset does not contain mesh resource."), (!!Mesh));
			if (Mesh)
#endif
			{
				SetLocalBound(Mesh->GetBoundAABB());
				SetMaterialSlotsNum(Mesh->GetSubmeshPart().Num<UINT32>());
			}
		}
		// A different mesh is an asset level change: the render side re-resolves the geometry and
		// the pass slots rather than only rewriting values.
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_ASSETS);
	}
	const ESkeletonAsset* PSkeletalMeshComponent::GetSkeletonAsset()const
	{
		return SkeletonAsset;
	}
	void PSkeletalMeshComponent::SetSkeletonAsset(const ESkeletonAsset* InSkeletonAsset)
	{
		BOOL32 NeedRegenerate = (SkeletonAsset != InSkeletonAsset);
		SkeletonAsset = InSkeletonAsset;
		if ((!!SkeletonAsset) && NeedRegenerate)
		{
			if (!!MeshAsset)
			{
				MeshAsset->SetSkeleton(SkeletonAsset->GetStoragedResource());
			}
			if (SkeletonAsset->IsResourceValid())
			{
				BoneMemoryPool.GenerateFromSkeleton(SkeletonAsset->GetStoragedResource());
			}
		}
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_ASSETS);
	}
	void PSkeletalMeshComponent::RequestSkeletalMeshAssets(const EString& InMeshAssetName, const EString& InSkeletonAssetName
		, const FAssetImportSource* InImportSource)
	{
		// Copied by value: the binding outlives this call.
		FAssetImportSource ImportSource;
		if (InImportSource)
		{
			ImportSource = *InImportSource;
		}
		PSkeletalMeshComponent* Component = this;
		TFunction<BOOL32()> Step = [Component, InMeshAssetName, InSkeletonAssetName, ImportSource]() -> BOOL32
		{
			const ESkinnedMeshAsset* MeshAsset = nullptr;
			const ESkeletonAsset* SkeletonAsset = nullptr;
			// Both are asked for on every attempt, so neither asset waits for the other.
			const BOOL32 bHasMesh = EMeshAssetManager::GetManagerSingleton()->RequireLoadSkinnedMeshAsset(
				EBaseSettings::ENGINE_MESH_PATH, InMeshAssetName, MeshAsset, &ImportSource);
			const BOOL32 bHasSkeleton = ESkeletonAssetManager::GetManagerSingleton()->RequireLoadSkeletonAsset(
				EBaseSettings::ENGINE_SKELETON_PATH, InSkeletonAssetName, SkeletonAsset, &ImportSource);
			if ((!bHasMesh) || (!bHasSkeleton))
			{
				return FALSE;	// not loaded yet: the component retries on its next tick
			}
			// Order matters and the component knows it: SetSkeletonAsset is what points the mesh at its
			// skeleton and rebuilds the bone pool, so the mesh goes in first. The pool's root transform
			// is what the pool is generated against, so it is set before that happens.
			Component->GetBoneMemoryPool().SetRootTransform(Vector3::Zero(), Quaternion::Identity(), Vector3::One());
			Component->SetMeshAsset(MeshAsset);
			Component->SetSkeletonAsset(SkeletonAsset);
			return TRUE;
		};
		// Tried once here as well, so an already loaded pair is applied before this call returns.
		if (Step() == FALSE)
		{
			AddDeferredApply(static_cast<TFunction<BOOL32()>&&>(Step));
		}
	}
	const ESkeletonBoneMemoryPool& PSkeletalMeshComponent::GetBoneMemoryPool()const
	{
		return BoneMemoryPool;
	}
	ESkeletonBoneMemoryPool& PSkeletalMeshComponent::GetBoneMemoryPool()
	{
		return BoneMemoryPool;
	}
	void PSkeletalMeshComponent::MarkBoneMemoryPoolDirty()
	{
		// Bone matrices are a value level change: the render side rewrites the bone buffers and
		// leaves the pass slots alone. Whoever writes the pool - the animation update - is the
		// one that has to call this, because nothing else can see that the matrices moved.
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PSkeletalMeshComponent::TickRenderImpl()
	{
		if (IsRenderStateDirty())
		{
			SendUpdateRenderState();
		}
		for (const auto& elem : ChildrenComponents)
		{
			elem->TickRender();
		}
	}

	// Render proxy functions START
	void PSkeletalMeshComponent::CreateRenderState()
	{
		PMeshComponent::CreateRenderState();
		if (ShouldRender())
		{
			this->GetWorld()->GetRenderScene()->AddSkeletalMesh(this);
		}
	}
	void PSkeletalMeshComponent::DestroyRenderState()
	{
		this->GetWorld()->GetRenderScene()->RemoveSkeletalMesh(this);
		PMeshComponent::DestroyRenderState();
	}
	void PSkeletalMeshComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			this->GetWorld()->GetRenderScene()->UpdateSkeletalMesh(this);
		}
		PMeshComponent::SendUpdateRenderState();
	}
	// Render proxy functions END

}
