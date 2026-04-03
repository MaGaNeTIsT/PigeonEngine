#include "PrimitiveComponent.h"
#include <MaterialAsset/MaterialAsset.h>

#if _EDITOR_ONLY
#include <TextureAsset/TextureAsset.h>
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
        if (MaterialSlots.Num<INT32>() <= 0u)
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
        Out.Empty();
        if (InSlotIdx >= MaterialStandings.Num<UINT32>())
        {
            return;
        }
        const EMaterialSlotStanding& Standing = MaterialStandings[InSlotIdx];
        for (INT32 i = 0; i < Standing.ConstantBufferSlots.Num<INT32>(); i++)
        {
            Out.Add(Standing.ConstantBufferSlots[i].Data);
        }
    }
    void PPrimitiveComponent::GetMaterialTextureBySlot(UINT32 InSlotIdx, TArray<RMaterialTextureSRV>& Out) const
    {
        Out.Empty();
        if (InSlotIdx >= MaterialStandings.Num<UINT32>())
        {
            return;
        }
        const EMaterialSlotStanding& Standing = MaterialStandings[InSlotIdx];
        const EMaterialAsset* Mat = GetMaterialAsset(InSlotIdx);
        if (!Mat)
        {
            return;
        }
        const EMaterialVariant* Var = Mat->GetFirstVariant();
        if (!Var)
        {
            return;
        }
        const EMaterialReflection& Refl = Var->Reflection;
        const INT32 NumSrvs = Refl.SRVs.Num<INT32>();
        for (INT32 i = 0, n = Standing.TextureSlots.Num<INT32>(); (i < NumSrvs) && (i < n); i++)
        {
            const EMaterialTextureStanding& TexSlot = Standing.TextureSlots[i];
            if ((!!(TexSlot.Texture)) && (!!(TexSlot.Texture->GetRenderResource())) && (!!(TexSlot.Texture->GetRenderResource()->ShaderResourceView)))
            {
                RMaterialTextureSRV Entry;
                Entry.Slot = Refl.SRVs[i].Slot;
                Entry.SRV = TexSlot.Texture->GetRenderResource()->ShaderResourceView;
                Out.Add(Entry);
            }
        }
    }

    static CHAR s_MatNameBuf[256]  = {};
    static bool s_EmitShaderDebug  = false;

    static bool DrawCBFieldEditor(const EMaterialCBField& Field, TArray<UINT8>& Data, INT32 UniqueId)
    {
        ImGui::PushID(UniqueId);
        const CHAR* FieldType = *Field.Type;
        const BOOL8 bColor = Field.bIsColor;
        FLOAT* Value = (FLOAT*)(&(Data[Field.Offset]));
        const CHAR* FieldName = *Field.Name;
        BOOL8 bChanged = FALSE;

        if (::strcmp(FieldType, "float4") == 0)
        {
            if (bColor)
            {
                bChanged = ImGui::ColorEdit4(FieldName, Value);
            }
            else
            {
                bChanged = ImGui::DragFloat4(FieldName, Value);
            }
        }
        else if (::strcmp(FieldType, "float3") == 0)
        {
            if (bColor)
            {
                bChanged = ImGui::ColorEdit3(FieldName, Value);
            }
            else
            {
                bChanged = ImGui::DragFloat3(FieldName, Value);
            }
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

    void PPrimitiveComponent::RebuildSlot(UINT32 SlotIdx)
    {
        // Ensure EditorSlotStates is large enough
        while (MaterialStandings.Num<UINT32>() <= SlotIdx)
        {
            MaterialStandings.Add(EMaterialSlotStanding());
        }

        EMaterialSlotStanding& Standing = MaterialStandings[SlotIdx];

        const EMaterialAsset* Mat = GetMaterialAsset(SlotIdx);
        if (!Mat)
        {
            Standing.ConstantBufferSlots.Empty();
            Standing.TextureSlots.Empty();
            return;
        }

        const EMaterialVariant* Var = Mat->GetFirstVariant();
        if (!Var)
        {
            Standing.ConstantBufferSlots.Empty();
            Standing.TextureSlots.Empty();
            return;
        }

        const EMaterialReflection& Refl = Var->Reflection;

        // Constant buffers - preserve existing data when structure matches
        const UINT32 NumCbs = Refl.ConstantBuffers.Num<UINT32>();
        if (Standing.ConstantBufferSlots.Num<UINT32>() != NumCbs)
        {
            Standing.ConstantBufferSlots.Empty();
            for (UINT32 CbIdx = 0u; CbIdx < NumCbs; CbIdx++)
            {
                const EMaterialCBRefl& CbRefl = Refl.ConstantBuffers[static_cast<INT32>(CbIdx)];
                EMaterialCBStanding CbState;
                CbState.Name = CbRefl.Name;
                CbState.Data.SetNum(static_cast<INT32>(CbRefl.SizeBytes));
                for (INT32 ByteIdx = 0; ByteIdx < static_cast<INT32>(CbRefl.SizeBytes); ByteIdx++)
                {
                    CbState.Data[ByteIdx] = 0u;
                }
                Standing.ConstantBufferSlots.Add(std::move(CbState));
            }
        }

        // SRV texture slots - preserve existing data when count matches
        const UINT32 NumSrvs = Refl.SRVs.Num<UINT32>();
        if (Standing.TextureSlots.Num<UINT32>() != NumSrvs)
        {
            Standing.TextureSlots.Empty();
            for (UINT32 SrvIdx = 0u; SrvIdx < NumSrvs; SrvIdx++)
            {
                Standing.TextureSlots.Add(EMaterialTextureStanding());
            }
        }
    }

    void PPrimitiveComponent::RebuildAllSlot()
    {
        const UINT32 NumSlots = MaterialSlots.Num<UINT32>();
        // Trim excess slots
        while (MaterialStandings.Num<UINT32>() > NumSlots)
        {
            MaterialStandings.Pop();
        }
        for (UINT32 SlotIdx = 0u; SlotIdx < NumSlots; SlotIdx++)
        {
            RebuildSlot(SlotIdx);
        }
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

        if (ImGui::TreeNode("Materials"))
        {
            const INT32 NumSlots = MaterialSlots.Num<INT32>();

            for (INT32 i = 0; i < NumSlots; i++)
            {
                const UINT32 SlotIdx = (UINT32)i;
                const EMaterialAsset* SlotMat = MaterialSlots[i];
                const char* matName = SlotMat ? *SlotMat->GetName() : "<None>";
                ImGui::Text("Slot [%d]: %s", i, matName);

                ImGui::PushID(i);
                ImGui::InputText("##name", s_MatNameBuf, sizeof(s_MatNameBuf));
                ImGui::SameLine();
                if (ImGui::Button("Load"))
                {
                    const EMaterialAsset* NewMat = nullptr;
                    EMaterialAssetManager::GetManagerSingleton()->LoadOrCompileMaterialAsset(
                        EEngineSettings::ENGINE_MATERIAL_OUTPUT_DIR,
                        EString(s_MatNameBuf),
                        EEngineSettings::ENGINE_MATERIAL_SOURCE_DIR,
                        EEngineSettings::ENGINE_MATERIAL_SHADER_INCLUDE_DIR,
                        NewMat,
                        s_EmitShaderDebug ? TRUE : FALSE);
                    SetMaterialAsset(SlotIdx, NewMat);
                    RebuildSlot(SlotIdx);
                }
                ImGui::SameLine();
                ImGui::Checkbox("Shader Debug", &s_EmitShaderDebug);
                ImGui::SameLine();
                if (ImGui::Button("Clear"))
                {
                    SetMaterialAsset(SlotIdx, nullptr);
                    RebuildSlot(SlotIdx);
                }
                ImGui::PopID();
            }

            if (NumSlots == 0)
            {
                if (ImGui::Button("Add Slot"))
                {
                    SetMaterialSlotsNum(1u);
                    RebuildSlot(0u);
                }
            }

            static INT32 StaticSelectedSlot = 0;
            const INT32 MaxSlot = (NumSlots > 0) ? (NumSlots - 1) : 0;
            ImGui::SliderInt("Edit Slot##slot", &StaticSelectedSlot, 0, MaxSlot);

            if (StaticSelectedSlot < NumSlots && MaterialSlots[StaticSelectedSlot] != nullptr)
            {
                const EMaterialAsset* EditMat = MaterialSlots[StaticSelectedSlot];
                const EMaterialVariant* Var = EditMat->GetFirstVariant();

                if (Var)
                {
                    EString ParamHeader = EString("Parameters [Slot ") + EString::FromInt((UINT32)StaticSelectedSlot) + EString("]");
                    if (ImGui::TreeNode(*ParamHeader))
                    {
                        const EMaterialReflection& Refl = Var->Reflection;

                        // Ensure editor state exists for this slot
                        if (MaterialStandings.Num<INT32>() > StaticSelectedSlot)
                        {
                            EMaterialSlotStanding& Standing = MaterialStandings[StaticSelectedSlot];

                            // Constant buffers
                            for (INT32 CbIdx = 0; CbIdx < Refl.ConstantBuffers.Num<INT32>(); CbIdx++)
                            {
                                const EMaterialCBRefl& CbRefl = Refl.ConstantBuffers[CbIdx];
                                ImGui::PushID(CbIdx);
                                if (ImGui::TreeNode(*CbRefl.Name))
                                {
                                    if (CbIdx < Standing.ConstantBufferSlots.Num<INT32>())
                                    {
                                        EMaterialCBStanding& CbStanding = Standing.ConstantBufferSlots[CbIdx];
                                        for (INT32 FIdx = 0; FIdx < CbRefl.Fields.Num<INT32>(); FIdx++)
                                        {
                                            if (DrawCBFieldEditor(CbRefl.Fields[FIdx], CbStanding.Data, FIdx))
                                            {
                                                MarkMaterialAsDirty();
                                            }
                                        }
                                    }
                                    ImGui::TreePop();
                                }
                                ImGui::PopID();
                            }

                            // SRV texture slots
                            for (INT32 SrvIdx = 0; SrvIdx < Refl.SRVs.Num<INT32>(); SrvIdx++)
                            {
                                const EMaterialSRVRefl& Srv = Refl.SRVs[SrvIdx];
                                ImGui::PushID(SrvIdx);

                                const char* texName = "<None>";
                                if (SrvIdx < Standing.TextureSlots.Num<INT32>() && Standing.TextureSlots[SrvIdx].Texture)
                                {
                                    texName = *Standing.TextureSlots[SrvIdx].Texture->GetAssetName();
                                }
                                ImGui::Text("%s (slot %u): %s", *Srv.Name, Srv.Slot, texName);

                                if (SrvIdx < Standing.TextureSlots.Num<INT32>())
                                {
                                    EMaterialTextureStanding& TexSlot = Standing.TextureSlots[SrvIdx];
                                    EString PathLabel = EString("Path##") + EString::FromInt(static_cast<UINT32>(SrvIdx));
                                    ImGui::InputText(*PathLabel, TexSlot.PathBuf, sizeof(TexSlot.PathBuf));
                                    ImGui::SameLine();
                                    EString LoadLabel = EString("Load##") + EString::FromInt(static_cast<UINT32>(SrvIdx));
                                    if (ImGui::Button(*LoadLabel))
                                    {
                                        const ETexture2DAsset* NewTex = nullptr;
                                        TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, EString(TexSlot.PathBuf), NewTex);
                                        TexSlot.Texture = NewTex;
                                        MarkMaterialAsDirty();
                                    }
                                }

                                ImGui::PopID();
                            }
                        }

                        ImGui::TreePop();
                    }
                }
            }

            ImGui::TreePop();
        }
    }
#endif // _EDITOR_ONLY

};
