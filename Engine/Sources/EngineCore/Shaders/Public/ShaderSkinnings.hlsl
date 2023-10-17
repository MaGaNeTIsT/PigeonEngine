#ifndef _SHADER_SKINNINGS_HLSL
#define _SHADER_SKINNINGS_HLSL

#include "./ShaderFunctions.hlsl"
#include "./ShaderStructCommon.hlsl"
#include "./ShaderVariables.hlsl"
#include "./ShaderSpaceTransform.hlsl"

#if (SHADER_USE_SKELETON_INPUT)

#ifndef SHADER_SKELETON_BONE_INPUT_SLOT
#define SHADER_SKELETON_BONE_INPUT_SLOT							t0
#endif	//SHADER_SKELETON_BONE_INPUT_SLOT

#ifndef SHADER_SKELETON_BONE_INVERSE_TRANSPOSE_INPUT_SLOT
#define SHADER_SKELETON_BONE_INVERSE_TRANSPOSE_INPUT_SLOT		t1
#endif	//SHADER_SKELETON_BONE_INVERSE_TRANSPOSE_INPUT_SLOT

#define SHADER_SKELETON_BONE_MAX_NUM							512
#define SHADER_MESH_SKELETON_BONE_MAX_NUM						256

StructuredBuffer<float4x4> _SkeletonBoneInput					: register(SHADER_SKELETON_BONE_INPUT_SLOT);
StructuredBuffer<float4x4> _SkeletonBoneInverseTransposeInput	: register(SHADER_SKELETON_BONE_INVERSE_TRANSPOSE_INPUT_SLOT);

#define USED_SKELETON_INPUT							_SkeletonBoneInput
#define USED_SKELETON_INVERSE_TRANSPOSE_INPUT		_SkeletonBoneInverseTransposeInput

struct BoneInputAttribute
{
#if (SHADER_SKELETON_EFFECT_BONE_NUM == 1)
	uint	BlendIndex0;
	float	BlendWeight0;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 2)
	uint2	BlendIndex0;
	float2	BlendWeight0;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 3)
	uint3	BlendIndex0;
	float3	BlendWeight0;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 4)
	uint4	BlendIndex0;
	float4	BlendWeight0;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 5)
	uint4	BlendIndex0;
	uint	BlendIndex1;
	float4	BlendWeight0;
	float	BlendWeight1;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 6)
	uint4	BlendIndex0;
	uint2	BlendIndex1;
	float4	BlendWeight0;
	float2	BlendWeight1;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 7)
	uint4	BlendIndex0;
	uint3	BlendIndex1;
	float4	BlendWeight0;
	float3	BlendWeight1;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 8)
	uint4	BlendIndex0;
	uint4	BlendIndex1;
	float4	BlendWeight0;
	float4	BlendWeight1;
#endif
};

float4x4 GetFinalBoneMatrix(in Attribute Input)
{
	float4x4 OutFinalMatrix = USED_SKELETON_INPUT[Input.BlendIndex0.x] * Input.BlendWeight0.x;
	float FinalWeight = Input.BlendWeight0.x;
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 2)
	OutFinalMatrix += USED_SKELETON_INPUT[Input.BlendIndex0.y] * Input.BlendWeight0.y;
	FinalWeight += Input.BlendWeight0.y;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 3)
	OutFinalMatrix += USED_SKELETON_INPUT[Input.BlendIndex0.z] * Input.BlendWeight0.z;
	FinalWeight += Input.BlendWeight0.z;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 4)
	OutFinalMatrix += USED_SKELETON_INPUT[Input.BlendIndex0.w] * Input.BlendWeight0.w;
	FinalWeight += Input.BlendWeight0.z;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 5)
	OutFinalMatrix += USED_SKELETON_INPUT[Input.BlendIndex1.x] * Input.BlendWeight1.x;
	FinalWeight += Input.BlendWeight1.x;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 6)
	OutFinalMatrix += USED_SKELETON_INPUT[Input.BlendIndex1.y] * Input.BlendWeight1.y;
	FinalWeight += Input.BlendWeight1.y;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 7)
	OutFinalMatrix += USED_SKELETON_INPUT[Input.BlendIndex1.z] * Input.BlendWeight1.z;
	FinalWeight += Input.BlendWeight1.z;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 8)
	OutFinalMatrix += USED_SKELETON_INPUT[Input.BlendIndex1.w] * Input.BlendWeight1.w;
	FinalWeight += Input.BlendWeight1.w;
#endif
	OutFinalMatrix /= FinalWeight;
	return OutFinalMatrix;
}

float4x4 GetFinalBoneInverseTransposeMatrix(in Attribute Input)
{
	const uint BoneNum = _SkeletonParams.x;
	float4x4 OutFinalMatrix = USED_SKELETON_INVERSE_TRANSPOSE_INPUT[BoneNum + Input.BlendIndex0.x] * Input.BlendWeight0.x;
	float FinalWeight = Input.BlendWeight0.x;
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 2)
	OutFinalMatrix += USED_SKELETON_INVERSE_TRANSPOSE_INPUT[BoneNum + Input.BlendIndex0.y] * Input.BlendWeight0.y;
	FinalWeight += Input.BlendWeight0.y;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 3)
	OutFinalMatrix += USED_SKELETON_INVERSE_TRANSPOSE_INPUT[BoneNum + Input.BlendIndex0.z] * Input.BlendWeight0.z;
	FinalWeight += Input.BlendWeight0.z;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 4)
	OutFinalMatrix += USED_SKELETON_INVERSE_TRANSPOSE_INPUT[BoneNum + Input.BlendIndex0.w] * Input.BlendWeight0.w;
	FinalWeight += Input.BlendWeight0.z;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 5)
	OutFinalMatrix += USED_SKELETON_INVERSE_TRANSPOSE_INPUT[BoneNum + Input.BlendIndex1.x] * Input.BlendWeight1.x;
	FinalWeight += Input.BlendWeight1.x;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 6)
	OutFinalMatrix += USED_SKELETON_INVERSE_TRANSPOSE_INPUT[BoneNum + Input.BlendIndex1.y] * Input.BlendWeight1.y;
	FinalWeight += Input.BlendWeight1.y;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 7)
	OutFinalMatrix += USED_SKELETON_INVERSE_TRANSPOSE_INPUT[BoneNum + Input.BlendIndex1.z] * Input.BlendWeight1.z;
	FinalWeight += Input.BlendWeight1.z;
#endif
#if (SHADER_SKELETON_EFFECT_BONE_NUM >= 8)
	OutFinalMatrix += USED_SKELETON_INVERSE_TRANSPOSE_INPUT[BoneNum + Input.BlendIndex1.w] * Input.BlendWeight1.w;
	FinalWeight += Input.BlendWeight1.w;
#endif
	OutFinalMatrix /= FinalWeight;
	return OutFinalMatrix;
}

float3 TransformBoneToObject(in float3 InPosition, in float4x4 InBoneToObjectMatrix4x4/*GetFinalBoneMatrix(Attribute Input)*/)
{
	return (TransformPositionToSpecificSpace(float4(InPosition.xyz, 1.0), InBoneToObjectMatrix4x4).xyz);
}

float3 TransformBoneToObjectNormal(in float3 InNormal, in float3x3 InBoneToObjectMatrix3x3_InverseTranspose/*(float3x3)GetFinalBoneInverseTransposeMatrix(Attribute Input)*/, uniform bool bNormalize = true)
{
	float3 NormalOS = mul(InNormal.xyz, InBoneToObjectMatrix3x3_InverseTranspose);
	if (bNormalize == true)
	{
		NormalOS = SafeNormalize(NormalOS);
	}
	return NormalOS;
}

float3 TransformBoneToObjectDir(in float3 InDirection, in float3x3 InBoneToObjectMatrix3x3/*(float3x3)GetFinalBoneMatrix(Attribute Input)*/, uniform bool bNormalize = true)
{
	float3 DirOS = mul(InDirection.xyz, InBoneToObjectMatrix3x3);
	if (bNormalize == true)
	{
		DirOS = SafeNormalize(DirOS);
	}
	return DirOS;
}

#endif	//SHADER_USE_SKELETON_INPUT

#endif	//_SHADER_SKINNINGS_HLSL