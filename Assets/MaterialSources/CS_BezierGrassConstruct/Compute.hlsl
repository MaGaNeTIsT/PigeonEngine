float GetLandscapeWorldHeight(in FBezierGrassConstructParameter InParameter, in float2 InOriginInTileXY)
{
    float2 TexelSize = InParameter.TileSize.xy / ((float)(InParameter.HeightMapPixelSize - (InParameter.HeightMapPixelBorder * 2u)));
    float2 TexPixelSizeMinusOne = ((float)(InParameter.HeightMapPixelSize - 1)).xx;
    float2 SampleCoords = (clamp(InOriginInTileXY.xy / TexelSize.xy + float2(InParameter.HeightMapPixelBorder, InParameter.HeightMapPixelBorder), 0.0, TexPixelSizeMinusOne.xy) + 0.5) / float2(InParameter.HeightMapPixelSize, InParameter.HeightMapPixelSize);
    float4 SampleValue = LandscapeHeightTexture.SampleLevel(sLinearClamp, SampleCoords, 0);
    float Height = SampleValue.x;
    return Height * InParameter.HeightMapWorldScaleOffset.x + InParameter.HeightMapWorldScaleOffset.y;
}

FSampledLayerIndexMapData GetLayerTypeIndex(in FBezierGrassConstructParameter InParameter, in float2 InOriginInTileXY)
{
    FSampledLayerIndexMapData Result = (FSampledLayerIndexMapData)0;

    {
        float2 TexelSize = InParameter.TileSize.xy / ((float)(InParameter.IndexLayerPixelSize - (InParameter.IndexLayerPixelBorder * 2u)));
        float2 TexPixelSizeMinusOne = ((float)(InParameter.IndexLayerPixelSize - 1)).xx;
        float2 SampleCoords = (clamp(InOriginInTileXY.xy / TexelSize.xy + float2(InParameter.IndexLayerPixelBorder, InParameter.IndexLayerPixelBorder), 0.0, TexPixelSizeMinusOne.xy) + 0.5) / float2(InParameter.IndexLayerPixelSize, InParameter.IndexLayerPixelSize);
        float SampleValue = BezierGrassLayerIndexTexture.SampleLevel(sLinearClamp, SampleCoords, 0).x;
        Result.Index = (uint)(floor(((float)(InParameter.IndexLayerNumMaxTypes - 1)) * SampleValue + 0.5));
    }

    {
        float2 TexelSize = InParameter.TileSize.xy / ((float)(InParameter.DensityPixelSize - (InParameter.DensityPixelBorder * 2u)));
        float2 TexPixelSizeMinusOne = ((float)(InParameter.DensityPixelSize - 1)).xx;
        float2 SampleCoords = (clamp(InOriginInTileXY.xy / TexelSize.xy + float2(InParameter.DensityPixelBorder, InParameter.DensityPixelBorder), 0.0, TexPixelSizeMinusOne.xy) + 0.5) / float2(InParameter.DensityPixelSize, InParameter.DensityPixelSize);
        float SampleValue = BezierGrassDensityTexture.SampleLevel(sLinearClamp, SampleCoords, 0).x;
        Result.Weight = SampleValue;
    }

    return Result;
}

bool AllocateInstance(uint InNumAllocatedInstances, uint InLODIndex, out uint OutIndex)
{
    OutIndex = 0;
    InterlockedAdd(DrawIndirectDatas[InLODIndex * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 1], 1u, OutIndex);
#if BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
    InterlockedMin(DrawIndirectDatas[InLODIndex * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 1], InNumAllocatedInstances - 1);
    return (OutIndex < InNumAllocatedInstances);
#else
    return true;
#endif
}

[numthreads(BEZIER_GRASS_CONSTRUCT_TILE_SIZE, 1, 1)]
void main(uint InDispatchThreadID : SV_DispatchThreadID)
{
    FBezierGrassConstructParameter Parameter = InitBezierGrassConstructParameter();

    if (InDispatchThreadID == 0)
    {
        DispatchDatas[0] = Parameter.XNumInstances;
        DispatchDatas[1] = Parameter.YNumInstances;

        const uint LODNumInstances = Parameter.XNumInstances * Parameter.YNumInstances;

        [loop]
        for (uint i = Parameter.LODMinMax.x; i <= Parameter.LODMinMax.y; i++)
        {
#if BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
            DispatchDatas[2 + i] = i * Parameter.NumAllocatedInstances;
#else
            DispatchDatas[2 + i] = i * LODNumInstances;
#endif
        }

        [loop]
        for (uint j = Parameter.LODMinMax.x; j <= Parameter.LODMinMax.y; j++)
        {
            DrawIndirectDatas[j * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 0] = (Parameter.BodyPart[j] * 2 + 1) * 3;
            DrawIndirectDatas[j * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 2] = Parameter.IndexOffset[j];
            DrawIndirectDatas[j * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 3] = Parameter.VertexOffset[j];
            DrawIndirectDatas[j * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 4] = 0;
        }
    }

    const uint2 XYNumInstances = uint2(Parameter.XNumInstances, Parameter.YNumInstances);
    uint2 BaseBladeXY = uint2(InDispatchThreadID % XYNumInstances.x, InDispatchThreadID / XYNumInstances.x);

    [branch]
    if (any(BaseBladeXY >= ((XYNumInstances.xy + 1) >> Parameter.MaskExp)))
    {
        return;
    }

    uint2 BladeXY = BaseBladeXY.xy * Parameter.Mask;
    const uint BladeShuffleUintValue = TileShuffleBuffer[BladeXY.y * XYNumInstances.x + BladeXY.x];
    const float BladeShuffleValue = asfloat(BladeShuffleUintValue);

    const float DensityScale = (1.0f - Parameter.DensityScale) * 0.75f + 0.25f;

    [branch]
    if (BladeShuffleValue > DensityScale)
    {
        return;
    }

    float2 InstanceSize = Parameter.SubTileSize.xy / ((float2)XYNumInstances.xy);
    float2 CurrentTileOrigin = ((float2)Parameter.TileXY.xy) * Parameter.TileSize.xy + Parameter.TileAnchor.xy;
    float2 SubTileOrigin = ((float2)Parameter.SubTileXY.xy) * Parameter.SubTileSize.xy + CurrentTileOrigin.xy;
    float2 BaseBladeOriginXY = InstanceSize * (((float2)BladeXY.xy) + 0.5) + SubTileOrigin.xy;

    const int2 ActualInstanceGrid = (int2)((Parameter.TileXY.xy * (Parameter.NumSubTiles.xy * XYNumInstances.xy)) + (Parameter.SubTileXY.xy * XYNumInstances.xy) + BladeXY.xy);

    float2 TargetWorldOrigin;
    float2 MinOffset;
    {
        float MinDistSq = 10.0;
        int MinDistOffsetIndex = 0;
        float2 MinDistRandomPart = 0;

        const int2 LoopParams[8] = { {-1,-1}, {0,-1}, {1,-1}, {-1,0}, {1,0}, {-1,1}, {0,1}, {1,1} };

        [unroll]
        for (int Index = 0; Index < 8; Index++)
        {
            int2 Offset = LoopParams[Index];
            int2 TestPosition = ActualInstanceGrid + Offset;
            uint UsedHash = Parameter.RandomSeed;

            float2 RandomValue2 = Rand3DPCG16(uint3(TestPosition.xy, UsedHash)).xy / 65535.0f;
            float2 PositionVector = ((float2)Offset.xy) + RandomValue2.xy - float2(0.5, 0.5);
            float DistSq = dot(PositionVector.xy, PositionVector.xy);

            MinDistOffsetIndex = (DistSq < MinDistSq) ? Index : MinDistOffsetIndex;
            MinDistRandomPart = (DistSq < MinDistSq) ? RandomValue2.xy : MinDistRandomPart.xy;
            MinDistSq = min(DistSq, MinDistSq);
        }

        MinOffset = ((float2)LoopParams[MinDistOffsetIndex].xy) + MinDistRandomPart.xy;
        TargetWorldOrigin = lerp(MinOffset.xy * InstanceSize.xy + BaseBladeOriginXY.xy, BaseBladeOriginXY.xy, 0.25);
    }

    float2 CurrentInstanceOriginInTile = TargetWorldOrigin.xy - CurrentTileOrigin.xy;

    FSampledLayerIndexMapData LayerData = GetLayerTypeIndex(Parameter, CurrentInstanceOriginInTile.xy);

    [branch]
    if ((BladeShuffleValue >= LayerData.Weight) || (LayerData.Index >= Parameter.IndexLayerNumMaxTypes))
    {
        return;
    }

    float WorldHeight = GetLandscapeWorldHeight(Parameter, CurrentInstanceOriginInTile.xy);

    FBezierGrassUnpackedInstanceData UnpackedInstanceData = (FBezierGrassUnpackedInstanceData)0;
    FBezierGrassLayerTypeBaseData LayerTypeBaseData;
    {
        const uint ActualBladeIndex = ((uint)ActualInstanceGrid.y) * Parameter.NumTiles.x * Parameter.NumSubTiles.x * XYNumInstances.x + ((uint)ActualInstanceGrid.x);
        UnpackedInstanceData.PerBladeHash = ActualBladeIndex ^ Parameter.RandomSeedParams.x ^ Parameter.RandomSeed ^ (BladeShuffleUintValue & 0xffffffu);
        UnpackedInstanceData.LayerTypeIndex = LayerData.Index & 0xffu;
        LayerTypeBaseData = GetBezierGrassLayerTypeBaseData(UnpackedInstanceData.LayerTypeIndex, Parameter.IndexLayerNumTotalElems);
    }

#if 0
    {
        const float3 BoundCenter = float3(TargetWorldOrigin.xy, WorldHeight);
        const float BoundLength = LayerTypeBaseData.Height.x + LayerTypeBaseData.Height.z;
        bool bInside = true;

        [unroll]
        for (uint i = 0; i < 5; i++)
        {
            bInside = bInside && ((dot(BoundCenter.xyz, ViewFrustumPlane[i].xyz) - ViewFrustumPlane[i].w - BoundLength) <= 0);
        }

        [branch]
        if (!bInside)
        {
            return;
        }
    }
#endif  // 0

    float ViewDistSq;
    float LOD = 0.0;
    uint DrawLODIndex = 0;
    {
        float3 TranslatedWorldPosition = TransformWorldToView(float3(TargetWorldOrigin.xy, WorldHeight)).xyz;
        ViewDistSq = dot(TranslatedWorldPosition.xyz, TranslatedWorldPosition.xyz);

        [branch]
        if (ViewDistSq > Parameter.LODDistancesSq[Parameter.LODMinMax.y])
        {
            return;
        }

        [loop]
        for (uint i = Parameter.LODMinMax.x; i <= Parameter.LODMinMax.y; i++)
        {
            DrawLODIndex = (ViewDistSq > Parameter.LODDistancesSq[i]) ? i : DrawLODIndex;
        }

        DrawLODIndex = clamp(DrawLODIndex, Parameter.LODMinMax.x, Parameter.LODMinMax.y);
        uint LODNextIndex = clamp(DrawLODIndex + 1, Parameter.LODMinMax.x, Parameter.LODMinMax.y);

        float LODOffsetDistSq = Parameter.LODDistancesSq[LODNextIndex] - Parameter.LODDistancesSq[DrawLODIndex];
        float OffsetDistSq = clamp(ViewDistSq - Parameter.LODDistancesSq[DrawLODIndex], 0.0, LODOffsetDistSq);

        LOD = (LODNextIndex == DrawLODIndex) ? ((float)DrawLODIndex) : ((OffsetDistSq / LODOffsetDistSq) + ((float)DrawLODIndex));
    }

    {
        const float3 RandValue3Y = ((float3)Rand3DPCG16(uint3(ActualInstanceGrid.xy ^ Parameter.RandomSeedParams.y, UnpackedInstanceData.PerBladeHash)).xyz) / 65535.0f;
        const float2 RandValue3Z = ((float2)Rand3DPCG16(uint3(ActualInstanceGrid.xy ^ Parameter.RandomSeedParams.z, UnpackedInstanceData.PerBladeHash)).xy) / 65535.0f;

        UnpackedInstanceData.Height = RandValue3Y.x * (LayerTypeBaseData.Height.z - LayerTypeBaseData.Height.y) + (LayerTypeBaseData.Height.x + LayerTypeBaseData.Height.y);
        UnpackedInstanceData.Width = RandValue3Y.y * (LayerTypeBaseData.Width.z - LayerTypeBaseData.Width.y) + (LayerTypeBaseData.Width.x + LayerTypeBaseData.Width.y);
        UnpackedInstanceData.LOD = LOD;
        UnpackedInstanceData.OriginXY = TargetWorldOrigin.xy;
        UnpackedInstanceData.OriginHeight = WorldHeight;
        UnpackedInstanceData.bBent = LayerTypeBaseData.bBent;

        const float BaseTilt = saturate(RandValue3Y.z * (LayerTypeBaseData.Tilt.z - LayerTypeBaseData.Tilt.y) + (LayerTypeBaseData.Tilt.x + LayerTypeBaseData.Tilt.y));
        const float BaseBend = saturate(RandValue3Z.x * (LayerTypeBaseData.Bend.z - LayerTypeBaseData.Bend.y) + (LayerTypeBaseData.Bend.x + LayerTypeBaseData.Bend.y));
        const float BaseMidPointT = saturate(RandValue3Z.y * (LayerTypeBaseData.MidPointT.z - LayerTypeBaseData.MidPointT.y) + (LayerTypeBaseData.MidPointT.x + LayerTypeBaseData.MidPointT.y));

        {
            float WindStrength = 0;
            float Tilt = BaseTilt;
            float Bend = BaseBend;
            float MidPointT = BaseMidPointT;
            float WindFacing = 0;
            float FacingOffset = 0;

            ModifyBezierGrassBlade(Parameter, LayerTypeBaseData, TargetWorldOrigin.xy, UnpackedInstanceData.PerBladeHash, Parameter.CurrentTimeParams.xyzw, WindStrength, Tilt, Bend, MidPointT, WindFacing, FacingOffset);

            const float Facing = LayerTypeBaseData.bUseFacing ? (LayerTypeBaseData.Facing.x + FacingOffset) : WindFacing;
            float2 FacingSinCos;
            SinCosFunction(Facing, FacingSinCos.y, FacingSinCos.x);

            UnpackedInstanceData.CurrentTimeRelevanceData.Facing = FacingSinCos.xy;
            UnpackedInstanceData.CurrentTimeRelevanceData.WindStrength = WindStrength;
            UnpackedInstanceData.CurrentTimeRelevanceData.Bend = Bend;
            UnpackedInstanceData.CurrentTimeRelevanceData.MidPointT = MidPointT;
            SinCosFunction(Tilt * 1.5707963267948966, UnpackedInstanceData.CurrentTimeRelevanceData.TiltSin, UnpackedInstanceData.CurrentTimeRelevanceData.TiltCos);
        }

        {
            float WindStrength = 0;
            float Tilt = BaseTilt;
            float Bend = BaseBend;
            float MidPointT = BaseMidPointT;
            float WindFacing = 0;
            float FacingOffset = 0;

            ModifyBezierGrassBlade(Parameter, LayerTypeBaseData, TargetWorldOrigin.xy, UnpackedInstanceData.PerBladeHash, Parameter.PreviousTimeParams.xyzw, WindStrength, Tilt, Bend, MidPointT, WindFacing, FacingOffset);

            const float Facing = LayerTypeBaseData.bUseFacing ? (LayerTypeBaseData.Facing.x + FacingOffset) : WindFacing;
            float2 FacingSinCos;
            SinCosFunction(Facing, FacingSinCos.y, FacingSinCos.x);

            UnpackedInstanceData.PreviousTimeRelevanceData.Facing = FacingSinCos.xy;
            UnpackedInstanceData.PreviousTimeRelevanceData.WindStrength = WindStrength;
            UnpackedInstanceData.PreviousTimeRelevanceData.Bend = Bend;
            UnpackedInstanceData.PreviousTimeRelevanceData.MidPointT = MidPointT;
            SinCosFunction(Tilt * 1.5707963267948966, UnpackedInstanceData.PreviousTimeRelevanceData.TiltSin, UnpackedInstanceData.PreviousTimeRelevanceData.TiltCos);
        }
    }

    {
        uint InstanceIndex;
#if BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
        if (!(AllocateInstance(Parameter.NumAllocatedInstances, DrawLODIndex, InstanceIndex)))
        {
            return;
        }
#else
        AllocateInstance(Parameter.NumAllocatedInstances, DrawLODIndex, InstanceIndex);
#endif

        FBezierGrassParameter TempParameter = (FBezierGrassParameter)0;
        TempParameter.NumTiles = Parameter.NumTiles.xy;
        TempParameter.LODMinMax = (float2)Parameter.LODMinMax.xy;
        TempParameter.TileAnchor = Parameter.TileAnchor.xy;
        TempParameter.TileSize = Parameter.TileSize.xy;
        TempParameter.SubTileSize = Parameter.SubTileSize.xy;
        TempParameter.BorderSize = Parameter.BorderSize.xy;
        TempParameter.NumSubTiles = Parameter.NumSubTiles.xy;
        TempParameter.NumInstances = XYNumInstances.xy;
        TempParameter.WindStrengthRange = Parameter.WindStrengthRange.xy;

        const uint2 Grid = Parameter.TileXY.xy;
        const uint2 SubGrid = Parameter.SubTileXY.xy;
        FBezierGrassTileInstanceData TileData = InitBezierGrassTileInstanceData(TempParameter, Grid, SubGrid);

#if BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
        const uint FinalInstanceIndex = DrawLODIndex * Parameter.NumAllocatedInstances + InstanceIndex;
#else
        const uint FinalInstanceIndex = DrawLODIndex * XYNumInstances.x * XYNumInstances.y + InstanceIndex;
#endif
        FBezierGrassPackedInstanceData PackedInstanceData = PackBezierGrassInstanceData(TempParameter, TileData, LayerTypeBaseData, UnpackedInstanceData);
        SetPackedInstanceData(FinalInstanceIndex, PackedInstanceData);
    }
}
