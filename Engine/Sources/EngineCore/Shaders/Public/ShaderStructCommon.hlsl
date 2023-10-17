#ifndef _SHADER_STRUCT_COMMON_HLSL
#define _SHADER_STRUCT_COMMON_HLSL

struct Attribute
{
#if (SHADER_USE_POSITION_INPUT)
#if (SHADER_USE_POSITION_INPUT_NUM >= 1)
	float4  Position0		: POSITION0;
#endif
#if (SHADER_USE_POSITION_INPUT_NUM >= 2)
	float4  Position1		: POSITION1;
#endif
#if (SHADER_USE_POSITION_INPUT_NUM >= 3)
	float4  Position2		: POSITION2;
#endif
#if (SHADER_USE_POSITION_INPUT_NUM == 4)
	float4  Position3		: POSITION3;
#endif
#endif	//SHADER_USE_POSITION_INPUT

#if (SHADER_USE_NORMAL_INPUT)
#if (SHADER_USE_NORMAL_INPUT_NUM >= 1)
	float4  Normal0			: NORMAL0;
#endif
#if (SHADER_USE_NORMAL_INPUT_NUM >= 2)
	float4  Normal1			: NORMAL1;
#endif
#if (SHADER_USE_NORMAL_INPUT_NUM >= 3)
	float4  Normal2			: NORMAL2;
#endif
#if (SHADER_USE_NORMAL_INPUT_NUM == 4)
	float4  Normal3			: NORMAL3;
#endif
#endif	//SHADER_USE_NORMAL_INPUT

#if (SHADER_USE_TEXCOORD_INPUT)
#if (SHADER_USE_TEXCOORD_INPUT_NUM == 1)
	float2  Texcoord0		: TEXCOORD0;
#elif (SHADER_USE_TEXCOORD_INPUT_NUM == 2)
	float4  Texcoord0		: TEXCOORD0;
#elif (SHADER_USE_TEXCOORD_INPUT_NUM == 3)
	float4  Texcoord0		: TEXCOORD0;
	float2  Texcoord1		: TEXCOORD1;
#elif (SHADER_USE_TEXCOORD_INPUT_NUM == 4)
	float4  Texcoord0		: TEXCOORD0;
	float4  Texcoord1		: TEXCOORD1;
#endif
#endif	//SHADER_USE_TEXCOORD_INPUT

#if (SHADER_USE_TANGENT_INPUT)
#if (SHADER_USE_TANGENT_INPUT_NUM >= 1)
	float4  Tangent0		: TANGENT0;
#endif
#if (SHADER_USE_TANGENT_INPUT_NUM >= 2)
	float4  Tangent1		: TANGENT1;
#endif
#if (SHADER_USE_TANGENT_INPUT_NUM >= 3)
	float4  Tangent2		: TANGENT2;
#endif
#if (SHADER_USE_TANGENT_INPUT_NUM == 4)
	float4  Tangent3		: TANGENT3;
#endif
#endif	//SHADER_USE_TANGENT_INPUT

#if (SHADER_USE_COLOR_INPUT)
#if (SHADER_USE_COLOR_INPUT_NUM >= 1)
	float4  Color0			: COLOR0;
#endif
#if (SHADER_USE_COLOR_INPUT_NUM >= 2)
	float4  Color1			: COLOR1;
#endif
#if (SHADER_USE_COLOR_INPUT_NUM >= 3)
	float4  Color2			: COLOR2;
#endif
#if (SHADER_USE_COLOR_INPUT_NUM == 4)
	float4  Color3			: COLOR3;
#endif
#endif	//SHADER_USE_COLOR_INPUT

#if (SHADER_USE_SKELETON_INPUT)
#if (SHADER_SKELETON_EFFECT_BONE_NUM == 1)
	uint	BlendIndex0		: BLENDINDICES0;
	float	BlendWeight0	: BLENDWEIGHT0;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 2)
	uint2	BlendIndex0		: BLENDINDICES0;
	float2	BlendWeight0	: BLENDWEIGHT0;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 3)
	uint3	BlendIndex0		: BLENDINDICES0;
	float3	BlendWeight0	: BLENDWEIGHT0;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 4)
	uint4	BlendIndex0		: BLENDINDICES0;
	float4	BlendWeight0	: BLENDWEIGHT0;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 5)
	uint4	BlendIndex0		: BLENDINDICES0;
	uint	BlendIndex1		: BLENDINDICES1;
	float4	BlendWeight0	: BLENDWEIGHT0;
	float	BlendWeight1	: BLENDWEIGHT1;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 6)
	uint4	BlendIndex0		: BLENDINDICES0;
	uint2	BlendIndex1		: BLENDINDICES1;
	float4	BlendWeight0	: BLENDWEIGHT0;
	float2	BlendWeight1	: BLENDWEIGHT1;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 7)
	uint4	BlendIndex0		: BLENDINDICES0;
	uint3	BlendIndex1		: BLENDINDICES1;
	float4	BlendWeight0	: BLENDWEIGHT0;
	float3	BlendWeight1	: BLENDWEIGHT1;
#elif (SHADER_SKELETON_EFFECT_BONE_NUM == 8)
	uint4	BlendIndex0		: BLENDINDICES0;
	uint4	BlendIndex1		: BLENDINDICES1;
	float4	BlendWeight0	: BLENDWEIGHT0;
	float4	BlendWeight1	: BLENDWEIGHT1;
#endif	//SHADER_SKELETON_EFFECT_BONE_NUM
#endif	//SHADER_USE_SKELETON_INPUT
};

struct ShadingLightParams
{
	float3	Color;
	float	Intensity;
	float3	Direction;
	float	Attenuation;
};
struct NormalViewLightDotParams
{
	float	LdotHClamped;
	float	LdotH;
	float	NdotVClamped;
	float	NdotV;
	float	NdotLClamped;
	float	NdotL;
	float	NdotHClamped;
	float	NdotH;
};
struct GBufferTerm
{
	float3	Normal;
	float3	BaseColor;
	float	Roughness;
	float	Reflectance;
	float	Metallicness;
	uint	ShadingModelID;
	float	PrecomputedShadowFactor;
};

struct ShadingPixelParams
{
	float3	ShadingNormal;
	float3	DiffuseColor;
	float3	F0;
	float	PerceptualRoughness;
	float	PerceptualRoughnessUnclamped;
	float	Roughness;
#if (SHADER_HAS_ANISOTROPY)
	float	Anisotropy;
	float3	AnisotropyT;
	float3	AnisotropyB;
#endif
#if (SHADER_HAS_CLEAR_COAT)
	float	ClearCoatStrength;
	float	ClearCoatPerceptualRoughness;
	float	ClearCoatRoughness;
#if (SHADER_HAS_CLEAR_COAT_NORMAL)
	float3	ClearCoatNormal;
#endif
#endif
#if (SHADER_HAS_SHEEN_COLOR)
	float3	SheenColor;
#endif
#if (SHADER_HAS_SUBSURFACE_COLOR)
	float3	SubsurfaceColor;
#endif
};

#endif	//_SHADER_STRUCT_COMMON_HLSL