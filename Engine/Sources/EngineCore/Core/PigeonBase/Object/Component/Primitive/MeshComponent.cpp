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
    {
    }
    PMeshComponent::~PMeshComponent()
    {
    }

    // Render proxy functions START
    void PMeshComponent::CreateRenderState()
    {
        PPrimitiveComponent::CreateRenderState();
        if (ShouldRender())
        {
            //TODO
        }
    }
    void PMeshComponent::DestroyRenderState()
    {
        //TODO
        PPrimitiveComponent::DestroyRenderState();
    }
    void PMeshComponent::SendUpdateRenderState()
    {
        if (ShouldRender() && IsRenderStateDirty())
        {
            //TODO
        }
        PPrimitiveComponent::SendUpdateRenderState();
    }
    void PMeshComponent::MarkRenderTransformAsDirty()
    {
        //TODO
        PPrimitiveComponent::MarkRenderTransformAsDirty();
    }
    void PMeshComponent::MarkRenderStateAsDirty()
    {
        PPrimitiveComponent::MarkRenderStateAsDirty();
    }
    void PMeshComponent::CleanMarkRenderStateDirty()
    {
        //TODO
        PPrimitiveComponent::CleanMarkRenderStateDirty();
    }
    // Render proxy functions END

}