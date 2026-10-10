#ifndef BEZIER_GRASS_COMMON_HLSL
#define BEZIER_GRASS_COMMON_HLSL

// ---------------------------------------------------------------------------
// Shared bezier grass maths.
//
// The algorithm is the legacy BezierGrassCommon.hlsl, kept as it is: the blade
// curve, the instance packing, the layer type decode and the wind model are
// untouched, down to the literal constants. What the port changed, and it is
// only the parameter path:
//
//   old two copies, Common/ and Public/
//       new this one file. Common/BezierGrassCommon.hlsl is the live revision
//       and the source here; Public/BezierGrassCommon.hlsl is the same body with
//       its constants stripped and the whole file wrapped in "#if 0", and
//       nothing included it.
//   old #define BezierGrassLayerTypeBinding LayerTypeDatas, and the two siblings
//       new LayerTypeDatas[...], PackedInstanceDatas[...], DispatchDatas[...]
//       directly. The indirection existed so each consumer could point the code
//       at its own binding. A resource is now declared once in the component
//       JSON and its name is the binding, so there is nothing left to point.
//   old FBezierGrassXxx / FSampledLayerIndexMapData type names
//       new BezierGrassXxx / SampledLayerIndexMapData. No type in the library
//       carries the F prefix, and the port did not want this file to be the one
//       exception to that.
//   old Texture2DArraySample / Texture2DArraySampleLevel, defined by the render
//       material and relied on here
//       new PE_SAMPLE_TEX2DARRAY_LEVEL, from Public/System/ShaderBindless.hlsl,
//       which this file includes itself. The header no longer depends on a macro
//       the caller happens to have defined.
//   old BezierGrassBendMin / BezierGrassBendRange / BezierGrassMidPointTMin /
//       BezierGrassMidPointTRange / DEFINE_BEZIER_GRASS_WIND_LEVEL_MAX
//       new the same values under ALL_CAPS_WITH_UNDERSCORES names, which is what
//       the style asks of a macro.
//   old #ifndef _BEZIER_GRASS_COMMON_HLSL
//       new BEZIER_GRASS_COMMON_HLSL, without the leading underscore. The guard
//       is also why an injected file must not carry #pragma once: it is spliced
//       into the middle of a translation unit.
//
// One file serves two consumers, so the write side keeps the guard the legacy
// used. The construct compute pass compiles the readers and the writers against
// the RWStructuredBuffer it declares; the vertex factory defines
// BEZIER_GRASS_RENDER_ONLY and gets only the readers, which is what lets it bind
// the same data as a read only SRV.
//
// The resource names indexed below are declared by the component JSON of the
// pass that includes this file, in the new compile only layout of
// Compute/BezierGrass/BezierGrassConstruct.json. Nothing here declares a
// binding.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"

// The layout constants are spelled as macros rather than as literals because the
// pack and unpack sides have to agree across three files. Each one is guarded so
// a -D from the component JSON stays authoritative.
#ifndef BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM
#define BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM				16
#endif	//BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM

#ifndef BEZIER_GRASS_BLADE_LODS_MAX_NUM
#define BEZIER_GRASS_BLADE_LODS_MAX_NUM						8
#endif	//BEZIER_GRASS_BLADE_LODS_MAX_NUM

#ifndef BEZIER_GRASS_LAYER_TYPE_BASE_NUM_MEMBERS
#define BEZIER_GRASS_LAYER_TYPE_BASE_NUM_MEMBERS			19
#endif	//BEZIER_GRASS_LAYER_TYPE_BASE_NUM_MEMBERS

#ifndef BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS
#define BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS	0
#endif	//BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS

#ifndef BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS
#define BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS		6
#endif	//BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS

#ifndef BEZIER_GRASS_PACKED_INSTANCE_TIME_RELEVANCE_NUM_MEMBERS
#define BEZIER_GRASS_PACKED_INSTANCE_TIME_RELEVANCE_NUM_MEMBERS	2
#endif	//BEZIER_GRASS_PACKED_INSTANCE_TIME_RELEVANCE_NUM_MEMBERS

#ifndef BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS
#define BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS			((BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS) + ((BEZIER_GRASS_PACKED_INSTANCE_TIME_RELEVANCE_NUM_MEMBERS) * 2))
#endif	//BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS

// One entry of the layer type table, unpacked. Facing, Height, Width, Tilt, Bend
// and MidPointT are (base, random range, random range end) triples rather than
// three separate values, which is why they are float3.
struct BezierGrassLayerTypeBaseData
{
	float3	Facing;
	float3	Height;
	float3	Width;
	float3	Tilt;
	float3	Bend;
	float3	MidPointT;
	bool	bBent;
	bool	bUseFacing;
	float	SideCurve[BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM];
};

struct BezierGrassTimeRelevanceUnpackedData
{
	float2	Facing;
	float	WindStrength;
	float	TiltSin;
	float	TiltCos;
	float	Bend;
	float	MidPointT;
};

struct BezierGrassTimeRelevancePackedData
{
	uint	PackedFacingWindStrength;
	uint	PackedTiltSinCosBendMidPointT;
};

struct BezierGrassUnpackedInstanceData
{
	float	LOD;
	uint	LayerTypeIndex;
	float2	OriginXY;
	float	OriginHeight;
	uint	PerBladeHash;
	bool	bBent;
	float	Height;
	float	Width;
	BezierGrassTimeRelevanceUnpackedData	CurrentTimeRelevanceData;
	BezierGrassTimeRelevanceUnpackedData	PreviousTimeRelevanceData;
};

struct BezierGrassPackedInstanceData
{
	uint	GridIndex;
	uint	PackedLODTypeIndexIsBent;
	uint	PackedOriginXY;
	float	OriginHeight;
	uint	PerBladeHash;
	uint	PackedHeightWidth;
	BezierGrassTimeRelevancePackedData	CurrentTimeRelevanceData;
	BezierGrassTimeRelevancePackedData	PreviousTimeRelevanceData;
};

struct BezierGrassVertexData
{
	float3	Position;
	float3	Normal;
	float3	GeomNormal;
	float3	GeomTangent;
	float3	RootPosition;
	float3	ControlPosition;
	float3	TipPosition;
	float	BezierT;
	float	CurveUV;
};

// The parameter bundle of the render side. The construct pass has its own, below.
struct BezierGrassParameter
{
	uint	RandomSeed;
	uint	NumAllocatedInstances;
	uint	CurrentLODIndex;
	float2	LODMinMax;
	uint2	NumTiles;
	uint2	NumSubTiles;
	uint2	NumInstances;
	uint	LayerTypeNumTotalElems;
	uint	LayerTypeNumBaseElems;
	uint	LayerTypeNumCustomElems;
	float2	TileAnchor;
	float2	TileSize;
	float2	SubTileSize;
	float2	BorderSize;
	float2	WindStrengthRange;	// x is min, y is [max - min]
	float	BentBezierT;
};

struct BezierGrassDispatchData
{
	uint2	XYNumInstances;
	uint	InstanceOffset[BEZIER_GRASS_BLADE_LODS_MAX_NUM];
};

struct BezierGrassTileInstanceData
{
	uint2	Grid;
	uint2	SubGrid;
	float2	AnchorWithBorder;
	float2	SizeWithBorder;
};

struct BezierGrassUnpackedVertexInputData
{
	float	TargetT;
	float	PrevT;
	float	SideOffset;
	bool	bBentLeftPart;
};

// The parameter bundle of the construct pass. Filled from the constant buffer by
// InitBezierGrassConstructParameter, which lives next to the entry point.
struct BezierGrassConstructParameter
{
	float4	CurrentTimeParams;
	float4	PreviousTimeParams;
	uint4	RandomSeedParams;
	uint	RandomSeed;
	uint	NumAllocatedInstances;
	uint	BodyPart[BEZIER_GRASS_BLADE_LODS_MAX_NUM];
	uint	IndexOffset[BEZIER_GRASS_BLADE_LODS_MAX_NUM];
	uint	VertexOffset[BEZIER_GRASS_BLADE_LODS_MAX_NUM];
	uint2	LODMinMax;
	float2	TileAnchor;
	float2	TileSize;
	uint2	TileXY;
	uint2	NumTiles;
	float2	SubTileSize;
	float2	BorderSize;
	uint2	SubTileXY;
	uint2	NumSubTiles;
	uint	MaskExp;
	uint	Mask;
	uint	XNumInstances;
	uint	YNumInstances;
	uint	NumInstances;
	float	DensityScale;
	float2	HeightMapWorldScaleOffset;
	uint	HeightMapPixelBorder;
	uint	HeightMapPixelSize;
	uint	IndexLayerPixelBorder;
	uint	IndexLayerPixelSize;
	uint	DensityPixelBorder;
	uint	DensityPixelSize;
	uint	IndexLayerNumMaxTypes;
	uint	IndexLayerNumTotalElems;
	uint	IndexLayerNumBaseElems;
	uint	IndexLayerNumCustomElems;
	float4	WindDirectionStrength;
	float2	WindStrengthRange;
	float	LODDistancesSq[BEZIER_GRASS_BLADE_LODS_MAX_NUM];
	// The five view frustum half spaces the construct pass culls against. The
	// constant buffer declares them as ViewFrustumPlane0 through ViewFrustumPlane4
	// because a component JSON field is one scalar or vector and cannot be an
	// array; they are gathered here exactly as LODDistancesSq is.
	float4	ViewFrustumPlane[5];
};

struct SampledLayerIndexMapData
{
	uint	Index;
	float	Weight;
};

// Decodes one row of the layer type table. The table is a flat float array with
// a fixed stride, hence the multiplication by InLayerNumTotalElems. The row
// layout is the one BezierGrassConstructParameter.IndexLayerNumBaseElems and
// its siblings describe.
BezierGrassLayerTypeBaseData GetBezierGrassLayerTypeBaseData(uint InLayerIndex, uint InLayerNumTotalElems)
{
	BezierGrassLayerTypeBaseData Result = (BezierGrassLayerTypeBaseData)0;

	Result.Facing = float3(
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 0],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 1],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 2]);
	Result.Height = float3(
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 3],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 4],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 5]);
	Result.Width = float3(
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 6],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 7],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 8]);
	Result.Tilt = float3(
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 9],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 10],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 11]);
	Result.Bend = float3(
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 12],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 13],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 14]);
	Result.MidPointT = float3(
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 15],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 16],
		LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 17]);

	const uint PackedFlags = asuint(LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + 18]);
	Result.bBent = (PackedFlags & 0x1u) != 0x0u;
	Result.bUseFacing = (PackedFlags & (1u << 1u)) != 0x0u;

	{
		[unroll]
		for (int Index = 0; Index < BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM; Index++)
		{
			Result.SideCurve[Index] = LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + BEZIER_GRASS_LAYER_TYPE_BASE_NUM_MEMBERS + Index];
		}
	}

	return Result;
}

// The custom data tail of a layer type row, past the base members and the side
// curve. Declared for the render side; the construct pass reads the same table
// through GetBezierGrassLayerTypeBaseData.
float GetBezierGrassLayerTypeCustomData(uint InLayerIndex, uint InCustomElementOffset, uint InLayerNumTotalElems)
{
	return (LayerTypeDatas[InLayerNumTotalElems * InLayerIndex + BEZIER_GRASS_LAYER_TYPE_BASE_NUM_MEMBERS + BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM + InCustomElementOffset]);
}

// Both of the custom texture readers below are dead code while
// BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS is 0, which is the value the
// component JSON passes: the four array textures and their samplers are a
// feature that no shipped grass layer uses, and the legacy had the same #if
// ladder around the same calls. The identifiers are the names those resources
// would carry in the component JSON that declares them.
float4 SampleLevelLayerCustomTexture(uint InLayerIndex, float2 InUV, float InMipLevel, float InMipBias, uint InArrayIndex)
{
	const float SliceIndex = (float)(max(InLayerIndex, 0));
	float LODValue = InMipLevel + InMipBias;

#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 0
	if (InArrayIndex == 0)
	{
		return (PE_SAMPLE_TEX2DARRAY_LEVEL(BezierGrassLayerTypeCustomTexArrayBinding0, BezierGrassLayerTypeCustomSamplerBinding0, float3(InUV.xy, SliceIndex), LODValue));
	}
#endif	//BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 0
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 1
	if (InArrayIndex == 1)
	{
		return (PE_SAMPLE_TEX2DARRAY_LEVEL(BezierGrassLayerTypeCustomTexArrayBinding1, BezierGrassLayerTypeCustomSamplerBinding1, float3(InUV.xy, SliceIndex), LODValue));
	}
#endif	//BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 1
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 2
	if (InArrayIndex == 2)
	{
		return (PE_SAMPLE_TEX2DARRAY_LEVEL(BezierGrassLayerTypeCustomTexArrayBinding2, BezierGrassLayerTypeCustomSamplerBinding2, float3(InUV.xy, SliceIndex), LODValue));
	}
#endif	//BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 2
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 3
	if (InArrayIndex == 3)
	{
		return (PE_SAMPLE_TEX2DARRAY_LEVEL(BezierGrassLayerTypeCustomTexArrayBinding3, BezierGrassLayerTypeCustomSamplerBinding3, float3(InUV.xy, SliceIndex), LODValue));
	}
#endif	//BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 3

	return 0;
}

float4 SampleLayerCustomTexture(uint InLayerIndex, float2 InUV, uint InArrayIndex)
{
	const float SliceIndex = (float)(max(InLayerIndex, 0));

#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 0
	if (InArrayIndex == 0)
	{
		return (PE_SAMPLE_TEX2DARRAY(BezierGrassLayerTypeCustomTexArrayBinding0, BezierGrassLayerTypeCustomSamplerBinding0, float3(InUV.xy, SliceIndex)));
	}
#endif	//BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 0
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 1
	if (InArrayIndex == 1)
	{
		return (PE_SAMPLE_TEX2DARRAY(BezierGrassLayerTypeCustomTexArrayBinding1, BezierGrassLayerTypeCustomSamplerBinding1, float3(InUV.xy, SliceIndex)));
	}
#endif	//BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 1
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 2
	if (InArrayIndex == 2)
	{
		return (PE_SAMPLE_TEX2DARRAY(BezierGrassLayerTypeCustomTexArrayBinding2, BezierGrassLayerTypeCustomSamplerBinding2, float3(InUV.xy, SliceIndex)));
	}
#endif	//BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 2
#if BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 3
	if (InArrayIndex == 3)
	{
		return (PE_SAMPLE_TEX2DARRAY(BezierGrassLayerTypeCustomTexArrayBinding3, BezierGrassLayerTypeCustomSamplerBinding3, float3(InUV.xy, SliceIndex)));
	}
#endif	//BEZIER_GRASS_LAYER_TYPE_CUSTOM_TEXTURE_NUM_ARRAYS > 3

	return 0;
}

void GetPackedInstanceTimeRelevanceData(in uint InInstanceIndex, inout BezierGrassPackedInstanceData InOutPackedInstanceData)
{
	InOutPackedInstanceData.CurrentTimeRelevanceData.PackedFacingWindStrength = PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 0];
	InOutPackedInstanceData.CurrentTimeRelevanceData.PackedTiltSinCosBendMidPointT = PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 1];

	InOutPackedInstanceData.PreviousTimeRelevanceData.PackedFacingWindStrength = PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 2];
	InOutPackedInstanceData.PreviousTimeRelevanceData.PackedTiltSinCosBendMidPointT = PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 3];
}

// The full reader, and the render side is its caller: the vertex factory indexes
// it with SV_InstanceID. It sits outside the guard below on purpose. The guard is
// about the buffer's role, not about a direction of travel in the abstract: the
// construct pass owns the buffer as a UAV and is the only writer, while the render
// side binds the same buffer as a read only SRV, and a read is legal against both.
// Only the writers need the UAV.
BezierGrassPackedInstanceData GetPackedInstanceData(in uint InInstanceIndex)
{
	BezierGrassPackedInstanceData Result = (BezierGrassPackedInstanceData)0;

	Result.GridIndex = PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 0];
	Result.PackedLODTypeIndexIsBent = PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 1];
	Result.PackedOriginXY = PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 2];
	Result.OriginHeight = asfloat(PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 3]);
	Result.PerBladeHash = PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 4];
	Result.PackedHeightWidth = PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 5];

	GetPackedInstanceTimeRelevanceData(InInstanceIndex, Result);

	return Result;
}

#ifndef BEZIER_GRASS_RENDER_ONLY
// The two writers below need the instance buffer as a UAV, and only a UAV. The
// render side defines BEZIER_GRASS_RENDER_ONLY, binds the same buffer as an SRV
// and never reaches this block; the reader above stays available to it.
void SetPackedInstanceTimeRelevanceData(in uint InInstanceIndex, in BezierGrassPackedInstanceData InPackedInstanceData)
{
	PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 0] = InPackedInstanceData.CurrentTimeRelevanceData.PackedFacingWindStrength;
	PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 1] = InPackedInstanceData.CurrentTimeRelevanceData.PackedTiltSinCosBendMidPointT;

	PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 2] = InPackedInstanceData.PreviousTimeRelevanceData.PackedFacingWindStrength;
	PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + BEZIER_GRASS_PACKED_INSTANCE_BASE_NUM_MEMBERS + 3] = InPackedInstanceData.PreviousTimeRelevanceData.PackedTiltSinCosBendMidPointT;
}

void SetPackedInstanceData(in uint InInstanceIndex, in BezierGrassPackedInstanceData InPackedInstanceData)
{
	PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 0] = InPackedInstanceData.GridIndex;
	PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 1] = InPackedInstanceData.PackedLODTypeIndexIsBent;
	PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 2] = InPackedInstanceData.PackedOriginXY;
	PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 3] = asuint(InPackedInstanceData.OriginHeight);
	PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 4] = InPackedInstanceData.PerBladeHash;
	PackedInstanceDatas[BEZIER_GRASS_PACKED_INSTANCE_NUM_MEMBERS * InInstanceIndex + 5] = InPackedInstanceData.PackedHeightWidth;

	SetPackedInstanceTimeRelevanceData(InInstanceIndex, InPackedInstanceData);
}
#endif	//BEZIER_GRASS_RENDER_ONLY

BezierGrassDispatchData GetBezierGrassDispatchData()
{
	BezierGrassDispatchData Result = (BezierGrassDispatchData)0;

	Result.XYNumInstances = uint2(DispatchDatas[0], DispatchDatas[1]);

	{
		[unroll]
		for (int Index = 0; Index < BEZIER_GRASS_BLADE_LODS_MAX_NUM; Index++)
		{
			Result.InstanceOffset[Index] = DispatchDatas[2 + Index];
		}
	}

	return Result;
}

BezierGrassTileInstanceData InitBezierGrassTileInstanceData(in BezierGrassParameter InParams, in uint2 InGrid, in uint2 InSubGrid)
{
	BezierGrassTileInstanceData Result = (BezierGrassTileInstanceData)0;

	Result.Grid = InGrid;
	Result.SubGrid = InSubGrid;
	Result.AnchorWithBorder = ((float2)(InGrid.xy)) * InParams.TileSize.xy + InParams.TileAnchor.xy + (((float2)(InSubGrid.xy)) * InParams.SubTileSize.xy) - InParams.BorderSize.xy;
	Result.SizeWithBorder = 2.0f * InParams.BorderSize.xy + InParams.SubTileSize.xy;

	return Result;
}

// Bit budgets of the packed time relevance word, and the ranges the two 8 bit
// members are normalized against.
#define BEZIER_GRASS_BEND_MIN			(-0.5)
#define BEZIER_GRASS_BEND_RANGE			(2.0)
#define BEZIER_GRASS_MID_POINT_T_MIN	(0.2)
#define BEZIER_GRASS_MID_POINT_T_RANGE	(0.6)

BezierGrassTimeRelevancePackedData PackBezierGrassInstanceTimeRelevanceData(in BezierGrassParameter InParams, in BezierGrassTimeRelevanceUnpackedData InData)
{
	BezierGrassTimeRelevancePackedData Result = (BezierGrassTimeRelevancePackedData)0;

	const uint2 PackedFacing = (uint2)(InData.Facing.xy * 511.0f + 511.0f);	// 1023 == 0x3ffu == 10-Bit		511 == 0x1ffu == 9-Bit
	const uint PackedWindStrength = (uint)(saturate((InData.WindStrength - InParams.WindStrengthRange.x) / InParams.WindStrengthRange.y) * 4095.0f);	// 4095 == 0xfffu == 12-Bit
	Result.PackedFacingWindStrength = (PackedFacing.x << 22u) | (PackedFacing.y << 12u) | PackedWindStrength;

	const uint PackedTiltSinCos = (((uint)(InData.TiltSin * 255.0f)) << 8u) | ((uint)(InData.TiltCos * 255.0f));	// 255 == 0xffu == 8-Bit
	const uint PackedBend = (uint)(saturate((InData.Bend - BEZIER_GRASS_BEND_MIN) / BEZIER_GRASS_BEND_RANGE) * 255.0f);	// 255 == 0xffu == 8-Bit
	const uint PackedMidPointT = (uint)(saturate((InData.MidPointT - BEZIER_GRASS_MID_POINT_T_MIN) / BEZIER_GRASS_MID_POINT_T_RANGE) * 255.0f);	// 255 == 0xffu == 8-Bit
	Result.PackedTiltSinCosBendMidPointT = (PackedTiltSinCos << 16u) | (PackedBend << 8u) | PackedMidPointT;

	return Result;
}

BezierGrassTimeRelevanceUnpackedData UnpackBezierGrassInstanceTimeRelevanceData(in BezierGrassParameter InParams, in BezierGrassTimeRelevancePackedData InData)
{
	BezierGrassTimeRelevanceUnpackedData Result = (BezierGrassTimeRelevanceUnpackedData)0;

	Result.Facing = (float2(InData.PackedFacingWindStrength >> 22u, (InData.PackedFacingWindStrength >> 12u) & 0x3ffu) - 511.0f) / 511.0f;
	Result.WindStrength = (((float)(InData.PackedFacingWindStrength & 0xfffu)) / 4095.0f) * InParams.WindStrengthRange.y + InParams.WindStrengthRange.x;
	Result.TiltSin = ((float)(InData.PackedTiltSinCosBendMidPointT >> 24u)) / 255.0f;
	Result.TiltCos = ((float)((InData.PackedTiltSinCosBendMidPointT >> 16u) & 0xffu)) / 255.0f;
	Result.Bend = ((float)((InData.PackedTiltSinCosBendMidPointT >> 8u) & 0xffu)) / 255.0f * BEZIER_GRASS_BEND_RANGE + BEZIER_GRASS_BEND_MIN;
	Result.MidPointT = ((float)(InData.PackedTiltSinCosBendMidPointT & 0xffu)) / 255.0f * BEZIER_GRASS_MID_POINT_T_RANGE + BEZIER_GRASS_MID_POINT_T_MIN;

	return Result;
}

BezierGrassPackedInstanceData PackBezierGrassInstanceData(
	in BezierGrassParameter InParams,
	in BezierGrassTileInstanceData InTileData,
	in BezierGrassLayerTypeBaseData InLayerTypeBaseData,
	in BezierGrassUnpackedInstanceData InData)
{
	BezierGrassPackedInstanceData Result = (BezierGrassPackedInstanceData)0;

	const uint TileGridIndex = ((InTileData.Grid.y * InParams.NumTiles.x) + InTileData.Grid.x) & 0xfffu;	// 0xfffu == 12-Bit
	const uint SubTileGridIndex = ((InTileData.SubGrid.y * InParams.NumSubTiles.x) + InTileData.SubGrid.x) & 0xfffffu;	// 0xfffffu == 20-Bit
	Result.GridIndex = (TileGridIndex << 20u) | SubTileGridIndex;

	const uint LODNormalized = (uint)((InData.LOD - InParams.LODMinMax.x) / max(InParams.LODMinMax.y - InParams.LODMinMax.x, 1.0f) * ((float)0x7fffffu));	// 0x7fffffu == 23-Bit
	const uint LayerTypeIndex = InData.LayerTypeIndex & 0xffu;	// 0xffu == 8-Bit
	const uint bBent = InData.bBent ? 0x1u : 0x0u;
	Result.PackedLODTypeIndexIsBent = (LODNormalized << 9u) | (LayerTypeIndex << 1u) | bBent;

	const uint2 OriginXYNormalized = (uint2)(saturate((InData.OriginXY.xy - InTileData.AnchorWithBorder.xy) / InTileData.SizeWithBorder.xy) * 65535.0f);	// 65535 == 0xffffu == 16-Bit
	Result.PackedOriginXY = (OriginXYNormalized.x << 16u) | (OriginXYNormalized.y);
	Result.OriginHeight = InData.OriginHeight;

	Result.PerBladeHash = InData.PerBladeHash;

	const uint PackedHeight = (uint)(saturate((InData.Height - (InLayerTypeBaseData.Height.x + InLayerTypeBaseData.Height.y)) / max(InLayerTypeBaseData.Height.z - InLayerTypeBaseData.Height.y, 0.01f)) * 65535.0f);	// 65535 == 0xffffu == 16-Bit
	const uint PackedWidth = (uint)(saturate((InData.Width - (InLayerTypeBaseData.Width.x + InLayerTypeBaseData.Width.y)) / max(InLayerTypeBaseData.Width.z - InLayerTypeBaseData.Width.y, 0.01f)) * 65535.0f);	// 65535 == 0xffffu == 16-Bit
	Result.PackedHeightWidth = (PackedHeight << 16u) | PackedWidth;

	// Time relevance datas
	Result.CurrentTimeRelevanceData = PackBezierGrassInstanceTimeRelevanceData(InParams, InData.CurrentTimeRelevanceData);
	Result.PreviousTimeRelevanceData = PackBezierGrassInstanceTimeRelevanceData(InParams, InData.PreviousTimeRelevanceData);

	return Result;
}

void UnpackBezierGrassInstanceDataIndexInfo(
	in BezierGrassParameter InParams,
	in BezierGrassPackedInstanceData InData,
	inout BezierGrassTileInstanceData OutTileData,
	inout BezierGrassUnpackedInstanceData OutUnpackedInstanceData)
{
	const uint GridIndex = InData.GridIndex >> 20u;
	const uint2 Grid = uint2(GridIndex % InParams.NumTiles.x, GridIndex / InParams.NumTiles.x);
	const uint SubGridIndex = InData.GridIndex & 0xfffffu;
	const uint2 SubGrid = uint2(SubGridIndex % InParams.NumSubTiles.x, SubGridIndex / InParams.NumSubTiles.x);
	OutTileData = InitBezierGrassTileInstanceData(InParams, Grid, SubGrid);

	OutUnpackedInstanceData.LOD = saturate((InData.PackedLODTypeIndexIsBent >> 9u) / ((float)0x7fffffu)) * (InParams.LODMinMax.y - InParams.LODMinMax.x) + InParams.LODMinMax.x;
	OutUnpackedInstanceData.LayerTypeIndex = (InData.PackedLODTypeIndexIsBent >> 1u) & 0xffu;
	OutUnpackedInstanceData.bBent = (InData.PackedLODTypeIndexIsBent & 0x1u) != 0x0u;
}

void UnpackBezierGrassInstanceDataPropertyInfo(
	in BezierGrassParameter InParams,
	in BezierGrassPackedInstanceData InData,
	in BezierGrassTileInstanceData InTileData,
	in BezierGrassLayerTypeBaseData InLayerTypeBaseData,
	inout BezierGrassUnpackedInstanceData OutUnpackedInstanceData)
{
	const float2 UnpackedNormalizedOriginXY = float2(InData.PackedOriginXY >> 16u, InData.PackedOriginXY & 0xffffu) / 65535.0f;
	OutUnpackedInstanceData.OriginXY = UnpackedNormalizedOriginXY * InTileData.SizeWithBorder.xy + InTileData.AnchorWithBorder.xy;
	OutUnpackedInstanceData.OriginHeight = InData.OriginHeight;

	OutUnpackedInstanceData.PerBladeHash = InData.PerBladeHash;

	OutUnpackedInstanceData.Height = ((float)(InData.PackedHeightWidth >> 16u)) / 65535.0f * (InLayerTypeBaseData.Height.z - InLayerTypeBaseData.Height.y) + (InLayerTypeBaseData.Height.x + InLayerTypeBaseData.Height.y);
	OutUnpackedInstanceData.Width = ((float)(InData.PackedHeightWidth & 0xffffu)) / 65535.0f * (InLayerTypeBaseData.Width.z - InLayerTypeBaseData.Width.y) + (InLayerTypeBaseData.Width.x + InLayerTypeBaseData.Width.y);

	OutUnpackedInstanceData.CurrentTimeRelevanceData = UnpackBezierGrassInstanceTimeRelevanceData(InParams, InData.CurrentTimeRelevanceData);
	OutUnpackedInstanceData.PreviousTimeRelevanceData = UnpackBezierGrassInstanceTimeRelevanceData(InParams, InData.PreviousTimeRelevanceData);
}

// Unpacks the per vertex word the grass index buffer carries. The bit widths are
// 2, 1, 14 and 15, in that order.
BezierGrassUnpackedVertexInputData UnpackedBezierGrassPosition(in uint InPosition)
{
	BezierGrassUnpackedVertexInputData Result = (BezierGrassUnpackedVertexInputData)0;

	Result.SideOffset = ((float)(InPosition & 0x3u)) - 1.0f;
	Result.bBentLeftPart = ((InPosition >> 2u) & 0x1u) == 0x1u;
	Result.PrevT = ((float)((InPosition >> 3u) & 0x3fffu)) / 16383.0f;
	Result.TargetT = ((float)(InPosition >> 17u)) / 32767.0f;

	return Result;
}

float SinFunction(float InVal)
{
	return (sin(InVal));
}

void SinCosFunction(in float InVal, inout float OutSinVal, inout float OutCosVal)
{
	sincos(InVal, OutSinVal, OutCosVal);
}

// The grass hashes are PCG16, and both the wind model and the per blade noise
// depend on them being exactly this and not the PCG in Public/Core, which has a
// different constant and a different number of rounds.
uint3 Rand3DPCG16(uint3 InValue)
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

float2 NoiseTile2DWrap(float2 InVec2D, bool InIsTiling, float InRepeatSize)
{
	return (InIsTiling ? (frac(InVec2D / InRepeatSize) * InRepeatSize) : InVec2D);
}

float4 Voronoi2DCompare(float4 InMinVal, float3 InCandidate, float2 InOffset, bool InIsDistanceOnly)
{
	if (InIsDistanceOnly)
	{
		return float4(0, 0, 0, min(InMinVal.w, dot(InOffset, InOffset)));
	}
	else
	{
		float NewDist = dot(InOffset, InOffset);
		return ((NewDist > InMinVal.w) ? InMinVal : float4(InCandidate, NewDist));
	}
}

float3 Voronoi2DCornerSample(uint InSeed, float2 InPos)
{
	const uint3 UintPosition = uint3(int2(InPos.xy), InSeed);
	float3 RandValue = float3(Rand3DPCG16(UintPosition)) / 0xffff;

	// searches a 3x3 neighborhood with points distributed on a sphere
	// scale factor to guarantee jittered points will be found within a 3x3 search
	return float3(normalize(RandValue.xy - 0.5) * 0.25, RandValue.z);
}

float4 VoronoiNoise2DAlu(uint InSeed, float2 InVec2D, bool InIsTiling, float InRepeatSize, bool InIsDistanceOnly)
{
	float2 FractionPart = frac(InVec2D.xy + 0.5);
	float2 IntegerPart = floor(InVec2D.xy + 0.5);

	// with initial minimum distance = infinity (or at least bigger than 4), first min is optimized away
	float4 MinDist = float4(0, 0, 0, 100);
	float3 NewOffset;
	int2 IntegerOffset;

	// do a 3x3 search
	{
		[unroll]
		for (IntegerOffset.x = -1; IntegerOffset.x <= 1; IntegerOffset.x++)
		{
			[unroll]
			for (IntegerOffset.y = -1; IntegerOffset.y <= 1; IntegerOffset.y++)
			{
				NewOffset = Voronoi2DCornerSample(InSeed, NoiseTile2DWrap(IntegerPart + ((float2)IntegerOffset), InIsTiling, InRepeatSize));
				NewOffset.x = ((float)(IntegerOffset.x)) + NewOffset.x;
				NewOffset.y = ((float)(IntegerOffset.y)) + NewOffset.y;
				MinDist = Voronoi2DCompare(MinDist, float3(IntegerPart + NewOffset.xy, NewOffset.z), FractionPart - NewOffset.xy, InIsDistanceOnly);
			}
		}
	}

	return float4(MinDist.xyz, sqrt(MinDist.w));
}

float2 QuadraticBezierCurve(in float2 InP0, in float2 InP1, in float2 InP2, in float InT)
{
	float TSq = InT * InT;
	float OneMinusT = 1.0f - InT;
	float OneMinusTSq = OneMinusT * OneMinusT;
	return (InP0 * OneMinusTSq + InP1 * (2.0f * OneMinusT * InT) + InP2 * TSq);
}

float2 DerivativeQuadraticBezierCurve(in float2 InP0, in float2 InP1, in float2 InP2, in float InT)
{
	return ((InP1 - InP0) * (2.0f * (1.0f - InT)) + (InP2 - InP1) * (2.0f * InT));
}

float2 CubicBezierCurve(in float2 InP0, in float2 InP1, in float2 InP2, in float2 InP3, in float InT)
{
	float TSq = InT * InT;
	float TCu = TSq * InT;
	float OneMinusT = 1.0f - InT;
	float OneMinusTSq = OneMinusT * OneMinusT;
	float OneMinusTCu = OneMinusTSq * OneMinusT;
	return (InP0 * OneMinusTCu + InP1 * (3.0f * OneMinusTSq * InT) + InP2 * (3.0f * OneMinusT * TSq) + InP3 * TCu);
}

float2 DerivativeCubicBezierCurve(in float2 InP0, in float2 InP1, in float2 InP2, in float2 InP3, in float InT)
{
	float TSq = InT * InT;
	float OneMinusT = 1.0f - InT;
	float OneMinusTSq = OneMinusT * OneMinusT;
	return ((InP1 - InP0) * (3.0f * OneMinusTSq) + (InP2 - InP1) * (6.0f * OneMinusT * InT) + (InP3 - InP2) * (3.0f * TSq));
}

// The render side of the blade: turns one unpacked instance into the four points
// the vertex factory needs. The construct pass never calls it, and it is kept
// here because both sides need the same curve. The view vector comes from the
// caller rather than from a matrix this file would have to reach for, which is
// the same shape the legacy Common/ revision had.
BezierGrassVertexData BezierGrassBladeLocalPosition(
	in BezierGrassLayerTypeBaseData InLayerBaseData,
	in BezierGrassParameter InParams,
	in BezierGrassTileInstanceData InTileData,
	in BezierGrassUnpackedInstanceData InInstanceData,
	in BezierGrassTimeRelevanceUnpackedData InInstanceTimeRelevanceData,
	in BezierGrassUnpackedVertexInputData InVertexData,
	in float3 InPreViewTranslation)
{
	BezierGrassVertexData Result = (BezierGrassVertexData)0;

	const bool bBent = InInstanceData.bBent;
	const bool bBentLeftPart = InVertexData.bBentLeftPart;
	const float2 BentRotate = bBent ? (bBentLeftPart ? float2(0.5, 0.8660254) : float2(-0.5, 0.8660254)) : float2(0, 1);	// bBentLeftPart ? float2(Sin(30), Cos(30)) : float2(Sin(-30), Cos(-30))

	const float UnmappedBentBezierT = lerp(abs(InVertexData.TargetT - InParams.BentBezierT), abs(InVertexData.PrevT - InParams.BentBezierT), frac(InInstanceData.LOD));
	const float BezierT = bBent ? (bBentLeftPart ? (UnmappedBentBezierT / (1 - InParams.BentBezierT)) : (UnmappedBentBezierT / InParams.BentBezierT)) : lerp(InVertexData.TargetT, InVertexData.PrevT, frac(InInstanceData.LOD));
	const float SideOffset = InVertexData.SideOffset;

	const float Height = InInstanceData.Height;
	const float2 EndPoint = float2(InInstanceTimeRelevanceData.TiltSin * Height, InInstanceTimeRelevanceData.TiltCos * Height);

	const float MidPointT = InInstanceTimeRelevanceData.MidPointT;
	const float Bend = InInstanceTimeRelevanceData.Bend;
	const float2 ContolPoint = lerp(0.0f, EndPoint.xy, MidPointT) + float2(-EndPoint.y, EndPoint.x) * Bend;

	const float2 PositionOS = QuadraticBezierCurve(0.0f, ContolPoint, EndPoint, BezierT);
	const float2 NormalOS = DerivativeQuadraticBezierCurve(0.0f, ContolPoint, EndPoint, BezierT);

	const float3 BaseFacing3D = float3(InInstanceTimeRelevanceData.Facing.x, InInstanceTimeRelevanceData.Facing.y, 0.0f);
	const float3 BaseNmlOrthWS = float3(InInstanceTimeRelevanceData.Facing.y, -InInstanceTimeRelevanceData.Facing.x, 0.0f);
	const float3 Facing3D = BaseNmlOrthWS * BentRotate.x + BaseFacing3D * BentRotate.y;
	const float3 NmlOrthWS = float3(Facing3D.y, -Facing3D.x, 0.0f);
	const float3 UpVector3D = float3(0.0f, 0.0f, 1.0f);
	const float3 NmlBezierWS = normalize((Facing3D.xyz * NormalOS.x) + (UpVector3D.xyz * NormalOS.y));	// Need SafeNormalize

	const float FlipNormalMultipler = bBent ? (bBentLeftPart ? -1 : 1) : (-1);	// WHY CCW ?
	const float3 BezierFaceNormal = normalize(cross(NmlOrthWS.xyz, NmlBezierWS.xyz));	// Need SafeNormalize

	Result.GeomNormal = BezierFaceNormal * FlipNormalMultipler;
	Result.GeomTangent = -NmlOrthWS.xyz;

	const float CurveIndex = BezierT * ((float)(BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM - 1));
	const uint CurveStartIndex = (uint)(floor(CurveIndex));
	const uint CurveEndIndex = min(CurveStartIndex + 1, BEZIER_GRASS_BLADE_CURVE_POINTS_MAX_NUM - 1);
	const float SideCurveValue = lerp(InLayerBaseData.SideCurve[CurveStartIndex], InLayerBaseData.SideCurve[CurveEndIndex], frac(CurveIndex));

	Result.BezierT = BezierT;
	Result.CurveUV = (-SideOffset) * 0.5f + 0.5f;	// SideCurveValue * (-SideOffset)

	const float3 RootPosition = float3(InInstanceData.OriginXY.xy, InInstanceData.OriginHeight);
	const float3 ControlPosition = (Facing3D.xyz * ContolPoint.x) + (UpVector3D.xyz * ContolPoint.y) + RootPosition.xyz;
	const float3 TipPosition = (Facing3D.xyz * EndPoint.x) + (UpVector3D.xyz * EndPoint.y) + RootPosition.xyz;

	Result.RootPosition = RootPosition.xyz;
	Result.ControlPosition = ControlPosition.xyz;
	Result.TipPosition = TipPosition.xyz;

	Result.Position = (Facing3D.xyz * PositionOS.x) + (UpVector3D.xyz * PositionOS.y) + RootPosition.xyz;

	// The legacy Public/ revision built this vector through
	// TransformLocalToTranslatedWorld with a Matrix it never filled in; the live
	// Common/ revision had already replaced it with the plain subtraction below,
	// which is what is kept.
	float3 ViewVectorWorldSpace = RootPosition.xyz - InPreViewTranslation.xyz;
	float3 NormalizedViewVectorWorldSpace = normalize(ViewVectorWorldSpace.xyz);

	const float NdotV = dot(NormalizedViewVectorWorldSpace.xyz, NmlOrthWS.xyz);
	const bool bFlipNormal = sign(NdotV) < (-0.5);
	const float SignNdotV = bFlipNormal ? (-1) : 1;
	const float AbsNdotV = abs(NdotV);
	const float Width = InInstanceData.Width;

#if 1
	Result.Normal = Result.GeomNormal.xyz;
#else
	Result.Normal = lerp(lerp(-BezierFaceNormal.xyz, -NormalizedViewVectorWorldSpace.xyz, 0.05), BezierFaceNormal.xyz, (((bFlipNormal ? (-(SideOffset)) : SideOffset) + 1.0f) * 0.5f));
	Result.Normal = lerp(Result.GeomNormal.xyz, Result.Normal.xyz, AbsNdotV);
#endif
	Result.Position = lerp(NmlOrthWS.xyz, SignNdotV * BezierFaceNormal.xyz, AbsNdotV) * Width * SideCurveValue * SideOffset + Result.Position.xyz;

	return Result;
}

// Four wind levels, the tables below give one row per level plus a fifth row that
// acts as the upper end of the lerp.
#define BEZIER_GRASS_WIND_LEVEL_MAX					4

// Wind is a function of world position and time only: the voronoi value is
// sampled on a lattice around the blade root and the rest is oscillation, so two
// blades that share a root cell move together and the field looks continuous.
void ModifyBezierGrassBlade(
	in BezierGrassConstructParameter InParameter,
	in BezierGrassLayerTypeBaseData InLayerTypeBaseData,
	in float2 InOriginInWorldXY,
	in uint InPerBladeHash,
	in float4 InTimeParams,
	inout float InOutWindStrength,
	inout float InOutTilt,
	inout float InOutBend,
	inout float InOutMidPointT,
	inout float InOutWindFacing,
	inout float InOutFacingOffset)
{
	// Properties that need convert to parameters for changing easily
	//***************************
	// Properties Field Start
	//***************************
	const float2 WorldPosScale = 0.008;

	// Wind level mappings. This is example for 0~3 level. Each level contains 4 float params
	const float WindLevelStepValue[BEZIER_GRASS_WIND_LEVEL_MAX] =
	{
		2.0,	// Wind Level 0 if wind strength is in [0.0, 2.0)
		4.0,	// Wind Level 1 if wind strength is in [2.0, 4.0)
		6.0,	// Wind Level 2 if wind strength is in [4.0, 6.0)
		10.0	// Wind Level 3 if wind strength is in [6.0, 10.0)
				// ...
	};
	const float WindMovementScale[(BEZIER_GRASS_WIND_LEVEL_MAX + 1) * 1] =
	{
		0.312578 * 125.0,
		0.512347 * 125.0,
		4.793710 * 125.0,
		9.510091 * 125.0,
		13.56123 * 125.0
	};
	const float WindFrequencyScale[(BEZIER_GRASS_WIND_LEVEL_MAX + 1) * 1] =
	{
		0.27318,
		3.92699,
		7.85398,
		11.423973,
		15.707963
	};
	const float WindTiltLevel[(BEZIER_GRASS_WIND_LEVEL_MAX + 1) * 2] =
	{
		0.00, 0.05,	// Tilt Offset & Tilt Oscillation
		0.10, 0.08,	// Tilt Offset & Tilt Oscillation when wind is level 0
		0.15, 0.09,	// ...
		0.20, 0.10,	// ...
		0.25, 0.11	// ...
					// ...
	};
	const float WindBendLevel[(BEZIER_GRASS_WIND_LEVEL_MAX + 1) * 1] =
	{
		0.02,	// Bend Oscillation
		0.04,	// Bend Oscillation when wind is level 0
		0.06,	// ...
		0.08,	// ...
		0.10	// ...
				// ...
	};
	const float WindMidTLevel[(BEZIER_GRASS_WIND_LEVEL_MAX + 1) * 1] =
	{
		0.0,	// MidPointT Offset Base
		-0.05,	// MidPointT Offset when wind is level 0
		-0.1,	// ...
		-0.25,	// ...
		-0.4	// ...
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
		[unroll]
		for (int TestWindLevelIndex = 0; TestWindLevelIndex < BEZIER_GRASS_WIND_LEVEL_MAX; TestWindLevelIndex++)
		{
			WindLevel = (GlobalWindStrength > WindLevelStepValue[TestWindLevelIndex]) ? (TestWindLevelIndex + 1) : WindLevel;
		}
		WindLevel = min(WindLevel, BEZIER_GRASS_WIND_LEVEL_MAX - 1);
		WindLevelStepValueMin = (WindLevel == 0) ? (InParameter.WindStrengthRange.x) : (WindLevelStepValue[WindLevel - 1]);
		WindLevelStepValueRange = max(0.01, WindLevelStepValue[WindLevel] - WindLevelStepValueMin);
		RemappedStrength = (GlobalWindStrength - WindLevelStepValueMin) / WindLevelStepValueRange;
	}

	// Voronoi Wind
	float NoiseValue2D = 0;
	const float MovementScale = lerp(WindMovementScale[WindLevel], WindMovementScale[WindLevel + 1], RemappedStrength);
	const float2 SampledPosOffset = (GlobalWindDirection.xy * MovementScale) * InTimeParams.x;
#if 1	// Sampling voronoi get smooth result. Use texture instead.
	{
		const float2 IPos = floor(InOriginInWorldXY.xy);
		const float2 FPos = InOriginInWorldXY.xy - IPos.xy;
		float NoizeValueY0;
		{
			float2 SampledPos00 = (IPos.xy + float2(0, 0) + SampledPosOffset.xy) * WorldPosScale;
			const float4 NoiseValue00 = VoronoiNoise2DAlu(InParameter.RandomSeedParams.w, SampledPos00.xy, false, 1.0, false);
			float2 SampledPos10 = (IPos.xy + float2(1, 0) + SampledPosOffset.xy) * WorldPosScale;
			const float4 NoiseValue10 = VoronoiNoise2DAlu(InParameter.RandomSeedParams.w, SampledPos10.xy, false, 1.0, false);
			NoizeValueY0 = lerp(NoiseValue00.z, NoiseValue10.z, FPos.x);
		}
		float NoizeValueY1;
		{
			float2 SampledPos01 = (IPos.xy + float2(0, 1) + SampledPosOffset.xy) * WorldPosScale;
			const float4 NoiseValue01 = VoronoiNoise2DAlu(InParameter.RandomSeedParams.w, SampledPos01.xy, false, 1.0, false);
			float2 SampledPos11 = (IPos.xy + float2(1, 1) + SampledPosOffset.xy) * WorldPosScale;
			const float4 NoiseValue11 = VoronoiNoise2DAlu(InParameter.RandomSeedParams.w, SampledPos11.xy, false, 1.0, false);
			NoizeValueY1 = lerp(NoiseValue01.z, NoiseValue11.z, FPos.x);
		}
		NoiseValue2D = lerp(NoizeValueY0, NoizeValueY1, FPos.y);
	}
#else	// ALU Only
	{
		float2 SampledPos = (InOriginInWorldXY.xy + SampledPosOffset.xy) * WorldPosScale;
		NoiseValue2D = VoronoiNoise2DAlu(InParameter.RandomSeedParams.w, SampledPos.xy, false, 1.0, false).z;
	}
#endif	// Get voronoi value
	const float Strength = NoiseValue2D * WindLevelStepValueRange + WindLevelStepValueMin;

	const float UsedWindFrequency = lerp(WindFrequencyScale[WindLevel], WindFrequencyScale[WindLevel + 1], RemappedStrength);
	const float ScaledTime = UsedWindFrequency * InTimeParams.x;

	// Random values
	const float3 RandValue3Z = ((float3)(Rand3DPCG16(uint3(((uint2)(InOriginInWorldXY.xy)) ^ (InParameter.RandomSeedParams.z), InPerBladeHash)).xyz)) / 65535.0;
	const float3 RandValue3W = ((float3)(Rand3DPCG16(uint3(((uint2)(InOriginInWorldXY.xy)) ^ (InParameter.RandomSeedParams.w), InPerBladeHash)).xyz)) / 65535.0;

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
		float TiltMinOffset = WindTiltLevel[WindLevel * 2 + 0];
		float TiltMinOscillation = WindTiltLevel[WindLevel * 2 + 1];
		float TiltMaxOffset = WindTiltLevel[(WindLevel + 1) * 2 + 0];
		float TiltMaxOscillation = WindTiltLevel[(WindLevel + 1) * 2 + 1];
		float UsedTiltOffset = lerp(TiltMinOffset, TiltMaxOffset, RemappedStrength);
		float UsedTiltOscillation = lerp(TiltMinOscillation, TiltMaxOscillation, RemappedStrength);
		float CurrentOscillation = SinFunction(ScaledTime + Noise + 1.5707963267948966) * UsedTiltOscillation;
		InOutTilt = saturate(InOutTilt + UsedTiltOffset);
		InOutTilt = InOutTilt + CurrentOscillation;
	}
#endif

#if 1
	// Modify Bend
	{
		float Noise = NoiseValue2D * 3.14159265358979 + RandValue3W.y;
		float BendMinOscillation = WindBendLevel[WindLevel];
		float BendMaxOscillation = WindBendLevel[WindLevel + 1];
		float UsedBendOscillation = lerp(BendMinOscillation, BendMaxOscillation, RemappedStrength);
		float CurrentOscillation = SinFunction(ScaledTime + Noise) * UsedBendOscillation;
		InOutBend = InOutBend + CurrentOscillation;
	}
#endif

#if 1
	// Modify MidPointT
	{
		float MidTMinOffset = WindMidTLevel[WindLevel];
		float MidTMaxOffset = WindMidTLevel[WindLevel + 1];
		float UsedMidTOffset = lerp(MidTMinOffset, MidTMaxOffset, RemappedStrength);
		InOutMidPointT = InOutMidPointT + UsedMidTOffset;
	}
#endif
}

#endif	//BEZIER_GRASS_COMMON_HLSL
