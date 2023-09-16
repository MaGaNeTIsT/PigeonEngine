﻿#include "PrimitiveComponent.h"

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<PPrimitiveComponent, PSceneComponent>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PPrimitiveComponent::PPrimitiveComponent()
        : Bounds(EBoundAABB(Vector3::Zero(), Vector3::Zero(), FALSE)), IsCastShadow(FALSE), IsReceiveShadow(FALSE)
    {
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
        if (ShouldRender() && IsRenderStateDirty())
        {
            //TODO
        }
        PSceneComponent::SendUpdateRenderState();
    }
    void PPrimitiveComponent::MarkRenderTransformAsDirty()
    {
        //TODO
        PSceneComponent::MarkRenderTransformAsDirty();
    }
    void PPrimitiveComponent::MarkRenderStateAsDirty()
    {
        PSceneComponent::MarkRenderStateAsDirty();
    }
    void PPrimitiveComponent::CleanMarkRenderStateDirty()
    {
        //TODO
        PSceneComponent::CleanMarkRenderStateDirty();
    }
    // Render proxy functions END

};