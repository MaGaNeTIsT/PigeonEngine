#ifndef _SKY_LIGHT_PE_VS_HLSL
#define _SKY_LIGHT_PE_VS_HLSL

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

#define SHADER_USE_VIEW_INPUT           1

#define SHADER_USE_POSITION_INPUT       1
#define SHADER_USE_POSITION_INPUT_NUM   1

#include "../Common/ShaderCommon.hlsl"

void main(in Attribute Input, out Varying Output)
{
    Output.WorldLocation    = TransformPositionToSpecificSpace(float4(Input.Position0.xyz, 1.0), _WorldMatrix);
    Output.Position         = TransformWorldToClip(Output.WorldLocation.xyz).xyww;
}

#endif