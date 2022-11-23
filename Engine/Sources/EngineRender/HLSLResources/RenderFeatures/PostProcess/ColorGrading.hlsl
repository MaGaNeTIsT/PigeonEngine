#ifndef _COLOR_GRADING_PS_HLSL
#define _COLOR_GRADING_PS_HLSL

#include "../../Common/ShaderCommon.hlsl"

float4 main(VaryingScreenPolygon2D input) : SV_Target
{
	float4 color = float4(0.0,0.0,0.0,1.0);
	color.rgb = _EngineCameraColor.Sample(_LinearClampSampler, input.uv0).rgb;
	color.rgb = pow(color.rgb, 0.41667);
	return color;
}

#endif