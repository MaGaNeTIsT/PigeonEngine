#ifndef _SCENE_LIGHTING_PE_PS_HLSL
#define _SCENE_LIGHTING_PE_PS_HLSL

#define SHADER_USE_SAMPLER_INPUT        1
#define SHADER_USE_VIEW_INPUT           1

#include "../Common/ShaderCommon.hlsl"

void main(in float4 InPosition : SV_Position, in float2 InTexcoord : TEXCOORD0, out float4 OutTarget : SV_Target)
{
    OutTarget = float4(1.0, 1.0, 1.0, 1.0);
}

#endif