#ifndef _BEZIER_GRASS_RENDER_COMMON_HLSL
#define _BEZIER_GRASS_RENDER_COMMON_HLSL

struct FDFVector3 { float3 High; float3 Low; };
struct Matrix { float4x4 m; };

FDFVector3 MakeDFVector3(float3 High, float3 Low) { FDFVector3 r; r.High = High; r.Low = Low; return r; }
float3 TransformLocalToTranslatedWorld(float3 LocalPos, Matrix LocalToWorld, FDFVector3 PreViewTranslation)
{
    return mul(float4(LocalPos, 1.0), LocalToWorld.m).xyz + PreViewTranslation.High + PreViewTranslation.Low;
}

#define Texture2DArraySample(tex, samp, uv) tex.Sample(samp, uv)
#define Texture2DArraySampleLevel(tex, samp, uv, lod) tex.SampleLevel(samp, uv, lod)

StructuredBuffer<uint> PackedInstanceDatas : register(t0);
StructuredBuffer<uint> DispatchDatas : register(t1);
StructuredBuffer<float> LayerTypeDatas : register(t2);

#define BezierGrassLayerTypeBinding LayerTypeDatas
#define BezierGrassPackedInstanceBinding PackedInstanceDatas
#define BezierGrassDispatchBinding DispatchDatas

#include "../Common/BezierGrassCommon.hlsl"

#endif
