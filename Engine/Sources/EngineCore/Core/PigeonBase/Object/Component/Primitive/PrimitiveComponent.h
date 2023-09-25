#pragma once
#include "EngineCommon.h"
#include <PigeonBase/Object/Component/SceneComponent.h>


namespace PigeonEngine
{

    struct ERenderPrimitiveMatrices : public ERenderTransformInfo
    {
    public:

    public:
        ERenderPrimitiveMatrices(const Vector3& InWorldLocation, const Quaternion& InWorldRotation, const Vector3& InWorldScaling)
            : ERenderTransformInfo(InWorldLocation, InWorldRotation, InWorldScaling)
        {
        }
        ERenderPrimitiveMatrices() = default;
        ERenderPrimitiveMatrices(const ERenderPrimitiveMatrices& Other)
            : ERenderTransformInfo(Other)
        {
        }
        ERenderPrimitiveMatrices& operator=(const ERenderPrimitiveMatrices& Other)
        {
            ERenderTransformInfo::CopyFromOther(Other);
            //TODO
            return (*this);
        }
    };

    class PPrimitiveComponent : public PSceneComponent
    {

        CLASS_VIRTUAL_NOCOPY_BODY(PPrimitiveComponent)

    public:
        PE_NODISCARD EBoundAABB GetBounds() const;
        PE_NODISCARD BOOL32 IsPrimitiveCastShadow() const;
        PE_NODISCARD BOOL32 IsPrimitiveReceiveShadow() const;
        PE_NODISCARD BOOL32 IsPrimitiveRenderHidden() const;
    public:
        void SetPrimitiveCastShadow(BOOL32 InIsCastShadow);
        void SetPrimitiveReceiveShadow(BOOL32 InIsReceiveShadow);
        void SetPrimitiveRenderHidden(BOOL32 InIsRenderHidden);
    private:
        EBoundAABB  Bounds;
        BOOL32      IsCastShadow;
        BOOL32      IsReceiveShadow;
        BOOL32      IsRenderHidden;

        // Render proxy functions START
    public:
        virtual BOOL32	CheckIsRenderHidden()const override;
        virtual void    CreateRenderState()override;
        virtual void    DestroyRenderState()override;
        virtual void    SendUpdateRenderState()override;
    protected:
        virtual void    MarkRenderTransformAsDirty()override;
        virtual void    MarkRenderStateAsDirty()override;
        virtual void    CleanMarkRenderStateDirty()override;
        // Render proxy functions END
    };

};