#ifndef BEZIER_GRASS_CONSTRUCT_HLSL
#define BEZIER_GRASS_CONSTRUCT_HLSL

// ---------------------------------------------------------------------------
// Bezier grass construct compute pass.
//
// The algorithm is the legacy CS_BezierGrassConstruct/Compute.hlsl, together with
// InitBezierGrassConstructParameter, which is taken from the same material's
// CSHelpers.hlsl: that file was otherwise nothing but binding declarations, so
// this is where its one piece of code belongs. The body is injected at the
// COMPUTE_BODY placeholder of Templates/ComputePass_CS.tmpl.hlsl, which is why
// the [numthreads] attribute lives here rather than in the template.
//
// What the port changed, and it is only the parameter path:
//
//   old the cbuffer ConstantBufferBezierGrassLOD : register(b1) that CSHelpers.hlsl
//       declared, plus BezierGrassConstructParams from the old material JSON
//       new BezierGrassLODParams and BezierGrassConstructParams, declared in
//       Compute/BezierGrass/BezierGrassConstruct.json. The fields keep the legacy
//       names, types and order, so every member read below is unchanged. Only the
//       three world matrices at the head of the legacy block are gone: nothing in
//       this pass read them, and a world matrix now comes from
//       Components/C_DrawParams.json while the view matrix comes from
//       Components/C_EngineSystem.json.
//   old the #define aliases of CSHelpers.hlsl (BezierGrassLayerTypeBinding and its
//       two siblings) and the register slots it spelled out
//       new nothing. Every binding of this pass, including the three textures and
//       the five buffers, is one line in the component JSON.
//   old LandscapeHeightTexture.SampleLevel(sLinearClamp, UV, 0)
//       new PE_SAMPLE_TEX2D_LEVEL(LandscapeHeightTexture, SamplerLinearClamp, UV, 0)
//       so the bindless switch stays a one file change. The sampler is the
//       engine's, from C_EngineSystem.json, because the legacy one was the same
//       filter and address mode under a private name; this component declares no
//       sampler of its own.
//   old TransformWorldToView(float3) from Public/ShaderSpaceTransform.hlsl, which
//       read the view matrix out of the per frame cbuffer and computed
//       mul(float4(Position, 1.0), ViewMatrix)
//       new TransformWorldToView(float3, float4x4) from
//       Public/Core/ShaderSpaceTransform.hlsl, called with the ViewParams
//       ViewMatrix field and computing mul(ViewMatrix, float4(Position, 1.0)).
//       This is the mul(v, M) -> mul(M, v) flip the port asks for. It is the same
//       transform: the new upload path no longer transposes, so the view matrix
//       reaches the shader in the orientation the matrix first product expects.
//   old [numthreads(BEZIER_GRASS_CONSTRUCT_TILE_SIZE, 1, 1)], a 64 thread, one
//       dimensional tile, with SV_DispatchThreadID as the flat index
//       new [numthreads(SHADER_THREAD_GROUP_X, SHADER_THREAD_GROUP_Y, 1)], which
//       the program description sets to 8 x 8: the same 64 thread tile, expressed
//       in two dimensions. The flat index is rebuilt from SV_GroupID and
//       SV_GroupIndex below, so one dispatch thread still covers the tile exactly
//       once and the InterlockedAdd of AllocateInstance counts blades, not
//       threads. The dispatch itself stays one dimensional in X, which is what
//       RBezierGrassSceneProxy does.
//   old the whole frustum cull, inside #if 0, reading ViewFrustumPlane, a name the
//       grass constant buffer never declared
//       new ported and live, in the legacy's own place in the flow and with the
//       legacy's arithmetic. The five planes are ViewFrustumPlane0 through
//       ViewFrustumPlane4 of BezierGrassLODParams; the bundle gathers them into
//       Parameter.ViewFrustumPlane, which is where the global array the legacy
//       indexed went. Two things about what it reads: the render side still
//       writes the placeholder box into those fields, so the cull runs but
//       rejects nothing until a real frustum is filled in, and
//       PreViewTranslationHigh and PreViewTranslationLow, which the same buffer
//       declares, are read neither by this block nor by TransformWorldToView,
//       which takes the view matrix alone
//   old void main(...), FBezierGrassConstructParameter, FSampledLayerIndexMapData
//       new void CSMain(...), the Entry the program description names, and type
//       names without the F prefix.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"
#include "Public/Core/ShaderSpaceTransform.hlsl"
#include "Public/BezierGrass/BezierGrassCommon.hlsl"

// Copies the two constant buffers into the bundle the algorithm works with. This
// is the only place in the pass that touches the loose constant buffer globals.
BezierGrassConstructParameter InitBezierGrassConstructParameter()
{
	BezierGrassConstructParameter Result = (BezierGrassConstructParameter)0;

	Result.CurrentTimeParams = CurrentTimeParams.xyzw;
	Result.PreviousTimeParams = PreviousTimeParams.xyzw;
	Result.RandomSeedParams = RandomSeedParams.xyzw;
	Result.RandomSeed = RandomSeed.x;
	Result.NumAllocatedInstances = NumAllocatedLODMaxNumInstances.x;
	Result.LODMinMax = uint2(NumAllocatedLODMaxNumInstances.y, NumAllocatedLODMaxNumInstances.z);
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
	Result.NumInstances = NumAllocatedLODMaxNumInstances.w;
	Result.DensityScale = DensityScale.x;
	Result.WindDirectionStrength = GlobalWindDirectionStrength.xyzw;
	Result.WindStrengthRange = WindStrengthRange.xy;
	Result.HeightMapWorldScaleOffset = asfloat(HeightMapWorldScaleOffsetBorderPixelSize.xy);
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

	Result.ViewFrustumPlane[0] = ViewFrustumPlane0;
	Result.ViewFrustumPlane[1] = ViewFrustumPlane1;
	Result.ViewFrustumPlane[2] = ViewFrustumPlane2;
	Result.ViewFrustumPlane[3] = ViewFrustumPlane3;
	Result.ViewFrustumPlane[4] = ViewFrustumPlane4;

	return Result;
}

// Bilinear fetch of the landscape height under a blade root. The six lines of
// arithmetic are the legacy tile and border mapping and are untouched; only the
// fetch itself goes through the macro.
float GetLandscapeWorldHeight(in BezierGrassConstructParameter InParameter, in float2 InOriginInTileXY)
{
	float2 TexelSize = InParameter.TileSize.xy / ((float)(InParameter.HeightMapPixelSize - (InParameter.HeightMapPixelBorder * 2u)));
	float2 TexPixelSizeMinusOne = ((float)(InParameter.HeightMapPixelSize - 1)).xx;
	float2 SampleCoords = (clamp(InOriginInTileXY.xy / TexelSize.xy + float2(InParameter.HeightMapPixelBorder, InParameter.HeightMapPixelBorder), 0.0, TexPixelSizeMinusOne.xy) + 0.5) / float2(InParameter.HeightMapPixelSize, InParameter.HeightMapPixelSize);
	float4 SampleValue = PE_SAMPLE_TEX2D_LEVEL(LandscapeHeightTexture, SamplerLinearClamp, SampleCoords, 0);
	float Height = SampleValue.x;
	return Height * InParameter.HeightMapWorldScaleOffset.x + InParameter.HeightMapWorldScaleOffset.y;
}

// The layer type index and the density weight of the blade at that spot. Both
// fetches use the same mapping as the height fetch above, with their own sizes.
SampledLayerIndexMapData GetLayerTypeIndex(in BezierGrassConstructParameter InParameter, in float2 InOriginInTileXY)
{
	SampledLayerIndexMapData Result = (SampledLayerIndexMapData)0;

	{
		float2 TexelSize = InParameter.TileSize.xy / ((float)(InParameter.IndexLayerPixelSize - (InParameter.IndexLayerPixelBorder * 2u)));
		float2 TexPixelSizeMinusOne = ((float)(InParameter.IndexLayerPixelSize - 1)).xx;
		float2 SampleCoords = (clamp(InOriginInTileXY.xy / TexelSize.xy + float2(InParameter.IndexLayerPixelBorder, InParameter.IndexLayerPixelBorder), 0.0, TexPixelSizeMinusOne.xy) + 0.5) / float2(InParameter.IndexLayerPixelSize, InParameter.IndexLayerPixelSize);
		float SampleValue = PE_SAMPLE_TEX2D_LEVEL(BezierGrassLayerIndexTexture, SamplerLinearClamp, SampleCoords, 0).x;
		Result.Index = (uint)(floor(((float)(InParameter.IndexLayerNumMaxTypes - 1)) * SampleValue + 0.5));
	}

	{
		float2 TexelSize = InParameter.TileSize.xy / ((float)(InParameter.DensityPixelSize - (InParameter.DensityPixelBorder * 2u)));
		float2 TexPixelSizeMinusOne = ((float)(InParameter.DensityPixelSize - 1)).xx;
		float2 SampleCoords = (clamp(InOriginInTileXY.xy / TexelSize.xy + float2(InParameter.DensityPixelBorder, InParameter.DensityPixelBorder), 0.0, TexPixelSizeMinusOne.xy) + 0.5) / float2(InParameter.DensityPixelSize, InParameter.DensityPixelSize);
		float SampleValue = PE_SAMPLE_TEX2D_LEVEL(BezierGrassDensityTexture, SamplerLinearClamp, SampleCoords, 0).x;
		Result.Weight = SampleValue;
	}

	return Result;
}

// Claims the next instance slot of a LOD and reports the index it got. The
// counter lives in the indirect args, so the number of blades a LOD ends up
// drawing is known without a readback.
//
// BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE is 0 in the component JSON, which is the
// state the legacy build was in too: it was never defined, so every #if on it
// took the false branch. It stays a flag because the alternate path is the one
// the clamp and the return value below belong to.
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

[numthreads(SHADER_THREAD_GROUP_X, SHADER_THREAD_GROUP_Y, 1)]
void CSMain(uint3 InGroupId : SV_GroupID, uint InGroupIndex : SV_GroupIndex)
{
	// The legacy index was SV_DispatchThreadID under a one dimensional 64 thread
	// group; this reproduces it under the two dimensional tile of the same size.
	// SV_GroupIndex is the group's own flattening, x fastest, so one group covers
	// 64 consecutive indices and the whole dispatch covers each index once, which
	// is what AllocateInstance depends on. The dispatch has to stay one
	// dimensional in X for the arithmetic to hold.
	const uint DispatchThreadIndex = InGroupId.x * (SHADER_THREAD_GROUP_X * SHADER_THREAD_GROUP_Y) + InGroupIndex;

	BezierGrassConstructParameter Parameter = InitBezierGrassConstructParameter();

	if (DispatchThreadIndex == 0)
	{
		DispatchDatas[0] = Parameter.XNumInstances;
		DispatchDatas[1] = Parameter.YNumInstances;

		const uint LODNumInstances = Parameter.XNumInstances * Parameter.YNumInstances;

		[loop]
		for (uint Index = Parameter.LODMinMax.x; Index <= Parameter.LODMinMax.y; Index++)
		{
#if BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
			DispatchDatas[2 + Index] = Index * Parameter.NumAllocatedInstances;
#else
			DispatchDatas[2 + Index] = Index * LODNumInstances;
#endif
		}

		[loop]
		for (uint Index = Parameter.LODMinMax.x; Index <= Parameter.LODMinMax.y; Index++)
		{
			DrawIndirectDatas[Index * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 0] = (Parameter.BodyPart[Index] * 2 + 1) * 3;
			DrawIndirectDatas[Index * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 2] = Parameter.IndexOffset[Index];
			DrawIndirectDatas[Index * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 3] = Parameter.VertexOffset[Index];
			DrawIndirectDatas[Index * BEZIER_GRASS_INDIRECT_ARGS_NUM_WORDS + 4] = 0;
		}
	}

	const uint2 XYNumInstances = uint2(Parameter.XNumInstances, Parameter.YNumInstances);
	uint2 BaseBladeXY = uint2(DispatchThreadIndex % XYNumInstances.x, DispatchThreadIndex / XYNumInstances.x);

	[branch]
	if (any(BaseBladeXY >= ((XYNumInstances.xy + 1) >> Parameter.MaskExp)))
	{
		return;
	}

	uint2 BladeXY = BaseBladeXY.xy * Parameter.Mask;
	const uint BladeShuffleUintValue = TileShuffleBuffer[BladeXY.y * XYNumInstances.x + BladeXY.x];
	const float BladeShuffleValue = asfloat(BladeShuffleUintValue);

	const float DensityThreshold = (1.0f - Parameter.DensityScale) * 0.75f + 0.25f;

	[branch]
	if (BladeShuffleValue > DensityThreshold)
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

	SampledLayerIndexMapData LayerData = GetLayerTypeIndex(Parameter, CurrentInstanceOriginInTile.xy);

	[branch]
	if ((BladeShuffleValue >= LayerData.Weight) || (LayerData.Index >= Parameter.IndexLayerNumMaxTypes))
	{
		return;
	}

	float WorldHeight = GetLandscapeWorldHeight(Parameter, CurrentInstanceOriginInTile.xy);

	BezierGrassUnpackedInstanceData UnpackedInstanceData = (BezierGrassUnpackedInstanceData)0;
	BezierGrassLayerTypeBaseData LayerTypeBaseData;
	{
		const uint ActualBladeIndex = ((uint)ActualInstanceGrid.y) * Parameter.NumTiles.x * Parameter.NumSubTiles.x * XYNumInstances.x + ((uint)ActualInstanceGrid.x);
		UnpackedInstanceData.PerBladeHash = ActualBladeIndex ^ Parameter.RandomSeedParams.x ^ Parameter.RandomSeed ^ (BladeShuffleUintValue & 0xffffffu);
		UnpackedInstanceData.LayerTypeIndex = LayerData.Index & 0xffu;
		LayerTypeBaseData = GetBezierGrassLayerTypeBaseData(UnpackedInstanceData.LayerTypeIndex, Parameter.IndexLayerNumTotalElems);
	}

	// The view frustum cull of the legacy source, which sits here between the layer
	// table fetch and the LOD block, and which the port had to drop because the
	// grass constant buffer declared no ViewFrustumPlane for it to read. Those five
	// fields are declared now, so the block runs as it was written: the sphere
	// around the blade's root, radius Height.x + Height.z, is tested against each
	// half space, and the blade is dropped when it lies outside one of them. The
	// return is where the legacy put it, ahead of the LOD work and ahead of the
	// instance allocation, so a culled blade claims no slot.
	{
		const float3 BoundCenter = float3(TargetWorldOrigin.xy, WorldHeight);
		const float BoundLength = LayerTypeBaseData.Height.x + LayerTypeBaseData.Height.z;
		bool IsInside = true;

		[unroll]
		for (uint PlaneIndex = 0; PlaneIndex < 5; PlaneIndex++)
		{
			IsInside = IsInside && ((dot(BoundCenter.xyz, Parameter.ViewFrustumPlane[PlaneIndex].xyz) - Parameter.ViewFrustumPlane[PlaneIndex].w - BoundLength) <= 0);
		}

		[branch]
		if (!IsInside)
		{
			return;
		}
	}

	float ViewDistSq;
	float LOD = 0.0;
	uint DrawLODIndex = 0;
	{
		float3 TranslatedWorldPosition = TransformWorldToView(float3(TargetWorldOrigin.xy, WorldHeight), ViewMatrix).xyz;
		ViewDistSq = dot(TranslatedWorldPosition.xyz, TranslatedWorldPosition.xyz);

		[branch]
		if (ViewDistSq > Parameter.LODDistancesSq[Parameter.LODMinMax.y])
		{
			return;
		}

		[loop]
		for (uint Index = Parameter.LODMinMax.x; Index <= Parameter.LODMinMax.y; Index++)
		{
			DrawLODIndex = (ViewDistSq > Parameter.LODDistancesSq[Index]) ? Index : DrawLODIndex;
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

		BezierGrassParameter TempParameter = (BezierGrassParameter)0;
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
		BezierGrassTileInstanceData TileData = InitBezierGrassTileInstanceData(TempParameter, Grid, SubGrid);

#if BEZIER_GRASS_COMBINE_CONSTRUCT_ENABLE
		const uint FinalInstanceIndex = DrawLODIndex * Parameter.NumAllocatedInstances + InstanceIndex;
#else
		const uint FinalInstanceIndex = DrawLODIndex * XYNumInstances.x * XYNumInstances.y + InstanceIndex;
#endif
		BezierGrassPackedInstanceData PackedInstanceData = PackBezierGrassInstanceData(TempParameter, TileData, LayerTypeBaseData, UnpackedInstanceData);
		SetPackedInstanceData(FinalInstanceIndex, PackedInstanceData);
	}
}

#endif	//BEZIER_GRASS_CONSTRUCT_HLSL
