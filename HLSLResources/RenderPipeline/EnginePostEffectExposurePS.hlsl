#ifndef _SHADER_POST_EFFECT_PS_HLSL
#define _SHADER_POST_EFFECT_PS_HLSL

#include "../Common/ShaderCommon.hlsl"

float4 main(VaryingScreenPolygon2D input) : SV_Target
{
	float4 color = 1;
	color.a = 1;
	return color;
}

#endif