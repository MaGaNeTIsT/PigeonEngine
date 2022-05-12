#ifndef _SHADER_POLYGON_2D_VS_HLSL
#define _SHADER_POLYGON_2D_VS_HLSL

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

void main(	in float4 inPosition			: POSITION0,
			in float4 inNormal				: NORMAL0,
			in float4 inDiffuse				: COLOR0,
			in float2 inTexCoord			: TEXCOORD0,

			out float4 outPosition			: SV_POSITION,
			out float2 outTexCoord			: TEXCOORD0)
{
	outPosition.x = (inPosition.x / Parameter2.x) * 2.0 - 1.0;
	outPosition.y = ((-inPosition.y) / Parameter2.y) * 2.0 + 1.0;
	outPosition.z = 0.0;
	outPosition.w = 1.0;

	outTexCoord = inTexCoord;
}

#endif