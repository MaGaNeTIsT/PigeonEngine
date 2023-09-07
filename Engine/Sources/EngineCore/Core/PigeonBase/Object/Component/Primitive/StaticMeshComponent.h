#pragma once

#include <CoreMinimal.h>
#include "MeshComponent.h"

namespace PigeonEngine
{

    class RStaticMeshSceneProxy;

    class PStaticMeshComponent : public PMeshComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PStaticMeshComponent)

        // Render proxy functions START
    protected:
        RStaticMeshSceneProxy* SceneProxy;
    public:
        RStaticMeshSceneProxy*          GetSceneProxy();
        const RStaticMeshSceneProxy*    GetSceneProxy()const;
        RStaticMeshSceneProxy*          CreateSceneProxy();
        virtual void    CreateRenderState()override;
        virtual void    DestroyRenderState()override;
        virtual void    SendUpdateRenderState()override;
        // Render proxy functions END
    };

};