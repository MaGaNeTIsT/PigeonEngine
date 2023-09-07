#pragma once

#include <CoreMinimal.h>
#include "PrimitiveComponent.h"

namespace PigeonEngine
{

    class RPrimitiveSceneProxy;

    class PMeshComponent : public PPrimitiveComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PMeshComponent)

        // Render proxy functions START
    public:
        virtual void	CreateRenderState()override;
        virtual void	DestroyRenderState()override;
        virtual void	SendUpdateRenderState()override;
        // Render proxy functions END
    };

};