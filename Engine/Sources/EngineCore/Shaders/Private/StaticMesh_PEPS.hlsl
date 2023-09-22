#ifndef _STATIC_MESH_PE_PS_HLSL
#define _STATIC_MESH_PE_PS_HLSL

struct Varying
{
    float4 Position : SV_POSITION;
    float4 Normal   : NORMAL0;
    float2 Texcoord : TEXCOORD0;
};

#define SHADER_USE_SAMPLER_INPUT        1
#define SHADER_USE_VIEW_INPUT           1
#define SHADER_USE_PERDRAW_INPUT        1
#define SHADER_USE_TRANSFORM_INPUT      1

#include "../Common/ShaderCommon.hlsl"

void main(in Varying Input, out float4 Ouput : SV_Target)
{
    Ouput = float4(1.0, 1.0, 1.0, 1.0);
}

#endif