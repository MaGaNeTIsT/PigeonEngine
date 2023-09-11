#include "PrimitiveComponent.h"

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<PPrimitiveComponent, PSceneComponent>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PPrimitiveComponent::PPrimitiveComponent()
    {
        Bounds = EBoundAABB(Vector3(0), Vector3(0), false);
    }

    PPrimitiveComponent::~PPrimitiveComponent()
    {

    }

    EBoundAABB PPrimitiveComponent::GetBounds() const
    {
        return Bounds;
    }

    BOOL32 PPrimitiveComponent::IsPrimitiveCastShadow() const
    {
        return IsCastShadow;
    }

    BOOL32 PPrimitiveComponent::IsPrimitiveReceiveShadow() const
    {
        return IsReceiveShadow;
    }

    void PPrimitiveComponent::SetPrimitiveCastShadow(BOOL32 InIsCastShadow)
    {
        IsCastShadow = InIsCastShadow;
    }

    void PPrimitiveComponent::SetPrimitiveReceiveShadow(BOOL32 InIsReceiveShadow)
    {
        IsReceiveShadow = InIsReceiveShadow;
    }

    // Render proxy functions START
    void PPrimitiveComponent::CreateRenderState()
    {
        PSceneComponent::CreateRenderState();
        if (ShouldRender())
        {
            //TODO
        }
    }
    void PPrimitiveComponent::DestroyRenderState()
    {
        //TODO
        PSceneComponent::DestroyRenderState();
    }
    void PPrimitiveComponent::SendUpdateRenderState()
    {
        PSceneComponent::SendUpdateRenderState();
        if (ShouldRender())
        {
            //TODO
        }
    }
    // Render proxy functions END

};