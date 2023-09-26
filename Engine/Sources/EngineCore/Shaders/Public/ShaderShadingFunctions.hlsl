#ifndef _SHADER_SHADING_FUNCTIONS_HLSL
#define _SHADER_SHADING_FUNCTIONS_HLSL

#include "./ShaderStructCommon.hlsl"
#include "./ShaderFunctions.hlsl"
#include "./ShaderBxDF.hlsl"

float ClampNdotV(float NdotV)
{
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	return max(NdotV, MIN_N_DOT_V);
}

float3 ClampDiffuseColor(const float3 InDiffuseColor)
{
	return (clamp(InDiffuseColor, 1.0 / 25.5, 24.0 / 25.5));
}

float3 ClampSpecularColor(const float3 InSpecularColor)
{
	return (clamp(InSpecularColor, 17.0 / 25.5, 1.0));
}

float3 ComputeDiffuseColor(const float3 InBaseColor, float InMetallicness)
{
	return (InBaseColor * (1.0 - InMetallicness));
}

float ComputeFresnelF0Dielectric(float InReflectance)
{
	return (0.16 * InReflectance * InReflectance);
}

float3 ComputeFresnelF0Conductor(const float3 InBaseColor, float InMetallicness)
{
	return (InBaseColor * InMetallicness);
}

float3 ComputeFresnelF0(float InReflectance, const float3 InBaseColor, float InMetallicness)
{
	return (InReflectance * (1.0 - InMetallicness) + InBaseColor * InMetallicness);
}

float ComputeMetallicFromSpecularColor(const float3 InSpecularColor)
{
	return (Max3(InSpecularColor));
}

float RemappingRoughnessFromSmoothness(float InSmoothness)
{
	return (1.0 - InSmoothness);
}

float ComputePerceptualRoughnessToRoughness(float InPerceptualRoughness)
{
	return (InPerceptualRoughness * InPerceptualRoughness);
}

float ComputeRoughnessToPerceptualRoughness(float InRoughness)
{
	return (sqrt(InRoughness));
}

float ComputeIORToF0(float InTransmittedIOR, float InIncidentIOR)
{
	return (Power2((InTransmittedIOR - InIncidentIOR) / (InTransmittedIOR + InIncidentIOR)));
}

float ComputeF0ToIOR(float InF0)
{
	float r = sqrt(InF0);
	return ((1.0 + r) / (1.0 - r));
}

float3 ComputeF0ClearCoatToSurface(const float3 InF0)
{
	// Approximation of ComputeIORToF0(ComputeF0ToIOR(F0), 1.5)
	// This assumes that the clear coat layer has an IOR of 1.5
#if (ENABLE_CONSOLE_SHADING)
	return saturate(InF0 * (InF0 * (0.941892 - 0.263008 * InF0) + 0.346479) - 0.0285998);
#else
	return saturate(InF0 * (InF0 * 0.526868 + 0.529324) - 0.0482256);
#endif
}

float3 SurfaceShading_Standard(
	const ShadingPixelParams InPixelParams,
	const NormalViewLightDotParams InNVLContent,
	const ShadingLightParams InLightParams,
	const float3 InViewDirection,
	const float3 InHalfVector,
#if (SHADER_HAS_ANISOTROPY)
	float3 InAnisotropicT,
	float3 InAnisotropicB,
	float InAnisotropy,
#endif
#if (SHADER_HAS_CLEAR_COAT)
	float InClearCoatStrength,
	float InClearCoatRoughness,
#if (SHADER_HAS_CLEAR_COAT_NORMAL)
	float3 InClearCoatNormal,
#endif
#endif
	float InShadowOcclusion)
{
#if (SHADER_HAS_ANISOTROPY)
	float3 Fr = AnisotropicBRDF(InNVLContent, InPixelParams, InViewDirection, InLightParams.Direction, InHalfVector, InAnisotropicT, InAnisotropicB, InAnisotropy);
#else
	float3 Fr = IsotropicBRDF(InNVLContent, InPixelParams, InPixelParams.ShadingNormal, InHalfVector);
#endif
	float3 Fd = DiffuseBRDF(InNVLContent, InPixelParams) * InPixelParams.DiffuseColor;

	float EnergyCompensation = 1.0;
	float3 Color = Fd + Fr * EnergyCompensation;

#if (SHADER_HAS_CLEAR_COAT)
	float Fcc;
#if (SHADER_HAS_CLEAR_COAT_NORMAL)
	float ClearCoat = ClearCoatBRDF(InNVLContent, InPixelParams, InPixelParams.ShadingNormal, InHalfVector, InClearCoatNormal, InClearCoatRoughness, InClearCoatStrength, Fcc);
#else
	float ClearCoat = ClearCoatBRDF(InNVLContent, InPixelParams, InPixelParams.ShadingNormal, InHalfVector, float3(1.0, 0.0, 0.0), InClearCoatRoughness, InClearCoatStrength, Fcc);
#endif
	float Attenuation = 1.0 - Fcc;

#if (SHADER_HAS_CLEAR_COAT_NORMAL)
	Color *= Attenuation * InNVLContent.NdotLClamped;

	// If the material has a normal map, we want to use the geometric normal
	// Instead to avoid applying the normal map details to the clear coat layer
	float ClearCoatNdotL = saturate(dot(InClearCoatNormal, InLightParams.Direction));
	Color += ClearCoat * ClearCoatNdotL;

	// Early exit to avoid the extra multiplication by NdotL
	return ((Color * InLightParams.Color.rgb) * (InLightParams.Intensity * InLightParams.Attenuation * InShadowOcclusion));
#else
	Color *= Attenuation;
	Color += ClearCoat;
#endif
#endif

	return ((Color * InLightParams.Color.rgb) * (InLightParams.Intensity * InLightParams.Attenuation * InNVLContent.NdotLClamped * InShadowOcclusion));
}

float3 SurfaceShading_Cloth(const ShadingPixelParams InPixelParams, const NormalViewLightDotParams InNVLContent, const ShadingLightParams InLightParams, float InShadowOcclusion)
{
	float3 Fr = ClothSheenBRDF(InNVLContent, InPixelParams);

	// Diffuse BRDF
	float Diffuse = DiffuseBRDF(InNVLContent, InPixelParams);
#if (SHADER_HAS_SUBSURFACE_COLOR)
	// Energy conservative wrap diffuse to simulate subsurface scattering
	Diffuse *= Diffuse_Wrap(InNVLContent.NdotL, 0.5);
#endif

	// We do not multiply the diffuse term by the Fresnel term as discussed in
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	// The effect is fairly subtle and not deemed worth the cost for mobile
	float3 Fd = Diffuse * InPixelParams.DiffuseColor;

#if (SHADER_HAS_SUBSURFACE_COLOR)
	// Cheap subsurface scatter
	Fd *= saturate(InPixelParams.SubsurfaceColor + InNVLContent.NdotLClamped);
	// We need to apply NoL separately to the specular lobe since we already took
	// it into account in the diffuse lobe
	float3 Color = Fd + Fr * InNVLContent.NdotLClamped;
	Color *= InLightParams.Color.rgb * (InLightParams.Intensity * InLightParams.Attenuation * InShadowOcclusion);
#else
	float3 Color = Fd + Fr;
	Color *= InLightParams.Color.rgb * (InLightParams.Intensity * InLightParams.Attenuation * InNVLContent.NdotLClamped * InShadowOcclusion);
#endif

	return Color;
}

float3 SurfaceShading_ClothAnisotropic(
	const ShadingPixelParams InPixelParams,
	const NormalViewLightDotParams InNVLContent,
	const ShadingLightParams InLightParams,
	const float3 InViewDirection,
	const float3 InHalfVector,
#if (SHADER_HAS_ANISOTROPY)
	float3 InAnisotropicT,
	float3 InAnisotropicB,
	float InAnisotropy,
#endif
	float InShadowOcclusion)
{
	float3 Fr = ClothSheenBRDF(InNVLContent, InPixelParams);

#if (SHADER_HAS_ANISOTROPY)
	Fr += AnisotropicBRDF(InNVLContent, InPixelParams, InViewDirection, InLightParams.Direction, InHalfVector, InAnisotropicT, InAnisotropicB, InAnisotropy);
#endif

	// Diffuse BRDF
	float Diffuse = DiffuseBRDF(InNVLContent, InPixelParams);
#if (SHADER_HAS_SUBSURFACE_COLOR)
	// Energy conservative wrap diffuse to simulate subsurface scattering
	Diffuse *= Diffuse_Wrap(InNVLContent.NdotL, 0.5);
#endif

	// We do not multiply the diffuse term by the Fresnel term as discussed in
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	// The effect is fairly subtle and not deemed worth the cost for mobile
	float3 Fd = Diffuse * InPixelParams.DiffuseColor;

#if (SHADER_HAS_SUBSURFACE_COLOR)
	// Cheap subsurface scatter
	Fd *= saturate(InPixelParams.SubsurfaceColor + InNVLContent.NdotLClamped);
	// We need to apply NoL separately to the specular lobe since we already took
	// it into account in the diffuse lobe
	float3 Color = Fd + Fr * InNVLContent.NdotLClamped;
	Color *= InLightParams.Color.rgb * (InLightParams.Intensity * InLightParams.Attenuation * InShadowOcclusion);
#else
	float3 Color = Fd + Fr;
	Color *= InLightParams.Color.rgb * (InLightParams.Intensity * InLightParams.Attenuation * InNVLContent.NdotLClamped * InShadowOcclusion);
#endif

	return Color;
}

#endif	//_SHADER_SHADING_FUNCTIONS_HLSL