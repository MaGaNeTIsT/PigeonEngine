#include "SkeletalMeshComponent.h"
#include <MeshAsset/MeshAsset.h>
#include <RenderProxy/SkeletalMeshSceneProxy.h>
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
        : MeshAsset(nullptr), SkeletonAsset(nullptr), SceneProxy(nullptr), UpdateState(PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_NONE)
    {
        POBJ_DEBUGNAME_SET(this, "SkeletalMeshComponent");
    }
    PSkeletalMeshComponent::~PSkeletalMeshComponent()
    {
        PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("SceneProxy is not null in skeletal mesh component distruction."), (!SceneProxy));
    }
    const ESkinnedMeshAsset* PSkeletalMeshComponent::GetMeshAsset()const
    {
        return MeshAsset;
    }
    void PSkeletalMeshComponent::SetMeshAsset(const ESkinnedMeshAsset* InMeshAsset)
    {
        MeshAsset = InMeshAsset;
        if (!!InMeshAsset)
        {
            const ESkinnedMesh* Mesh = InMeshAsset->GetStoragedResource();
#if _EDITOR_ONLY
            PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Skinned mesh asset does not contain mesh resource."), (!!Mesh));
            if (Mesh)
#endif
            {
                SetLocalBound(Mesh->GetBoundAABB());
            }
        }
        MarkAsDirty(PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_MESHASSET);
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
        MarkAsDirty(PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_SKELETONASSET);
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
        MarkAsDirty(PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_BONE_DATA);
    }

	void PSkeletalMeshComponent::TickRender()
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
    UINT8 PSkeletalMeshComponent::GetUpdateRenderState()const
    {
        return UpdateState;
    }
    RSkeletalMeshSceneProxy* PSkeletalMeshComponent::CreateSceneProxy()
    {
        PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Try creating skeletal mesh scene proxy, but already exist scene proxy."), (!SceneProxy));
        SceneProxy = new RSkeletalMeshSceneProxy(this);
        return SceneProxy;
    }
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
    void PSkeletalMeshComponent::MarkAsDirty(PSkeletalMeshUpdateState InState)
    {
        if (InState == PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_MATRIX)
        {
            MarkRenderTransformAsDirty();
        }
        else
        {
            UpdateState |= InState;
            MarkRenderStateAsDirty();
        }
    }
    void PSkeletalMeshComponent::MarkRenderTransformAsDirty()
    {
        UpdateState |= PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_MATRIX;
        PMeshComponent::MarkRenderTransformAsDirty();
    }
    void PSkeletalMeshComponent::MarkRenderStateAsDirty()
    {
        PMeshComponent::MarkRenderStateAsDirty();
    }
    void PSkeletalMeshComponent::CleanMarkRenderStateDirty()
    {
        UpdateState = PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_NONE;
        PMeshComponent::CleanMarkRenderStateDirty();
    }
    // Render proxy functions END

}