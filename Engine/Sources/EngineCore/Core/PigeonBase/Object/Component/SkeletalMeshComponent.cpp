#include "SkeletalMeshComponent.h"
#include <RenderProxy/SkeletalMeshSceneProxy.h>

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
        Check((ENGINE_RENDER_CORE_ERROR), ("SceneProxy is not null in mesh component distruction."), (!!SceneProxy));
    }
    RPrimitiveSceneProxy* PSkeletalMeshComponent::GetSceneProxy()
    {
        return SceneProxy;
    }
    const RPrimitiveSceneProxy* PSkeletalMeshComponent::GetSceneProxy()const
    {
        return SceneProxy;
    }
    RPrimitiveSceneProxy* PSkeletalMeshComponent::CreateSceneProxy()
    {
        Check((ENGINE_RENDER_CORE_ERROR), ("Try creating mesh scene proxy, but already exist scene proxy."), (!SceneProxy));
        SceneProxy = new RSkeletonMeshSceneProxy(this);
        return SceneProxy;
    }
    void PSkeletalMeshComponent::CreateRenderState()
    {

    }
    void PSkeletalMeshComponent::DestroyRenderState()
    {

    }
    void PSkeletalMeshComponent::SendUpdateRenderState()
    {

    }

}