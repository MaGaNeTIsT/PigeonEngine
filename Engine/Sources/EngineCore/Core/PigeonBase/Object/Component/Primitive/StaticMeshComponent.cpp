#include "StaticMeshComponent.h"
#include <MeshAsset/MeshAsset.h>
#include <RenderProxy/StaticMeshSceneProxy.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<PStaticMeshComponent, PMeshComponent>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PStaticMeshComponent::PStaticMeshComponent()
        : MeshAsset(nullptr), SceneProxy(nullptr), UpdateState(PStaticMeshUpdateState::STATIC_MESH_UPDATE_STATE_NONE)
    {
        POBJ_DEBUGNAME_SET(this, "StaticMeshComponent");
    }
    PStaticMeshComponent::~PStaticMeshComponent()
    {
        PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("SceneProxy is not null in static mesh component distruction."), (!SceneProxy));
    }
    const EStaticMeshAsset* PStaticMeshComponent::GetMeshAsset()const
    {
        return MeshAsset;
    }
    void PStaticMeshComponent::SetMeshAsset(const EStaticMeshAsset* InMeshAsset)
    {
        MeshAsset = InMeshAsset;
        if (!!InMeshAsset)
        {
            const EStaticMesh* Mesh = InMeshAsset->GetStoragedResource();
#if _EDITOR_ONLY
            PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Static mesh asset does not contain mesh resource."), (!!Mesh));
            if (Mesh)
#endif
            {
                SetLocalBound(Mesh->GetBoundAABB());
            }
        }
        MarkAsDirty(PStaticMeshUpdateState::STATIC_MESH_UPDATE_STATE_ASSET);
    }

    // Render proxy functions START
    UINT8 PStaticMeshComponent::GetUpdateRenderState()const
    {
        return UpdateState;
    }
    RStaticMeshSceneProxy* PStaticMeshComponent::CreateSceneProxy()
    {
        PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Try creating static mesh scene proxy, but already exist scene proxy."), (!SceneProxy));
        SceneProxy = new RStaticMeshSceneProxy(this);
        return SceneProxy;
    }
    void PStaticMeshComponent::CreateRenderState()
    {
        PMeshComponent::CreateRenderState();
        if (ShouldRender())
        {
            this->GetWorld()->GetRenderScene()->AddStaticMesh(this);
        }
    }
    void PStaticMeshComponent::DestroyRenderState()
    {
        this->GetWorld()->GetRenderScene()->RemoveStaticMesh(this);
        PMeshComponent::DestroyRenderState();
    }
    void PStaticMeshComponent::SendUpdateRenderState()
    {
        if (ShouldRender() && IsRenderStateDirty())
        {
            this->GetWorld()->GetRenderScene()->UpdateStaticMesh(this);
        }
        PMeshComponent::SendUpdateRenderState();
    }
    void PStaticMeshComponent::MarkAsDirty(PStaticMeshUpdateState InState)
    {
        if (InState == PStaticMeshUpdateState::STATIC_MESH_UPDATE_STATE_MATRIX)
        {
            MarkRenderTransformAsDirty();
        }
        else
        {
            UpdateState |= InState;
            MarkRenderStateAsDirty();
        }
    }
    void PStaticMeshComponent::MarkRenderTransformAsDirty()
    {
        UpdateState |= PStaticMeshUpdateState::STATIC_MESH_UPDATE_STATE_MATRIX;
        PMeshComponent::MarkRenderTransformAsDirty();
    }
    void PStaticMeshComponent::MarkRenderStateAsDirty()
    {
        PMeshComponent::MarkRenderStateAsDirty();
    }
    void PStaticMeshComponent::CleanMarkRenderStateDirty()
    {
        UpdateState = PStaticMeshUpdateState::STATIC_MESH_UPDATE_STATE_NONE;
        PMeshComponent::CleanMarkRenderStateDirty();
    }
    // Render proxy functions END

}