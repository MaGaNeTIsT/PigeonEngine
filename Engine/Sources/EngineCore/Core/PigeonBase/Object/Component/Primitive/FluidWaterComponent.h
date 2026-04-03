#pragma once

#include <CoreMinimal.h>
#include "PrimitiveComponent.h"

namespace PigeonEngine
{

    class EMaterialAsset;
    class RFluidWaterSceneProxy;

    class PFluidWaterComponent : public PPrimitiveComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PFluidWaterComponent)

    public:
        virtual void                SetMaterialAsset(const EMaterialAsset* InMaterialAsset)override;

    protected:
        // Render proxy functions START
    public:
        enum PFluidWaterUpdateState : UINT8
        {
            FLUID_WATER_UPDATE_STATE_NONE     = 0,
            FLUID_WATER_UPDATE_STATE_MATRIX   = (1 << 0),
            FLUID_WATER_UPDATE_STATE_MATERIAL = (1 << 1)
        };
    public:
        RFluidWaterSceneProxy*  SceneProxy;
    public:
        UINT8                   GetUpdateRenderState()const;
        RFluidWaterSceneProxy*  CreateSceneProxy();
        virtual void            CreateRenderState()override;
        virtual void            DestroyRenderState()override;
        virtual void            SendUpdateRenderState()override;
    protected:
        void                    MarkAsDirty(PFluidWaterUpdateState InState);
        virtual void            MarkRenderTransformAsDirty()override;
        virtual void            MarkRenderStateAsDirty()override;
        virtual void            CleanMarkRenderStateDirty()override;
    protected:
        UINT8                   UpdateState;
        // Render proxy functions END
    };

};
