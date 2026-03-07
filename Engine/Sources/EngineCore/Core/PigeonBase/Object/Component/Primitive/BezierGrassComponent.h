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
			, RootColor(Color4::Black())
			, TipColor(Color4::Black())
		{
		}
        EBezierGrassProperty(BOOL8 InIsWireframe, FLOAT InLOD, FLOAT InLeafWidth, const Color4& InRootColor, const Color4& InTipColor) noexcept
            : bWireframe(InIsWireframe)
            , LOD(InLOD)
            , LeafWidth(InLeafWidth)
            , RootColor(InRootColor)
            , TipColor(InTipColor)
		{
		}
		EBezierGrassProperty(const EBezierGrassProperty& Other)
			: bWireframe(Other.bWireframe)
            ,LOD(Other.LOD)
			, LeafWidth(Other.LeafWidth)
			, RootColor(Other.RootColor)
			, TipColor(Other.TipColor)
		{
		}
        EBezierGrassProperty& operator=(const EBezierGrassProperty& Other)
		{
            bWireframe  = Other.bWireframe;
            LOD         = Other.LOD;
            LeafWidth   = Other.LeafWidth;
            RootColor   = Other.RootColor;
            TipColor    = Other.TipColor;
			return (*this);
		}

        BOOL8       bWireframe;
        FLOAT       LOD;
        FLOAT		LeafWidth;
        Color4		RootColor;
        Color4		TipColor;
	};
    struct EBezierGrassInstanceData
    {
        EBezierGrassInstanceData() noexcept
            : Origin(Vector3::Zero())
            , Tilt(0.f)
            , Direction(Vector3::Zero())
            , Bend(0.f)
            , Tip(Vector2::Zero())
        {
        }
        EBezierGrassInstanceData(const Vector3& InOrigin, const Vector3& InDirection, FLOAT InTilt, FLOAT InBend, const Vector2& InTip) noexcept
            : Origin(InOrigin)
            , Tilt(InTilt)
            , Direction(InDirection)
            , Bend(InBend)
            , Tip(InTip)
        {
        }
        EBezierGrassInstanceData(const EBezierGrassInstanceData& Other)
            : Origin(Other.Origin)
            , Tilt(Other.Tilt)
            , Direction(Other.Direction)
            , Bend(Other.Bend)
            , Tip(Other.Tip)
        {
        }
        EBezierGrassInstanceData& operator=(const EBezierGrassInstanceData& Other)
        {
            Origin      = Other.Origin;
            Tilt        = Other.Tilt;
            Direction   = Other.Direction;
            Bend        = Other.Bend;
            Tip         = Other.Tip;
            return (*this);
        }

        Vector3     Origin;
        FLOAT       Tilt;
        Vector3     Direction;
        FLOAT       Bend;
        Vector2     Tip;
    };

    class PBezierGrassComponent : public PPrimitiveComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PBezierGrassComponent)

    public:
        void SetProperty(BOOL8 InIsWireframe, FLOAT InLOD, FLOAT InLeafWidth, Color4 InRootColor, Color4 InTipColor);
        void GenerateInstanceData(const Vector3& InOrigin, FLOAT InBaseHeight, FLOAT InOffsetHeight, FLOAT InLengthX, FLOAT InLengthZ, UINT32 InNumX, UINT32 InNumZ);
    protected:
        BOOL8                               bWireframe = FALSE;
        EBezierGrassProperty                Property;
        TArray<EBezierGrassInstanceData>    InstanceData;
        FLOAT                               BaseHeight;
        FLOAT                               OffsetHeight;
        FLOAT                               LengthX;
        FLOAT                               LengthZ;
        UINT32                              NumX;
        UINT32                              NumZ;

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