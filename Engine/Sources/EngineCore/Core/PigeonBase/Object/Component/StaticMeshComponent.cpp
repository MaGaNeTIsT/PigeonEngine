#include "StaticMeshComponent.h"
#include <RenderProxy/StaticMeshSceneProxy.h>

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
    RPrimitiveSceneProxy* PStaticMeshComponent::GetSceneProxy()
    {
        return SceneProxy;
    }
    const RPrimitiveSceneProxy* PStaticMeshComponent::GetSceneProxy()const
    {
        return SceneProxy;
    }
    RPrimitiveSceneProxy* PStaticMeshComponent::CreateSceneProxy()
    {
        Check((ENGINE_RENDER_CORE_ERROR), ("Try creating mesh scene proxy, but already exist scene proxy."), (!SceneProxy));
        SceneProxy = new RStaticMeshSceneProxy(this);
        return SceneProxy;
    }
    void PStaticMeshComponent::CreateRenderState()
    {

    }
    void PStaticMeshComponent::DestroyRenderState()
    {

    }
    void PStaticMeshComponent::SendUpdateRenderState()
    {

    }

}