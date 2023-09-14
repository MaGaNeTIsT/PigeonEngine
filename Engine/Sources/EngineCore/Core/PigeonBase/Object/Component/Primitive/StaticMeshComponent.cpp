#include "StaticMeshComponent.h"
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
        : SceneProxy(nullptr)
    {
    }
    PStaticMeshComponent::~PStaticMeshComponent()
    {
        Check((ENGINE_RENDER_CORE_ERROR), ("SceneProxy is not null in mesh component distruction."), (!!SceneProxy));
    }
    RStaticMeshSceneProxy* PStaticMeshComponent::GetSceneProxy()
    {
        return SceneProxy;
    }
    const RStaticMeshSceneProxy* PStaticMeshComponent::GetSceneProxy()const
    {
        return SceneProxy;
    }
    RStaticMeshSceneProxy* PStaticMeshComponent::CreateSceneProxy()
    {
        Check((ENGINE_RENDER_CORE_ERROR), ("Try creating mesh scene proxy, but already exist scene proxy."), (!SceneProxy));
        SceneProxy = new RStaticMeshSceneProxy(this, GetMobility() != EMobilityType::EMT_STATIC, IsPrimitiveCastShadow(), IsPrimitiveReceiveShadow());
        return SceneProxy;
    }
    void PStaticMeshComponent::CreateRenderState()
    {
        PMeshComponent::CreateRenderState();
        if (ShouldRender())
        {
            PWorldManager::GetWorld()->GetRenderScene()->AddStaticMesh(this);
        }
    }
    void PStaticMeshComponent::DestroyRenderState()
    {
        PWorldManager::GetWorld()->GetRenderScene()->RemoveStaticMesh(this);
        PMeshComponent::DestroyRenderState();
    }
    void PStaticMeshComponent::SendUpdateRenderState()
    {
        if (ShouldRender() && IsRenderStateDirty())
        {
            PWorldManager::GetWorld()->GetRenderScene()->UpdateStaticMesh(this);
        }
        PMeshComponent::SendUpdateRenderState();
    }

}