#ifndef _STATIC_MESH_PE_PS_HLSL
#define _STATIC_MESH_PE_PS_HLSL

struct Varying
{
    float4 Position : SV_POSITION;
    float4 Normal   : NORMAL0;
    float2 Texcoord : TEXCOORD0;
};

#define SHADER_USE_DEFERRED_SHADING     1
#define SHADER_USE_SAMPLER_INPUT        1
#define SHADER_USE_VIEW_INPUT           1
#define SHADER_USE_PERDRAW_INPUT        1
#define SHADER_USE_TRANSFORM_INPUT      1

#include "../Common/ShaderCommon.hlsl"

void main(in Varying Input,
    out float4 OutSceneColor    : SV_Target,
    out float4 OutGBufferA      : SV_Target1,
    out float4 OutGBufferB      : SV_Target2,
    out float4 OutGBufferC      : SV_Target3)
{
    GBufferTerm GBuffer = InitGBuffer(Input.Normal.xyz, float3(0.9, 0.9, 0.9),
        0.5, 0.0, SHADER_SHADING_MODEL_DEFAULT_LIT, 1.0);

    OutSceneColor = float4(0.0, 0.0, 0.0, 1.0);

    EncodeGBuffer(GBuffer, OutGBufferA, OutGBufferB, OutGBufferC);
}

#endif