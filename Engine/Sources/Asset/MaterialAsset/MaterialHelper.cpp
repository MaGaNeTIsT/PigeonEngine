#include "MaterialHelper.h"
#include "MaterialAsset.h"

#if _EDITOR_ONLY
#include <TextureAsset/TextureAsset.h>
#include <imgui.h>
#include <Config/EngineConfig.h>
#endif

namespace PigeonEngine
{

    void EMaterialHelper::RebuildSlot(
        UINT32                          InSlotIdx,
        const EMaterialAsset*           InMaterial,
        TArray<EMaterialSlotStanding>&  InOutStandings)
    {
        while (InOutStandings.Num<UINT32>() <= InSlotIdx)
        {
            InOutStandings.Add(EMaterialSlotStanding());
        }

        EMaterialSlotStanding& Standing = InOutStandings[InSlotIdx];

        if (!InMaterial)
        {
            Standing.ConstantBufferSlots.Empty();
            Standing.TextureSlots.Empty();
            return;
        }

        const EMaterialVariant* Variant = InMaterial->GetFirstVariant();
        if (!Variant)
        {
            Standing.ConstantBufferSlots.Empty();
            Standing.TextureSlots.Empty();
            return;
        }

        const EMaterialReflection& Reflection = Variant->Reflection;

        const UINT32 NumConstantBuffers = Reflection.ConstantBuffers.Num<UINT32>();
        if (Standing.ConstantBufferSlots.Num<UINT32>() != NumConstantBuffers)
        {
            Standing.ConstantBufferSlots.Empty();
            for (UINT32 CbIdx = 0u; CbIdx < NumConstantBuffers; CbIdx++)
            {
                const EMaterialCBRefl& CbRefl = Reflection.ConstantBuffers[static_cast<INT32>(CbIdx)];
                EMaterialCBStanding CbStanding;
                CbStanding.Name = CbRefl.Name;
                CbStanding.Data.SetNum(static_cast<INT32>(CbRefl.SizeBytes));
                for (INT32 ByteIdx = 0; ByteIdx < static_cast<INT32>(CbRefl.SizeBytes); ByteIdx++)
                {
                    CbStanding.Data[ByteIdx] = 0u;
                }
                Standing.ConstantBufferSlots.Add(std::move(CbStanding));
            }
        }

        const UINT32 NumSRVs = Reflection.SRVs.Num<UINT32>();
        if (Standing.TextureSlots.Num<UINT32>() != NumSRVs)
        {
            Standing.TextureSlots.Empty();
            for (UINT32 SrvIdx = 0u; SrvIdx < NumSRVs; SrvIdx++)
            {
                Standing.TextureSlots.Add(EMaterialTextureStanding());
            }
        }
    }

    void EMaterialHelper::RebuildAllSlots(
        const TArray<const EMaterialAsset*>&    InMaterialSlots,
        TArray<EMaterialSlotStanding>&          InOutStandings)
    {
        const UINT32 NumSlots = InMaterialSlots.Num<UINT32>();
        while (InOutStandings.Num<UINT32>() > NumSlots)
        {
            InOutStandings.Pop();
        }
        for (UINT32 SlotIdx = 0u; SlotIdx < NumSlots; SlotIdx++)
        {
            RebuildSlot(SlotIdx, InMaterialSlots[SlotIdx], InOutStandings);
        }
    }

    void EMaterialHelper::GetConstantBufferData(
        UINT32                              InSlotIdx,
        const TArray<EMaterialSlotStanding>& InStandings,
        TArray<TArray<UINT8>>&              Out)
    {
        Out.Empty();
        if (InSlotIdx >= InStandings.Num<UINT32>())
        {
            return;
        }
        const EMaterialSlotStanding& Standing = InStandings[InSlotIdx];
        for (INT32 i = 0; i < Standing.ConstantBufferSlots.Num<INT32>(); i++)
        {
            Out.Add(Standing.ConstantBufferSlots[i].Data);
        }
    }

    void EMaterialHelper::GetTextureSRVs(
        UINT32                              InSlotIdx,
        const TArray<EMaterialSlotStanding>& InStandings,
        const EMaterialAsset*               InMaterial,
        TArray<RMaterialTextureSRV>&        Out)
    {
        Out.Empty();
        if (InSlotIdx >= InStandings.Num<UINT32>() || !InMaterial)
        {
            return;
        }

        const EMaterialVariant* Variant = InMaterial->GetFirstVariant();
        if (!Variant)
        {
            return;
        }

        const EMaterialSlotStanding& Standing = InStandings[InSlotIdx];
        const EMaterialReflection& Reflection = Variant->Reflection;
        const INT32 NumSRVs = Reflection.SRVs.Num<INT32>();

        for (INT32 i = 0, n = Standing.TextureSlots.Num<INT32>(); (i < NumSRVs) && (i < n); i++)
        {
            const EMaterialTextureStanding& TextureSlot = Standing.TextureSlots[i];
            if ((!!(TextureSlot.Texture)) && (!!(TextureSlot.Texture->GetRenderResource())) && (!!(TextureSlot.Texture->GetRenderResource()->ShaderResourceView)))
            {
                RMaterialTextureSRV Entry;
                Entry.Slot = Reflection.SRVs[i].Slot;
                Entry.SRV = TextureSlot.Texture->GetRenderResource()->ShaderResourceView;
                Out.Add(Entry);
            }
        }
    }

#if _EDITOR_ONLY

    BOOL8 EMaterialHelper::DrawCBFieldEditor(const EMaterialCBField& InField, TArray<UINT8>& InOutData, INT32 InUniqueId)
    {
        ImGui::PushID(InUniqueId);
        const CHAR* FieldType = *InField.Type;
        const BOOL8 bColor = InField.bIsColor;
        FLOAT* Value = (FLOAT*)(&(InOutData[InField.Offset]));
        const CHAR* FieldName = *InField.Name;
        BOOL8 bChanged = FALSE;

        if (::strcmp(FieldType, "float4") == 0)
        {
            bChanged = bColor ? ImGui::ColorEdit4(FieldName, Value) : ImGui::DragFloat4(FieldName, Value);
        }
        else if (::strcmp(FieldType, "float3") == 0)
        {
            bChanged = bColor ? ImGui::ColorEdit3(FieldName, Value) : ImGui::DragFloat3(FieldName, Value);
        }
        else if (::strcmp(FieldType, "float2") == 0)
        {
            bChanged = ImGui::DragFloat2(FieldName, Value);
        }
        else if (::strcmp(FieldType, "float") == 0 || ::strcmp(FieldType, "float1") == 0)
        {
            bChanged = ImGui::DragFloat(FieldName, Value);
        }
        else if (::strcmp(FieldType, "int4") == 0)
        {
            bChanged = ImGui::DragScalarN(FieldName, ImGuiDataType_S32, Value, 4);
        }
        else if (::strcmp(FieldType, "int3") == 0)
        {
            bChanged = ImGui::DragScalarN(FieldName, ImGuiDataType_S32, Value, 3);
        }
        else if (::strcmp(FieldType, "int2") == 0)
        {
            bChanged = ImGui::DragScalarN(FieldName, ImGuiDataType_S32, Value, 2);
        }
        else if (::strcmp(FieldType, "int") == 0 || ::strcmp(FieldType, "int1") == 0)
        {
            bChanged = ImGui::DragScalar(FieldName, ImGuiDataType_S32, Value);
        }
        else if (::strcmp(FieldType, "uint4") == 0)
        {
            bChanged = ImGui::DragScalarN(FieldName, ImGuiDataType_U32, Value, 4);
        }
        else if (::strcmp(FieldType, "uint3") == 0)
        {
            bChanged = ImGui::DragScalarN(FieldName, ImGuiDataType_U32, Value, 3);
        }
        else if (::strcmp(FieldType, "uint2") == 0)
        {
            bChanged = ImGui::DragScalarN(FieldName, ImGuiDataType_U32, Value, 2);
        }
        else if (::strcmp(FieldType, "uint") == 0 || ::strcmp(FieldType, "uint1") == 0)
        {
            bChanged = ImGui::DragScalar(FieldName, ImGuiDataType_U32, Value);
        }
        else
        {
            ImGui::Text("%s %s (unsupported)", FieldType, FieldName);
        }

        ImGui::PopID();
        return bChanged;
    }

    BOOL8 EMaterialHelper::DrawSlotDetail(
        UINT32                  InSlotIdx,
        const EMaterialAsset*   InMaterial,
        EMaterialSlotStanding&  InOutStanding)
    {
        if (!InMaterial)
        {
            return FALSE;
        }

        const EMaterialVariant* Variant = InMaterial->GetFirstVariant();
        if (!Variant)
        {
            return FALSE;
        }

        BOOL8 bDirty = FALSE;
        const EMaterialReflection& Reflection = Variant->Reflection;

        EString ParamHeader = EString("Parameters [Slot ") + EString::FromInt(InSlotIdx) + EString("]");
        if (ImGui::TreeNode(*ParamHeader))
        {
            for (INT32 CbIdx = 0; CbIdx < Reflection.ConstantBuffers.Num<INT32>(); CbIdx++)
            {
                const EMaterialCBRefl& CbRefl = Reflection.ConstantBuffers[CbIdx];
                ImGui::PushID(CbIdx);
                if (ImGui::TreeNode(*CbRefl.Name))
                {
                    if (CbIdx < InOutStanding.ConstantBufferSlots.Num<INT32>())
                    {
                        EMaterialCBStanding& CbStanding = InOutStanding.ConstantBufferSlots[CbIdx];
                        for (INT32 FieldIdx = 0; FieldIdx < CbRefl.Fields.Num<INT32>(); FieldIdx++)
                        {
                            if (DrawCBFieldEditor(CbRefl.Fields[FieldIdx], CbStanding.Data, FieldIdx))
                            {
                                bDirty = TRUE;
                            }
                        }
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            for (INT32 SrvIdx = 0; SrvIdx < Reflection.SRVs.Num<INT32>(); SrvIdx++)
            {
                const EMaterialSRVRefl& SrvRefl = Reflection.SRVs[SrvIdx];
                ImGui::PushID(SrvIdx);

                const CHAR* TextureName = "<None>";
                if (SrvIdx < InOutStanding.TextureSlots.Num<INT32>() && InOutStanding.TextureSlots[SrvIdx].Texture)
                {
                    TextureName = *InOutStanding.TextureSlots[SrvIdx].Texture->GetAssetName();
                }
                ImGui::Text("%s (slot %u): %s", *SrvRefl.Name, SrvRefl.Slot, TextureName);

                if (SrvIdx < InOutStanding.TextureSlots.Num<INT32>())
                {
                    EMaterialTextureStanding& TextureSlot = InOutStanding.TextureSlots[SrvIdx];
                    EString PathLabel = EString("Path##") + EString::FromInt(static_cast<UINT32>(SrvIdx));
                    ImGui::InputText(*PathLabel, TextureSlot.PathBuf, sizeof(TextureSlot.PathBuf));
                    ImGui::SameLine();
                    EString LoadLabel = EString("Load##") + EString::FromInt(static_cast<UINT32>(SrvIdx));
                    if (ImGui::Button(*LoadLabel))
                    {
                        const ETexture2DAsset* NewTexture = nullptr;
                        TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, EString(TextureSlot.PathBuf), NewTexture);
                        TextureSlot.Texture = NewTexture;
                        bDirty = TRUE;
                    }
                }

                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        return bDirty;
    }

    BOOL8 EMaterialHelper::DrawMaterialsPanel(
        TArray<const EMaterialAsset*>&  InOutMaterialSlots,
        TArray<EMaterialSlotStanding>&  InOutStandings,
        BOOL8&                          InOutEmitShaderDebug)
    {
        BOOL8 bDirty = FALSE;

        if (!ImGui::TreeNode("Materials"))
        {
            return FALSE;
        }

        const INT32 NumSlots = InOutMaterialSlots.Num<INT32>();

        for (INT32 SlotIdx = 0; SlotIdx < NumSlots; SlotIdx++)
        {
            const EMaterialAsset* SlotMaterial = InOutMaterialSlots[SlotIdx];
            const CHAR* MaterialName = SlotMaterial ? *SlotMaterial->GetName() : "<None>";
            ImGui::Text("Slot [%d]: %s", SlotIdx, MaterialName);

            ImGui::PushID(SlotIdx);

            CHAR MaterialNameBuf[256] = {};
            ImGui::InputText("##name", MaterialNameBuf, sizeof(MaterialNameBuf));
            ImGui::SameLine();
            if (ImGui::Button("Load"))
            {
                const EMaterialAsset* NewMaterial = nullptr;
                TryLoadMaterialAsset(
                    EEngineSettings::ENGINE_MATERIAL_OUTPUT_DIR,
                    EString(MaterialNameBuf),
                    EEngineSettings::ENGINE_MATERIAL_SOURCE_DIR,
                    EEngineSettings::ENGINE_MATERIAL_SHADER_INCLUDE_DIR,
                    InOutEmitShaderDebug ? TRUE : FALSE,
                    NewMaterial
                );
                if (static_cast<UINT32>(SlotIdx) >= InOutMaterialSlots.Num<UINT32>())
                {
                    InOutMaterialSlots.SetNum(SlotIdx + 1);
                }
                InOutMaterialSlots[SlotIdx] = NewMaterial;
                RebuildSlot(static_cast<UINT32>(SlotIdx), NewMaterial, InOutStandings);
            }
            ImGui::SameLine();
            ImGui::Checkbox("Shader Debug", &InOutEmitShaderDebug);
            ImGui::SameLine();
            if (ImGui::Button("Clear"))
            {
                InOutMaterialSlots[SlotIdx] = nullptr;
                RebuildSlot(static_cast<UINT32>(SlotIdx), nullptr, InOutStandings);
            }

            ImGui::PopID();
        }

        if (NumSlots == 0)
        {
            if (ImGui::Button("Add Slot"))
            {
                InOutMaterialSlots.SetNum(1);
                InOutMaterialSlots[0] = nullptr;
                RebuildSlot(0u, nullptr, InOutStandings);
            }
        }

        static INT32 StaticSelectedSlot = 0;
        const INT32 MaxSlot = (NumSlots > 0) ? (NumSlots - 1) : 0;
        ImGui::SliderInt("Edit Slot##slot", &StaticSelectedSlot, 0, MaxSlot);

        if (StaticSelectedSlot < NumSlots && InOutMaterialSlots[StaticSelectedSlot] != nullptr)
        {
            if (StaticSelectedSlot < InOutStandings.Num<INT32>())
            {
                if (DrawSlotDetail(
                    static_cast<UINT32>(StaticSelectedSlot),
                    InOutMaterialSlots[StaticSelectedSlot],
                    InOutStandings[StaticSelectedSlot]))
                {
                    bDirty = TRUE;
                }
            }
        }

        ImGui::TreePop();
        return bDirty;
    }

#endif // _EDITOR_ONLY

} // namespace PigeonEngine
