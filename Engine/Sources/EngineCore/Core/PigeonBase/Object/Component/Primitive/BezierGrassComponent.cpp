#include "BezierGrassComponent.h"
#include <RenderProxy/BezierGrassSceneProxy.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

#if _EDITOR_ONLY
#include <imgui.h>
#endif

namespace PigeonEngine
{

    static void RegisterClassTypes()
    {
        RegisterClassType<PBezierGrassComponent, PPrimitiveComponent>();
    }

    PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

    PBezierGrassComponent::PBezierGrassComponent()
        : PPrimitiveComponent()
        , SceneProxy(nullptr)
        , UpdateState(PBezierGrassUpdateState::BEZIER_GRASS_UPDATE_STATE_NONE)
    {
        POBJ_DEBUGNAME_SET(this, "BezierGrassComponent");
        SetProperty(bWireframe, 0.f, 0.5f, Color4(0.f, 0.f, 0.f), Color4(1.f, 1.f, 1.f));
        BaseHeight = 0.5f;
        OffsetHeight = 0.5f;
        LengthX = 1.0f;
        LengthZ = 1.0f;
        NumX = 1;
        NumZ = 1;
        GenerateInstanceData(Vector3::Zero(), BaseHeight, OffsetHeight, LengthX, LengthZ, NumX, NumZ);
    }
    PBezierGrassComponent::~PBezierGrassComponent()
    {
        PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("SceneProxy is not null in bezier grass component distruction."), (!SceneProxy));
    }
    void PBezierGrassComponent::SetProperty(BOOL8 InIsWireframe, FLOAT InLOD, FLOAT InLeafWidth, Color4 InRootColor, Color4 InTipColor)
    {
        Property.bWireframe = InIsWireframe;
        Property.LOD        = InLOD;
        Property.LeafWidth  = InLeafWidth;
        Property.RootColor  = InRootColor;
        Property.TipColor   = InTipColor;

        MarkAsDirty(PBezierGrassUpdateState::BEZIER_GRASS_UPDATE_STATE_ASSET);
    }
#if 0
    void InitIndices(const TArray<uint32>& InBodyParts, bool InIsSplitBentPart) noexcept
    {
        const int32 NumLODs = InBodyParts.Num();
        check(NumLODs > 0);

        bSplitBentPart = InIsSplitBentPart;
        IndexData.Reset();
        BodyParts.Reset();
        BodyParts.Append(InBodyParts);
        NumIndices = 0;
        NumLODIndices.Reset();
        IndexOffsets.Reset();

        auto GenerateLODIndices = [InIsSplitBentPart](const TArray<uint32>& InBodyParts, uint32 InLODIndex, TArray<uint16>& OutIndices, int32& OutNumIndices)->void
        {
            const uint32 UsedBodyParts = InBodyParts[InLODIndex];

            const uint32 NumFirstParts = InIsSplitBentPart ? ((UsedBodyParts + 1) / 2) : UsedBodyParts;
            const uint32 NumSecondParts = UsedBodyParts - NumFirstParts;

            const uint32 NumTriangles = UsedBodyParts * 2 + 1;

            OutNumIndices = (int32)NumTriangles * 3;
            OutIndices.Reset();
            OutIndices.Reserve(OutNumIndices);

            for (uint32 PartIndex = 0; PartIndex < NumFirstParts; PartIndex++)
            {
                OutIndices.Add(static_cast<uint16>(PartIndex * 2 + 0));
                OutIndices.Add(static_cast<uint16>((PartIndex + 1) * 2 + 0));
                OutIndices.Add(static_cast<uint16>(PartIndex * 2 + 1));
                OutIndices.Add(static_cast<uint16>((PartIndex + 1) * 2 + 0));
                OutIndices.Add(static_cast<uint16>((PartIndex + 1) * 2 + 1));
                OutIndices.Add(static_cast<uint16>(PartIndex * 2 + 1));
            }

            if (NumSecondParts > 0)
            {
                const uint32 StartVertexIndex = NumFirstParts * 2 + 2;
                for (uint32 PartIndex = 0; PartIndex < NumSecondParts; PartIndex++)
                {
                    OutIndices.Add(static_cast<uint16>(PartIndex * 2 + 0 + StartVertexIndex));
                    OutIndices.Add(static_cast<uint16>((PartIndex + 1) * 2 + 0 + StartVertexIndex));
                    OutIndices.Add(static_cast<uint16>(PartIndex * 2 + 1 + StartVertexIndex));
                    OutIndices.Add(static_cast<uint16>((PartIndex + 1) * 2 + 0 + StartVertexIndex));
                    OutIndices.Add(static_cast<uint16>((PartIndex + 1) * 2 + 1 + StartVertexIndex));
                    OutIndices.Add(static_cast<uint16>(PartIndex * 2 + 1 + StartVertexIndex));
                }
            }

            const uint32 UsedNumVertices = InIsSplitBentPart ? (UsedBodyParts * 2 + 5) : (UsedBodyParts * 2 + 3);
            OutIndices.Add(static_cast<uint16>(UsedNumVertices - 3));
            OutIndices.Add(static_cast<uint16>(UsedNumVertices - 1));
            OutIndices.Add(static_cast<uint16>(UsedNumVertices - 2));
        };

        TArray<TArray<uint16>> LODIndices;
        LODIndices.Reserve(NumLODs);
        NumLODIndices.Reserve(NumLODs);
        IndexOffsets.Reserve(NumLODs);
        for (int32 LODIndex = 0; LODIndex < NumLODs; LODIndex++)
        {
            TArray<uint16>& Indices = LODIndices.AddDefaulted_GetRef();
            int32 CurrentNumIndices = 0;
            GenerateLODIndices(BodyParts, (uint32)LODIndex, Indices, CurrentNumIndices);
            NumLODIndices.Add(CurrentNumIndices);
            IndexOffsets.Add(NumIndices);
            NumIndices += CurrentNumIndices;
        }

        IndexData.Reserve(NumIndices);
        for (int32 LODIndex = 0, UsedNumLODs = LODIndices.Num(); LODIndex < UsedNumLODs; LODIndex++)
        {
            IndexData.Append(LODIndices[LODIndex]);
        }
    }
    void InitPosition(const TArray<uint32>& InBodyParts, bool InIsSplitBentPart) noexcept
    {
        constexpr float T15BitFloat = (float)(0x7fffu);
        constexpr float T14BitFloat = (float)(0x3fffu);

        if (!VertexData)
        {
            VertexData = new TStaticMeshVertexData<FBezierGrassPositionVertex>{ false };
        }

        const int32 NumLODs = InBodyParts.Num();
        check(NumLODs > 0);

        bSplitBentPart = InIsSplitBentPart;
        BodyParts.Reset();
        BodyParts.Append(InBodyParts);
        NumVertices = 0;
        NumLODVertices.Reset();
        VertexOffsets.Reset();
        BentBezierTs.Reset();

        struct FRawPositionData
        {
            float	TParam = 0.0f;
            float	PrevTParam = 0.0f;
            bool	bSecondPart = false;
        };

        auto GenerateLODTVertices = [InIsSplitBentPart](const TArray<uint32>& InBodyParts, uint32 InLODIndex, TArray<FRawPositionData>& OutTVertices, int32& OutNumVertices, float& OutMidT)->void
        {
            const uint32 UsedNumLODs = (uint32)(InBodyParts.Num());
            const uint32 UsedBodyParts = InBodyParts[InLODIndex];
            const bool bGeneratePrevT = (InLODIndex < (UsedNumLODs - 1)) && (UsedBodyParts > BezierGrass::MinBodyParts);

            const uint32 NumFirstParts = InIsSplitBentPart ? ((UsedBodyParts + 1) / 2) : UsedBodyParts;
            const uint32 NumSecondParts = UsedBodyParts - NumFirstParts;
            const uint32 UsedNumVertices = InIsSplitBentPart ? (UsedBodyParts * 2 + 5) : (UsedBodyParts * 2 + 3);

            OutNumVertices = UsedNumVertices;
            OutMidT = 0.0f;
            OutTVertices.Reset();
            OutTVertices.Reserve((int32)UsedNumVertices);

            TArray<float> PrevTFirstVertices;
            TArray<float> PrevTSecondVertices;
            if (bGeneratePrevT)
            {
                const uint32 NumPrevTParts = InBodyParts[InLODIndex + 1];
                const uint32 NumTs = NumPrevTParts + 2;
                const uint32 NumPrevTFirstParts = InIsSplitBentPart ? ((NumPrevTParts + 1) / 2) : NumPrevTParts;
                const uint32 NumPrevTSecondParts = NumPrevTParts - NumPrevTFirstParts;

                PrevTFirstVertices.Reserve((int32)NumPrevTFirstParts + 1);
                for (uint32 PartIndex = 0, NumParts = NumPrevTFirstParts + 1; PartIndex < NumParts; PartIndex++)
                {
                    const float TParam = ((float)PartIndex) / ((float)(NumTs - 1));
                    PrevTFirstVertices.Add(TParam);
                }

                PrevTSecondVertices.Reserve((int32)NumPrevTSecondParts + 1);
                for (uint32 PartIndex = 0, NumParts = NumPrevTSecondParts + 1; PartIndex < NumParts; PartIndex++)
                {
                    const float TParam = ((float)(PartIndex + NumPrevTFirstParts)) / ((float)(NumTs - 1));
                    PrevTSecondVertices.Add(TParam);
                }
            }

            // First Part
            {
                const FRawPositionData CurrentTParam{ 0.0f, 0.0f, false };

                OutTVertices.Add(CurrentTParam);
                OutTVertices.Add(CurrentTParam);
            }
            for (uint32 PartIndex = 0; PartIndex < NumFirstParts; PartIndex++)
            {
                const float TParam = ((float)(PartIndex + 1)) / ((float)(UsedBodyParts + 1));

                float PrevTParam = TParam;
                if (bGeneratePrevT && (PartIndex != (NumFirstParts - 1)))
                {
                    float ClosestTDist = 1.0f;
                    int32 ClosestTDistIndex = 0;
                    for (int32 PrevTIndex = 0, NumPrevTs = PrevTFirstVertices.Num(); PrevTIndex < NumPrevTs; PrevTIndex++)
                    {
                        float TParamDist = FMath::Abs(TParam - PrevTFirstVertices[PrevTIndex]);
                        if (TParamDist < ClosestTDist)
                        {
                            ClosestTDist = TParamDist;
                            ClosestTDistIndex = PrevTIndex;
                        }
                    }
                    PrevTParam = PrevTFirstVertices[ClosestTDistIndex];
                }

                const FRawPositionData CurrentTParam{ TParam, PrevTParam, false };

                OutTVertices.Add(CurrentTParam);
                OutTVertices.Add(CurrentTParam);
            }

            // Second Part
            if (NumSecondParts > 0)
            {
                {
                    const FRawPositionData CurrentTParam{ OutTVertices.Last().TParam, OutTVertices.Last().PrevTParam, true };

                    OutMidT = FMath::FloorToFloat(CurrentTParam.TParam * T15BitFloat) / T15BitFloat;

                    OutTVertices.Add(CurrentTParam);
                    OutTVertices.Add(CurrentTParam);
                }
                for (uint32 PartIndex = 0; PartIndex < NumSecondParts; PartIndex++)
                {
                    const float TParam = ((float)(PartIndex + NumFirstParts + 1)) / ((float)(UsedBodyParts + 1));

                    float PrevTParam = TParam;
                    if (bGeneratePrevT && (PartIndex != (NumSecondParts - 1)))
                    {
                        float ClosestTDist = 1.0f;
                        int32 ClosestTDistIndex = 0;
                        for (int32 PrevTIndex = 0, NumPrevTs = PrevTSecondVertices.Num(); PrevTIndex < NumPrevTs; PrevTIndex++)
                        {
                            float TParamDist = FMath::Abs(TParam - PrevTSecondVertices[PrevTIndex]);
                            if (TParamDist < ClosestTDist)
                            {
                                ClosestTDist = TParamDist;
                                ClosestTDistIndex = PrevTIndex;
                            }
                        }
                        PrevTParam = PrevTSecondVertices[ClosestTDistIndex];
                    }

                    const FRawPositionData CurrentTParam{ TParam, PrevTParam, true };

                    OutTVertices.Add(CurrentTParam);
                    OutTVertices.Add(CurrentTParam);
                }
            }

            // Tip
            OutTVertices.Add(FRawPositionData{ 1.0f, 1.0f, true });
        };
        auto GenerateLODVertices = [](const TArray<FRawPositionData>& InTVertices, TArray<FBezierGrassPositionVertex>& OutVertices)->void
        {
            OutVertices.Reset();
            OutVertices.Reserve(InTVertices.Num());
            for (int32 VertexIndex = 0, UsedNumVertices = InTVertices.Num(); VertexIndex < UsedNumVertices; VertexIndex++)
            {
                const bool bPosiOffset = (VertexIndex & 0x1) == 0;
                const FRawPositionData& VertexRawData = InTVertices[VertexIndex];
                const float TargetT = VertexRawData.TParam;
                const float PrevT = VertexRawData.PrevTParam;
                const bool bSecondPart = VertexRawData.bSecondPart;
                FBezierGrassPositionVertex& VertexData = OutVertices.AddDefaulted_GetRef();
                VertexData.Position =
                    (((uint32)(TargetT * T15BitFloat)) << 17u) |
                    (((uint32)(PrevT * T14BitFloat)) << 3u) |
                    (bSecondPart ? 0x4u : 0x0u) |
                    (bPosiOffset ? 0x2u : 0x0u);
            }
            const uint32 LastPosition = OutVertices.Last().Position;
            OutVertices.Last().Position = ((LastPosition >> 2u) << 2u) | 0x1u;
        };

        TArray<TArray<FRawPositionData>> LODTVertices;
        LODTVertices.Reserve(NumLODs);
        NumLODVertices.Reserve(NumLODs);
        VertexOffsets.Reserve(NumLODs);
        BentBezierTs.Reserve(NumLODs);
        for (int32 LODIndex = 0; LODIndex < NumLODs; LODIndex++)
        {
            TArray<FRawPositionData>& TVertices = LODTVertices.AddDefaulted_GetRef();
            int32 CurrentNumVertices = 0;
            float CurrentMidT = 0.0f;
            GenerateLODTVertices(BodyParts, (uint32)LODIndex, TVertices, CurrentNumVertices, CurrentMidT);
            NumLODVertices.Add(CurrentNumVertices);
            VertexOffsets.Add(NumVertices);
            BentBezierTs.Add(CurrentMidT);
            NumVertices += CurrentNumVertices;
        }

        TArray<FBezierGrassPositionVertex> FinalVertices;
        FinalVertices.Reserve(NumVertices);
        for (int32 LODIndex = 0, UsedNumLODs = LODTVertices.Num(); LODIndex < UsedNumLODs; LODIndex++)
        {
            TArray<FBezierGrassPositionVertex> Vertices{};
            GenerateLODVertices(LODTVertices[LODIndex], Vertices);
            FinalVertices.Append(MoveTemp(Vertices));
        }

        VertexData->ResizeBuffer(NumVertices, EResizeBufferFlags::None);
        FMemory::Memcpy(VertexData->GetDataPointer(), FinalVertices.GetData(), sizeof(FBezierGrassPositionVertex) * NumVertices);
    }
#endif
    void PBezierGrassComponent::GenerateInstanceData(const Vector3& InOrigin, FLOAT InBaseHeight, FLOAT InOffsetHeight, FLOAT InLengthX, FLOAT InLengthZ, UINT32 InNumX, UINT32 InNumZ)
    {
        if (InNumX == 0u || InNumZ == 0u || InLengthX < 1e-3f || InLengthZ < 1e-3f || InBaseHeight < 1e-3f || InOffsetHeight < 0.f)
        {
            return;
        }

        auto _RandomFloat = [](FLOAT X, FLOAT Y)->FLOAT
        {
            return (EMath::Frac(EMath::Sin(X * 12.9898f + Y * 78.233f) * 43758.5453123f));
        };
        auto _RandomVector2 = [](FLOAT X, FLOAT Y, FLOAT& nX, FLOAT& nY)->void
        {
            nX = EMath::Frac(EMath::Sin(X * 127.1f + Y * 311.7f) * 43758.5453123f);
            nY = EMath::Frac(EMath::Sin(X * 269.5f + Y * 183.3f) * 43758.5453123f);
        };

        UINT32 NumInstances = EMath::Min((UINT32)(BEZIER_GRASS_INSTANCE_PER_GROUP_MAX_NUM), InNumX * InNumZ);
        if (InstanceData.Num<UINT32>() != NumInstances)
        {
            InstanceData.SetNum(NumInstances);
        }

        {
            Vector3 TempOrigin(InOrigin);
            FLOAT DeltaX = InLengthX / static_cast<FLOAT>(InNumX), DeltaZ = InLengthZ / static_cast<FLOAT>(InNumZ);
            FLOAT OffsetXStart = -InLengthX * 0.5f + DeltaX * 0.5f, OffsetXEnd = InLengthX * 0.5f - DeltaX * 0.5f;
            FLOAT OffsetZStart = -InLengthZ * 0.5f + DeltaZ * 0.5f, OffsetZEnd = InLengthZ * 0.5f - DeltaZ * 0.5f;
            for (UINT32 Z = 0u; Z < InNumZ; Z++)
            {
                for (UINT32 X = 0u; X < InNumX; X++)
                {
                    UINT32 InstanceIndex = Z * InNumX + X;
                    if (InstanceIndex >= NumInstances)
                    {
                        //TODO return the over size part.
                        continue;
                    }
                    FLOAT tX = static_cast<FLOAT>(X); FLOAT tZ = static_cast<FLOAT>(Z);
                    if (InNumX > 1) { tX = tX / static_cast<FLOAT>(InNumX - 1u); }
                    if (InNumZ > 1) { tZ = tZ / static_cast<FLOAT>(InNumZ - 1u); }
                    FLOAT nX, nZ;
                    _RandomVector2(tX, tZ, nX, nZ);
                    Vector3 TempOffset(OffsetXStart * (1.f - tX) + OffsetXEnd * tX + nX * DeltaX * 0.5f, 0.f, OffsetZStart * (1.f - tZ) + OffsetZEnd * tZ + nZ * DeltaZ * 0.5f);
                    EBezierGrassInstanceData& Data = InstanceData[InstanceIndex];
                    Data.Origin = TempOrigin + TempOffset;
                    FLOAT TempDirection = EMath::DegreesToRadians(_RandomFloat(tX * 15.f, tZ * 15.f) * 360.f);
                    FLOAT TempSin, TempCos;
                    EMath::SinCos(TempSin, TempCos, TempDirection);
                    Data.Direction = Vector3::Normalize(Vector3(TempCos, 0.f, TempSin));
                    Data.Tip = Vector2(0.f, InBaseHeight + InOffsetHeight + _RandomFloat(tX, tZ) * InOffsetHeight);
                    Data.Tilt = EMath::DegreesToRadians((0.3f + 0.3f * _RandomFloat(tX * 5.f, tZ * 5.f)) * 90.f);
                    Data.Bend = 0.2f + 0.6f * _RandomFloat(tX * 10.f, tZ * 10.f);
                }
            }
        }

        MarkAsDirty(PBezierGrassUpdateState::BEZIER_GRASS_UPDATE_STATE_INSTANCE);
    }

    // Render proxy functions START
    UINT8 PBezierGrassComponent::GetUpdateRenderState()const
    {
        return UpdateState;
    }
    RBezierGrassSceneProxy* PBezierGrassComponent::CreateSceneProxy()
    {
        PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Try creating bezier grass scene proxy, but already exist scene proxy."), (!SceneProxy));
        SceneProxy = new RBezierGrassSceneProxy(this);
        return SceneProxy;
    }
    void PBezierGrassComponent::CreateRenderState()
    {
        PPrimitiveComponent::CreateRenderState();
        if (ShouldRender())
        {
            this->GetWorld()->GetRenderScene()->AddBezierGrass(this);
        }
    }
    void PBezierGrassComponent::DestroyRenderState()
    {
        this->GetWorld()->GetRenderScene()->RemoveBezierGrass(this);
        PPrimitiveComponent::DestroyRenderState();
    }
    void PBezierGrassComponent::SendUpdateRenderState()
    {
        if (ShouldRender() && IsRenderStateDirty())
        {
            this->GetWorld()->GetRenderScene()->UpdateBezierGrass(this);
        }
        PPrimitiveComponent::SendUpdateRenderState();
    }
    void PBezierGrassComponent::MarkAsDirty(PBezierGrassUpdateState InState)
    {
        if (InState == PBezierGrassUpdateState::BEZIER_GRASS_UPDATE_STATE_MATRIX)
        {
            MarkRenderTransformAsDirty();
        }
        else
        {
            UpdateState |= InState;
            MarkRenderStateAsDirty();
        }
    }
    void PBezierGrassComponent::MarkRenderTransformAsDirty()
    {
        UpdateState |= PBezierGrassUpdateState::BEZIER_GRASS_UPDATE_STATE_MATRIX;
        PPrimitiveComponent::MarkRenderTransformAsDirty();
    }
    void PBezierGrassComponent::MarkRenderStateAsDirty()
    {
        PPrimitiveComponent::MarkRenderStateAsDirty();
    }
    void PBezierGrassComponent::CleanMarkRenderStateDirty()
    {
        UpdateState = PBezierGrassUpdateState::BEZIER_GRASS_UPDATE_STATE_NONE;
        PPrimitiveComponent::CleanMarkRenderStateDirty();
    }
    // Render proxy functions END

#if _EDITOR_ONLY
    void PBezierGrassComponent::GenerateComponentOutline(const class PActorComponent* WorldCurrentSelectedComponent)
    {
        PPrimitiveComponent::GenerateComponentOutline(WorldCurrentSelectedComponent);
    }
    void PBezierGrassComponent::GenerateComponentDetail()
    {
        PPrimitiveComponent::GenerateComponentDetail();

        const BOOL8 bBezierGrassExpand = ImGui::TreeNodeEx("BezierGrass");
        if (bBezierGrassExpand)
        {
            if (ImGui::TreeNodeEx("Property"))
            {
                FLOAT TempLOD = Property.LOD;
                FLOAT TempLeafWidth = Property.LeafWidth;
                FLOAT TempRootColor[3] = { Property.RootColor.r, Property.RootColor.g, Property.RootColor.b };
                FLOAT TempTipColor[3] = {Property.TipColor.r, Property.TipColor.g, Property.TipColor.b};

                const FLOAT LODMin = 0.f, LODMax = (FLOAT)BEZIER_GRASS_MAX_LOD_INDEX;
                const BOOL8 bChangedWireframe = ImGui::Button("ToggleWireframe");
                const BOOL8 bChangedLOD = ImGui::SliderScalar(TEXT("LOD"), ImGuiDataType_Float, &TempLOD, &LODMin, &LODMax, "%.3f");
                const FLOAT LeafWidthMin = 0.01f, LeafWidthMax = 50.0f;
                const BOOL8 bChangedLeafWidth = ImGui::SliderScalar(TEXT("LeafWidth"), ImGuiDataType_Float, &TempLeafWidth, &LeafWidthMin, &LeafWidthMax, "%.3f");
                const BOOL8 bChangedTipColor = ImGui::ColorEdit3(TEXT("TipColor"), TempTipColor);
                const BOOL8 bChangedRootColor = ImGui::ColorEdit3(TEXT("RootColor"), TempRootColor);
                if (bChangedWireframe || bChangedLOD || bChangedLeafWidth || bChangedTipColor || bChangedRootColor)
                {
                    if (bChangedWireframe)
                    {
                        bWireframe = bWireframe ^ 0x1u;
                    }
                    SetProperty(bWireframe, TempLOD, TempLeafWidth, Color4(TempRootColor[0], TempRootColor[1], TempRootColor[2]), Color4(TempTipColor[0], TempTipColor[1], TempTipColor[2]));
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Instance"))
            {
                FLOAT TempBaseHeight = BaseHeight;
                FLOAT TempOffsetHeight = OffsetHeight;
                FLOAT TempLengthX = LengthX;
                FLOAT TempLengthZ = LengthZ;
                UINT32 TempNumX = NumX;
                UINT32 TempNumZ = NumZ;

                const FLOAT HeightMin = 0.01f;
                const FLOAT HeightMax = 100.0f;
                const FLOAT LengthMin = 0.01f;
                const FLOAT LengthMax = 5000.0f;
                const UINT32 NumMin = 1;
                const UINT32 NumMax = 1024;
                const BOOL8 bChangedBaseHeight = ImGui::SliderScalar(TEXT("BaseHeight"), ImGuiDataType_Float, &TempBaseHeight, &HeightMin, &HeightMax, "%.3f");
                const BOOL8 bChangedOffsetHeight = ImGui::SliderScalar(TEXT("OffsetHeight"), ImGuiDataType_Float, &TempOffsetHeight, &HeightMin, &HeightMax, "%.3f");
                const BOOL8 bChangedLengthX = ImGui::SliderScalar(TEXT("LengthX"), ImGuiDataType_Float, &TempLengthX, &LengthMin, &LengthMax, "%.3f");
                const BOOL8 bChangedLengthZ = ImGui::SliderScalar(TEXT("LengthZ"), ImGuiDataType_Float, &TempLengthZ, &LengthMin, &LengthMax, "%.3f");
                const BOOL8 bChangedNumX = ImGui::SliderScalar(TEXT("NumX"), ImGuiDataType_U32, &TempNumX, &NumMin, &NumMax, "%u");
                const BOOL8 bChangedNumZ = ImGui::SliderScalar(TEXT("NumZ"), ImGuiDataType_U32, &TempNumZ, &NumMin, &NumMax, "%u");
                const BOOL8 bApply = ImGui::Button("Apply");
                if (bChangedBaseHeight || bChangedOffsetHeight || bChangedLengthX || bChangedLengthZ || bChangedNumX || bChangedNumZ || bApply)
                {
                    BaseHeight = TempBaseHeight;
                    OffsetHeight = TempOffsetHeight;
                    LengthX = TempLengthX;
                    LengthZ = TempLengthZ;
                    NumX = TempNumX;
                    NumZ = TempNumZ;
                }
                if (bApply)
                {
                    const Vector3 Origin = GetComponentWorldLocation();
                    GenerateInstanceData(Origin, BaseHeight, OffsetHeight, LengthX, LengthZ, NumX, NumZ);
                }
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }
    }
    void PBezierGrassComponent::OnSelectedByImGui()
    {
        PPrimitiveComponent::OnSelectedByImGui();
    }
#endif

}