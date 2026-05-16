#pragma once

#include "EngineCommon.h"
#include <PigeonBase/Object/Component/SceneComponent.h>
#include <RenderMaterials/MaterialBinding.h>
#include <MaterialAsset/MaterialHelper.h>

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
        PE_NODISCARD BOOL32                 IsCastShadow() const;
        PE_NODISCARD BOOL32                 IsReceiveShadow() const;
        PE_NODISCARD BOOL32                 IsMaterialDirty() const;
        PE_NODISCARD const EMaterialAsset*  GetMaterialAsset(UINT32 InSlotIdx = 0u) const;
        PE_NODISCARD UINT32                 GetMaterialSlotCount() const;
    public:
        void            SetCastShadow(BOOL32 InIsCastShadow);
        void            SetReceiveShadow(BOOL32 InIsReceiveShadow);
        virtual void    SetMaterialAsset(const EMaterialAsset* InMaterialAsset);
        virtual void    SetMaterialAsset(UINT32 InSlotIdx, const EMaterialAsset* InMaterialAsset);
        void            SetMaterialSlotsNum(UINT32 InNum);

        // Render proxy functions START
    public:
        virtual void    CreateRenderState()override;
        virtual void    DestroyRenderState()override;
        virtual void    SendUpdateRenderState()override;
    protected:
        virtual void    MarkRenderTransformAsDirty()override;
        virtual void    MarkRenderStateAsDirty()override;
        virtual void    CleanMarkRenderStateDirty()override;
        virtual void    MarkMaterialAsDirty();
        virtual void    CleanMaterialDirty();
        // Render proxy functions END

    public:
        void            GetMaterialConstantBufferDataBySlot(UINT32 InSlotIdx, TArray<TArray<UINT8>>& Out) const;
        void            GetMaterialTextureBySlot(UINT32 InSlotIdx, TArray<struct RMaterialTextureSRV>& Out) const;
        void            RebuildSlot(UINT32 InSlotIdx);
        void            RebuildAllSlot();
#if _EDITOR_ONLY
    public:
        virtual void    GenerateComponentDetail() override;
        virtual void    OnSelectedByImGui() override;
#endif
    protected:
        BOOL32                              bMaterialDirty;
        BOOL32                              bCastShadow;
        BOOL32                              bReceiveShadow;
        TArray<const EMaterialAsset*>       MaterialSlots;
        TArray<EMaterialSlotStanding>       MaterialStandings;
#if _EDITOR_ONLY
        BOOL8                               bEmitShaderDebug = FALSE;
#endif

    };

};
