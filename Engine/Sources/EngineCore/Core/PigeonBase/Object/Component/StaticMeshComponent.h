#pragma once

#include <CoreMinimal.h>
#include "MeshComponent.h"

namespace PigeonEngine
{

    class RPrimitiveSceneProxy;
    class RStaticMeshSceneProxy;

    class PStaticMeshComponent : public PMeshComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PStaticMeshComponent)


        // Render proxy functions START
    public:
        virtual RPrimitiveSceneProxy* GetSceneProxy()override;
        virtual const RPrimitiveSceneProxy* GetSceneProxy()const override;
        virtual RPrimitiveSceneProxy* CreateSceneProxy()override;
        virtual void    CreateRenderState()override;
        virtual void    DestroyRenderState()override;
        virtual void    SendUpdateRenderState()override;
    protected:
        RStaticMeshSceneProxy* SceneProxy;
        // Render proxy functions END
    };

};