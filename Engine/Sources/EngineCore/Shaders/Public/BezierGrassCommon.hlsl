#ifndef _BEZIER_GRASS_COMMON_HLSL
#define _BEZIER_GRASS_COMMON_HLSL

struct FBezierGrassLayerTypeBaseData
{
    float3 Facing;
    float3 Height;
    float3 Width;
    float3 Tilt;
    float3 Bend;
    float3 MidPointT;
    bool bBent;
    bool bUseFacing;
    float SideCurve[ BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM ];
};

struct FBezierGrassTimeRelevanceUnpackedData
{
    float2 Facing;
    float WindStrength;
    float TiltSin;
    float TiltCos;
    float Bend;
    float MidPointT;
};

struct FBezierGrassTimeRelevancePackedData
{
    uint PackedFacingWindStrength;
    uint PackedTiltSinCosBendMidPointT;
};

struct FBezierGrassUnpackedInstanceData
{
    float LOD;
    uint LayerTypeIndex;
    float2 OriginXY;
    float OriginHeight;
    uint PerBladeHash;
    bool bBent;
    float Height;
    float Width;
    FBezierGrassTimeRelevanceUnpackedData CurrentTimeRelevanceData;
    FBezierGrassTimeRelevanceUnpackedData PreviousTimeRelevanceData;
};

struct FBezierGrassPackedInstanceData
{
    uint GridIndex;
    uint PackedLODTypeIndexIsBent;
    uint PackedOriginXY;
    float OriginHeight;
    uint PerBladeHash;
    uint PackedHeightWidth;
    FBezierGrassTimeRelevancePackedData CurrentTimeRelevanceData;
    FBezierGrassTimeRelevancePackedData PreviousTimeRelevanceData;
};

struct FBezierGrassVertexData
{
    float3 Position;
    float3 Normal;
    float3 GeomNormal;
    float3 GeomTangent;
    float3 RootPosition;
    float3 ControlPosition;
    float3 TipPosition;
    float BezierT;
    float CurveUV;
};

struct FBezierGrassParameter
{
    uint RandomSeed;
    uint NumAllocatedInstances;
    uint CurrentLODIndex;
    float2 LODMinMax;
    uint2 NumTiles;
    uint2 NumSubTiles;
    uint2 NumInstances;
    uint LayerTypeNumTotalElems;
    uint LayerTypeNumBaseElems;
    uint LayerTypeNumCustomElems;
    float2 TileAnchor;
    float2 TileSize;
    float2 SubTileSize;
    float2 BorderSize;
    float2 WindStrengthRange; // x is min, y is [max - min]
    float BentBezierT;
};

struct FBezierGrassDispatchData
{
    uint2 XYNumInstances;
    uint InstanceOffset[ BEZIER_GRASS_BLADE_LODS_MAX_NUM ];
};

struct FBezierGrassTileInstanceData
{
    uint2 Grid;
    uint2 SubGrid;
    float2 AnchorWithBorder;
    float2 SizeWithBorder;
};

struct FBezierGrassUnpackedVertexInputData
{
    float TargetT;
    float PrevT;
    float SideOffset;
    bool bBentLeftPart;
};

struct FBezierGrassConstructParameter
{
    float4 CurrentTimeParams;
    float4 PreviousTimeParams;
    uint4 RandomSeedParams;
    uint RandomSeed;
    uint NumAllocatedInstances;
    uint BodyPart[ BEZIER_GRASS_BLADE_LODS_MAX_NUM ];
    uint IndexOffset[ BEZIER_GRASS_BLADE_LODS_MAX_NUM ];
    uint VertexOffset[ BEZIER_GRASS_BLADE_LODS_MAX_NUM ];
    uint2 LODMinMax;
    float2 TileAnchor;
    float2 TileSize;
    uint2 TileXY;
    uint2 NumTiles;
    float2 SubTileSize;
    float2 BorderSize;
    uint2 SubTileXY;
    uint2 NumSubTiles;
    uint MaskExp;
    uint Mask;
    uint XNumInstances;
    uint YNumInstances;
    uint NumInstances;
    float DensityScale;
    float2 HeightMapWorldScaleOffset;
    uint HeightMapPixelBorder;
    uint HeightMapPixelSize;
    uint IndexLayerPixelBorder;
    uint IndexLayerPixelSize;
    uint DensityPixelBorder;
    uint DensityPixelSize;
    uint IndexLayerNumMaxTypes;
    uint IndexLayerNumTotalElems;
    uint IndexLayerNumBaseElems;
    uint IndexLayerNumCustomElems;
    float4 WindDirectionStrength;
    float2 WindStrengthRange;
    float LODDistancesSq[ BEZIER_GRASS_BLADE_LODS_MAX_NUM ];
};

struct FSampledLayerIndexMapData
{
    uint Index;
    float Weight;
};

FBezierGrassLayerTypeBaseData GetBezierGrassLayerTypeBaseData( uint InLayerIndex, uint InLayerNumTotalElems )
{
    FBezierGrassLayerTypeBaseData Result = (FBezierGrassLayerTypeBaseData)0;
	
    Result.Facing = float3(
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 0 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 1 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 2 ] );
    Result.Height = float3(
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 3 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 4 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 5 ] );
    Result.Width = float3(
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 6 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 7 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 8 ] );
    Result.Tilt = float3(
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 9 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 10 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 11 ] );
    Result.Bend = float3(
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 12 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 13 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 14 ] );
    Result.MidPointT = float3(
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 15 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 16 ],
		BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 17 ] );
	
    const uint PackedFlags = asuint( BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + 18 ] );
    Result.bBent = (PackedFlags & 0x1u) != 0x0u;
    Result.bUseFacing = (PackedFlags & (1u << 1u)) != 0x0u;
	
	{
        UNROLL

        for (int i = 0; i < BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM; i++)
        {
            Result.SideCurve[ i ] = BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + BEZIER_GRASS_LAYER_TYPE_BASE_NUM_MEMBERS + i ];
        }
    }
	
    return Result;
}

// Require define [BezierGrassLayerTypeBinding] for custom data values
float GetBezierGrassLayerTypeCustomData( uint InLayerIndex, uint InCustomElementOffset, uint InLayerNumTotalElems )
{
    return (BezierGrassLayerTypeBinding[ InLayerNumTotalElems * InLayerIndex + BEZIER_GRASS_LAYER_TYPE_BASE_NUM_MEMBERS + BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM + InCustomElementOffset ]);
}

// Require define [BezierGrassLayerTypeCustomTexArrayBinding0] [BezierGrassLayerTypeCustomSamplerBinding0] for custom data textures
// Require define [BezierGrassLayerTypeCustomTexArrayBinding1] [BezierGrassLayerTypeCustomSamplerBinding1] for custom data textures
// Require define [BezierGrassLayerTypeCustomTexArrayBinding2] [BezierGrassLayerTypeCustomSamplerBinding2] for custom data textures
// Require define [BezierGrassLayerTypeCustomTexArrayBinding3] [BezierGrassLayerTypeCustomSamplerBinding3] for custom data textures
float4 SampleLevelLayerCustomTexture( uint InLayerIndex, float2 InUV, float InMipLevel, float InMipBias, uint InArrayIndex )
{
    const float SliceIndex = (float)(max( InLayerIndex, 0 ));
    float LODValue = InMipLevel + InMipBias;
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 0
	if (InArrayIndex == 0)
	{
		return (Texture2DArraySampleLevel(BezierGrassLayerTypeCustomTexArrayBinding0, BezierGrassLayerTypeCustomSamplerBinding0, float3(InUV.xy, SliceIndex), LODValue));
	}
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 0
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 1
	if (InArrayIndex == 1)
	{
		return (Texture2DArraySampleLevel(BezierGrassLayerTypeCustomTexArrayBinding1, BezierGrassLayerTypeCustomSamplerBinding1, float3(InUV.xy, SliceIndex), LODValue));
	}
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 1
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 2
	if (InArrayIndex == 2)
	{
		return (Texture2DArraySampleLevel(BezierGrassLayerTypeCustomTexArrayBinding2, BezierGrassLayerTypeCustomSamplerBinding2, float3(InUV.xy, SliceIndex), LODValue));
	}
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 2
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 3
	if (InArrayIndex == 3)
	{
		return (Texture2DArraySampleLevel(BezierGrassLayerTypeCustomTexArrayBinding3, BezierGrassLayerTypeCustomSamplerBinding3, float3(InUV.xy, SliceIndex), LODValue));
	}
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 3
    return 0;
}

// Require define [BezierGrassLayerTypeCustomTexArrayBinding0] [BezierGrassLayerTypeCustomSamplerBinding0] for custom data textures
// Require define [BezierGrassLayerTypeCustomTexArrayBinding1] [BezierGrassLayerTypeCustomSamplerBinding1] for custom data textures
// Require define [BezierGrassLayerTypeCustomTexArrayBinding2] [BezierGrassLayerTypeCustomSamplerBinding2] for custom data textures
// Require define [BezierGrassLayerTypeCustomTexArrayBinding3] [BezierGrassLayerTypeCustomSamplerBinding3] for custom data textures
float4 SampleLayerCustomTexture( uint InLayerIndex, float2 InUV, uint InArrayIndex )
{
    const float SliceIndex = (float)(max( InLayerIndex, 0 ));
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 0
	if (InArrayIndex == 0)
	{
		return (Texture2DArraySample(BezierGrassLayerTypeCustomTexArrayBinding0, BezierGrassLayerTypeCustomSamplerBinding0, float3(InUV.xy, SliceIndex)));
	}
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 0
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 1
	if (InArrayIndex == 1)
	{
		return (Texture2DArraySample(BezierGrassLayerTypeCustomTexArrayBinding1, BezierGrassLayerTypeCustomSamplerBinding1, float3(InUV.xy, SliceIndex)));
	}
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 1
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 2
	if (InArrayIndex == 2)
	{
		return (Texture2DArraySample(BezierGrassLayerTypeCustomTexArrayBinding2, BezierGrassLayerTypeCustomSamplerBinding2, float3(InUV.xy, SliceIndex)));
	}
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 2
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 3
	if (InArrayIndex == 3)
	{
		return (Texture2DArraySample(BezierGrassLayerTypeCustomTexArrayBinding3, BezierGrassLayerTypeCustomSamplerBinding3, float3(InUV.xy, SliceIndex)));
	}
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 3
    return 0;
}

// Require define [BezierGrassPackedInstanceBinding] for instance datas
void GetPackedInstanceTimeRelevanceData( in uint InInstanceIndex, inout FBezierGrassPackedInstanceData InOutPackedInstanceData )
{
    InOutPackedInstanceData.CurrentTimeRelevanceData.PackedFacingWindStrength = BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 0 ];
    InOutPackedInstanceData.CurrentTimeRelevanceData.PackedTiltSinCosBendMidPointT = BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 1 ];
	
    InOutPackedInstanceData.PreviousTimeRelevanceData.PackedFacingWindStrength = BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 2 ];
    InOutPackedInstanceData.PreviousTimeRelevanceData.PackedTiltSinCosBendMidPointT = BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 3 ];
}

// Require define [BezierGrassPackedInstanceBinding] for instance datas
void SetPackedInstanceTimeRelevanceData( in uint InInstanceIndex, in FBezierGrassPackedInstanceData InPackedInstanceData )
{
    BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 0 ] = InPackedInstanceData.CurrentTimeRelevanceData.PackedFacingWindStrength;
    BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 1 ] = InPackedInstanceData.CurrentTimeRelevanceData.PackedTiltSinCosBendMidPointT;
	
    BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 2 ] = InPackedInstanceData.PreviousTimeRelevanceData.PackedFacingWindStrength;
    BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 3 ] = InPackedInstanceData.PreviousTimeRelevanceData.PackedTiltSinCosBendMidPointT;
}

// Require define [BezierGrassPackedInstanceBinding] for instance datas
FBezierGrassPackedInstanceData GetPackedInstanceData( in uint InInstanceIndex )
{
    FBezierGrassPackedInstanceData Result = (FBezierGrassPackedInstanceData)0;
	
    Result.GridIndex = BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 0 ];
    Result.PackedLODTypeIndexIsBent = BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 1 ];
    Result.PackedOriginXY = BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 2 ];
    Result.OriginHeight = asfloat( BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 3 ] );
    Result.PerBladeHash = BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 4 ];
    Result.PackedHeightWidth = BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 5 ];
	
    GetPackedInstanceTimeRelevanceData( InInstanceIndex, Result );
	
    return Result;
}

// Require define [BezierGrassPackedInstanceBinding] for instance datas
void SetPackedInstanceData( in uint InInstanceIndex, in FBezierGrassPackedInstanceData InPackedInstanceData )
{
    BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 0 ] = InPackedInstanceData.GridIndex;
    BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 1 ] = InPackedInstanceData.PackedLODTypeIndexIsBent;
    BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 2 ] = InPackedInstanceData.PackedOriginXY;
    BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 3 ] = asuint( InPackedInstanceData.OriginHeight );
    BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 4 ] = InPackedInstanceData.PerBladeHash;
    BezierGrassPackedInstanceBinding[ BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 5 ] = InPackedInstanceData.PackedHeightWidth;
	
    SetPackedInstanceTimeRelevanceData( InInstanceIndex, InPackedInstanceData );
}

// Require define [BezierGrassDispatchBinding] for instance datas
FBezierGrassDispatchData GetBezierGrassDispatchData()
{
    FBezierGrassDispatchData Result = (FBezierGrassDispatchData)0;
	
    Result.XYNumInstances = uint2( BezierGrassDispatchBinding[ 0 ], BezierGrassDispatchBinding[ 1 ] );
	{
        UNROLL

        for (int i = 0; i < BEZIER_GRASS_BLADE_LODS_MAX_NUM; i++)
        {
            Result.InstanceOffset[ i ] = BezierGrassDispatchBinding[ 2 + i ];
        }
    }
	
    return Result;
}

FBezierGrassTileInstanceData InitBezierGrassTileInstanceData( in FBezierGrassParameter InParams, in uint2 Grid, in uint2 SubGrid )
{
    FBezierGrassTileInstanceData Result = (FBezierGrassTileInstanceData)0;
	
    Result.Grid = Grid;
    Result.SubGrid = SubGrid;
    Result.AnchorWithBorder = ((float2)Grid.xy) * InParams.TileSize.xy + InParams.TileAnchor.xy + (((float2)SubGrid.xy) * InParams.SubTileSize.xy) - InParams.BorderSize.xy;
    Result.SizeWithBorder = 2.0f * InParams.BorderSize.xy + InParams.SubTileSize.xy;
	
    return Result;
}

#define BezierGrassBendMin				(-0.5)
#define BezierGrassBendRange			(2.0)
#define BezierGrassMidPointTMin			(0.2)
#define BezierGrassMidPointTRange		(0.6)

FBezierGrassTimeRelevancePackedData PackBezierGrassInstanceTimeRelevanceData( in FBezierGrassParameter InParams, in FBezierGrassTimeRelevanceUnpackedData InData )
{
    FBezierGrassTimeRelevancePackedData Result = (FBezierGrassTimeRelevancePackedData)0;
	
    const uint2 PackedFacing = (uint2)(InData.Facing.xy * 511.0f + 511.0f); // 1023 == 0x3ffu == 10-Bit		511 == 0x1ffu == 9-Bit
    const uint PackedWindStrength = (uint)(saturate( (InData.WindStrength - InParams.WindStrengthRange.x) / InParams.WindStrengthRange.y ) * 4095.0f); // 4095 == 0xfffu == 12-Bit
    Result.PackedFacingWindStrength = (PackedFacing.x << 22u) | (PackedFacing.y << 12u) | PackedWindStrength;
	
    const uint PackedTiltSinCos = (((uint)(InData.TiltSin * 255.0f)) << 8u) | ((uint)(InData.TiltCos * 255.0f)); // 255 == 0xffu == 8-Bit
    const uint PackedBend = (uint)(saturate( (InData.Bend - BezierGrassBendMin) / BezierGrassBendRange ) * 255.0f); // 255 == 0xffu == 8-Bit
    const uint PackedMidPointT = (uint)(saturate( (InData.MidPointT - BezierGrassMidPointTMin) / BezierGrassMidPointTRange ) * 255.0f); // 255 == 0xffu == 8-Bit
    Result.PackedTiltSinCosBendMidPointT = (PackedTiltSinCos << 16u) | (PackedBend << 8u) | PackedMidPointT;
	
    return Result;
}

FBezierGrassTimeRelevanceUnpackedData UnpackBezierGrassInstanceTimeRelevanceData( in FBezierGrassParameter InParams, in FBezierGrassTimeRelevancePackedData InData )
{
    FBezierGrassTimeRelevanceUnpackedData Result = (FBezierGrassTimeRelevanceUnpackedData)0;
	
    Result.Facing = (float2( InData.PackedFacingWindStrength >> 22u, (InData.PackedFacingWindStrength >> 12u) & 0x3ffu ) - 511.0f) / 511.0f;
    Result.WindStrength = (((float)(InData.PackedFacingWindStrength & 0xfffu)) / 4095.0f) * InParams.WindStrengthRange.y + InParams.WindStrengthRange.x;
    Result.TiltSin = ((float)(InData.PackedTiltSinCosBendMidPointT >> 24u)) / 255.0f;
    Result.TiltCos = ((float)((InData.PackedTiltSinCosBendMidPointT >> 16u) & 0xffu)) / 255.0f;
    Result.Bend = ((float)((InData.PackedTiltSinCosBendMidPointT >> 8u) & 0xffu)) / 255.0f * BezierGrassBendRange + BezierGrassBendMin;
    Result.MidPointT = ((float)(InData.PackedTiltSinCosBendMidPointT & 0xffu)) / 255.0f * BezierGrassMidPointTRange + BezierGrassMidPointTMin;
	
    return Result;
}

FBezierGrassPackedInstanceData PackBezierGrassInstanceData(
	in FBezierGrassParameter InParams,
	in FBezierGrassTileInstanceData InTileData,
	in FBezierGrassLayerTypeBaseData InLayerTypeBaseData,
	in FBezierGrassUnpackedInstanceData InData )
{
    FBezierGrassPackedInstanceData Result = (FBezierGrassPackedInstanceData)0;
	
    const uint TileGridIndex = ((InTileData.Grid.y * InParams.NumTiles.x) + InTileData.Grid.x) & 0xfffu; // 0xfffu == 12-Bit
    const uint SubTileGridIndex = ((InTileData.SubGrid.y * InParams.NumSubTiles.x) + InTileData.SubGrid.x) & 0xfffffu; // 0xfffffu == 20-Bit
    Result.GridIndex = (TileGridIndex << 20u) | SubTileGridIndex;
	
    const uint LODNormalized = (uint)((InData.LOD - InParams.LODMinMax.x) / max( InParams.LODMinMax.y - InParams.LODMinMax.x, 1.0f ) * ((float)0x7fffffu)); // 0x7fffffu == 23-Bit
    const uint LayerTypeIndex = InData.LayerTypeIndex & 0xffu; // 0xffu == 8-Bit
    const uint bBent = InData.bBent ? 0x1u : 0x0u;
    Result.PackedLODTypeIndexIsBent = (LODNormalized << 9u) | (LayerTypeIndex << 1u) | bBent;
	
    const uint2 OriginXYNormalized = (uint2)(saturate( (InData.OriginXY.xy - InTileData.AnchorWithBorder.xy) / InTileData.SizeWithBorder.xy ) * 65535.0f); // 65535 == 0xffffu == 16-Bit
    Result.PackedOriginXY = (OriginXYNormalized.x << 16u) | (OriginXYNormalized.y);
    Result.OriginHeight = InData.OriginHeight;
	
    Result.PerBladeHash = InData.PerBladeHash;
	
    const uint PackedHeight = (uint)(saturate( (InData.Height - (InLayerTypeBaseData.Height.x + InLayerTypeBaseData.Height.y)) / max( InLayerTypeBaseData.Height.z - InLayerTypeBaseData.Height.y, 0.01f ) ) * 65535.0f); // 65535 == 0xffffu == 16-Bit
    const uint PackedWidth = (uint)(saturate( (InData.Width - (InLayerTypeBaseData.Width.x + InLayerTypeBaseData.Width.y)) / max( InLayerTypeBaseData.Width.z - InLayerTypeBaseData.Width.y, 0.01f ) ) * 65535.0f); // 65535 == 0xffffu == 16-Bit
    Result.PackedHeightWidth = (PackedHeight << 16u) | PackedWidth;
	
	// Time relevance datas
    Result.CurrentTimeRelevanceData = PackBezierGrassInstanceTimeRelevanceData( InParams, InData.CurrentTimeRelevanceData );
    Result.PreviousTimeRelevanceData = PackBezierGrassInstanceTimeRelevanceData( InParams, InData.PreviousTimeRelevanceData );
	
    return Result;
}

void UnpackBezierGrassInstanceDataIndexInfo(
	in FBezierGrassParameter InParams,
	in FBezierGrassPackedInstanceData InData,
	inout FBezierGrassTileInstanceData OutTileData,
	inout FBezierGrassUnpackedInstanceData OutUnpackedInstanceData )
{
    const uint GridIndex = InData.GridIndex >> 20u;
    const uint2 Grid = uint2( GridIndex % InParams.NumTiles.x, GridIndex / InParams.NumTiles.x );
    const uint SubGridIndex = InData.GridIndex & 0xfffffu;
    const uint2 SubGrid = uint2( SubGridIndex % InParams.NumSubTiles.x, SubGridIndex / InParams.NumSubTiles.x );
    OutTileData = InitBezierGrassTileInstanceData( InParams, Grid, SubGrid );
	
    OutUnpackedInstanceData.LOD = saturate( (InData.PackedLODTypeIndexIsBent >> 9u) / ((float)0x7fffffu) ) * (InParams.LODMinMax.y - InParams.LODMinMax.x) + InParams.LODMinMax.x;
    OutUnpackedInstanceData.LayerTypeIndex = (InData.PackedLODTypeIndexIsBent >> 1u) & 0xffu;
    OutUnpackedInstanceData.bBent = (InData.PackedLODTypeIndexIsBent & 0x1u) != 0x0u;
}

void UnpackBezierGrassInstanceDataPropertyInfo(
	in FBezierGrassParameter InParams,
	in FBezierGrassPackedInstanceData InData,
	in FBezierGrassTileInstanceData InTileData,
	in FBezierGrassLayerTypeBaseData InLayerTypeBaseData,
	inout FBezierGrassUnpackedInstanceData OutUnpackedInstanceData )
{
    const float2 UnpackedNormalizedOriginXY = float2( InData.PackedOriginXY >> 16u, InData.PackedOriginXY & 0xffffu ) / 65535.0f;
    OutUnpackedInstanceData.OriginXY = UnpackedNormalizedOriginXY * InTileData.SizeWithBorder.xy + InTileData.AnchorWithBorder.xy;
    OutUnpackedInstanceData.OriginHeight = InData.OriginHeight;
	
    OutUnpackedInstanceData.PerBladeHash = InData.PerBladeHash;
	
    OutUnpackedInstanceData.Height = ((float)(InData.PackedHeightWidth >> 16u)) / 65535.0f * (InLayerTypeBaseData.Height.z - InLayerTypeBaseData.Height.y) + (InLayerTypeBaseData.Height.x + InLayerTypeBaseData.Height.y);
    OutUnpackedInstanceData.Width = ((float)(InData.PackedHeightWidth & 0xffffu)) / 65535.0f * (InLayerTypeBaseData.Width.z - InLayerTypeBaseData.Width.y) + (InLayerTypeBaseData.Width.x + InLayerTypeBaseData.Width.y);
	
    OutUnpackedInstanceData.CurrentTimeRelevanceData = UnpackBezierGrassInstanceTimeRelevanceData( InParams, InData.CurrentTimeRelevanceData );
    OutUnpackedInstanceData.PreviousTimeRelevanceData = UnpackBezierGrassInstanceTimeRelevanceData( InParams, InData.PreviousTimeRelevanceData );
}

FBezierGrassUnpackedVertexInputData UnpackedBezierGrassPosition( in uint InPosition )
{
    FBezierGrassUnpackedVertexInputData Result = (FBezierGrassUnpackedVertexInputData)0;
	
    Result.SideOffset = ((float)(InPosition & 0x3u)) - 1.0f;
    Result.bBentLeftPart = ((InPosition >> 2u) & 0x1u) == 0x1u;
    Result.PrevT = ((float)((InPosition >> 3u) & 0x3fffu)) / 16383.0f;
    Result.TargetT = ((float)(InPosition >> 17u)) / 32767.0f;
	
    return Result;
}

float SinFunction( float InVal )
{
    return (sin( InVal ));
}

void SinCosFunction( in float InVal, inout float OutSinVal, inout float OutCosVal )
{
    sincos( InVal, OutSinVal, OutCosVal );
}

uint3 Rand3DPCG16( uint3 InValue )
{
    uint3 v = InValue;

    v = v * 1664525u + 1013904223u;

    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;

    return (v >> 16u);
}

float2 NoiseTile2DWrap( float2 InVec2D, bool InIsTiling, float InRepeatSize )
{
    return (InIsTiling ? (frac( InVec2D / InRepeatSize ) * InRepeatSize) : InVec2D);
}

float4 Voronoi2DCompare( float4 InMinVal, float3 InCandidate, float2 InOffset, bool InIsDistanceOnly )
{
    if (InIsDistanceOnly)
    {
        return float4( 0, 0, 0, min( InMinVal.w, dot( InOffset, InOffset ) ) );
    }
    else
    {
        float NewDist = dot( InOffset, InOffset );
        return ((NewDist > InMinVal.w) ? InMinVal : float4( InCandidate, NewDist ));
    }
}

float3 Voronoi2DCornerSample( uint InSeed, float2 InPos )
{
    const uint3 UintPosition = uint3( int2( InPos.xy ), InSeed );
    float3 RandValue = float3( Rand3DPCG16( UintPosition ) ) / 0xffff;
	
	// searches a 3x3 neighborhood with points distributed on a sphere
	// scale factor to guarantee jittered points will be found within a 3x3 search
    return float3( normalize( RandValue.xy - 0.5 ) * 0.25, RandValue.z );
}

float4 VoronoiNoise2D_ALU( uint InSeed, float2 InVec2D, bool InIsTiling, float InRepeatSize, bool InIsDistanceOnly )
{
    float2 FractionPart = frac( InVec2D.xy + 0.5 );
    float2 IntegerPart = floor( InVec2D.xy + 0.5 );

	// with initial minimum distance = infinity (or at least bigger than 4), first min is optimized away
    float4 MinDist = float4( 0, 0, 0, 100 );
    float3 NewOffset;
    int2 IntegerOffset;

	// do a 3x3 search
	{
        UNROLL_N( 3 )

        for (IntegerOffset.x = -1; IntegerOffset.x <= 1; IntegerOffset.x++)
        {
            UNROLL_N( 3 )

            for (IntegerOffset.y = -1; IntegerOffset.y <= 1; IntegerOffset.y++)
            {
                NewOffset = Voronoi2DCornerSample( InSeed, NoiseTile2DWrap( IntegerPart + ((float2)IntegerOffset), InIsTiling, InRepeatSize ) );
                NewOffset.x = ((float)(IntegerOffset.x)) + NewOffset.x;
                NewOffset.y = ((float)(IntegerOffset.y)) + NewOffset.y;
                MinDist = Voronoi2DCompare( MinDist, float3( IntegerPart + NewOffset.xy, NewOffset.z ), FractionPart - NewOffset.xy, InIsDistanceOnly );
            }
        }
    }
	
    return float4( MinDist.xyz, sqrt( MinDist.w ) );
}

float2 QuadraticBezierCurve( in float2 P0, in float2 P1, in float2 P2, in float T )
{
    float TSq = T * T;
    float OneMinusT = 1.0f - T;
    float OneMinusTSq = OneMinusT * OneMinusT;
    return (P0 * OneMinusTSq + P1 * (2.0f * OneMinusT * T) + P2 * TSq);
}

float2 DerivativeQuadraticBezierCurve( in float2 P0, in float2 P1, in float2 P2, in float T )
{
    return ((P1 - P0) * (2.0f * (1.0f - T)) + (P2 - P1) * (2.0f * T));
}

float2 CubicBezierCurve( in float2 P0, in float2 P1, in float2 P2, in float2 P3, in float T )
{
    float TSq = T * T;
    float TCu = TSq * T;
    float OneMinusT = 1.0f - T;
    float OneMinusTSq = OneMinusT * OneMinusT;
    float OneMinusTCu = OneMinusTSq * OneMinusT;
    return (P0 * OneMinusTCu + P1 * (3.0f * OneMinusTSq * T) + P2 * (3.0f * OneMinusT * TSq) + P3 * TCu);
}

float2 DerivativeCubicBezierCurve( in float2 P0, in float2 P1, in float2 P2, in float2 P3, in float T )
{
    float TSq = T * T;
    float OneMinusT = 1.0f - T;
    float OneMinusTSq = OneMinusT * OneMinusT;
    return ((P1 - P0) * (3.0f * OneMinusTSq) + (P2 - P1) * (6.0f * OneMinusT * T) + (P3 - P2) * (3.0f * TSq));
}

FBezierGrassVertexData BezierGrassBladeLocalPosition(
	in FBezierGrassLayerTypeBaseData InLayerBaseData,
	in FBezierGrassParameter InParams,
	in FBezierGrassTileInstanceData InTileData,
	in FBezierGrassUnpackedInstanceData InInstanceData,
	in FBezierGrassTimeRelevanceUnpackedData InInstanceTimeRelevanceData,
	in FBezierGrassUnpackedVertexInputData InVertexData,
	in FDFVector3 InPreViewTranslation )
{
    FBezierGrassVertexData Result = (FBezierGrassVertexData)0;
	
    const bool bBent = InInstanceData.bBent;
    const bool bBentLeftPart = InVertexData.bBentLeftPart;
    const float2 BentRotate = bBent ? (bBentLeftPart ? float2( 0.5, 0.8660254 ) : float2( -0.5, 0.8660254 )) : float2( 0, 1 ); // bBentLeftPart ? float2(Sin(30), Cos(30)) : float2(Sin(-30), Cos(-30))
	
    const float UnmappedBentBezierT = lerp( abs( InVertexData.TargetT - InParams.BentBezierT ), abs( InVertexData.PrevT - InParams.BentBezierT ), frac( InInstanceData.LOD ) );
    const float BezierT = bBent ? (bBentLeftPart ? (UnmappedBentBezierT / (1 - InParams.BentBezierT)) : (UnmappedBentBezierT / InParams.BentBezierT)) : lerp( InVertexData.TargetT, InVertexData.PrevT, frac( InInstanceData.LOD ) );
    const float SideOffset = InVertexData.SideOffset;
	
    const float Height = InInstanceData.Height;
    const float2 EndPoint = float2( InInstanceTimeRelevanceData.TiltSin * Height, InInstanceTimeRelevanceData.TiltCos * Height );
	
    const float MidPointT = InInstanceTimeRelevanceData.MidPointT;
    const float Bend = InInstanceTimeRelevanceData.Bend;
    const float2 ContolPoint = lerp( 0.0f, EndPoint.xy, MidPointT ) + float2( -EndPoint.y, EndPoint.x ) * Bend;
	
    const float2 PositionOS = QuadraticBezierCurve( 0.0f, ContolPoint, EndPoint, BezierT );
    const float2 NormalOS = DerivativeQuadraticBezierCurve( 0.0f, ContolPoint, EndPoint, BezierT );
	
    const float3 BaseFacing3D = float3( InInstanceTimeRelevanceData.Facing.x, InInstanceTimeRelevanceData.Facing.y, 0.0f );
    const float3 BaseNmlOrthWS = float3( InInstanceTimeRelevanceData.Facing.y, -InInstanceTimeRelevanceData.Facing.x, 0.0f );
    const float3 Facing3D = BaseNmlOrthWS * BentRotate.x + BaseFacing3D * BentRotate.y;
    const float3 NmlOrthWS = float3( Facing3D.y, -Facing3D.x, 0.0f );
    const float3 UpVector3D = float3( 0.0f, 0.0f, 1.0f );
    const float3 NmlBezierWS = normalize( (Facing3D.xyz * NormalOS.x) + (UpVector3D.xyz * NormalOS.y) ); // Need SafeNormalize
	
    const float FlipNormalMultipler = bBent ? (bBentLeftPart ? -1 : 1) : (-1); // WHY CCW ?
    const float3 BezierFaceNormal = normalize( cross( NmlOrthWS.xyz, NmlBezierWS.xyz ) ); // Need SafeNormalize
	
    Result.GeomNormal = BezierFaceNormal * FlipNormalMultipler;
    Result.GeomTangent = -NmlOrthWS.xyz;
	
    const float CurveIndex = BezierT * ((float)(BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM - 1));
    const uint CurveStartIndex = (uint)(floor( CurveIndex ));
    const uint CurveEndIndex = min( CurveStartIndex + 1, BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM - 1 );
    const float SideCurveValue = lerp( InLayerBaseData.SideCurve[ CurveStartIndex ], InLayerBaseData.SideCurve[ CurveEndIndex ], frac( CurveIndex ) );
	
    Result.BezierT = BezierT;
    Result.CurveUV = (-SideOffset) * 0.5f + 0.5f; // SideCurveValue * (-SideOffset)
	
    const float3 RootPosition = float3( InInstanceData.OriginXY.xy, InInstanceData.OriginHeight );
    const float3 ControlPosition = (Facing3D.xyz * ContolPoint.x) + (UpVector3D.xyz * ContolPoint.y) + RootPosition.xyz;
    const float3 TipPosition = (Facing3D.xyz * EndPoint.x) + (UpVector3D.xyz * EndPoint.y) + RootPosition.xyz;
	
    Result.RootPosition = RootPosition.xyz;
    Result.ControlPosition = ControlPosition.xyz;
    Result.TipPosition = TipPosition.xyz;
	
    Result.Position = (Facing3D.xyz * PositionOS.x) + (UpVector3D.xyz * PositionOS.y) + RootPosition.xyz;
	
	// TODO : LocalToWorld
    Matrix LocalToWorld;
    float3 ViewVectorWorldSpace = TransformLocalToTranslatedWorld( RootPosition.xyz, LocalToWorld, InPreViewTranslation ).xyz;
    float3 NormalizedViewVectorWorldSpace = normalize( ViewVectorWorldSpace.xyz );
	//float	ViewDistSq						= dot(ViewVectorWorldSpace.xyz, ViewVectorWorldSpace.xyz);
	
    const float NdotV = dot( NormalizedViewVectorWorldSpace.xyz, NmlOrthWS.xyz );
    const bool bFlipNormal = sign( NdotV ) < (-0.5);
    const float SignNdotV = bFlipNormal ? (-1) : 1;
    const float AbsNdotV = abs( NdotV );
    const float Width = InInstanceData.Width;
	
#if 1
    Result.Normal = Result.GeomNormal.xyz;
#else
	Result.Normal			= lerp(lerp(-BezierFaceNormal.xyz, -NormalizedViewVectorWorldSpace.xyz, 0.05), BezierFaceNormal.xyz, (((bFlipNormal ? (-(SideOffset)) : SideOffset) + 1.0f) * 0.5f));
	Result.Normal			= lerp(Result.GeomNormal.xyz, Result.Normal.xyz, AbsNdotV);
#endif
    Result.Position = lerp( NmlOrthWS.xyz, SignNdotV * BezierFaceNormal.xyz, AbsNdotV ) * Width * SideCurveValue * SideOffset + Result.Position.xyz;

    return Result;
}


#define DEFINE_BEZIER_GRASS_WIND_LEVEL_MAX					4

void ModifyBezierGrassBlade(
	in FBezierGrassConstructParameter InParameter,
	in FBezierGrassLayerTypeBaseData InLayerTypeBaseData,
	in float2 InOriginInWorldXY,
	in uint InPerBladeHash,
	in float4 InTimeParams,
	inout float InOutWindStrength,
	inout float InOutTilt,
	inout float InOutBend,
	inout float InOutMidPointT,
	inout float InOutWindFacing,
	inout float InOutFacingOffset )
{
	// Properties that need convert to parameters for changing easily
	//***************************
	// Properties Field Start
	//***************************
    const float2 WorldPosScale = 0.008;
	
	// Wind level mappings. This is example for 0~3 level. Each level contains 4 float params
    const float WindLevelStepValue[ DEFINE_BEZIER_GRASS_WIND_LEVEL_MAX ] =
    {
        2.0, // Wind Level 0 if wind strength ∈ [0.0, 2.0)
		4.0, // Wind Level 1 if wind strength ∈ [2.0, 4.0)
		6.0, // Wind Level 2 if wind strength ∈ [4.0, 6.0)
		10.0 // Wind Level 3 if wind strength ∈ [6.0, 10.0)
				// ...
    };
    const float WindMovementScale[ (DEFINE_BEZIER_GRASS_WIND_LEVEL_MAX + 1) * 1 ] =
    {
        0.312578 * 125.0,
		0.512347 * 125.0,
		4.793710 * 125.0,
		9.510091 * 125.0,
		13.56123 * 125.0
    };
    const float WindFrequencyScale[ (DEFINE_BEZIER_GRASS_WIND_LEVEL_MAX + 1) * 1 ] =
    {
        0.27318,
		3.92699,
		7.85398,
		11.423973,
		15.707963
    };
    const float WindTiltLevel[ (DEFINE_BEZIER_GRASS_WIND_LEVEL_MAX + 1) * 2 ] =
    {
        0.00, 0.05, // Tilt Offset & Tilt Oscillation
		0.10, 0.08, // Tilt Offset & Tilt Oscillation when wind is level 0
		0.15, 0.09, // ...
		0.20, 0.10, // ...
		0.25, 0.11 // ...
						// ...
    };
    const float WindBendLevel[ (DEFINE_BEZIER_GRASS_WIND_LEVEL_MAX + 1) * 1 ] =
    {
        0.02, // Bend Oscillation
		0.04, // Bend Oscillation when wind is level 0
		0.06, // ...
		0.08, // ...
		0.10 // ...
					// ...
    };
    const float WindMidTLevel[ (DEFINE_BEZIER_GRASS_WIND_LEVEL_MAX + 1) * 1 ] =
    {
        0.0, // MidPointT Offset Base
		-0.05, // MidPointT Offset when wind is level 0
		-0.1, // ...
		-0.25, // ...
		-0.4 // ...
					// ...
    };
	//***************************
	// Properties Field End
	//***************************
	
    const float2 GlobalWindDirection = InParameter.WindDirectionStrength.xy;
    const float GlobalWindFacing = InParameter.WindDirectionStrength.z;
    const float GlobalWindStrength = InParameter.WindDirectionStrength.w;
	
	// Determine wind level
    int WindLevel = 0;
    float WindLevelStepValueMin;
    float WindLevelStepValueRange;
    float RemappedStrength;
	{
        UNROLL
        for (int TestWindLevelIndex = 0; TestWindLevelIndex < DEFINE_BEZIER_GRASS_WIND_LEVEL_MAX; TestWindLevelIndex++)
        {
            WindLevel = (GlobalWindStrength > WindLevelStepValue[ TestWindLevelIndex ]) ? (TestWindLevelIndex + 1) : WindLevel;
        }
        WindLevel = min( WindLevel, DEFINE_BEZIER_GRASS_WIND_LEVEL_MAX - 1 );
        WindLevelStepValueMin = (WindLevel == 0) ? (InParameter.WindStrengthRange.x) : (WindLevelStepValue[ WindLevel - 1 ]);
        WindLevelStepValueRange = max( 0.01, WindLevelStepValue[ WindLevel ] - WindLevelStepValueMin );
        RemappedStrength = (GlobalWindStrength - WindLevelStepValueMin) / WindLevelStepValueRange;
    }
	
	// Voronoi Wind
    float NoiseValue2D = 0;
    const float MovementScale = lerp( WindMovementScale[ WindLevel ], WindMovementScale[ WindLevel + 1 ], RemappedStrength );
    const float2 SampledPosOffset = (GlobalWindDirection.xy * MovementScale) * InTimeParams.x;
#if 1	// Sampling voronoi get smooth result. Use texture instead.
	{
        const float2 IPos = floor( InOriginInWorldXY.xy );
        const float2 FPos = InOriginInWorldXY.xy - IPos.xy;
        float NoizeValueY0;
		{
            float2 SampledPos00 = (IPos.xy + float2( 0, 0 ) + SampledPosOffset.xy) * WorldPosScale;
            const float4 NoiseValue00 = VoronoiNoise2D_ALU( InParameter.RandomSeedParams.w, SampledPos00.xy, false, 1.0, false );
            float2 SampledPos10 = (IPos.xy + float2( 1, 0 ) + SampledPosOffset.xy) * WorldPosScale;
            const float4 NoiseValue10 = VoronoiNoise2D_ALU( InParameter.RandomSeedParams.w, SampledPos10.xy, false, 1.0, false );
            NoizeValueY0 = lerp( NoiseValue00.z, NoiseValue10.z, FPos.x );
        }
        float NoizeValueY1;
		{
            float2 SampledPos01 = (IPos.xy + float2( 0, 1 ) + SampledPosOffset.xy) * WorldPosScale;
            const float4 NoiseValue01 = VoronoiNoise2D_ALU( InParameter.RandomSeedParams.w, SampledPos01.xy, false, 1.0, false );
            float2 SampledPos11 = (IPos.xy + float2( 1, 1 ) + SampledPosOffset.xy) * WorldPosScale;
            const float4 NoiseValue11 = VoronoiNoise2D_ALU( InParameter.RandomSeedParams.w, SampledPos11.xy, false, 1.0, false );
            NoizeValueY1 = lerp( NoiseValue01.z, NoiseValue11.z, FPos.x );
        }
        NoiseValue2D = lerp( NoizeValueY0, NoizeValueY1, FPos.y );
    }
#else	// ALU Only
	{
		float2 SampledPos = (InOriginInWorldXY.xy + SampledPosOffset.xy) * WorldPosScale;
		NoiseValue2D = VoronoiNoise2D_ALU(InParameter.RandomSeedParams.w, SampledPos.xy, false, 1.0, false).z;
	}
#endif	// Get voronoi value
    const float Strength = NoiseValue2D * WindLevelStepValueRange + WindLevelStepValueMin;
	
    const float UsedWindFrequency = lerp( WindFrequencyScale[ WindLevel ], WindFrequencyScale[ WindLevel + 1 ], RemappedStrength );
    const float ScaledTime = UsedWindFrequency * InTimeParams.x;
	
	// Random values
    const float3 RandValue3Z = ((float3)(Rand3DPCG16( uint3( ((uint2)(InOriginInWorldXY.xy)) ^ (InParameter.RandomSeedParams.z), InPerBladeHash ) ).xyz)) / 65535.0;
    const float3 RandValue3W = ((float3)(Rand3DPCG16( uint3( ((uint2)(InOriginInWorldXY.xy)) ^ (InParameter.RandomSeedParams.w), InPerBladeHash ) ).xyz)) / 65535.0;
	
#if 1
	// Modify Facing
	{
        InOutWindStrength = Strength;
        InOutFacingOffset = RandValue3Z.z * (InLayerTypeBaseData.Facing.z - InLayerTypeBaseData.Facing.y) + InLayerTypeBaseData.Facing.y;
        InOutWindFacing = GlobalWindFacing + InOutFacingOffset;
    }
#endif
	
#if 1
	// Modify Tilt
	{
        float Noise = NoiseValue2D * 3.14159265358979 + RandValue3W.x;
        float TiltMinOffset = WindTiltLevel[ WindLevel * 2 + 0 ];
        float TiltMinOscillation = WindTiltLevel[ WindLevel * 2 + 1 ];
        float TiltMaxOffset = WindTiltLevel[ (WindLevel + 1) * 2 + 0 ];
        float TiltMaxOscillation = WindTiltLevel[ (WindLevel + 1) * 2 + 1 ];
        float UsedTiltOffset = lerp( TiltMinOffset, TiltMaxOffset, RemappedStrength );
        float UsedTiltOscillation = lerp( TiltMinOscillation, TiltMaxOscillation, RemappedStrength );
        float CurrentOscillation = SinFunction( ScaledTime + Noise + 1.5707963267948966 ) * UsedTiltOscillation;
        InOutTilt = saturate( InOutTilt + UsedTiltOffset );
        InOutTilt = InOutTilt + CurrentOscillation;
    }
#endif
	
#if 1
	// Modify Bend
	{
        float Noise = NoiseValue2D * 3.14159265358979 + RandValue3W.y;
        float BendMinOscillation = WindBendLevel[ WindLevel ];
        float BendMaxOscillation = WindBendLevel[ WindLevel + 1 ];
        float UsedBendOscillation = lerp( BendMinOscillation, BendMaxOscillation, RemappedStrength );
        float CurrentOscillation = SinFunction( ScaledTime + Noise ) * UsedBendOscillation;
        InOutBend = InOutBend + CurrentOscillation;
    }
#endif

#if 1
	// Modify MidPointT
	{
        float MidTMinOffset = WindMidTLevel[ WindLevel ];
        float MidTMaxOffset = WindMidTLevel[ WindLevel + 1 ];
        float UsedMidTOffset = lerp( MidTMinOffset, MidTMaxOffset, RemappedStrength );
        InOutMidPointT = InOutMidPointT + UsedMidTOffset;
    }
#endif
}

#endif