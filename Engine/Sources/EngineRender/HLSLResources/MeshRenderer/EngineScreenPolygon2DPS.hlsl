#ifndef _ENGINE_SCREEN_POLYGON_2D_PS_HLSL
#define _ENGINE_SCREEN_POLYGON_2D_PS_HLSL

#include "../Common/ShaderCommon.hlsl"

float4 main(VaryingScreenPolygon2D input) : SV_Target
{
	float4 color = float4(_AlbedoTexture.Sample(_LinearClampSampler, input.uv0).rgb, 1);
	return color;
}

#endif