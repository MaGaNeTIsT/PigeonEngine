#ifndef _SHADER_DEBUG_POLYGON_2D_ALPHA_HLSL
#define _SHADER_DEBUG_POLYGON_2D_ALPHA_HLSL

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

}

Texture2D		g_Texture		: register(t6);
SamplerState	g_SamplerState	: register(s0);

void main(in float4 inPosition	: SV_POSITION,
	in float2 inTexCoord : TEXCOORD0,

	out float4 outDiffuse : SV_Target)
{

	outDiffuse.rgb = g_Texture.Sample(g_SamplerState, inTexCoord).a;
	outDiffuse.a = 1.0;

}

#endif