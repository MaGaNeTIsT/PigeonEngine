#ifndef _SHADER_SHADING_FUNCTIONS_HLSL
#define _SHADER_SHADING_FUNCTIONS_HLSL

#include "./ShaderDefCommon.hlsl"
#include "./ShaderFunctions.hlsl"
#include "./ShaderBxDF.hlsl"

float ClampNdotV(float NdotV)
{
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	return max(NdotV, MIN_N_DOT_V);
}

float3 ClampDiffuseColor(const float3 diffuseColor)
{
	return (clamp(diffuseColor, 1.0 / 25.5, 24.0 / 25.5));
}

float3 ClampSpecularColor(const float3 specularColor)
{
	return (clamp(specularColor, 17 / 25.5, 1.0));
}

float3 ComputeDiffuseColor(const float3 baseColor, float metallicness)
{
	return (baseColor * (1.0 - metallicness));
}

float ComputeFresnelF0Dielectric(float reflectance)
{
	return (0.16 * reflectance * reflectance);
}

float3 ComputeFresnelF0Conductor(const float3 baseColor, float metallicness)
{
	return (baseColor * metallicness);
}

float3 ComputeFresnelF0(float reflectance, const float3 baseColor, float metallicness)
{
	return (reflectance * (1.0 - metallicness) + baseColor * metallicness);
}

float ComputeMetallicFromSpecularColor(const float3 specularColor)
{
	return (Max3(specularColor));
}

float RemappingRoughnessFromSmoothness(float smoothness)
{
	return (1.0 - smoothness);
}

float ComputePerceptualRoughnessToRoughness(float perceptualRoughness)
{
	return (perceptualRoughness * perceptualRoughness);
}

float ComputeRoughnessToPerceptualRoughness(float roughness)
{
	return (sqrt(roughness));
}

float ComputeIORToF0(float transmittedIOR, float incidentIOR)
{
	return (Power2((transmittedIOR - incidentIOR) / (transmittedIOR + incidentIOR)));
}

float ComputeF0ToIOR(float f0)
{
	float r = sqrt(f0);
	return ((1.0 + r) / (1.0 - r));
}

float3 ComputeF0ClearCoatToSurface(const float3 f0)
{
	// Approximation of ComputeIORToF0(ComputeF0ToIOR(F0), 1.5)
	// This assumes that the clear coat layer has an IOR of 1.5
#ifdef ENABLE_CONSOLE_SHADING
	return saturate(f0 * (f0 * (0.941892 - 0.263008 * f0) + 0.346479) - 0.0285998);
#else
	return saturate(f0 * (f0 * 0.526868 + 0.529324) - 0.0482256);
#endif
}

NormalViewLightDotParams InitNormalViewLightDotParams(const float3 normal, const float3 lightDir, const float3 viewDir, const float3 halfVector)
{
	NormalViewLightDotParams output;

	output.LdotH		= dot(lightDir, halfVector);
	output.LdotHClamped	= saturate(output.LdotH);
	output.NdotV		= dot(normal, viewDir);
	output.NdotVClamped = ClampNdotV(output.NdotV);
	output.NdotL		= dot(normal, lightDir);
	output.NdotLClamped	= saturate(output.NdotL);
	output.NdotH		= dot(normal, halfVector);
	output.NdotHClamped	= saturate(output.NdotH);

	return output;
}

float3 SurfaceShading_DefaultLit(
	const PixelParams pixel,
	const NormalViewLightDotParams content,
	const ShadingLightParams light,
	const float3 viewDir,
	const float3 halfVector,
#ifdef MATERIAL_HAS_ANISOTROPY
	float3 anisotropicT,
	float3 anisotropicB,
	float anisotropy,
#endif
#ifdef MATERIAL_HAS_CLEAR_COAT
	float clearCoatStrength,
	float clearCoatRoughness,
#ifdef MATERIAL_HAS_CLEAR_COAT_NORMAL
	float3 clearCoatNormal,
#endif
#endif
	float shadowOcclusion)
{
#ifdef MATERIAL_HAS_ANISOTROPY
	float3 Fr = AnisotropicBRDF(content, pixel, viewDir, light.Direction, halfVector, anisotropicT, anisotropicB, anisotropy);
#else
	float3 Fr = IsotropicBRDF(content, pixel, pixel.ShadingNormalWS, halfVector);
#endif
	float3 Fd = DiffuseBRDF(content, pixel) * pixel.DiffuseColor;

	float energyCompensation = 1.0;
	float3 color = Fd + Fr * energyCompensation;

#ifdef MATERIAL_HAS_CLEAR_COAT
	float Fcc;
#ifdef MATERIAL_HAS_CLEAR_COAT_NORMAL
	float clearCoat = ClearCoatBRDF(content, pixel, pixel.ShadingNormalWS, halfVector, clearCoatNormal, clearCoatRoughness, clearCoatStrength, Fcc);
#else
	float clearCoat = ClearCoatBRDF(content, pixel, pixel.ShadingNormalWS, halfVector, float3(1.0, 0.0, 0.0), clearCoatRoughness, clearCoatStrength, Fcc);
#endif
	float attenuation = 1.0 - Fcc;

#if defined(MATERIAL_HAS_NORMALMAP) || defined(MATERIAL_HAS_CLEAR_COAT_NORMAL)
	color *= attenuation * content.NdotLClamped;

	// If the material has a normal map, we want to use the geometric normal
	// Instead to avoid applying the normal map details to the clear coat layer
	float clearCoatNdotL = saturate(dot(clearCoatNormal, light.Direction));
	color += clearCoat * clearCoatNdotL;

	// Early exit to avoid the extra multiplication by NdotL
	return ((color * light.ColorIntensity.rgb) * (light.ColorIntensity.w * light.Attenuation * shadowOcclusion));
#else
	color *= attenuation;
	color += clearCoat;
#endif
#endif

	return ((color * light.ColorIntensity.rgb) * (light.ColorIntensity.w * light.Attenuation * content.NdotLClamped * shadowOcclusion));
}

float3 SurfaceShading_Cloth(const PixelParams pixel, const NormalViewLightDotParams content, const ShadingLightParams light, float shadowOcclusion)
{
	float3 Fr = ClothSheenBRDF(content, pixel);

	// Diffuse BRDF
	float diffuse = DiffuseBRDF(content, pixel);
#ifdef MATERIAL_HAS_SUBSURFACE_COLOR
	// Energy conservative wrap diffuse to simulate subsurface scattering
	diffuse *= Diffuse_Wrap(content.NdotL, 0.5);
#endif

	// We do not multiply the diffuse term by the Fresnel term as discussed in
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	// The effect is fairly subtle and not deemed worth the cost for mobile
	float3 Fd = diffuse * pixel.DiffuseColor;

#ifdef MATERIAL_HAS_SUBSURFACE_COLOR
	// Cheap subsurface scatter
	Fd *= saturate(pixel.SubsurfaceColor + content.NdotLClamped);
	// We need to apply NoL separately to the specular lobe since we already took
	// it into account in the diffuse lobe
	float3 color = Fd + Fr * content.NdotL;
	color *= light.ColorIntensity.rgb * (light.ColorIntensity.w * light.Attenuation * shadowOcclusion);
#else
	float3 color = Fd + Fr;
	color *= light.ColorIntensity.rgb * (light.ColorIntensity.w * light.Attenuation * content.NdotLClamped * shadowOcclusion);
#endif

	return color;
}

#endif