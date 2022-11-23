#ifndef _SHADER_SHADING_FUNCTIONS_HLSL
#define _SHADER_SHADING_FUNCTIONS_HLSL

#include "./ShaderDefCommon.hlsl"
#include "./ShaderFunctions.hlsl"
#include "./ShaderBxDF.hlsl"

NormalViewLightDotParams InitNormalViewLightDotParams(const float3 normal, const float3 lightDir, const float3 viewDir)
{
	float3 halfVec = SafeNormalize(lightDir + viewDir);
	NormalViewLightDotParams output;
	output.LdotH = saturate(dot(lightDir, halfVec));
	output.NdotV = abs(dot(normal, viewDir)) + 1e-5;
	output.NdotL = saturate(dot(normal, lightDir));
	output.NdotH = saturate(dot(normal, halfVec));
	return output;
}

NormalViewLightDotParams_Anisotropic InitNormalViewLightDotParams_Anisotropic(const float3 normal, const float3 lightDir, const float3 viewDir, const float3 tangent, const float3 binormal)
{
	float3 halfVec = SafeNormalize(lightDir + viewDir);
	NormalViewLightDotParams_Anisotropic output;
	output.LdotH = saturate(dot(lightDir, halfVec));
	output.NdotV = abs(dot(normal, viewDir)) + 1e-5;
	output.NdotL = saturate(dot(normal, lightDir));
	output.NdotH = saturate(dot(normal, halfVec));
	output.TdotV = abs(dot(tangent, viewDir)) + 1e-5;
	output.TdotL = saturate(dot(tangent, lightDir));
	output.TdotH = saturate(dot(tangent, halfVec));
	output.BdotV = abs(dot(binormal, viewDir)) + 1e-5;
	output.BdotL = saturate(dot(binormal, lightDir));
	output.BdotH = saturate(dot(binormal, halfVec));
	return output;
}

float3 RemappingDiffuseColor(float3 albedo, float metallicness)
{
	return clamp(albedo * (1.0 - metallicness), 1.0 / 25.5, 24.0 / 25.5);
}

float3 RemappingSpecularColor(float3 albedo, float metallicness)
{
	return clamp(albedo * metallicness, 17 / 25.5, 1.0);
}

float RemappingFresnelf0DielectricsFast(float reflectance)
{
	return (0.16 * reflectance * reflectance);
}

float3 RemappingFresnelf0Conductors(float3 albedo, float metallicness)
{
	return (albedo * metallicness);
}

float3 RemappingFresnelf0(float reflectance, float3 albedo, float metallicness)
{
	return (0.16 * reflectance * reflectance * (1.0 - metallicness) + albedo * metallicness);
}

float3 SurfaceShading_Cloth(const PixelParams pixel, const NormalViewLightDotParams content, const ShadingLightParams light, float ambientOcclusion)
{
	BRDFTerm brdf = ClothBRDF(content, pixel.Roughness, pixel.SheenColor);

	// We do not multiply the diffuse term by the Fresnel term as discussed in
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	// The effect is fairly subtle and not deemed worth the cost for mobile
	float3 diffuse = brdf.Fd * pixel.DiffuseColor;

#ifdef CLOTH_HAS_SUBSURFACE_COLOR
	// Cheap subsurface scatter
	diffuse *= saturate(pixel.SubsurfaceColor + content.NdotL);
	// We need to apply NoL separately to the specular lobe since we already took
	// it into account in the diffuse lobe
	float3 color = diffuse + brdf.Fr * content.NdotL;
	color *= light.ColorIntensity.rgb * (light.ColorIntensity.w * light.Attenuation * ambientOcclusion);
#else
	float3 color = diffuse + brdf.Fr;
	color *= light.ColorIntensity.rgb * (light.ColorIntensity.w * light.Attenuation * content.NdotL * ambientOcclusion);
#endif

	return color;
}

#endif