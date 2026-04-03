#ifndef _BEZIER_GRASS_PE_CS_HLSL
#define _BEZIER_GRASS_PE_CS_HLSL
#if 0
float4 CurrentTimeParams;
float4 PreviousTimeParams;
uint4 LayerTypeElemsBaseCustomTotalNumTypes;
uint4 RandomSeedParams;
uint4 RandomSeedNumAllocatedLODMaxNumInstances;
float4 TileAnchorSize;
uint4 TileXYNumTiles;
uint4 SubTileXYNumSubTiles;
float4 SubTileSizeBorderSize;
float4 GlobalWindDirectionStrength;
float4 DensityScaleWindStrengthRange;
uint4 MaskXYNumInstances;
uint4 LODBodyPart1;
uint4 LODBodyPart2;
uint4 IndexOffset1;
uint4 IndexOffset2;
uint4 VertexOffset1;
uint4 VertexOffset2;
float4 LODDistancesSq1;
float4 LODDistancesSq2;
uint4 HeightMapWorldScaleOffsetBorderPixelSize;
uint4 LayerBorderPixelSizeDensityBorderPixelSize;
float4 PreViewTranslationHigh;
float4 PreViewTranslationLow;
float4 ViewFrustumPlane[ 5 ];

Texture2D LandscapeHeightTexture;
SamplerState LandscapeHeightTextureSampler;
Texture2D BezierGrassLayerIndexTexture;
SamplerState LayerIndexTextureSampler;
Texture2D BezierGrassDensityTexture;
SamplerState DensityTextureSampler;
StructuredBuffer<uint> TileShuffleBuffer;
RWStructuredBuffer<uint> PackedInstanceDatas;
RWStructuredBuffer<uint> DispatchDatas;
RWBuffer<uint> DrawIndirectDatas;
StructuredBuffer<float> LayerTypeDatas;
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 0
	Texture2DArray									LayerTypeCustomTexArray0;
	SamplerState									LayerTypeCustomSampler0;
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 0
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 1
	Texture2DArray									LayerTypeCustomTexArray1;
	SamplerState									LayerTypeCustomSampler1;
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 1
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 2
	Texture2DArray									LayerTypeCustomTexArray2;
	SamplerState									LayerTypeCustomSampler2;
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 2
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 3
	Texture2DArray									LayerTypeCustomTexArray3;
	SamplerState									LayerTypeCustomSampler3;
#endif	// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 3

#define BezierGrassLayerTypeBinding						LayerTypeDatas
#define BezierGrassLayerTypeCustomTexArrayBinding0		LayerTypeCustomTexArray0
#define BezierGrassLayerTypeCustomSamplerBinding0		LayerTypeCustomSampler0
#define BezierGrassLayerTypeCustomTexArrayBinding1		LayerTypeCustomTexArray1
#define BezierGrassLayerTypeCustomSamplerBinding1		LayerTypeCustomSampler1
#define BezierGrassLayerTypeCustomTexArrayBinding2		LayerTypeCustomTexArray2
#define BezierGrassLayerTypeCustomSamplerBinding2		LayerTypeCustomSampler2
#define BezierGrassLayerTypeCustomTexArrayBinding3		LayerTypeCustomTexArray3
#define BezierGrassLayerTypeCustomSamplerBinding3		LayerTypeCustomSampler3

#define BezierGrassPackedInstanceBinding				PackedInstanceDatas

FBezierGrassConstructParameter InitBezierGrassConstructParameter()
{
    FBezierGrassConstructParameter Result = (FBezierGrassConstructParameter)0;
	
    Result.CurrentTimeParams = CurrentTimeParams.xyzw;
    Result.PreviousTimeParams = PreviousTimeParams.xyzw;
    Result.RandomSeedParams = RandomSeedParams.xyzw;
    Result.RandomSeed = RandomSeedNumAllocatedLODMaxNumInstances.x;
    Result.NumAllocatedInstances = RandomSeedNumAllocatedLODMaxNumInstances.y;
    Result.LODMinMax = uint2( 0, RandomSeedNumAllocatedLODMaxNumInstances.z );
    Result.TileAnchor = TileAnchorSize.xy;
    Result.TileSize = TileAnchorSize.zw;
    Result.TileXY = TileXYNumTiles.xy;
    Result.NumTiles = TileXYNumTiles.zw;
    Result.SubTileSize = SubTileSizeBorderSize.xy;
    Result.BorderSize = SubTileSizeBorderSize.zw;
    Result.SubTileXY = SubTileXYNumSubTiles.xy;
    Result.NumSubTiles = SubTileXYNumSubTiles.zw;
    Result.MaskExp = MaskXYNumInstances.x;
    Result.Mask = MaskXYNumInstances.y;
    Result.XNumInstances = MaskXYNumInstances.z;
    Result.YNumInstances = MaskXYNumInstances.w;
    Result.NumInstances = RandomSeedNumAllocatedLODMaxNumInstances.w;
    Result.DensityScale = DensityScaleWindStrengthRange.x;
    Result.WindDirectionStrength = GlobalWindDirectionStrength.xyzw;
    Result.WindStrengthRange = DensityScaleWindStrengthRange.yz; // w is not used
    Result.HeightMapWorldScaleOffset = asfloat( HeightMapWorldScaleOffsetBorderPixelSize.xy );
    Result.HeightMapPixelBorder = HeightMapWorldScaleOffsetBorderPixelSize.z;
    Result.HeightMapPixelSize = HeightMapWorldScaleOffsetBorderPixelSize.w;
    Result.IndexLayerPixelBorder = LayerBorderPixelSizeDensityBorderPixelSize.x;
    Result.IndexLayerPixelSize = LayerBorderPixelSizeDensityBorderPixelSize.y;
    Result.DensityPixelBorder = LayerBorderPixelSizeDensityBorderPixelSize.z;
    Result.DensityPixelSize = LayerBorderPixelSizeDensityBorderPixelSize.w;
    Result.IndexLayerNumMaxTypes = LayerTypeElemsBaseCustomTotalNumTypes.w;
    Result.IndexLayerNumBaseElems = LayerTypeElemsBaseCustomTotalNumTypes.x;
    Result.IndexLayerNumCustomElems = LayerTypeElemsBaseCustomTotalNumTypes.y;
    Result.IndexLayerNumTotalElems = LayerTypeElemsBaseCustomTotalNumTypes.z;
	
    Result.BodyPart[ 0 ] = LODBodyPart1.x;
    Result.BodyPart[ 1 ] = LODBodyPart1.y;
    Result.BodyPart[ 2 ] = LODBodyPart1.z;
    Result.BodyPart[ 3 ] = LODBodyPart1.w;
    Result.BodyPart[ 4 ] = LODBodyPart2.x;
    Result.BodyPart[ 5 ] = LODBodyPart2.y;
    Result.BodyPart[ 6 ] = LODBodyPart2.z;
    Result.BodyPart[ 7 ] = LODBodyPart2.w;
	
    Result.IndexOffset[ 0 ] = IndexOffset1.x;
    Result.IndexOffset[ 1 ] = IndexOffset1.y;
    Result.IndexOffset[ 2 ] = IndexOffset1.z;
    Result.IndexOffset[ 3 ] = IndexOffset1.w;
    Result.IndexOffset[ 4 ] = IndexOffset2.x;
    Result.IndexOffset[ 5 ] = IndexOffset2.y;
    Result.IndexOffset[ 6 ] = IndexOffset2.z;
    Result.IndexOffset[ 7 ] = IndexOffset2.w;
	
    Result.VertexOffset[ 0 ] = VertexOffset1.x;
    Result.VertexOffset[ 1 ] = VertexOffset1.y;
    Result.VertexOffset[ 2 ] = VertexOffset1.z;
    Result.VertexOffset[ 3 ] = VertexOffset1.w;
    Result.VertexOffset[ 4 ] = VertexOffset2.x;
    Result.VertexOffset[ 5 ] = VertexOffset2.y;
    Result.VertexOffset[ 6 ] = VertexOffset2.z;
    Result.VertexOffset[ 7 ] = VertexOffset2.w;
	
    Result.LODDistancesSq[ 0 ] = LODDistancesSq1.x;
    Result.LODDistancesSq[ 1 ] = LODDistancesSq1.y;
    Result.LODDistancesSq[ 2 ] = LODDistancesSq1.z;
    Result.LODDistancesSq[ 3 ] = LODDistancesSq1.w;
    Result.LODDistancesSq[ 4 ] = LODDistancesSq2.x;
    Result.LODDistancesSq[ 5 ] = LODDistancesSq2.y;
    Result.LODDistancesSq[ 6 ] = LODDistancesSq2.z;
    Result.LODDistancesSq[ 7 ] = LODDistancesSq2.w;
	
    return Result;
}

float GetLandscapeWorldHeight( in FBezierGrassConstructParameter InParameter, in float2 InOriginInTileXY )
{
    float LODValue = 0; // TODO
    float LODBias = 0; // TODO
	
    float2 TexelSize = InParameter.TileSize.xy / ((float)(InParameter.HeightMapPixelSize - (InParameter.HeightMapPixelBorder * 2u)));
    float2 TexPixelSizeMinusOne = ((float)(InParameter.HeightMapPixelSize - 1)).xx;
    float2 OriginInTile = InOriginInTileXY.xy;
	
    float2 SampleCoords = (clamp( OriginInTile.xy / TexelSize.xy + float2( InParameter.HeightMapPixelBorder, InParameter.HeightMapPixelBorder ), 0.0, TexPixelSizeMinusOne.xy ) + 0.5) / float2( InParameter.HeightMapPixelSize, InParameter.HeightMapPixelSize );
    float4 SampleValue = Texture2DSampleLevel( LandscapeHeightTexture, LandscapeHeightTextureSampler, SampleCoords, LODValue + LODBias );
    float Height = DecodePackedHeight( SampleValue.xy );
    float WorldHeight = Height * InParameter.HeightMapWorldScaleOffset.x + InParameter.HeightMapWorldScaleOffset.y;
#if 0
	float2	Normal					= float2(SampleValue.z, SampleValue.w) * float2(2, 2) - float2(1, 1);
	float3	WorldNormal				= float3(Normal.xy, sqrt(max(1 - dot(Normal, Normal), 0)));
#endif

    return WorldHeight;
}

FSampledLayerIndexMapData GetLayerTypeIndex( in FBezierGrassConstructParameter InParameter, in float2 InOriginInTileXY )
{
    FSampledLayerIndexMapData Result = (FSampledLayerIndexMapData)0;
	
	{
        float LODValue = 0; // TODO
        float LODBias = 0; // TODO
        float2 TexelSize = InParameter.TileSize.xy / ((float)(InParameter.IndexLayerPixelSize - (InParameter.IndexLayerPixelBorder * 2u)));
        float2 TexPixelSizeMinusOne = ((float)(InParameter.IndexLayerPixelSize - 1)).xx;
        float2 OriginInTile = InOriginInTileXY.xy;
        float2 SampleCoords = (clamp( OriginInTile.xy / TexelSize.xy + float2( InParameter.IndexLayerPixelBorder, InParameter.IndexLayerPixelBorder ), 0.0, TexPixelSizeMinusOne.xy ) + 0.5) / float2( InParameter.IndexLayerPixelSize, InParameter.IndexLayerPixelSize );
        float SampleValue = Texture2DSampleLevel( BezierGrassLayerIndexTexture, LayerIndexTextureSampler, SampleCoords, LODValue + LODBias ).x;
		
        Result.Index = (uint)(floor( ((float)(InParameter.IndexLayerNumMaxTypes - 1)) * SampleValue + 0.5 ));
    }
	
	{
        float LODValue = 0; // TODO
        float LODBias = 0; // TODO
        float2 TexelSize = InParameter.TileSize.xy / ((float)(InParameter.DensityPixelSize - (InParameter.DensityPixelBorder * 2u)));
        float2 TexPixelSizeMinusOne = ((float)(InParameter.DensityPixelSize - 1)).xx;
        float2 OriginInTile = InOriginInTileXY.xy;
        float2 SampleCoords = (clamp( OriginInTile.xy / TexelSize.xy + float2( InParameter.DensityPixelBorder, InParameter.DensityPixelBorder ), 0.0, TexPixelSizeMinusOne.xy ) + 0.5) / float2( InParameter.DensityPixelSize, InParameter.DensityPixelSize );
        float SampleValue = Texture2DSampleLevel( BezierGrassDensityTexture, DensityTextureSampler, SampleCoords, LODValue + LODBias ).x;
		
        Result.Weight = SampleValue;
    }
	
    return Result;
}

bool AllocateInstance( uint InNumAllocatedInstances, uint InLODIndex, out uint OutIndex )
{
    OutIndex = 0;
    InterlockedAdd( DrawIndirectDatas[ InLODIndex * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 1 ], 1u, OutIndex );
#if BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
	InterlockedMin(DrawIndirectDatas[InLODIndex * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 1], InNumAllocatedInstances - 1);
	return (OutIndex < InNumAllocatedInstances);
#else	// !BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
    return true;
#endif	// BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
}

[numthreads( BEZIER_GRASS_CONSTRUCT_TILE_SIZE, 1, 1 )]
void main( uint InDispatchThreadID : SV_DispatchThreadID )
{
    FBezierGrassConstructParameter Parameter = InitBezierGrassConstructParameter();
	
    if (InDispatchThreadID == 0)
    {
		// Dispatch data
		{
            DispatchDatas[ 0 ] = Parameter.XNumInstances;
            DispatchDatas[ 1 ] = Parameter.YNumInstances;
			
            const uint LODNumInstances = Parameter.XNumInstances * Parameter.YNumInstances;
			
            LOOP
            for (uint i = Parameter.LODMinMax.x; i <= Parameter.LODMinMax.y; i++)
            {
#if BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
				DispatchDatas[2 + i] = i * Parameter.NumAllocatedInstances;
#else	// !BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
                DispatchDatas[ 2 + i ] = i * LODNumInstances;
#endif	// BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
            }
        }
		
		// Indirect data
        LOOP
        for (uint i = Parameter.LODMinMax.x; i <= Parameter.LODMinMax.y; i++)
        {
            DrawIndirectDatas[ i * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 0 ] = (Parameter.BodyPart[ i ] * 2 + 1) * 3;
            DrawIndirectDatas[ i * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 2 ] = Parameter.IndexOffset[ i ];
            DrawIndirectDatas[ i * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 3 ] = Parameter.VertexOffset[ i ];
            DrawIndirectDatas[ i * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 4 ] = 0;
        }
    }
	
    const uint2 XYNumInstances = uint2( Parameter.XNumInstances, Parameter.YNumInstances );
    uint2 BaseBladeXY = uint2( InDispatchThreadID % XYNumInstances.x, InDispatchThreadID / XYNumInstances.x );
	
    BRANCH
    if (any( BaseBladeXY >= ((XYNumInstances.xy + 1) >> Parameter.MaskExp) ))
    {
        return;
    }
	
    uint2 BladeXY = BaseBladeXY.xy * Parameter.Mask;
    const uint BladeShuffleUintValue = TileShuffleBuffer[ BladeXY.y * XYNumInstances.x + BladeXY.x ];
    const float BladeShuffleValue = asfloat( BladeShuffleUintValue );
	
    const float DensityScale = (1.0f - (Parameter.DensityScale)) * 0.75f + 0.25f;
	
#if 1
    BRANCH
    if (BladeShuffleValue > (DensityScale))
    {
        return;
    }
#endif
	
    float2 InstanceSize = Parameter.SubTileSize.xy / ((float2)(XYNumInstances.xy));
    float2 CurrentTileOrigin = ((float2)(Parameter.TileXY.xy)) * Parameter.TileSize.xy + Parameter.TileAnchor.xy;
    float2 SubTileOrigin = ((float2)(Parameter.SubTileXY.xy)) * Parameter.SubTileSize.xy + CurrentTileOrigin.xy;
    float2 BaseBladeOriginXY = InstanceSize * (((float2)(BladeXY.xy)) + 0.5) + SubTileOrigin.xy;
	
    const int2 ActualInstanceGrid = (int2)((Parameter.TileXY.xy * (Parameter.NumSubTiles.xy * XYNumInstances.xy)) + (Parameter.SubTileXY.xy * XYNumInstances.xy) + BladeXY.xy);
	
    float2 TargetWorldOrigin;
    float2 MinOffset;
    float2 MinPositionVector;
	//uint PerBladeHash[9];	// TODO
	// Noise Phase
	{
        float MinDistSq = 10.0;
        int MinDistOffsetIndex = 0;
        float2 MinDistRandomPart = 0;
		
        const int2 LoopParams[ 8 ] =
        {
            { -1, -1 },
            { 0, -1 },
            { 1, -1 },
            { -1, 0 },
            { 1, 0 },
            { -1, 1 },
            { 0, 1 },
            { 1, 1 }
        };
				
        UNROLL

        for (int Index = 0; Index < 8; Index++)
        {
            int2 Offset = LoopParams[ Index ];
            int2 TestPosition = ActualInstanceGrid + Offset;
			//uint	BladeIndex		= (TestPosition.y + 1) * (XYNumInstances.x + 2) + (TestPosition.x + 1);					// PreBladeHashes Need [-1,1] Border for Noise
            uint UsedHash = Parameter.RandomSeed; //PreBladeHashes[BladeIndex];
			
			//PerBladeHash[(Offset.y + 1) * 3 + Offset.x + 1]	= UsedHash;													// TODO : May use shared buffer?
			
            float2 RandomValue2 = Rand3DPCG16( uint3( TestPosition.xy, UsedHash ) ).xy / 65535.0f; // Return Value ∈ [0, 1]
            float2 PositionVector = ((float2)(Offset.xy)) + RandomValue2.xy - float2( 0.5, 0.5 );
            float DistSq = dot( PositionVector.xy, PositionVector.xy );
			
            MinDistOffsetIndex = (DistSq < MinDistSq) ? Index : MinDistOffsetIndex;
            MinDistRandomPart = (DistSq < MinDistSq) ? RandomValue2.xy : MinDistRandomPart.xy;
			
            MinDistSq = min( DistSq, MinDistSq );
        }
		
        MinOffset = ((float2)(LoopParams[ MinDistOffsetIndex ].xy)) + MinDistRandomPart.xy;
        MinPositionVector = MinOffset.xy - float2( 0.5, 0.5 );
        TargetWorldOrigin = lerp( MinOffset.xy * InstanceSize.xy + BaseBladeOriginXY.xy, BaseBladeOriginXY.xy, 0.25 );
    }
	
    float2 CurrentInstanceOriginInTile = TargetWorldOrigin.xy - CurrentTileOrigin.xy;
	
	// Sample layer type
    FSampledLayerIndexMapData LayerData = GetLayerTypeIndex( Parameter, CurrentInstanceOriginInTile.xy );
	
    BRANCH

    if ((BladeShuffleValue >= (LayerData.Weight)) || (LayerData.Index >= Parameter.IndexLayerNumMaxTypes))
    {
        return;
    }
	
	// Sample landscape
    float WorldHeight = GetLandscapeWorldHeight( Parameter, CurrentInstanceOriginInTile.xy );
	
	// Construct part of instance data
    FBezierGrassUnpackedInstanceData UnpackedInstanceData = (FBezierGrassUnpackedInstanceData)0;
    FBezierGrassLayerTypeBaseData LayerTypeBaseData;
	{
        const uint ActualBladeIndex = ((uint)(ActualInstanceGrid.y)) * Parameter.NumTiles.x * Parameter.NumSubTiles.x * XYNumInstances.x + ((uint)(ActualInstanceGrid.x));
		
        UnpackedInstanceData.PerBladeHash = ActualBladeIndex ^ (Parameter.RandomSeedParams.x) ^ (Parameter.RandomSeed) ^ (BladeShuffleUintValue & 0xffffffu); //PerBladeHash[4];	// TODO
        UnpackedInstanceData.LayerTypeIndex = (LayerData.Index) & 0xffu;
		
        LayerTypeBaseData = GetBezierGrassLayerTypeBaseData( UnpackedInstanceData.LayerTypeIndex, Parameter.IndexLayerNumTotalElems );
    }
	
	// View Culling
	{
		// Conservative-Bound. Move culling after instance data generated if want more accurate result.
        const float3 BoundCenter = float3( TargetWorldOrigin.xy, WorldHeight );
        const float BoundLength = LayerTypeBaseData.Height.x + LayerTypeBaseData.Height.z;
		
        bool bInside = true;
		
        UNROLL

        for (uint i = 0; i < 5; i++)
        {
            bInside = bInside && ((dot( BoundCenter.xyz, ViewFrustumPlane[ i ].xyz ) - ViewFrustumPlane[ i ].w - BoundLength) <= 0);
        }
		
        BRANCH

        if (!bInside)
        {
            return;
        }
    }
	
	// TODO : Occlusion Culling
		
    float ViewDistSq;
    float LOD = 0.0;
    uint DrawLODIndex = 0;
	// Generate LOD Info
	{
		// TODO : LocalToWorld
        Matrix LocalToWorld;
		
        float3 TranslatedWorldPosition = TransformLocalToTranslatedWorld( float3( TargetWorldOrigin.xy, WorldHeight ), LocalToWorld, MakeDFVector3( PreViewTranslationHigh.xyz, PreViewTranslationLow.xyz ) ).xyz;
        ViewDistSq = dot( TranslatedWorldPosition.xyz, TranslatedWorldPosition.xyz );
		
        BRANCH

        if (ViewDistSq > Parameter.LODDistancesSq[ Parameter.LODMinMax.y ])
        {
            return;
        }
		
        LOOP

        for (uint i = Parameter.LODMinMax.x; i <= Parameter.LODMinMax.y; i++)
        {
            DrawLODIndex = (ViewDistSq > Parameter.LODDistancesSq[ i ]) ? i : DrawLODIndex;
        }
		
        DrawLODIndex = clamp( DrawLODIndex, Parameter.LODMinMax.x, Parameter.LODMinMax.y );
        uint LODNextIndex = clamp( DrawLODIndex + 1, Parameter.LODMinMax.x, Parameter.LODMinMax.y );
		
        float LODOffsetDistSq = Parameter.LODDistancesSq[ LODNextIndex ] - Parameter.LODDistancesSq[ DrawLODIndex ];
        float OffsetDistSq = clamp( ViewDistSq - Parameter.LODDistancesSq[ DrawLODIndex ], 0.0, LODOffsetDistSq );
		
        LOD = (LODNextIndex == DrawLODIndex) ? ((float)DrawLODIndex) : ((OffsetDistSq / LODOffsetDistSq) + ((float)DrawLODIndex));
    }
	
	// Construct rest instance data
	{
        const float3 RandValue3Y = ((float3)(Rand3DPCG16( uint3( (ActualInstanceGrid.xy) ^ (Parameter.RandomSeedParams.y), UnpackedInstanceData.PerBladeHash ) ).xyz)) / 65535.0f;
        const float2 RandValue3Z = ((float2)(Rand3DPCG16( uint3( (ActualInstanceGrid.xy) ^ (Parameter.RandomSeedParams.z), UnpackedInstanceData.PerBladeHash ) ).xy)) / 65535.0f;
		
        UnpackedInstanceData.Height = RandValue3Y.x * (LayerTypeBaseData.Height.z - LayerTypeBaseData.Height.y) + (LayerTypeBaseData.Height.x + LayerTypeBaseData.Height.y);
        UnpackedInstanceData.Width = RandValue3Y.y * (LayerTypeBaseData.Width.z - LayerTypeBaseData.Width.y) + (LayerTypeBaseData.Width.x + LayerTypeBaseData.Width.y);
		
        UnpackedInstanceData.LOD = LOD;
        UnpackedInstanceData.OriginXY = TargetWorldOrigin.xy;
        UnpackedInstanceData.OriginHeight = WorldHeight;
        UnpackedInstanceData.bBent = LayerTypeBaseData.bBent;
		
        const float BaseTilt = saturate( RandValue3Y.z * (LayerTypeBaseData.Tilt.z - LayerTypeBaseData.Tilt.y) + (LayerTypeBaseData.Tilt.x + LayerTypeBaseData.Tilt.y) );
        const float BaseBend = saturate( RandValue3Z.x * (LayerTypeBaseData.Bend.z - LayerTypeBaseData.Bend.y) + (LayerTypeBaseData.Bend.x + LayerTypeBaseData.Bend.y) );
        const float BaseMidPointT = saturate( RandValue3Z.y * (LayerTypeBaseData.MidPointT.z - LayerTypeBaseData.MidPointT.y) + (LayerTypeBaseData.MidPointT.x + LayerTypeBaseData.MidPointT.y) );
		
		// Current
		{
            float WindStrength = 0;
            float Tilt = BaseTilt;
            float Bend = BaseBend;
            float MidPointT = BaseMidPointT;
            float WindFacing = 0;
            float FacingOffset = 0;
			
            ModifyBezierGrassBlade(
				Parameter,
				LayerTypeBaseData,
				TargetWorldOrigin.xy,
				UnpackedInstanceData.PerBladeHash,
				Parameter.CurrentTimeParams.xyzw,
				WindStrength,
				Tilt,
				Bend,
				MidPointT,
				WindFacing,
				FacingOffset );
			
            const float Facing = (LayerTypeBaseData.bUseFacing) ? (LayerTypeBaseData.Facing.x + FacingOffset) : (WindFacing);
            float2 FacingSinCos;
            SinCosFunction( Facing, FacingSinCos.y, FacingSinCos.x ); // Rad
			
            UnpackedInstanceData.CurrentTimeRelevanceData.Facing = FacingSinCos.xy;
            UnpackedInstanceData.CurrentTimeRelevanceData.WindStrength = WindStrength;
            UnpackedInstanceData.CurrentTimeRelevanceData.Bend = Bend;
            UnpackedInstanceData.CurrentTimeRelevanceData.MidPointT = MidPointT;
			
            SinCosFunction( Tilt * 1.5707963267948966, UnpackedInstanceData.CurrentTimeRelevanceData.TiltSin, UnpackedInstanceData.CurrentTimeRelevanceData.TiltCos ); // Half PI
        }
		
		#if 1
		// Previous for montion vector
		{
            float WindStrength = 0;
            float Tilt = BaseTilt;
            float Bend = BaseBend;
            float MidPointT = BaseMidPointT;
            float WindFacing = 0;
            float FacingOffset = 0;
			
            ModifyBezierGrassBlade(
				Parameter,
				LayerTypeBaseData,
				TargetWorldOrigin.xy,
				UnpackedInstanceData.PerBladeHash,
				Parameter.PreviousTimeParams.xyzw,
				WindStrength,
				Tilt,
				Bend,
				MidPointT,
				WindFacing,
				FacingOffset );
			
            const float Facing = (LayerTypeBaseData.bUseFacing) ? (LayerTypeBaseData.Facing.x + FacingOffset) : (WindFacing);
            float2 FacingSinCos;
            SinCosFunction( Facing, FacingSinCos.y, FacingSinCos.x ); // Rad
			
            UnpackedInstanceData.PreviousTimeRelevanceData.Facing = FacingSinCos.xy;
            UnpackedInstanceData.PreviousTimeRelevanceData.WindStrength = WindStrength;
            UnpackedInstanceData.PreviousTimeRelevanceData.Bend = Bend;
            UnpackedInstanceData.PreviousTimeRelevanceData.MidPointT = MidPointT;
			
            SinCosFunction( Tilt * 1.5707963267948966, UnpackedInstanceData.PreviousTimeRelevanceData.TiltSin, UnpackedInstanceData.PreviousTimeRelevanceData.TiltCos ); // Half PI
        }
#else
		UnpackedInstanceData.PreviousTimeRelevanceData = UnpackedInstanceData.CurrentTimeRelevanceData;
#endif
    }
	
	// Output
	{
        uint InstanceIndex;
#if BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
		if (!(AllocateInstance(Parameter.NumAllocatedInstances, DrawLODIndex, InstanceIndex)))
		{
			return;
		}
#else	// !BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
        AllocateInstance( Parameter.NumAllocatedInstances, DrawLODIndex, InstanceIndex );
#endif	// BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
        FBezierGrassParameter TempParameter = (FBezierGrassParameter)0;
        TempParameter.NumTiles = Parameter.NumTiles.xy;
        TempParameter.LODMinMax = (float2)(Parameter.LODMinMax.xy);
        TempParameter.TileAnchor = Parameter.TileAnchor.xy;
        TempParameter.TileSize = Parameter.TileSize.xy;
        TempParameter.SubTileSize = Parameter.SubTileSize.xy;
        TempParameter.BorderSize = Parameter.BorderSize.xy;
        TempParameter.NumSubTiles = Parameter.NumSubTiles.xy;
        TempParameter.NumInstances = XYNumInstances.xy;
        TempParameter.WindStrengthRange = Parameter.WindStrengthRange.xy;
		
        const uint2 Grid = Parameter.TileXY.xy;
        const uint2 SubGrid = Parameter.SubTileXY.xy;
        FBezierGrassTileInstanceData TileData = InitBezierGrassTileInstanceData( TempParameter, Grid, SubGrid );
		
#if BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
		const uint FinalInstanceIndex = DrawLODIndex * Parameter.NumAllocatedInstances + InstanceIndex;
#else	// !BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
        const uint FinalInstanceIndex = DrawLODIndex * XYNumInstances.x * XYNumInstances.y + InstanceIndex;
#endif	// BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
        FBezierGrassPackedInstanceData PackedInstanceData = PackBezierGrassInstanceData( TempParameter, TileData, LayerTypeBaseData, UnpackedInstanceData );
        SetPackedInstanceData( FinalInstanceIndex, PackedInstanceData );
    }
}

#else
[numthreads(8, 1, 1)]
void main()
{
    
}
#endif  // 0
#endif  // _BEZIER_GRASS_PE_CS_HLSL