#pragma once

#include <CoreMinimal.h>
#include "MeshComponent.h"

namespace PigeonEngine
{

    class RSkeletalMeshSceneProxy;

    class PSkeletalMeshComponent : public PMeshComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PSkeletalMeshComponent)

        // Render proxy functions START
    protected:
        RSkeletalMeshSceneProxy* SceneProxy;
    public:
        RSkeletalMeshSceneProxy*        GetSceneProxy();
        const RSkeletalMeshSceneProxy*  GetSceneProxy()const;
        RSkeletalMeshSceneProxy*        CreateSceneProxy();
        virtual void    CreateRenderState()override;
        virtual void    DestroyRenderState()override;
        virtual void    SendUpdateRenderState()override;
        // Render proxy functions END
    };

};