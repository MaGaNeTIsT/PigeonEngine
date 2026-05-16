#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderMaterials/MaterialBinding.h>

namespace PigeonEngine
{

    class EMaterialAsset;
    class ETexture2DAsset;

    // ---------------------------------------------------------------------------
    // Stateless helpers for material slot management, reflection, and binding.
    // Shared by any component that owns one or more material slots.
    // ---------------------------------------------------------------------------

    struct EMaterialHelper
    {
        // --- Slot rebuild ---

        // Rebuild a single slot's standing (CB + SRV arrays) from the material's reflection.
        // Preserves existing data when the structure already matches.
        static void RebuildSlot(
            UINT32                              InSlotIdx,
            const EMaterialAsset*               InMaterial,
            TArray<EMaterialSlotStanding>&      InOutStandings);

        // Rebuild all slots, trimming excess standings.
        static void RebuildAllSlots(
            const TArray<const EMaterialAsset*>&    InMaterialSlots,
            TArray<EMaterialSlotStanding>&          InOutStandings);

        // --- Data accessors ---

        static void GetConstantBufferData(
            UINT32                              InSlotIdx,
            const TArray<EMaterialSlotStanding>& InStandings,
            TArray<TArray<UINT8>>&              Out);

        static void GetTextureSRVs(
            UINT32                              InSlotIdx,
            const TArray<EMaterialSlotStanding>& InStandings,
            const EMaterialAsset*               InMaterial,
            TArray<struct RMaterialTextureSRV>& Out);

#if _EDITOR_ONLY
        // --- Editor UI ---

        // Draw ImGui widgets for one material slot's CB fields and SRV texture slots.
        // Returns TRUE if any value was changed.
        static BOOL8 DrawSlotDetail(
            UINT32                      InSlotIdx,
            const EMaterialAsset*       InMaterial,
            EMaterialSlotStanding&      InOutStanding);

        // Draw the full "Materials" tree node for all slots.
        // Calls OutSetMaterial / OutRebuildSlot callbacks on user action.
        // Returns TRUE if material dirty flag should be set.
        static BOOL8 DrawMaterialsPanel(
            TArray<const EMaterialAsset*>&  InOutMaterialSlots,
            TArray<EMaterialSlotStanding>&  InOutStandings,
            BOOL8&                          InOutEmitShaderDebug);
#endif

    private:
        EMaterialHelper() = delete;

#if _EDITOR_ONLY
        static BOOL8 DrawCBFieldEditor(const struct EMaterialCBField& InField, TArray<UINT8>& InOutData, INT32 InUniqueId);
#endif
    };

} // namespace PigeonEngine
