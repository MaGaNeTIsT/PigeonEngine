#ifndef _BEZIER_GRASS_PE_VS_HLSL
#define _BEZIER_GRASS_PE_VS_HLSL

struct StreamIn
{
    float4 BeizerStepData : TEXCOORD0;
};

struct Varying
{
    float4 Position		: SV_POSITION;
    float4 Normal		: NORMAL0;
};

#include "../Public/BezierGrassCommon.hlsl"
#include "../Public/ShaderFunctions.hlsl"

cbuffer ConstantBufferPerFrame : register(b0)
{
	float4x4		_ViewMatrix;
	float4x4		_ViewInvMatrix;
	float4x4		_ProjectionMatrix;
	float4x4		_ProjectionInvMatrix;
	float4x4		_ViewProjectionMatrix;
	float4x4		_ViewProjectionInvMatrix;
	float4			_TimeParams;
	float4			_DepthMultiAdd;
	float4			_ScreenToViewSpaceParams;
	float4			_CameraViewportMinSizeAndInvBufferSize;
	float4			_CameraViewportSizeAndInvSize;
	float4			_CameraViewportRect;
	float4			_CameraWorldPosition;
};

cbuffer ConstantBufferPerDraw : register(b1)
{
	float4x4		_WorldMatrix;
	float4x4		_WorldInvMatrix;
	float4x4		_WorldInvTransposeMatrix;
    float4			_BezierGrassParams1;
    float4			_BezierGrassRootColor;
    float4			_BezierGrassTipColor;
};

StructuredBuffer<GrassInstanceInfo> _InstanceGrass : register(t1);

Varying main(StreamIn InStream, uint InstanceID : SV_InstanceID, uint VertexID : SV_VertexID)
{
    BezierPropertyInfo BezierInfo = GetBezierPropertyInfo(InStream.BeizerStepData);
    GrassPropertyInfo PropertyInfo = GetGrassPropertyInfo(_BezierGrassParams1.xyzw);
    
    float T = lerp(BezierInfo.T, BezierInfo.PrevLODT, frac(PropertyInfo.LOD));
    float Offset = BezierInfo.NegOffset - BezierInfo.PosOffset;
    
    GrassInstanceInfo InstanceInfo = _InstanceGrass[InstanceID];
    float TimeSin = (sin(_TimeParams.x * 5.0 + RandomVector3(InstanceInfo.Origin.xyz) * 5.0) + 1.0) * 0.5;
    float Tilt = lerp(0.0, InstanceInfo.Tilt, TimeSin * 0.3 + 0.7);
    float Bend = lerp(0.0, InstanceInfo.Bend, TimeSin * 0.6 + 0.4);

    float TipSin, TipCos;
    sincos(Tilt, TipSin, TipCos);
    float2 TipPos = float2(
		InstanceInfo.Tip.x * TipCos + InstanceInfo.Tip.y * TipSin,
		InstanceInfo.Tip.y * TipCos - InstanceInfo.Tip.x * TipSin);
    float2 ControlPos = TipPos * 0.5 + (float2(-TipPos.y, TipPos.x) + TipPos) * 0.5 * Bend;

    float2 PosOS = QuadraticBezierCurve(float2(0.0, 0.0), ControlPos, TipPos, T);
    float2 NmlOS = DerivativeQuadraticBezierCurve(float2(0.0, 0.0), ControlPos, TipPos, T);

    float3 NmlOrthWS = float3(-InstanceInfo.Direction.z, InstanceInfo.Direction.y, InstanceInfo.Direction.x);
    float3 NmlBezierWS = SafeNormalize(InstanceInfo.Direction * NmlOS.x + float3(0.0, 1.0, 0.0) * NmlOS.y);
    float3 NmlWS = SafeNormalize(cross(NmlOrthWS, NmlBezierWS));
    float3 PosWS = InstanceInfo.Origin + InstanceInfo.Direction * PosOS.x + float3(0.0, 1.0, 0.0) * PosOS.y + NmlOrthWS * Offset * PropertyInfo.LeafWidth;

    Varying Output;
    
    Output.Position = TransformWorldToClip(PosWS, _ViewProjectionMatrix);
    Output.Normal = float4(NmlWS, T);

    return Output;
}

#endif