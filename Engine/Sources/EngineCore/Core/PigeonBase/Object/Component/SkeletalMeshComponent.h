#pragma once

#include <CoreMinimal.h>
#include "MeshComponent.h"

namespace PigeonEngine
{

    class RPrimitiveSceneProxy;
    class RSkeletonMeshSceneProxy;

    class PSkeletalMeshComponent : public PMeshComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PSkeletalMeshComponent)


        // Render proxy functions START
    public:
        virtual RPrimitiveSceneProxy* GetSceneProxy()override;
        virtual const RPrimitiveSceneProxy* GetSceneProxy()const override;
        virtual RPrimitiveSceneProxy* CreateSceneProxy()override;
        virtual void    CreateRenderState()override;
        virtual void    DestroyRenderState()override;
        virtual void    SendUpdateRenderState()override;
    protected:
        RSkeletonMeshSceneProxy* SceneProxy;
        // Render proxy functions END
    };

};