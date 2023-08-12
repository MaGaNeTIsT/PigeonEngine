#pragma once
#include "EngineCommon.h"
#include "SceneComponent.h"


namespace PigeonEngine
{
    class PPrimitiveComponent : public PSceneComponent
    {
        EClass(PPrimitiveComponent, PSceneComponent)
        CLASS_VIRTUAL_NOCOPY_BODY(PPrimitiveComponent)

    public:
        ENGINE_NODISCARD EBoundAABB GetBounds() const;

    private:
        EBoundAABB Bounds;
    };
}

