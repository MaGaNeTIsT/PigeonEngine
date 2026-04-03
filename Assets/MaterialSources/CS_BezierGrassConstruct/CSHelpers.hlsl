#define BezierGrassLayerTypeBinding         LayerTypeDatas
#define BezierGrassPackedInstanceBinding    PackedInstanceDatas
#define BezierGrassDispatchBinding          DispatchDatas

#include "Public/ShaderVariables.hlsl"
#include "Public/ShaderSpaceTransform.hlsl"
#include "../Common/BezierGrassCommon.hlsl"

FBezierGrassConstructParameter InitBezierGrassConstructParameter()
{
    FBezierGrassConstructParameter Result = (FBezierGrassConstructParameter)0;

    Result.CurrentTimeParams                = CurrentTimeParams.xyzw;
    Result.PreviousTimeParams               = PreviousTimeParams.xyzw;
    Result.RandomSeedParams                 = RandomSeedParams.xyzw;
    Result.RandomSeed                       = RandomSeedNumAllocatedLODMaxNumInstances.x;
    Result.NumAllocatedInstances            = RandomSeedNumAllocatedLODMaxNumInstances.y;
    Result.LODMinMax                        = uint2(0, RandomSeedNumAllocatedLODMaxNumInstances.z);
    Result.TileAnchor                       = TileAnchorSize.xy;
    Result.TileSize                         = TileAnchorSize.zw;
    Result.TileXY                           = TileXYNumTiles.xy;
    Result.NumTiles                         = TileXYNumTiles.zw;
    Result.SubTileSize                      = SubTileSizeBorderSize.xy;
    Result.BorderSize                       = SubTileSizeBorderSize.zw;
    Result.SubTileXY                        = SubTileXYNumSubTiles.xy;
    Result.NumSubTiles                      = SubTileXYNumSubTiles.zw;
    Result.MaskExp                          = MaskXYNumInstances.x;
    Result.Mask                             = MaskXYNumInstances.y;
    Result.XNumInstances                    = MaskXYNumInstances.z;
    Result.YNumInstances                    = MaskXYNumInstances.w;
    Result.NumInstances                     = RandomSeedNumAllocatedLODMaxNumInstances.w;
    Result.DensityScale                     = DensityScaleWindStrengthRange.x;
    Result.WindDirectionStrength            = GlobalWindDirectionStrength.xyzw;
    Result.WindStrengthRange                = DensityScaleWindStrengthRange.yz;
    Result.HeightMapWorldScaleOffset        = asfloat(HeightMapWorldScaleOffsetBorderPixelSize.xy);
    Result.HeightMapPixelBorder             = HeightMapWorldScaleOffsetBorderPixelSize.z;
    Result.HeightMapPixelSize               = HeightMapWorldScaleOffsetBorderPixelSize.w;
    Result.IndexLayerPixelBorder            = LayerBorderPixelSizeDensityBorderPixelSize.x;
    Result.IndexLayerPixelSize              = LayerBorderPixelSizeDensityBorderPixelSize.y;
    Result.DensityPixelBorder               = LayerBorderPixelSizeDensityBorderPixelSize.z;
    Result.DensityPixelSize                 = LayerBorderPixelSizeDensityBorderPixelSize.w;
    Result.IndexLayerNumMaxTypes            = LayerTypeElemsBaseCustomTotalNumTypes.w;
    Result.IndexLayerNumBaseElems           = LayerTypeElemsBaseCustomTotalNumTypes.x;
    Result.IndexLayerNumCustomElems         = LayerTypeElemsBaseCustomTotalNumTypes.y;
    Result.IndexLayerNumTotalElems          = LayerTypeElemsBaseCustomTotalNumTypes.z;

    Result.BodyPart[0] = LODBodyPart1.x;
    Result.BodyPart[1] = LODBodyPart1.y;
    Result.BodyPart[2] = LODBodyPart1.z;
    Result.BodyPart[3] = LODBodyPart1.w;
    Result.BodyPart[4] = LODBodyPart2.x;
    Result.BodyPart[5] = LODBodyPart2.y;
    Result.BodyPart[6] = LODBodyPart2.z;
    Result.BodyPart[7] = LODBodyPart2.w;

    Result.IndexOffset[0] = IndexOffset1.x;
    Result.IndexOffset[1] = IndexOffset1.y;
    Result.IndexOffset[2] = IndexOffset1.z;
    Result.IndexOffset[3] = IndexOffset1.w;
    Result.IndexOffset[4] = IndexOffset2.x;
    Result.IndexOffset[5] = IndexOffset2.y;
    Result.IndexOffset[6] = IndexOffset2.z;
    Result.IndexOffset[7] = IndexOffset2.w;

    Result.VertexOffset[0] = VertexOffset1.x;
    Result.VertexOffset[1] = VertexOffset1.y;
    Result.VertexOffset[2] = VertexOffset1.z;
    Result.VertexOffset[3] = VertexOffset1.w;
    Result.VertexOffset[4] = VertexOffset2.x;
    Result.VertexOffset[5] = VertexOffset2.y;
    Result.VertexOffset[6] = VertexOffset2.z;
    Result.VertexOffset[7] = VertexOffset2.w;

    Result.LODDistancesSq[0] = LODDistancesSq1.x;
    Result.LODDistancesSq[1] = LODDistancesSq1.y;
    Result.LODDistancesSq[2] = LODDistancesSq1.z;
    Result.LODDistancesSq[3] = LODDistancesSq1.w;
    Result.LODDistancesSq[4] = LODDistancesSq2.x;
    Result.LODDistancesSq[5] = LODDistancesSq2.y;
    Result.LODDistancesSq[6] = LODDistancesSq2.z;
    Result.LODDistancesSq[7] = LODDistancesSq2.w;

    return Result;
}
