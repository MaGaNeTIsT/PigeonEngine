#pragma once
#include <CoreMinimal.h>
#include "PrimitiveComponent.h"
namespace PigeonEngine
{
    class PLineComponent : public PPrimitiveComponent
    {
        CLASS_VIRTUAL_NOCOPY_BODY(PLineComponent)
    public:
        
        
    protected:
        TArray<Vector3> Points;
        
        // Render proxy functions START
    public:
        virtual void	CreateRenderState()override;
        virtual void	DestroyRenderState()override;
        virtual void	SendUpdateRenderState()override;
    protected:
        virtual void    MarkRenderTransformAsDirty()override;
        virtual void    MarkRenderStateAsDirty()override;
        virtual void    CleanMarkRenderStateDirty()override;
        // Render proxy functions END
    };
}
