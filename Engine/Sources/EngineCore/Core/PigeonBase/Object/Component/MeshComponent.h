#pragma once
#include "EngineCommon.h"
#include "SceneComponent.h"


namespace PigeonEngine
{
    class PMeshComponent : public PigeonEngine::PSceneComponent
    {
        EClass(PMeshComponent, PSceneComponent)
        CLASS_VIRTUAL_NOCOPY_BODY(PMeshComponent)
    public:
        ENGINE_NODISCARD EBoundAABB GetBounds() const;

    private:
        EBoundAABB Bounds;
    };
}

