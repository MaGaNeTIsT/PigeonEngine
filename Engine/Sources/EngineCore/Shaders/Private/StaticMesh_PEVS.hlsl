#ifndef _STATIC_MESH_PE_VS_HLSL
#define _STATIC_MESH_PE_VS_HLSL

struct Attribute
{
    float4 Position : POSITION0;
    float4 Normal   : NORMAL0;
    float2 Texcoord : TEXCOORD0;
};

struct Varying
{
    float4 Position : SV_POSITION;
    float4 Normal   : NORMAL0;
    float2 Texcoord : TEXCOORD0;
};

#define SHADER_USE_VIEW_INPUT           1
#define SHADER_USE_PERDRAW_INPUT        1
#define SHADER_USE_TRANSFORM_INPUT      1
#define SHADER_USE_TRANSFORM            1

#include "../Common/ShaderCommon.hlsl"

void main(in Attribute Input, out Varying Output)
{
    Output.Position = TransformObjectToClip(Input.Position.xyz);
    Output.Normal   = float4(TransformObjectToWorldNormal(Input.Normal.xyz), 1.0);
    Output.Texcoord = Input.Texcoord;
}

#endif