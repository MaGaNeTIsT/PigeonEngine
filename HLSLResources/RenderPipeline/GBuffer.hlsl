#ifndef _SHADER_GBUFFER_HLSL
#define _SHADER_GBUFFER_HLSL

#include "../Common/ShaderFunctions.hlsl"

cbuffer ConstatntBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;

	float4 CameraPosition;
	float4 Parameter;
	matrix WorldInvTranspose;
	float4 Parameter2;

	matrix LightViewMatrix;
	matrix LightProjectionMatrix;
}

Texture2D		g_PositionMap	: register(t0);
Texture2D		g_NormalMap		: register(t1);
Texture2D		g_DiffuseMap	: register(t2);
Texture2D		g_Specular		: register(t3);
Texture2D		g_ShadowMap		: register(t4);
SamplerState	g_SamplerState	: register(s0);

void main(	in float4 inPosition	: SV_POSITION,
			in float2 inTexCoord	: TEXCOORD0,

			out float4 outDiffuse	: SV_Target)
{
	float ka, depth;
	float3 pos, normal, kd;
	float4 ks, mapSample;
	mapSample = g_PositionMap.Sample(g_SamplerState, inTexCoord);
	pos = mapSample.xyz; depth = mapSample.w;
	mapSample = g_NormalMap.Sample(g_SamplerState, inTexCoord);
	normal = mapSample.xyz;
	mapSample = g_DiffuseMap.Sample(g_SamplerState, inTexCoord);
	kd = mapSample.xyz; ka = mapSample.w;
	ks = g_Specular.Sample(g_SamplerState, inTexCoord);

	float3 lightDir = normalize(Parameter.yzw);
	float3 eyeVec = normalize(pos - CameraPosition.xyz);
	float3 A, D, S;

	if (ComputeShadow(LightViewMatrix, LightProjectionMatrix, pos, g_ShadowMap, g_SamplerState))
	{
		D = 0.0; S = 0.0;
		A = ka * float3(0.15, 0.15, 0.15);
	}
	else
	{
		ComputeBlinnPhongLight(ka, kd, ks.xyz, ks.w, normal, lightDir, float3(0.8, 0.8, 0.8), float3(0.15, 0.15, 0.15), eyeVec, A, D, S);
	}

	outDiffuse.rgb = A + D + S;
	outDiffuse.a = 1.0;
}

#endif