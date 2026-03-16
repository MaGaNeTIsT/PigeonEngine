#pragma once

#include "EngineCommon.h"
#include <PigeonBase/Object/Component/SceneComponent.h>

namespace PigeonEngine
{

    class EMaterialAsset;

#if _EDITOR_ONLY
    class ETexture2DAsset;
#endif

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
        PE_NODISCARD BOOL32                 IsPrimitiveCastShadow() const;
        PE_NODISCARD BOOL32                 IsPrimitiveReceiveShadow() const;
        PE_NODISCARD BOOL32                 IsPrimitiveRenderHidden() const;
        PE_NODISCARD const EMaterialAsset*  GetMaterialAsset(UINT32 SlotIdx = 0u) const;
        PE_NODISCARD UINT32                 GetMaterialSlotCount() const;
    public:
        void SetPrimitiveCastShadow(BOOL32 InIsCastShadow);
        void SetPrimitiveReceiveShadow(BOOL32 InIsReceiveShadow);
        void SetPrimitiveRenderHidden(BOOL32 InIsRenderHidden);
        virtual void SetMaterialAsset(const EMaterialAsset* InMaterialAsset);
        virtual void SetMaterialAsset(UINT32 SlotIdx, const EMaterialAsset* InMaterialAsset);
        void SetMaterialSlotsNum(UINT32 Num);
    private:
        BOOL32                          IsCastShadow;
        BOOL32                          IsReceiveShadow;
        BOOL32                          IsRenderHidden;
        TArray<const EMaterialAsset*>   MaterialSlots;

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

#if _EDITOR_ONLY
    public:
        struct PMaterialEditorCBState
        {
            EString      CBName;
            TArray<BYTE> Data;
        };
        struct PMaterialEditorTextureSlot
        {
            const ETexture2DAsset* Texture;
            CHAR PathBuf[512];
            PMaterialEditorTextureSlot() : Texture(nullptr) { PathBuf[0] = '\0'; }
        };
        struct PMaterialEditorSlotState
        {
            TArray<PMaterialEditorCBState>     CBStates;
            TArray<PMaterialEditorTextureSlot> TextureSlots;
        };
    public:
        BOOL32  IsEditorMaterialParamsDirty() const;
        void    MarkEditorMaterialParamsDirty();
        void    CleanEditorMaterialParamsDirty();
        void    GetEditorSlotCBData(UINT32 SlotIdx, TArray<TArray<BYTE>>& Out) const;
    protected:
        BOOL32                           MaterialParamsDirty;
        TArray<PMaterialEditorSlotState> EditorSlotStates;
        void RebuildEditorSlotState(UINT32 SlotIdx);
        void RebuildAllEditorSlotStates();
        virtual void GenerateComponentDetail() override;
        virtual void OnSelectedByImGui() override;
#endif
    };

};
