#include "LineComponent.h"
namespace PigeonEngine
{
    static void RegisterClassTypes()
    {
        RegisterClassType<PLineComponent, PPrimitiveComponent>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PLineComponent::PLineComponent()
    {
        POBJ_DEBUGNAME_SET(this, "LineComponent");
    }
    PLineComponent::~PLineComponent()
    {
        
    }

    void PLineComponent::CreateRenderState()
    {
        PPrimitiveComponent::CreateRenderState();
    }

    void PLineComponent::DestroyRenderState()
    {
        PPrimitiveComponent::DestroyRenderState();
    }

    void PLineComponent::SendUpdateRenderState()
    {
        PPrimitiveComponent::SendUpdateRenderState();
    }

    void PLineComponent::MarkRenderTransformAsDirty()
    {
        PPrimitiveComponent::MarkRenderTransformAsDirty();
    }

    void PLineComponent::MarkRenderStateAsDirty()
    {
        PPrimitiveComponent::MarkRenderStateAsDirty();
    }

    void PLineComponent::CleanMarkRenderStateDirty()
    {
        PPrimitiveComponent::CleanMarkRenderStateDirty();
    }
}
