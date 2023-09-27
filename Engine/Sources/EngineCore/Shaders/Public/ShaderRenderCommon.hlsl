#ifndef _SHADER_RENDER_COMMON_HLSL
#define _SHADER_RENDER_COMMON_HLSL

#ifndef SHADER_HAS_ANISOTROPY
#define SHADER_HAS_ANISOTROPY				0
#endif	//SHADER_HAS_ANISOTROPY

#ifndef SHADER_HAS_CLEAR_COAT
#define SHADER_HAS_CLEAR_COAT				0
#endif	//SHADER_HAS_CLEAR_COAT

#if (SHADER_HAS_CLEAR_COAT)
#ifndef SHADER_HAS_CLEAR_COAT_NORMAL
#define SHADER_HAS_CLEAR_COAT_NORMAL		0
#endif	//SHADER_HAS_CLEAR_COAT_NORMAL
#else
#ifdef SHADER_HAS_CLEAR_COAT_NORMAL
#undef SHADER_HAS_CLEAR_COAT_NORMAL
#endif	//SHADER_HAS_CLEAR_COAT_NORMAL
#define SHADER_HAS_CLEAR_COAT_NORMAL		0
#endif	//SHADER_HAS_CLEAR_COAT

#ifndef SHADER_HAS_SHEEN_COLOR
#define SHADER_HAS_SHEEN_COLOR				0
#endif	//SHADER_HAS_SHEEN_COLOR

#ifndef SHADER_HAS_SUBSURFACE_COLOR
#define SHADER_HAS_SUBSURFACE_COLOR			0
#endif	//SHADER_HAS_SUBSURFACE_COLOR

#include "./ShaderStructCommon.hlsl"
#include "./ShaderFunctions.hlsl"
#include "./ShaderShadingFunctions.hlsl"

NormalViewLightDotParams InitNormalViewLightDotParams(const float3 InNormal,
	const float3 InLightDirection, const float3 InViewDirection, const float3 InHalfVector)
{
	NormalViewLightDotParams Result;

	Result.LdotH		= dot(InLightDirection, InHalfVector);
	Result.LdotHClamped	= saturate(Result.LdotH);
	Result.NdotV		= dot(InNormal, InViewDirection);
	Result.NdotVClamped	= ClampNdotV(Result.NdotV);
	Result.NdotL		= dot(InNormal, InLightDirection);
	Result.NdotLClamped	= saturate(Result.NdotL);
	Result.NdotH		= dot(InNormal, InHalfVector);
	Result.NdotHClamped	= saturate(Result.NdotH);

	return Result;
}

GBufferTerm InitGBuffer(const float3 InNormal, const float3 InBaseColor,
	const float InRoughness, const float InMetallicness, const uint InShadingModelID,
	float InReflectance = 1.0, float InPrecomputedShadowFactor = 1.0)
{
	GBufferTerm Result;

	Result.Normal.xyz				= InNormal.xyz;
	Result.BaseColor.rgb			= InBaseColor.rgb;
	Result.Roughness				= InRoughness;
	Result.Reflectance				= InReflectance;
	Result.Metallicness				= InMetallicness;
	Result.ShadingModelID			= InShadingModelID;
	Result.PrecomputedShadowFactor	= InPrecomputedShadowFactor;

	return Result;
}

ShadingLightParams InitShadingLightParams(const float3 InDirection,
	const float3 InColor, const float InIntensity, float InAttenuation = 1.0)
{
	ShadingLightParams Result;

	Result.Color.rgb		= InColor.rgb;
	Result.Intensity		= InIntensity;
	Result.Direction.xyz	= InDirection.xyz;
	Result.Attenuation		= InAttenuation;

	return Result;
}

float3 ShadingDefaultLight(const GBufferTerm InGBuffer, const ShadingLightParams InLight, const float3 InViewDirection, const float3 InHalfVector)
{
	NormalViewLightDotParams NVLDotParams = InitNormalViewLightDotParams(InGBuffer.Normal.xyz,
		InLight.Direction.xyz, InViewDirection.xyz, InHalfVector.xyz);

	ShadingPixelParams UsedPixelParams;
	{
		float F0Dielectric = ComputeFresnelF0Dielectric(InGBuffer.Reflectance);

		UsedPixelParams.ShadingNormal.xyz				= InGBuffer.Normal.xyz;
		UsedPixelParams.DiffuseColor					= ComputeDiffuseColor(InGBuffer.BaseColor, InGBuffer.Metallicness);
		UsedPixelParams.F0								= ComputeFresnelF0(F0Dielectric, InGBuffer.BaseColor, InGBuffer.Metallicness);
		UsedPixelParams.PerceptualRoughnessUnclamped	= InGBuffer.Roughness;
		UsedPixelParams.PerceptualRoughness				= clamp(UsedPixelParams.PerceptualRoughnessUnclamped, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
		UsedPixelParams.Roughness						= ComputePerceptualRoughnessToRoughness(UsedPixelParams.PerceptualRoughness);
	}

	float3 Color = SurfaceShading_Standard(
		UsedPixelParams,
		NVLDotParams,
		InLight,
		InViewDirection,
		InHalfVector,
		InGBuffer.PrecomputedShadowFactor);

	return Color;
}

#endif	//_SHADER_RENDER_COMMON_HLSL