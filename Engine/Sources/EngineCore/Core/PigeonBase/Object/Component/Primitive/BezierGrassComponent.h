#pragma once

#include <CoreMinimal.h>
#include "PrimitiveComponent.h"

namespace PigeonEngine
{

#define BEZIER_GRASS_START_BODY_PARTS               (2)
#define BEZIER_GRASS_MAX_LOD_BODY_PARTS             (6)
#define BEZIER_GRASS_MAX_LOD_INDEX                  (BEZIER_GRASS_MAX_LOD_BODY_PARTS - BEZIER_GRASS_START_BODY_PARTS)
#define BEZIER_GRASS_LOD_NUM                        (BEZIER_GRASS_MAX_LOD_INDEX + 1)
#define BEZIER_GRASS_INSTANCE_PER_GROUP_MAX_NUM     (99999999)

    class RBezierGrassSceneProxy;

	struct EBezierGrassProperty
	{
		EBezierGrassProperty() noexcept
			: bWireframe(FALSE)
            , LOD(0.f)
			, LeafWidth(0.f)
			, RootColor(Color4(0.2f, 0.4f, 0.1f, 1.f))
			, TipColor(Color4(0.4f, 0.8f, 0.2f, 1.f))
            , BentBezierT(0.5f)
            , Roughness(0.8f)
            , Metallic(0.0f)
		{
		}

        BOOL8       bWireframe;
        FLOAT       LOD;
        FLOAT		LeafWidth;
        Color4		RootColor;
        Color4		TipColor;
        FLOAT       BentBezierT;
        FLOAT       Roughness;
        FLOAT       Metallic;
	};
    struct EBezierGrassLayerTypeData
    {
        Vector3 Facing;         // x=base, y=min, z=max
        Vector3 Height;         // x=base, y=min, z=max
        Vector3 Width;          // x=base, y=min, z=max
        Vector3 Tilt;           // x=base, y=min, z=max
        Vector3 Bend;           // x=base, y=min, z=max
        Vector3 MidPointT;      // x=base, y=min, z=max
        BOOL8   bBent;
        BOOL8   bUseFacing;
        FLOAT   SideCurve[8];   // BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM = 8

        EBezierGrassLayerTypeData() noexcept
            : Facing(0.f, -0.1f, 0.1f)
            , Height(15.f, -5.f, 10.f)
            , Width(0.5f, -0.2f, 0.3f)
            , Tilt(0.3f, -0.2f, 0.3f)
            , Bend(0.5f, -0.3f, 0.4f)
            , MidPointT(0.5f, -0.1f, 0.2f)
            , bBent(FALSE)
            , bUseFacing(FALSE)
        {
            for (INT32 i = 0; i < 8; i++) SideCurve[i] = 1.0f;
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
        Vector2                             TileAnchor;
        Vector2                             TileSize;
        UINT32                              NumTilesX;
        UINT32                              NumTilesZ;
        Vector3                             WindDirection;
        FLOAT                               WindStrength;
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