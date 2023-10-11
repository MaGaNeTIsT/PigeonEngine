#include "SkeletalMeshComponent.h"
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
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
        MarkAsDirty(PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_MESHASSET);
    }
    const ESkeletonAsset* PSkeletalMeshComponent::GetSkeletonAsset()const
    {
        return SkeletonAsset;
    }
    void PSkeletalMeshComponent::SetSkeletonAsset(const ESkeletonAsset* InSkeletonAsset)
    {
        SkeletonAsset = InSkeletonAsset;
        MarkAsDirty(PSkeletalMeshUpdateState::SKELETAL_MESH_UPDATE_STATE_SKELETONASSET);
    }
    void PSkeletalMeshComponent::AddSkeletonBoneDataRef(const EString& InBoneName, const Matrix4x4* InBoneMatrixPtr)
    {
        //TODO
    }
    void PSkeletalMeshComponent::RemoveSkeletonBoneDataRef(const EString& InBoneName)
    {
        //TODO
    }
    void PSkeletalMeshComponent::ClearSkeletonBoneDataRef()
    {
        //TODO
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