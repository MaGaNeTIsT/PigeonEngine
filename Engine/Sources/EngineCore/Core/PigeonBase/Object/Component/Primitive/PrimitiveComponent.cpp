#include "PrimitiveComponent.h"
#include <MaterialAsset/MaterialHelper.h>

#if _EDITOR_ONLY
#include <RenderProxy/StaticMeshSceneProxy.h>
#include <imgui.h>
#endif

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<PPrimitiveComponent, PSceneComponent>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PPrimitiveComponent::PPrimitiveComponent()
        : PSceneComponent()
        , bMaterialDirty(FALSE)
        , bCastShadow(FALSE)
        , bReceiveShadow(FALSE)
    {
        POBJ_DEBUGNAME_SET(this, "PrimitiveComponent");
    }
    PPrimitiveComponent::~PPrimitiveComponent()
    {
    }
    BOOL32 PPrimitiveComponent::IsCastShadow() const
    {
        return bCastShadow;
    }
    BOOL32 PPrimitiveComponent::IsReceiveShadow() const
    {
        return bReceiveShadow;
    }
    BOOL32 PPrimitiveComponent::IsMaterialDirty() const
    {
        return bMaterialDirty;
    }
    void PPrimitiveComponent::SetCastShadow(BOOL32 InIsCastShadow)
    {
        bCastShadow = InIsCastShadow;
    }
    void PPrimitiveComponent::SetReceiveShadow(BOOL32 InIsReceiveShadow)
    {
        bReceiveShadow = InIsReceiveShadow;
    }
    const EMaterialAsset* PPrimitiveComponent::GetMaterialAsset(UINT32 InSlotIdx) const
    {
        if (InSlotIdx < MaterialSlots.Num<UINT32>())
        {
            return (MaterialSlots[InSlotIdx]);
        }
        return nullptr;
    }
    UINT32 PPrimitiveComponent::GetMaterialSlotCount() const
    {
        return (MaterialSlots.Num<UINT32>());
    }
    void PPrimitiveComponent::SetMaterialAsset(const EMaterialAsset* InMaterialAsset)
    {
        if (MaterialSlots.Num<INT32>() <= 0)
        {
            MaterialSlots.Add(nullptr);
        }
        MaterialSlots[0] = InMaterialAsset;
        MarkRenderStateAsDirty();
    }
    void PPrimitiveComponent::SetMaterialAsset(UINT32 InSlotIdx, const EMaterialAsset* InMaterialAsset)
    {
        if (InSlotIdx >= MaterialSlots.Num<UINT32>())
        {
            MaterialSlots.SetNum(InSlotIdx + 1);
        }
        MaterialSlots[InSlotIdx] = InMaterialAsset;
        MarkRenderStateAsDirty();
    }
    void PPrimitiveComponent::SetMaterialSlotsNum(UINT32 InNum)
    {
        const UINT32 OldNum = MaterialSlots.Num<UINT32>();
        MaterialSlots.SetNum(InNum);
        for (UINT32 i = OldNum; i < InNum; i++)
        {
            MaterialSlots[i] = nullptr;
        }
    }

    // Render proxy functions START
    void PPrimitiveComponent::CreateRenderState()
    {
        PSceneComponent::CreateRenderState();
        if (ShouldRender())
        {
            //TODO
        }
    }
    void PPrimitiveComponent::DestroyRenderState()
    {
        //TODO
        PSceneComponent::DestroyRenderState();
    }
    void PPrimitiveComponent::SendUpdateRenderState()
    {
        if (ShouldRender() && IsRenderStateDirty())
        {
            //TODO
        }
        PSceneComponent::SendUpdateRenderState();
    }
    void PPrimitiveComponent::MarkRenderTransformAsDirty()
    {
        //TODO
        PSceneComponent::MarkRenderTransformAsDirty();
    }
    void PPrimitiveComponent::MarkRenderStateAsDirty()
    {
        PSceneComponent::MarkRenderStateAsDirty();
    }
    void PPrimitiveComponent::CleanMarkRenderStateDirty()
    {
        //TODO
        PPrimitiveComponent::CleanMaterialDirty();
        PSceneComponent::CleanMarkRenderStateDirty();
    }
    void PPrimitiveComponent::MarkMaterialAsDirty()
    {
        bMaterialDirty = TRUE;
        MarkRenderStateAsDirty();
    }
    void PPrimitiveComponent::CleanMaterialDirty()
    {
        bMaterialDirty = FALSE;
    }
    // Render proxy functions END

    void PPrimitiveComponent::GetMaterialConstantBufferDataBySlot(UINT32 InSlotIdx, TArray<TArray<UINT8>>& Out) const
    {
        EMaterialHelper::GetConstantBufferData(InSlotIdx, MaterialStandings, Out);
    }
    void PPrimitiveComponent::GetMaterialTextureBySlot(UINT32 InSlotIdx, TArray<RMaterialTextureSRV>& Out) const
    {
        EMaterialHelper::GetTextureSRVs(InSlotIdx, MaterialStandings, GetMaterialAsset(InSlotIdx), Out);
    }
    void PPrimitiveComponent::RebuildSlot(UINT32 InSlotIdx)
    {
        EMaterialHelper::RebuildSlot(InSlotIdx, GetMaterialAsset(InSlotIdx), MaterialStandings);
    }
    void PPrimitiveComponent::RebuildAllSlot()
    {
        EMaterialHelper::RebuildAllSlots(MaterialSlots, MaterialStandings);
    }

#if _EDITOR_ONLY
    void PPrimitiveComponent::OnSelectedByImGui()
    {
        PSceneComponent::OnSelectedByImGui();
        RebuildAllSlot();
    }
    void PPrimitiveComponent::GenerateComponentDetail()
    {
        PSceneComponent::GenerateComponentDetail();
        if (EMaterialHelper::DrawMaterialsPanel(MaterialSlots, MaterialStandings, bEmitShaderDebug))
        {
            MarkMaterialAsDirty();
        }
    }
#endif // _EDITOR_ONLY

};
