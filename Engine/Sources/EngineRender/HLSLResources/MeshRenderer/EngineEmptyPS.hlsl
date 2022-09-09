#ifndef _ENGINE_EMPTY_PS_HLSL
#define _ENGINE_EMPTY_PS_HLSL

#include "../Common/ShaderCommon.hlsl"

float4 main(Varying input) : SV_Target
{
	return float4(1, 1, 1, 1);
}

#endif