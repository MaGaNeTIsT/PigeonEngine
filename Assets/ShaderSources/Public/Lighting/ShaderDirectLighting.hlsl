#ifndef SHADER_DIRECT_LIGHTING_HLSL
#define SHADER_DIRECT_LIGHTING_HLSL

// ---------------------------------------------------------------------------
// Direct lighting: one light at a time.
//
// A domain's ShadeSurface walks the light lists and calls this once per light,
// which keeps the loop out of the BRDF and the BRDF out of the loop. The
// contribution is zero for a light whose channel bit the surface does not take
// part in, so the caller does not need to test the mask itself.
//
// The light channel mask is what makes "this surface does not receive indirect
// light" a compile time decision the encoder made, applied here at runtime with
// a single bit test instead of a branch per light type.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/System/ShaderBindless.hlsl"
#include "Public/Core/ShaderMath.hlsl"
#include "Public/Core/ShaderPacking.hlsl"
#include "Public/Surface/SurfaceData.hlsl"
#include "Public/Surface/SurfaceCommon.hlsl"
#include "Public/BRDF/ShaderBxDF.hlsl"
#include "Public/Lighting/ShaderLightData.hlsl"
#include "Public/Lighting/ShaderShadows.hlsl"

// Ceiling on the direct specular term, in the scene referred domain the lighting
// pass writes.
//
// The value it guards against is physical and enormous. A mirror reflecting the sun
// returns the sun's own disc luminance, E / Omega_sun, which for 100 000 lux over the
// sun's 6.76e-5 sr is 1.5e9 cd/m2. Nothing holds that: R16G16B16A16_FLOAT tops out at
// 65504, and the value past that ceiling is an infinity, which meets the tone mapper as
// a NaN rather than as a saturated white pixel.
//
// 60000 sits just below the format's ceiling and far above display white, which is about
// 25 350 cd/m2 at EV100 14.64. The price is stated rather than hidden: at that exposure the
// ceiling tone maps to about 236/255, where the unclamped value would have reached 255, so the
// last few percent of a mirror highlight are compressed away. That is unavoidable - no format in
// the frame holds the physical value - and it only touches pixels already at the top of the
// display range. Everything below the ceiling is untouched, so this is a range guard and not a
// shading decision.
//
// The physical half of the same fix is the angular widening in ComputeLightSpecularAlphaFloor,
// which gives the highlight its real size. The two are not alternatives: a light with no angular
// size still needs the guard, and the guard alone would leave a delta light's highlight an
// unbounded point.
#ifndef PE_LIGHTING_SPECULAR_MAX
#define PE_LIGHTING_SPECULAR_MAX			(60000.0)
#endif	//PE_LIGHTING_SPECULAR_MAX

// How much of the base layer survives under the fresnel reflection of the light.
// One minus the fresnel term, which is what keeps the energy balance between the
// diffuse and the specular lobe.
float ComputeDiffuseFresnelWeight(in float InLdotH, in float3 InF0)
{
	float3 F = FresnelSchlick(InLdotH, InF0);
	return 1.0 - Luminance(F);
}

// Applies one light to a surface.
//
// InShadow is passed in rather than sampled here, because how the shadow term is
// produced is a property of the light and of the pass, not of the shading model.
// NoShadow is a constant so a caller that has no shadow data compiles the same
// code path with the multiply folded away.
void ShadeOneLight(in SurfaceData InSurface, in float3 InWorldPosition, in float3 InViewDirection,
	in LightData InLight, in float3 InF0, in bool InIsAnisotropic, in float3 InAnisotropicT, in float3 InAnisotropicB,
	in float InAnisotropy, inout ShadingResult InOutResult)
{
	if ((InLight.ChannelMask & InSurface.LightChannelMask) == 0u)
	{
		return;
	}

	// Past a local light's reach the packet arrives with a zero illuminance, and a zero
	// contribution would otherwise still pay for a full GGX evaluation. Summed rather than
	// tested per channel, because a colour with one live channel is still a live light.
	if ((InLight.Illuminance.r + InLight.Illuminance.g + InLight.Illuminance.b) <= 0.0)
	{
		return;
	}

	float3 LightDirection = InLight.Direction;
	float3 HalfVector = ComputeHalfVector(InViewDirection, LightDirection);

	float NdotL = dot(InSurface.Normal, LightDirection);
	float NdotV = ClampNdotV(dot(InSurface.Normal, InViewDirection));
	float NdotH = saturate(dot(InSurface.Normal, HalfVector));
	float LdotH = saturate(dot(LightDirection, HalfVector));

	// A surface facing away from the light receives nothing. Clamping NdotL to
	// zero here rather than at the end keeps the specular term from going
	// negative behind the terminator.
	float NdotLClamped = saturate(NdotL);
	if (NdotLClamped <= 0.0)
	{
		return;
	}

	// Illuminance at the surface [lux], including the light's own shadow term.
	float3 Illuminance = InLight.Illuminance * InLight.Shadow * NdotLClamped;

	// The specular lobe may be no sharper than the light that made it: a mirror reflecting a
	// sun of finite angular size returns a disc of that size, not a point. Substituting this
	// for the material roughness is what gives the highlight its real width, and it is what
	// keeps the GGX peak finite as roughness goes to zero.
	const float SpecularAlpha = max(InSurface.Roughness, InLight.SpecularAlphaFloor);

	float3 Specular;
	if (InIsAnisotropic)
	{
		Specular = SpecularBRDFAnisotropic(NdotLClamped, NdotV, NdotH, LdotH, InF0, SpecularAlpha,
			InSurface.Normal, HalfVector, InViewDirection, LightDirection,
			InAnisotropicT, InAnisotropicB, InAnisotropy);
	}
	else
	{
		Specular = SpecularBRDFIsotropic(NdotLClamped, NdotV, NdotH, LdotH, InF0, SpecularAlpha,
			InSurface.Normal, HalfVector);
	}

	// The diffuse lobe is scaled by the energy the specular lobe did not take.
	float DiffuseWeight = ComputeDiffuseFresnelWeight(LdotH, InF0) * DiffuseLambert();

	InOutResult.Diffuse += InSurface.DiffuseColor * DiffuseWeight * Illuminance;
	InOutResult.Specular += min(Specular * Illuminance, PE_LIGHTING_SPECULAR_MAX);
}

// Sums the direct contribution of every directional light.
//
// The light buffers are declared by C_Lighting.json, so they are plain globals
// here. A pixel stage that never calls this culls both buffers, which is why a
// depth only pass pays nothing for the lighting declaration.
void ShadeDirectionalLights(in SurfaceData InSurface, in float3 InWorldPosition, in float3 InViewDirection,
	in float3 InF0, in bool InIsAnisotropic, in float3 InAnisotropicT, in float3 InAnisotropicB,
	in float InAnisotropy, inout ShadingResult InOutResult)
{
	for (uint LightIndex = 0u; LightIndex < DirectionalLightCount; ++LightIndex)
	{
		DirectionalLightData StoredLight = DecodeDirectionalLight(DirectionalLights[LightIndex]);
		LightData Light = BuildDirectionalLightData(StoredLight, SampleDirectionalLightShadow(StoredLight, InWorldPosition));
		ShadeOneLight(InSurface, InWorldPosition, InViewDirection, Light, InF0,
			InIsAnisotropic, InAnisotropicT, InAnisotropicB, InAnisotropy, InOutResult);
	}
}

// Sums the direct contribution of every local light.
void ShadeLocalLights(in SurfaceData InSurface, in float3 InWorldPosition, in float3 InViewDirection,
	in float3 InF0, in bool InIsAnisotropic, in float3 InAnisotropicT, in float3 InAnisotropicB,
	in float InAnisotropy, inout ShadingResult InOutResult)
{
	for (uint LightIndex = 0u; LightIndex < LocalLightCount; ++LightIndex)
	{
		LocalLightData StoredLight = DecodeLocalLight(LocalLights[LightIndex]);
		LightData Light = BuildLocalLightData(StoredLight, InWorldPosition, SampleLocalLightShadow(StoredLight, InWorldPosition));
		ShadeOneLight(InSurface, InWorldPosition, InViewDirection, Light, InF0,
			InIsAnisotropic, InAnisotropicT, InAnisotropicB, InAnisotropy, InOutResult);
	}
}
#endif	//SHADER_DIRECT_LIGHTING_HLSL
