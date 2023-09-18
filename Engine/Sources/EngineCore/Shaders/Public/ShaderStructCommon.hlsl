#ifndef _SHADER_STRUCT_COMMON_HLSL
#define _SHADER_STRUCT_COMMON_HLSL

struct ShadingLightParams
{
	float4	ColorIntensity;
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
	float3	NormalWS;
	float3	BaseColor;
	float	Roughness;
	float	Metallicness;
	float	AmbientOcclusion;
};
struct PixelParams
{
	float3	GeometricNormalWS;
	float3	ShadingNormalWS;
	float3	EmissiveColor;
	float	AmbientOcclusion;
	float3	DiffuseColor;
	float3	F0;
	float	PerceptualRoughness;
	float	PerceptualRoughnessUnclamped;
	float	Roughness;
#ifdef MATERIAL_HAS_ANISOTROPY
	float	Anisotropy;
	float3	AnisotropyT;
	float3	AnisotropyB;
#endif
#ifdef MATERIAL_HAS_CLEAR_COAT
	float	ClearCoatStrength;
	float	ClearCoatPerceptualRoughness;
	float	ClearCoatRoughness;
#ifdef MATERIAL_HAS_CLEAR_COAT_NORMAL
	float3	ClearCoatNormal;
#endif
#endif
#ifdef MATERIAL_HAS_SHEEN_COLOR
	float3	SheenColor;
#endif
#ifdef MATERIAL_HAS_SUBSURFACE_COLOR
	float3	SubsurfaceColor;
#endif
};

#endif