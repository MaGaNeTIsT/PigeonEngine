#ifndef _SHADER_STRUCT_COMMON_HLSL
#define _SHADER_STRUCT_COMMON_HLSL

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