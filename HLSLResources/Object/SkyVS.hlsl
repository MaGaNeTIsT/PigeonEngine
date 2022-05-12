#ifndef _SHADER_SKY_VS_HLSL
#define _SHADER_SKY_VS_HLSL

#include "../Common/ShaderFunctions.hlsl"

cbuffer ConstatntBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;

	float4 CameraPosition;
}

void main(	in float4 inPosition		: POSITION0,
			in float4 inNormal			: NORMAL0,
			in float4 inDiffuse			: COLOR0,
			in float2 inTexCoord		: TEXCOORD0,
			
			out float4 outPosition		: SV_POSITION,
            out float4 outWorldPosition : POSITION0,
			out float4 outNormal		: NORMAL0,
			out float4 outDiffuse		: COLOR0,
			out float2 outTexCoord		: TEXCOORD0 )
{
	matrix wvp;
	wvp = mul(World, View);
	wvp = mul(wvp, Projection);
	
	outPosition = mul(inPosition, wvp);
    outWorldPosition = mul(inPosition, World);

	float4 worldNormal, normal;
	normal = float4(inNormal.xyz, 0.0);
	worldNormal = mul(normal, World);
	worldNormal = normalize(worldNormal);
	outNormal = worldNormal;

	outDiffuse = inDiffuse;

	outTexCoord = inTexCoord;
}

#endif