#ifndef _SHADER_WATER_FIELD_VS_HLSL
#define _SHADER_WATER_FIELD_VS_HLSL

#include "../Common/ShaderFunctions.hlsl"

cbuffer ConstatntBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;

	float4 CameraPosition;
	float4 Parameter;

	matrix WorldInvTranspose;
}

void main(  in  float4 inPosition		: POSITION0,
			in  float4 inNormal			: NORMAL0,
			in  float4 inDiffuse		: COLOR0,
			in  float2 inTexCoord		: TEXCOORD0,
			
			out float4 outPosition		: SV_POSITION,
            out float4 outWorldPosition : POSITION0,
			out float4 outNormal		: NORMAL0,
			out float4 outTangent       : TANGENT0,
			out float4 outBinormal      : BINORMAL0,
			out float4 outDiffuse		: COLOR0,
			out float2 outTexCoord		: TEXCOORD0 )
{
	matrix wvp;
	wvp = mul(World, View);
	wvp = mul(wvp, Projection);

	float3 Pos = inPosition.xyz;
	Pos += GerstnerWaveCalculatePos(inPosition.xyz, Parameter.x);

	outPosition = mul(float4(Pos, 1.0), wvp);
	outWorldPosition = mul(float4(Pos, 1.0), World);

	float3 T = float3(0.0, 0.0, 0.0), B = float3(0.0, 0.0, 0.0), N = float3(0.0, 0.0, 0.0);
	GerstnerWaveCalculateTBN(inPosition.xyz, Parameter.x, T, B, N);

	N = mul(float4(N, 0.0), WorldInvTranspose).xyz;
	N = normalize(N);
	B = mul(float4(B, 0.0), World).xyz;
	B = normalize(B);
	T = mul(float4(T, 0.0), World).xyz;
	T = normalize(T);
	outNormal = float4(N, 0.0);
	outTangent = float4(T, 0.0);
	outBinormal = float4(B, 0.0);

	outDiffuse = inDiffuse;

	outTexCoord = inTexCoord;
}

#endif