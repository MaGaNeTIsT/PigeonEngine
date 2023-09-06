#include "MeshComponent.h"
#include <RenderProxy/MeshSceneProxy.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<PMeshComponent, PPrimitiveComponent>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PMeshComponent::PMeshComponent()
        : SceneProxy(nullptr)
    {
    }
    PMeshComponent::~PMeshComponent()
    {
        Check((ENGINE_RENDER_CORE_ERROR), ("SceneProxy is not null in mesh component distruction."), (!!SceneProxy));
    }
    RPrimitiveSceneProxy* PMeshComponent::GetSceneProxy()
    {
        return SceneProxy;
    }
    const RPrimitiveSceneProxy* PMeshComponent::GetSceneProxy()const
    {
        return SceneProxy;
    }
    RPrimitiveSceneProxy* PMeshComponent::CreateSceneProxy()
    {
        Check((ENGINE_RENDER_CORE_ERROR), ("Try creating mesh scene proxy, but already exist scene proxy."), (!SceneProxy));
        SceneProxy = new RMeshSceneProxy(this);
        return SceneProxy;
    }
    void PMeshComponent::CreateRenderState()
    {

    }
    void PMeshComponent::DestroyRenderState()
    {

    }
    void PMeshComponent::SendUpdateRenderState()
    {

    }

}