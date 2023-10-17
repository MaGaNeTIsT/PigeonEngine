#ifndef _SKELETAL_MESH_PE_VS_HLSL
#define _SKELETAL_MESH_PE_VS_HLSL

struct Varying
{
    float4 Position : SV_POSITION;
    float4 Normal   : NORMAL0;
    float2 Texcoord : TEXCOORD0;
};

#define SHADER_USE_VIEW_INPUT                                   1
#define SHADER_USE_PERDRAW_INPUT                                1
#define SHADER_USE_TRANSFORM_INPUT                              1
#define SHADER_USE_TRANSFORM                                    1

#define SHADER_USE_SKELETON_INPUT                               1

#define SHADER_USE_POSITION_INPUT                               1
#define SHADER_USE_POSITION_INPUT_NUM                           1
#define SHADER_USE_NORMAL_INPUT                                 1
#define SHADER_USE_NORMAL_INPUT_NUM                             1
#define SHADER_USE_TEXCOORD_INPUT                               1
#define SHADER_USE_TEXCOORD_INPUT_NUM                           1

#define SHADER_SKELETON_BONE_INPUT_SLOT                         t0
#define SHADER_SKELETON_BONE_INVERSE_TRANSPOSE_INPUT_SLOT       t1
#define SHADER_SKELETON_EFFECT_BONE_NUM                         4

#include "../Common/ShaderCommon.hlsl"

void main(in Attribute Input, out Varying Output)
{
    float4x4    BoneMatrix      = GetFinalBoneMatrix(Input);
    float4x4    BoneMatrix_I_T  = GetFinalBoneInverseTransposeMatrix(Input);
    float3      PositionOS      = TransformBoneToObject(Input.Position0.xyz, BoneMatrix);
    float3      NormalOS        = TransformBoneToObjectNormal(Input.Normal0.xyz, (float3x3)BoneMatrix_I_T);

    Output.Position = TransformObjectToClip(PositionOS.xyz);
    Output.Normal   = float4(TransformObjectToWorldNormal(NormalOS.xyz), 1.0);
    Output.Texcoord = Input.Texcoord0.xy;
}

#endif