#ifndef _SHADER_DEFINE_COMMON_HLSL
#define _SHADER_DEFINE_COMMON_HLSL

#define ENABLE_CONSOLE_SHADING

#define MEDIUM_PRECISION_FLT_MAX		(65504.0)
#define MEDIUM_PRECISION_FLT_MIN		(0.00006103515625)

#ifdef ENABLE_CONSOLE_SHADING
#define FLT_EPS							(1e-5)
#define SaturateMediumPrecision(x)		(x)
#else
#define FLT_EPS							(MEDIUM_PRECISION_FLT_MIN)
#define SaturateMediumPrecision(x)		(min(x, MEDIUM_PRECISION_FLT_MAX))
#endif

// Min roughness such that (MIN_PERCEPTUAL_ROUGHNESS^4) > 0 in fp16 (i.e. 2^(-14/4), rounded up)
#ifdef ENABLE_CONSOLE_SHADING
#define MIN_PERCEPTUAL_ROUGHNESS		(0.045)
#define MIN_ROUGHNESS					(0.002025)
#else
#define MIN_PERCEPTUAL_ROUGHNESS		(0.089)
#define MIN_ROUGHNESS					(0.007921)
#endif

#define MIN_N_DOT_V						(1e-4)

#define CUSTOM_SHADER_PI				(3.1415926535897932)
#define CUSTOM_SHADER_PI_HALF			(1.5707963267948966)
#define CUSTOM_SHADER_PI_DERIVATIVE		(0.3183098861837907)

#define GLOBAL_LIGHT_COUNT				(16)
#define GLOBAL_SKELETON_BONE_MAX		(256)

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