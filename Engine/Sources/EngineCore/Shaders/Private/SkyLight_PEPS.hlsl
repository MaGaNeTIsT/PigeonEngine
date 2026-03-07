#ifndef _SKY_LIGHT_PE_PS_HLSL
#define _SKY_LIGHT_PE_PS_HLSL

struct Varying
{
    float4 Position         : SV_POSITION;
    float4 WorldLocation    : TEXCOORD0;
};

cbuffer ConstantBufferSkyLight : register(b1)
{
    float4x4		_WorldMatrix;
    float4x4		_WorldInvMatrix;
    float3          _LightAdjustColor;
    float           _LightAdjustIntensity;
};

TextureCube _EnvironmentInput : register(t0);

#define SHADER_USE_SAMPLER_INPUT        1
#define SHADER_USE_VIEW_INPUT           1

#include "../Common/ShaderCommon.hlsl"

void main(in Varying Input,
    out float4 OutSceneColor    : SV_Target,
    out float4 OutGBufferA      : SV_Target1,
    out float4 OutGBufferB      : SV_Target2,
    out float4 OutGBufferC      : SV_Target3)
{
    OutSceneColor   = float4(_EnvironmentInput.Sample(_LinearWrapSampler, SafeNormalize(Input.WorldLocation.xyz)).rgb, 1.0);
    OutGBufferA     = float4(0.0, 0.0, 0.0, 1.0);
    OutGBufferB     = float4(0.0, 0.0, 0.0, 1.0);
    OutGBufferC     = float4(0.0, 0.0, 0.0, 1.0);
}

#endif