#ifndef _ENGINE_BEZIER_GRASS_VS_HLSL
#define _ENGINE_BEZIER_GRASS_VS_HLSL

#include "./BezierGrassCommon.hlsl"

cbuffer BezierGrassPropertyConstantBuffer : register(b2)
{
	float4	_Params0;
	float4	_RootColor;
	float4	_TipColor;
}
//cbuffer BezierGrassInstanceConstantBuffer : register(b3)
//{
//	GrassInstanceParams	_InstanceGrass[1024];
//}
StructuredBuffer<GrassInstanceParams> _InstanceGrass : register(t5);

Varying main(uint InstanceID : SV_InstanceID, uint VertexID : SV_VertexID)
{
	GrassPropertyInfo Info = GetGrassPropertyInfo(_Params0.xyzw);
	uint SplitNum = Info.SplitNum;

	uint IndexTri = VertexID / 3u;
	uint IndexTriVex = VertexID % 3u;

	bool isFirstListTri = IndexTri / SplitNum == 0u;

	float BaseStep = (float)(IndexTri % SplitNum);

	float Offset = isFirstListTri ? (IndexTriVex == 1u ? 1.0 : 0.0) : (IndexTriVex == 2u ? 0.0 : 1.0);
	Offset = IndexTri == (SplitNum - 1u) && IndexTriVex == 2u ? 0.5 : Offset;
	float Up = isFirstListTri ? (IndexTriVex == 0u ? 0.0 : 1.0) : (IndexTriVex == 1u ? 1.0 : 0.0);
	Up = IndexTri == (SplitNum - 1u) && IndexTriVex == 1u ? 0.0 : Up;

	float T = (BaseStep + Up) * Info.StepLength;

	//GrassInstanceInfo InstanceInfo = GetGrassInstanceInfo(_InstanceGrass[0]);
	GrassInstanceInfo InstanceInfo = GetGrassInstanceInfo(_InstanceGrass[InstanceID]);
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

	float RootToTip = saturate(PosOS.y / TipPos.y);

	float3 NmlOrthWS = SafeNormalize(InstanceInfo.Direction * (-PosOS.y) + float3(0.0, 1.0, 0.0) * PosOS.x);
	float3 NmlBezierWS = SafeNormalize(InstanceInfo.Direction * NmlOS.x + float3(0.0, 1.0, 0.0) * NmlOS.y);
	NmlOrthWS = T == 0.0 ? -InstanceInfo.Direction : NmlOrthWS;
	NmlBezierWS = T == 0.0 ? float3(0.0, 1.0, 0.0) : NmlBezierWS;
	float3 NmlWS = cross(NmlOrthWS, NmlBezierWS);
	float3 PosWS = InstanceInfo.Origin + InstanceInfo.Direction * PosOS.x + float3(0.0, 1.0, 0.0) * PosOS.y + NmlOrthWS * Offset * Info.LeafWidth;

	Varying Output;

	Output.PositionCS	= TransformWorldToClip(PosWS);
	Output.NormalWS		= float4(NmlWS, RootToTip);

	return Output;
}

#endif