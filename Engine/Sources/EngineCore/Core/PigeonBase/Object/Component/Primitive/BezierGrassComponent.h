#pragma once

#include <CoreMinimal.h>
#include "PrimitiveComponent.h"

namespace PigeonEngine
{

#define BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM                 (16)

    class RBezierGrassSceneProxy;
#if 0
    struct EBezierGrassGlobalData
    {
        float GlobalWindDirection;
        float GlobalWindStrength;
        uint4           NumAllocatedLODMaxNumInstances;
        float4          TileAnchorSize;
        uint4           TileXYNumTiles;
        float4          SubTileSizeBorderSize;
        uint4           SubTileXYNumSubTiles;
        uint4           MaskXYNumInstances;
        float4          DensityScale;
        uint4           HeightMapWorldScaleOffsetBorderPixelSize;
        uint4           LayerBorderPixelSizeDensityBorderPixelSize;
        uint4           LayerTypeElemsBaseCustomTotalNumTypes;
        uint4           LODBodyPart1;
        uint4           LODBodyPart2;
        uint4           IndexOffset1;
        uint4           IndexOffset2;
        uint4           VertexOffset1;
        uint4           VertexOffset2;
        float4          LODDistancesSq1;
        float4          LODDistancesSq2;
    };
#endif
	struct EBezierGrassProperty
	{
        Vector3     WindDirection;
        FLOAT       WindStrength;

        EBezierGrassProperty() noexcept
            : WindDirection{ Vector3::Zero() }
            , WindStrength{ 0.0f }
        {
        }
	};
    struct EBezierGrassLayerTypeData
    {
        Vector3     Facing;
        Vector3     Height;
        Vector3     Width;
        Vector3     Tilt;
        Vector3     Bend;
        Vector3     MidPointT;
        BOOL8       bBent;
        BOOL8       bUseFacing;
        FLOAT       SideCurve[BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM];

        EBezierGrassLayerTypeData() noexcept
            : Facing(0.0f, -0.1f, 0.1f)
            , Height(15.0f, -5.0f, 10.0f)
            , Width(0.5f, -0.2f, 0.3f)
            , Tilt(0.3f, -0.2f, 0.3f)
            , Bend(0.5f, -0.3f, 0.4f)
            , MidPointT(0.5f, -0.1f, 0.2f)
            , bBent(FALSE)
            , bUseFacing(FALSE)
        {
            for (INT32 i = 0; i < BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM; i++)
            {
                SideCurve[i] = 1.0f - (((FLOAT)i) / ((FLOAT)(BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM - 1)));
            }
        }
    };

    class PBezierGrassComponent : public PPrimitiveComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PBezierGrassComponent)

    public:
        void SetProperty(BOOL8 InIsWireframe, FLOAT InLOD, FLOAT InLeafWidth, Color4 InRootColor, Color4 InTipColor, FLOAT InBentBezierT, FLOAT InRoughness, FLOAT InMetallic);
        void SetLayerTypeData(const EBezierGrassLayerTypeData& InLayerData);
        void SetTileParams(const Vector2& InTileAnchor, const Vector2& InTileSize, UINT32 InNumTilesX, UINT32 InNumTilesZ);
        void SetWindParams(const Vector3& InWindDirection, FLOAT InWindStrength);
        void GenerateInstanceData(const Vector3& InOrigin, FLOAT InBaseHeight, FLOAT InOffsetHeight, FLOAT InLengthX, FLOAT InLengthZ, UINT32 InNumX, UINT32 InNumZ); // Deprecated
    protected:
        BOOL8                               bWireframe = FALSE;
        EBezierGrassProperty                Property;
        EBezierGrassLayerTypeData           LayerTypeData;
        const class EMaterialAsset*         MaterialAsset;
        const class EMaterialAsset*         ComputeMaterialAsset;

        // Render proxy functions START
    public:
        enum PBezierGrassUpdateState : UINT8
        {
            BEZIER_GRASS_UPDATE_STATE_NONE      = 0,
            BEZIER_GRASS_UPDATE_STATE_MATRIX    = (1 << 0),
            BEZIER_GRASS_UPDATE_STATE_ASSET     = (1 << 1),
            BEZIER_GRASS_UPDATE_STATE_INSTANCE  = (1 << 2)
        };
    public:
        RBezierGrassSceneProxy* SceneProxy;
    public:
        UINT8                   GetUpdateRenderState()const;
        RBezierGrassSceneProxy* CreateSceneProxy();
        virtual void            CreateRenderState()override;
        virtual void            DestroyRenderState()override;
        virtual void            SendUpdateRenderState()override;
    protected:
        void                    MarkAsDirty(PBezierGrassUpdateState InState);
        virtual void            MarkRenderTransformAsDirty()override;
        virtual void            MarkRenderStateAsDirty()override;
        virtual void            CleanMarkRenderStateDirty()override;
    protected:
        UINT8                   UpdateState;
        // Render proxy functions END

#if _EDITOR_ONLY
    public:
        virtual void GenerateComponentOutline(const class PActorComponent* WorldCurrentSelectedComponent)override;
        virtual void GenerateComponentDetail()override;
        virtual void OnSelectedByImGui()override;
#endif

    private:
        friend class RBezierGrassSceneProxy;
        friend class RScene;

    };

};