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
    private:
        EBoundAABB Bounds;

        // Render proxy functions START
    public:
        virtual void	CreateRenderState()override;
        virtual void	DestroyRenderState()override;
        virtual void	SendUpdateRenderState()override;
        // Render proxy functions END
    };

};