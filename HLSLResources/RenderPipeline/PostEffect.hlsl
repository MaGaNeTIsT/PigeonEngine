#ifndef _SHADER_POST_EFFECT_HLSL
#define _SHADER_POST_EFFECT_HLSL

#include "../Common/ShaderNoises.hlsl"

cbuffer ConstatntBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;

	float4 CameraPosition;
	float4 Parameter;
	matrix WorldInvTranspose;
	float4 Parameter2;
}

Texture2D		g_PositionMap	: register(t0);
Texture2D		g_GBuffer		: register(t5);
SamplerState	g_SamplerState	: register(s0);

void main(	in float4 inPosition	: SV_POSITION,
			in float2 inTexCoord	: TEXCOORD0,

			out float4 outDiffuse	: SV_Target)
{
	outDiffuse.rgb = g_GBuffer.Sample(g_SamplerState, inTexCoord).rgb;
	outDiffuse.a = 1.0;
}

#endif