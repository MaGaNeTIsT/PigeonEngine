#ifndef _ENGINE_SCREEN_POLYGON_2D_PS_HLSL
#define _ENGINE_SCREEN_POLYGON_2D_PS_HLSL

#include "../Common/ShaderCommon.hlsl"

float4 main(VaryingScreenPolygon2D input) : SV_Target
{
	return float4(_CustomTextureA.Sample(_LinearClampSampler, input.uv0).rgb, 1.0);
}

#endif