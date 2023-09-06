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
    public:
        // Render proxy functions START
        virtual class RPrimitiveSceneProxy* GetSceneProxy() { return nullptr; }
        virtual const class RPrimitiveSceneProxy* GetSceneProxy()const { return nullptr; }
        virtual class RPrimitiveSceneProxy* CreateSceneProxy() { return nullptr; }
        // Render proxy functions END
    };

};