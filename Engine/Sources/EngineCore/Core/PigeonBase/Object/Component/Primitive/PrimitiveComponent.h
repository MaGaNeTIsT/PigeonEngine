#pragma once
#include "EngineCommon.h"
#include <PigeonBase/Object/Component/SceneComponent.h>


namespace PigeonEngine
{

    class PPrimitiveComponent : public PSceneComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PPrimitiveComponent)

    public:
        PE_NODISCARD EBoundAABB GetBounds() const;
        PE_NODISCARD BOOL32 IsPrimitiveCastShadow() const;
        PE_NODISCARD BOOL32 IsPrimitiveReceiveShadow() const;
    public:
        void SetPrimitiveCastShadow(BOOL32 InIsCastShadow);
        void SetPrimitiveReceiveShadow(BOOL32 InIsReceiveShadow);
    private:
        EBoundAABB  Bounds;
        BOOL32      IsCastShadow;
        BOOL32      IsReceiveShadow;

        // Render proxy functions START
    public:
        virtual void	CreateRenderState()override;
        virtual void	DestroyRenderState()override;
        virtual void	SendUpdateRenderState()override;
        // Render proxy functions END
    };

};