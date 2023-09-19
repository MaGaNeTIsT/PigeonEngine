#pragma once

#include <CoreMinimal.h>
#include "MeshComponent.h"
#include <MeshAsset/MeshAsset.h>

namespace PigeonEngine
{

    class RStaticMeshSceneProxy;

    class PStaticMeshComponent : public PMeshComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PStaticMeshComponent)

    public:
        const EStaticMeshAsset*     GetMeshAsset()const;
        void                        SetMeshAsset(const EStaticMeshAsset* InMeshAsset);
    protected:
        const EStaticMeshAsset*     MeshAsset;

        // Render proxy functions START
    public:
        enum PStaticMeshUpdateState : UINT8
        {
            STATIC_MESH_UPDATE_STATE_NONE   = 0,
            STATIC_MESH_UPDATE_STATE_MATRIX = (1 << 0),
            STATIC_MESH_UPDATE_STATE_ASSET  = (1 << 1)
        };
    public:
        RStaticMeshSceneProxy*  SceneProxy;
    public:
        UINT8                   GetUpdateRenderState()const;
        RStaticMeshSceneProxy*  CreateSceneProxy();
        virtual void            CreateRenderState()override;
        virtual void            DestroyRenderState()override;
        virtual void            SendUpdateRenderState()override;
    protected:
        void                    MarkAsDirty(PStaticMeshUpdateState InState);
        virtual void            MarkRenderTransformAsDirty()override;
        virtual void            MarkRenderStateAsDirty()override;
        virtual void            CleanMarkRenderStateDirty()override;
    protected:
        UINT8                   UpdateState;
        // Render proxy functions END
    };

};