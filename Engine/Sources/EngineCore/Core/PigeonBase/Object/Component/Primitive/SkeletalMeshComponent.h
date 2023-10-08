#pragma once

#include <CoreMinimal.h>
#include "MeshComponent.h"

namespace PigeonEngine
{

    class ESkinnedMeshAsset;
    class ESkeletonAsset;
    class RSkeletalMeshSceneProxy;

    class PSkeletalMeshComponent : public PMeshComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PSkeletalMeshComponent)

    public:
        const ESkinnedMeshAsset*    GetMeshAsset()const;
        void                        SetMeshAsset(const ESkinnedMeshAsset* InMeshAsset);
        const ESkeletonAsset*       GetSkeletonAsset()const;
        void                        SetSkeletonAsset(const ESkeletonAsset* InSkeletonAsset);
    protected:
        const ESkinnedMeshAsset*    MeshAsset;
        const ESkeletonAsset*       SkeletonAsset;

        // Render proxy functions START
    public:
        enum PSkeletalMeshUpdateState : UINT8
        {
            SKELETAL_MESH_UPDATE_STATE_NONE     = 0,
            SKELETAL_MESH_UPDATE_STATE_MATRIX   = (1 << 0),
            SKELETAL_MESH_UPDATE_STATE_ASSET    = (1 << 1)
        };
    public:
        RSkeletalMeshSceneProxy*    SceneProxy; // ONLY render thread could use this proxy pointer.
    public:
        UINT8                       GetUpdateRenderState()const;
        RSkeletalMeshSceneProxy*    CreateSceneProxy();
        virtual void                CreateRenderState()override;
        virtual void                DestroyRenderState()override;
        virtual void                SendUpdateRenderState()override;
    protected:
        void                        MarkAsDirty(PSkeletalMeshUpdateState InState);
        virtual void                MarkRenderTransformAsDirty()override;
        virtual void                MarkRenderStateAsDirty()override;
        virtual void                CleanMarkRenderStateDirty()override;
    protected:
        UINT8                       UpdateState;
        // Render proxy functions END
    };

};