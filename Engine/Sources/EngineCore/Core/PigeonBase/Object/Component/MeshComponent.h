#pragma once

#include <CoreMinimal.h>
#include "PrimitiveComponent.h"

namespace PigeonEngine
{

    class RPrimitiveSceneProxy;
    class RMeshSceneProxy;

    class PMeshComponent : public PPrimitiveComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PMeshComponent)


        // Render proxy functions START
    public:
        virtual RPrimitiveSceneProxy*       GetSceneProxy()override;
        virtual const RPrimitiveSceneProxy* GetSceneProxy()const override;
        virtual RPrimitiveSceneProxy*       CreateSceneProxy()override;
        virtual void    CreateRenderState()override;
        virtual void    DestroyRenderState()override;
        virtual void    SendUpdateRenderState()override;
    protected:
        RMeshSceneProxy*    SceneProxy;
        // Render proxy functions END
    };

};