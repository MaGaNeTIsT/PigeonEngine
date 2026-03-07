#ifndef _BEZIER_GRASS_PE_PS_HLSL
#define _BEZIER_GRASS_PE_PS_HLSL

struct Varying
{
    float4 Position     : SV_POSITION;
    float4 Normal       : NORMAL0;
};

#include "../Public/BezierGrassCommon.hlsl"

cbuffer ConstantBufferPerFrame : register(b0)
{
    float4x4        _ViewMatrix;
    float4x4        _ViewInvMatrix;
    float4x4        _ProjectionMatrix;
    float4x4        _ProjectionInvMatrix;
    float4x4        _ViewProjectionMatrix;
    float4x4        _ViewProjectionInvMatrix;
    float4          _TimeParams;
    float4          _DepthMultiAdd;
    float4          _ScreenToViewSpaceParams;
    float4          _CameraViewportMinSizeAndInvBufferSize;
    float4          _CameraViewportSizeAndInvSize;
    float4          _CameraViewportRect;
    float4          _CameraWorldPosition;
};

cbuffer ConstantBufferPerDraw : register(b1)
{
    float4x4        _WorldMatrix;
    float4x4        _WorldInvMatrix;
    float4x4        _WorldInvTransposeMatrix;
    float4          _BezierGrassParams1;
    float4          _BezierGrassRootColor;
    float4          _BezierGrassTipColor;
};

#define SHADER_USE_DEFERRED_SHADING 1
#include "../Common/ShaderCommon.hlsl"

void main(in Varying Input,
    in bool bFrontFace          : SV_IsFrontFace,
    out float4 OutSceneColor    : SV_Target,
    out float4 OutGBufferA      : SV_Target1,
    out float4 OutGBufferB      : SV_Target2,
    out float4 OutGBufferC      : SV_Target3)
{
    float3 Color = lerp(_BezierGrassRootColor.rgb, _BezierGrassTipColor.rgb, Input.Normal.a);
    float3 Normal = SafeNormalize(Input.Normal.xyz);
    GBufferTerm GBuffer = InitGBuffer(bFrontFace ? Normal : -Normal,
        Color.rgb,
        0.7, 0.0, SHADER_SHADING_MODEL_DEFAULT_LIT, 1.0, 1.0
    );

    OutSceneColor = float4(Color.rgb * 0.5, 1.0);

    EncodeGBuffer(GBuffer, OutGBufferA, OutGBufferB, OutGBufferC);
}

#endif