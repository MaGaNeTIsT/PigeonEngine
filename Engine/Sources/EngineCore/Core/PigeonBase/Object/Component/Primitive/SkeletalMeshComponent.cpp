#include "SkeletalMeshComponent.h"
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
        : SceneProxy(nullptr)
    {
    }
    PSkeletalMeshComponent::~PSkeletalMeshComponent()
    {
        PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("SceneProxy is not null in mesh component distruction."), (!!SceneProxy));
    }
    RSkeletalMeshSceneProxy* PSkeletalMeshComponent::GetSceneProxy()
    {
        return SceneProxy;
    }
    const RSkeletalMeshSceneProxy* PSkeletalMeshComponent::GetSceneProxy()const
    {
        return SceneProxy;
    }
    RSkeletalMeshSceneProxy* PSkeletalMeshComponent::CreateSceneProxy()
    {
        PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Try creating mesh scene proxy, but already exist scene proxy."), (!SceneProxy));
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

}