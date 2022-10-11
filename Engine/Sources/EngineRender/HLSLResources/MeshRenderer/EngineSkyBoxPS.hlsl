#ifndef _ENGINE_SKY_BOX_PS_HLSL
#define _ENGINE_SKY_BOX_PS_HLSL

#include "../Common/ShaderCommon.hlsl"

float4 main(VaryingSkyBox input) :SV_Target
{
	return float4(1.0, 1.0, 1.0, 1.0);
}

#endif