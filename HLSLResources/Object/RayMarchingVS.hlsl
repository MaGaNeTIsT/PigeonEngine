#ifndef _SHADER_RAY_MARCHING_VS_HLSL
#define _SHADER_RAY_MARCHING_VS_HLSL

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

void main(
			in  float4 inPosition			: POSITION0,
			in  float4 inNormal				: NORMAL0,
			in  float4 inDiffuse			: COLOR0,
			in  float2 inTexCoord			: TEXCOORD0,

			out float4 outPosition			: SV_POSITION,
			out float4 outWorldPosition		: POSITION0,
			out float4 outNormal			: NORMAL0,
			out float4 outDiffuse			: COLOR0,
			out float2 outTexCoord			: TEXCOORD0)
{
	matrix wvp;
	wvp = mul(World, View);
	wvp = mul(wvp, Projection);

	outPosition = mul(float4(inPosition.xyz, 1.0), wvp);
	outWorldPosition = mul(float4(inPosition.xyz, 1.0), World);

	outNormal = float4(mul(float4(inNormal.xyz, 0.0), WorldInvTranspose).xyz, 0.0);

	outDiffuse = inDiffuse;

	outTexCoord = inTexCoord;
}

#endif