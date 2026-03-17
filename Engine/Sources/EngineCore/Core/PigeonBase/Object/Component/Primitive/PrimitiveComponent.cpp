#include "PrimitiveComponent.h"
#include <MaterialAsset/MaterialAsset.h>

#if _EDITOR_ONLY
#include <TextureAsset/TextureAsset.h>
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
        : PSceneComponent(), IsCastShadow(FALSE), IsReceiveShadow(FALSE), IsRenderHidden(FALSE)
#if _EDITOR_ONLY
        , MaterialParamsDirty(FALSE)
#endif
    {
        POBJ_DEBUGNAME_SET(this, "PrimitiveComponent");
    }
    PPrimitiveComponent::~PPrimitiveComponent()
    {

    }
    BOOL32 PPrimitiveComponent::IsPrimitiveCastShadow() const
    {
        return IsCastShadow;
    }
    BOOL32 PPrimitiveComponent::IsPrimitiveReceiveShadow() const
    {
        return IsReceiveShadow;
    }
    BOOL32 PPrimitiveComponent::IsPrimitiveRenderHidden() const
    {
        return IsRenderHidden;
    }
    void PPrimitiveComponent::SetPrimitiveCastShadow(BOOL32 InIsCastShadow)
    {
        IsCastShadow = InIsCastShadow;
    }
    void PPrimitiveComponent::SetPrimitiveReceiveShadow(BOOL32 InIsReceiveShadow)
    {
        IsReceiveShadow = InIsReceiveShadow;
    }
    void PPrimitiveComponent::SetPrimitiveRenderHidden(BOOL32 InIsRenderHidden)
    {
        IsRenderHidden = InIsRenderHidden;
    }
    const EMaterialAsset* PPrimitiveComponent::GetMaterialAsset(UINT32 SlotIdx) const
    {
        if (SlotIdx < MaterialSlots.Num<UINT32>())
        {
            return MaterialSlots[SlotIdx];
        }
        return nullptr;
    }
    UINT32 PPrimitiveComponent::GetMaterialSlotCount() const
    {
        return MaterialSlots.Num<UINT32>();
    }
    void PPrimitiveComponent::SetMaterialAsset(const EMaterialAsset* InMaterialAsset)
    {
        if (MaterialSlots.Num<UINT32>() == 0u)
        {
            MaterialSlots.Add(nullptr);
        }
        MaterialSlots[0] = InMaterialAsset;
        MarkRenderStateAsDirty();
    }
    void PPrimitiveComponent::SetMaterialAsset(UINT32 SlotIdx, const EMaterialAsset* InMaterialAsset)
    {
        if (SlotIdx >= MaterialSlots.Num<UINT32>())
        {
            MaterialSlots.SetNum(static_cast<INT32>(SlotIdx) + 1);
        }
        MaterialSlots[SlotIdx] = InMaterialAsset;
        MarkRenderStateAsDirty();
    }
    void PPrimitiveComponent::SetMaterialSlotsNum(UINT32 Num)
    {
        const UINT32 OldNum = MaterialSlots.Num<UINT32>();
        MaterialSlots.SetNum(static_cast<INT32>(Num));
        for (UINT32 i = OldNum; i < Num; i++)
        {
            MaterialSlots[static_cast<INT32>(i)] = nullptr;
        }
    }

    // Render proxy functions START
    BOOL32 PPrimitiveComponent::CheckIsRenderHidden()const
    {
        return IsPrimitiveRenderHidden();
    }
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
        PSceneComponent::CleanMarkRenderStateDirty();
    }
    // Render proxy functions END

#if _EDITOR_ONLY

    BOOL32 PPrimitiveComponent::IsEditorMaterialParamsDirty() const
    {
        return MaterialParamsDirty;
    }
    void PPrimitiveComponent::MarkEditorMaterialParamsDirty()
    {
        MaterialParamsDirty = TRUE;
    }
    void PPrimitiveComponent::CleanEditorMaterialParamsDirty()
    {
        MaterialParamsDirty = FALSE;
    }
    void PPrimitiveComponent::GetEditorSlotCBData(UINT32 SlotIdx, TArray<TArray<BYTE>>& Out) const
    {
        Out.Empty();
        if (SlotIdx >= EditorSlotStates.Num<UINT32>()) { return; }
        const PMaterialEditorSlotState& State = EditorSlotStates[static_cast<INT32>(SlotIdx)];
        for (INT32 i = 0; i < State.CBStates.Num<INT32>(); i++)
        {
            Out.Add(State.CBStates[i].Data);
        }
    }

    static CHAR s_MatNameBuf[256]  = {};
    static bool s_EmitShaderDebug  = false;

    static bool DrawCBFieldEditor(const EMaterialCBField& Field, TArray<BYTE>& Data, int UniqueId)
    {
        ImGui::PushID(UniqueId);
        const char* type = *Field.Type;
        const bool isColor = (::strstr(*Field.Name, "olor") != nullptr) || (::strstr(*Field.Name, "Tint") != nullptr);
        FLOAT* f = reinterpret_cast<FLOAT*>(&Data[static_cast<INT32>(Field.Offset)]);
        const char* label = *Field.Name;
        bool changed = false;

        if (::strcmp(type, "float4") == 0)
        {
            if (isColor) changed = ImGui::ColorEdit4(label, f);
            else         changed = ImGui::DragFloat4(label, f);
        }
        else if (::strcmp(type, "float3") == 0)
        {
            if (isColor) changed = ImGui::ColorEdit3(label, f);
            else         changed = ImGui::DragFloat3(label, f);
        }
        else if (::strcmp(type, "float2") == 0)
        {
            changed = ImGui::DragFloat2(label, f);
        }
        else if (::strcmp(type, "float") == 0 || ::strcmp(type, "float1") == 0)
        {
            changed = ImGui::DragFloat(label, f);
        }
        else if (::strcmp(type, "int4") == 0)
        {
            changed = ImGui::DragScalarN(label, ImGuiDataType_S32, f, 4);
        }
        else if (::strcmp(type, "int3") == 0)
        {
            changed = ImGui::DragScalarN(label, ImGuiDataType_S32, f, 3);
        }
        else if (::strcmp(type, "int2") == 0)
        {
            changed = ImGui::DragScalarN(label, ImGuiDataType_S32, f, 2);
        }
        else if (::strcmp(type, "int") == 0 || ::strcmp(type, "int1") == 0)
        {
            changed = ImGui::DragScalar(label, ImGuiDataType_S32, f);
        }
        else if (::strcmp(type, "uint4") == 0)
        {
            changed = ImGui::DragScalarN(label, ImGuiDataType_U32, f, 4);
        }
        else if (::strcmp(type, "uint3") == 0)
        {
            changed = ImGui::DragScalarN(label, ImGuiDataType_U32, f, 3);
        }
        else if (::strcmp(type, "uint2") == 0)
        {
            changed = ImGui::DragScalarN(label, ImGuiDataType_U32, f, 2);
        }
        else if (::strcmp(type, "uint") == 0 || ::strcmp(type, "uint1") == 0)
        {
            changed = ImGui::DragScalar(label, ImGuiDataType_U32, f);
        }
        else
        {
            ImGui::Text("%s %s (unsupported)", type, label);
        }
        ImGui::PopID();
        return changed;
    }

    void PPrimitiveComponent::RebuildEditorSlotState(UINT32 SlotIdx)
    {
        // Ensure EditorSlotStates is large enough
        while (EditorSlotStates.Num<UINT32>() <= SlotIdx)
        {
            EditorSlotStates.Add(PMaterialEditorSlotState());
        }

        PMaterialEditorSlotState& State = EditorSlotStates[static_cast<INT32>(SlotIdx)];
        State.CBStates.Empty();
        State.TextureSlots.Empty();

        const EMaterialAsset* Mat = GetMaterialAsset(SlotIdx);
        if (!Mat) { return; }

        const EMaterialVariant* Var = Mat->GetFirstVariant();
        if (!Var) { return; }

        const EMaterialReflection& Refl = Var->Reflection;

        // Constant buffers
        for (UINT32 CbIdx = 0u, NumCbs = Refl.ConstantBuffers.Num<UINT32>(); CbIdx < NumCbs; CbIdx++)
        {
            const EMaterialCBRefl& CbRefl = Refl.ConstantBuffers[static_cast<INT32>(CbIdx)];
            PMaterialEditorCBState CbState;
            CbState.CBName = CbRefl.Name;
            CbState.Data.SetNum(static_cast<INT32>(CbRefl.SizeBytes));
            // Zero-initialize
            for (INT32 ByteIdx = 0; ByteIdx < static_cast<INT32>(CbRefl.SizeBytes); ByteIdx++)
            {
                CbState.Data[ByteIdx] = 0u;
            }
            State.CBStates.Add(std::move(CbState));
        }

        // SRV texture slots
        for (UINT32 SrvIdx = 0u, NumSrvs = Refl.SRVs.Num<UINT32>(); SrvIdx < NumSrvs; SrvIdx++)
        {
            State.TextureSlots.Add(PMaterialEditorTextureSlot());
        }
    }

    void PPrimitiveComponent::RebuildAllEditorSlotStates()
    {
        EditorSlotStates.Empty();
        const UINT32 NumSlots = MaterialSlots.Num<UINT32>();
        for (UINT32 SlotIdx = 0u; SlotIdx < NumSlots; SlotIdx++)
        {
            RebuildEditorSlotState(SlotIdx);
        }
    }

    void PPrimitiveComponent::OnSelectedByImGui()
    {
        PSceneComponent::OnSelectedByImGui();
        RebuildAllEditorSlotStates();
    }

    void PPrimitiveComponent::GenerateComponentDetail()
    {
        PSceneComponent::GenerateComponentDetail();

        if (ImGui::TreeNode("Materials"))
        {
            const INT32 NumSlots = MaterialSlots.Num<INT32>();

            for (INT32 i = 0; i < NumSlots; i++)
            {
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
                    SetMaterialAsset(static_cast<UINT32>(i), NewMat);
                    RebuildEditorSlotState(static_cast<UINT32>(i));
                }
                ImGui::SameLine();
                ImGui::Checkbox("Shader Debug", &s_EmitShaderDebug);
                ImGui::SameLine();
                if (ImGui::Button("Clear"))
                {
                    SetMaterialAsset(static_cast<UINT32>(i), nullptr);
                    RebuildEditorSlotState(static_cast<UINT32>(i));
                }
                ImGui::PopID();
            }

            if (NumSlots == 0)
            {
                if (ImGui::Button("Add Slot"))
                {
                    SetMaterialSlotsNum(1u);
                    RebuildEditorSlotState(0u);
                }
            }

            static INT32 s_SelectedSlot = 0;
            const INT32 MaxSlot = (NumSlots > 0) ? (NumSlots - 1) : 0;
            ImGui::SliderInt("Edit Slot##slot", &s_SelectedSlot, 0, MaxSlot);

            if (s_SelectedSlot < NumSlots && MaterialSlots[s_SelectedSlot] != nullptr)
            {
                const EMaterialAsset* EditMat = MaterialSlots[s_SelectedSlot];
                const EMaterialVariant* Var = EditMat->GetFirstVariant();

                if (Var)
                {
                    EString ParamHeader = EString("Parameters [Slot ") + EString::FromInt(static_cast<UINT32>(s_SelectedSlot)) + EString("]");
                    if (ImGui::TreeNode(*ParamHeader))
                    {
                        const EMaterialReflection& Refl = Var->Reflection;

                        // Ensure editor state exists for this slot
                        if (EditorSlotStates.Num<UINT32>() > static_cast<UINT32>(s_SelectedSlot))
                        {
                            PMaterialEditorSlotState& State = EditorSlotStates[s_SelectedSlot];

                            // Constant buffers
                            for (INT32 CbIdx = 0; CbIdx < Refl.ConstantBuffers.Num<INT32>(); CbIdx++)
                            {
                                const EMaterialCBRefl& CbRefl = Refl.ConstantBuffers[CbIdx];
                                ImGui::PushID(CbIdx);
                                if (ImGui::TreeNode(*CbRefl.Name))
                                {
                                    if (CbIdx < State.CBStates.Num<INT32>())
                                    {
                                        PMaterialEditorCBState& CbState = State.CBStates[CbIdx];
                                        for (INT32 FIdx = 0; FIdx < CbRefl.Fields.Num<INT32>(); FIdx++)
                                        {
                                            if (DrawCBFieldEditor(CbRefl.Fields[FIdx], CbState.Data, FIdx))
                                            {
                                                MarkEditorMaterialParamsDirty();
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
                                if (SrvIdx < State.TextureSlots.Num<INT32>() && State.TextureSlots[SrvIdx].Texture)
                                {
                                    texName = *State.TextureSlots[SrvIdx].Texture->GetAssetName();
                                }
                                ImGui::Text("%s (slot %u): %s", *Srv.Name, Srv.Slot, texName);

                                if (SrvIdx < State.TextureSlots.Num<INT32>())
                                {
                                    PMaterialEditorTextureSlot& TexSlot = State.TextureSlots[SrvIdx];
                                    EString PathLabel = EString("Path##") + EString::FromInt(static_cast<UINT32>(SrvIdx));
                                    ImGui::InputText(*PathLabel, TexSlot.PathBuf, sizeof(TexSlot.PathBuf));
                                    ImGui::SameLine();
                                    EString LoadLabel = EString("Load##") + EString::FromInt(static_cast<UINT32>(SrvIdx));
                                    if (ImGui::Button(*LoadLabel))
                                    {
                                        const ETexture2DAsset* NewTex = nullptr;
                                        TryLoadTexture2D(EBaseSettings::ENGINE_TEXTURE_PATH, EString(TexSlot.PathBuf), NewTex);
                                        TexSlot.Texture = NewTex;
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
