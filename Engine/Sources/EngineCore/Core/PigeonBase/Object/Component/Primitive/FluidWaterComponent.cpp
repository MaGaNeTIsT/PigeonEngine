#include "FluidWaterComponent.h"
#include <RenderProxy/FluidWaterSceneProxy.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<PFluidWaterComponent, PPrimitiveComponent>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PFluidWaterComponent::PFluidWaterComponent()
        : SceneProxy(nullptr), UpdateState(PFluidWaterUpdateState::FLUID_WATER_UPDATE_STATE_NONE)
    {
        POBJ_DEBUGNAME_SET(this, "FluidWaterComponent");
    }

    PFluidWaterComponent::~PFluidWaterComponent()
    {
        PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("SceneProxy is not null in fluid water component destruction."), (!SceneProxy));
    }

    void PFluidWaterComponent::SetMaterialAsset(const EMaterialAsset* InMaterialAsset)
    {
        PPrimitiveComponent::SetMaterialAsset(InMaterialAsset);
        MarkAsDirty(PFluidWaterUpdateState::FLUID_WATER_UPDATE_STATE_MATERIAL);
    }

    // Render proxy functions START
    UINT8 PFluidWaterComponent::GetUpdateRenderState()const
    {
        return UpdateState;
    }

    RFluidWaterSceneProxy* PFluidWaterComponent::CreateSceneProxy()
    {
        PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Try creating fluid water scene proxy, but already exist scene proxy."), (!SceneProxy));
        SceneProxy = new RFluidWaterSceneProxy(this);
        return SceneProxy;
    }

    void PFluidWaterComponent::CreateRenderState()
    {
        PPrimitiveComponent::CreateRenderState();
        if (ShouldRender())
        {
            this->GetWorld()->GetRenderScene()->AddFluidWater(this);
        }
    }

    void PFluidWaterComponent::DestroyRenderState()
    {
        this->GetWorld()->GetRenderScene()->RemoveFluidWater(this);
        PPrimitiveComponent::DestroyRenderState();
    }

    void PFluidWaterComponent::SendUpdateRenderState()
    {
        if (ShouldRender() && IsRenderStateDirty())
        {
            this->GetWorld()->GetRenderScene()->UpdateFluidWater(this);
        }
        PPrimitiveComponent::SendUpdateRenderState();
    }

    void PFluidWaterComponent::MarkAsDirty(PFluidWaterUpdateState InState)
    {
        if (InState == PFluidWaterUpdateState::FLUID_WATER_UPDATE_STATE_MATRIX)
        {
            MarkRenderTransformAsDirty();
        }
        else
        {
            UpdateState |= InState;
            MarkRenderStateAsDirty();
        }
    }

    void PFluidWaterComponent::MarkRenderTransformAsDirty()
    {
        UpdateState |= PFluidWaterUpdateState::FLUID_WATER_UPDATE_STATE_MATRIX;
        PPrimitiveComponent::MarkRenderTransformAsDirty();
    }

    void PFluidWaterComponent::MarkRenderStateAsDirty()
    {
        PPrimitiveComponent::MarkRenderStateAsDirty();
    }

    void PFluidWaterComponent::CleanMarkRenderStateDirty()
    {
        UpdateState = PFluidWaterUpdateState::FLUID_WATER_UPDATE_STATE_NONE;
        PPrimitiveComponent::CleanMarkRenderStateDirty();
    }
    // Render proxy functions END

}
