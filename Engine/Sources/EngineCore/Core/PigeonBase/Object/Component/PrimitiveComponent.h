#pragma once
#include "EngineCommon.h"
#include "SceneComponent.h"


namespace PigeonEngine
{
    class PPrimitiveComponent : public PSceneComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PPrimitiveComponent)

    public:
        PE_NODISCARD EBoundAABB GetBounds() const;

    private:
        EBoundAABB Bounds;
    };
}

