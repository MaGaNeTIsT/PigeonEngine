#ifndef _STATIC_MESH_PE_VS_HLSL
#define _STATIC_MESH_PE_VS_HLSL

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

#define SHADER_USE_POSITION_INPUT       1
#define SHADER_USE_POSITION_INPUT_NUM   1
#define SHADER_USE_NORMAL_INPUT         1
#define SHADER_USE_NORMAL_INPUT_NUM     1
#define SHADER_USE_TEXCOORD_INPUT       1
#define SHADER_USE_TEXCOORD_INPUT_NUM   1

#include "../Common/ShaderCommon.hlsl"

void main(in Attribute Input, out Varying Output)
{
    Output.Position = TransformObjectToClip(Input.Position0.xyz);
    Output.Normal   = float4(TransformObjectToWorldNormal(Input.Normal0.xyz), 1.0);
    Output.Texcoord = Input.Texcoord0.xy;
}

#endif