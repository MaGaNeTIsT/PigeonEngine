#ifndef SHADER_INDIRECT_DIFFUSE_HLSL
#define SHADER_INDIRECT_DIFFUSE_HLSL

// ---------------------------------------------------------------------------
// The indirect diffuse term: what the environment gives a surface that no light
// reaches directly.
//
// This is the consuming half of the environment bake. The bake - a compute
// dispatch over the sky's cubemap - produces nine spherical harmonics radiance
// coefficients; this file evaluates them at the surface normal, turns them into
// irradiance, and weighs the result by the diffuse BRDF.
//
// ---------------------------------------------------------------------------
// What is deliberately absent
//
// There is no ambient parameter here and there should never be one. Indirect light
// is a lighting contribution like any other, so it arrives through the lighting
// path rather than as a field a material declares, and a material that wants none
// of it clears LIGHT_CHANNEL_INDIRECT on its surface. That bit has existed since
// the GBuffer was laid out and this is its first producer.
//
// There is no specular term yet either. The environment's specular reflection
// needs the prefiltered mip chain, which is the next stage; this diffuse half
// stands on its own, and adding that one does not change this file.
//
// ---------------------------------------------------------------------------
// Units, and why the environment arrives in two pieces
//
// The coefficients carry the environment's shape: radiance normalized by the bake
// so its mean is one. The mean itself travels in IndirectLightingParams, in
// cd/m2, and is multiplied in here - which is what lets the sky's brightness be
// edited without re-running the bake.
//
// EvaluateIrradianceSH already applies the clamped cosine kernel, so what it
// returns is irradiance [lux]. Scaling that by the mean luminance of the
// environment keeps it in lux, and the diffuse BRDF (albedo over pi) turns it into
// luminance [cd/m2], which is the unit the scene colour target holds.
//
// The sky pass applies the same tint and the same mean luminance to the cubemap
// directly, so what a surface receives from the environment and what the camera
// sees of it cannot disagree about how bright the sky is.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/GBuffer/ShaderGBufferIO.hlsl"
#include "Public/Surface/SurfaceData.hlsl"
#include "Public/BRDF/ShaderIndirectBxDF.hlsl"
#include "Public/Lighting/ShaderSphericalHarmonics.hlsl"

// The environment's irradiance, in the units the coefficients and the parameter
// block together define.
//
// The coefficients are loaded into registers rather than passed as the buffer: an
// HLSL array parameter is what the basis evaluator takes, because the evaluator
// must not know how the environment is stored - and a structured buffer cannot be
// handed to one anyway. Nine loads, once per pixel, from a buffer that is 144 bytes.
float3 ComputeEnvironmentIrradiance(in float3 InNormal)
{
	float4 Coefficients[SH_COEFFICIENT_COUNT];
	[unroll]
	for (uint CoefficientIndex = 0u; CoefficientIndex < SH_COEFFICIENT_COUNT; CoefficientIndex++)
	{
		Coefficients[CoefficientIndex] = EnvironmentSH[CoefficientIndex];
	}

	// A cbuffer publishes its members, not its name: the field is read bare, and
	// "IndirectLightingParams.SkyLightColorLuminance" would not compile.
	const float4 SkyColorLuminance = SkyLightColorLuminance;
	// The tint scales radiance, and irradiance is linear in radiance, so the same pair of
	// numbers is what the sky pass multiplies its cube by.
	return EvaluateIrradianceSH(Coefficients, InNormal) * (SkyColorLuminance.rgb * SkyColorLuminance.w);
}

// Adds the environment's diffuse contribution to a surface.
//
// Ambient occlusion multiplies the term, which is what finally gives that channel
// something to do: it has been carried through the GBuffer since it was laid out
// and until now there was no ambient light for it to occlude.
void ShadeIndirectDiffuse(in SurfaceData InSurface, inout ShadingResult InOutResult)
{
	// The same bit test the light loops make, and for the same reason: a surface that does not
	// take part in a lighting term is skipped before the work, not multiplied by zero after it.
	if ((InSurface.LightChannelMask & LIGHT_CHANNEL_INDIRECT) == 0u)
	{
		return;
	}

	const float3 Irradiance = ComputeEnvironmentIrradiance(InSurface.Normal);
	InOutResult.Diffuse += ComputeEnvironmentDiffuseBRDF(InSurface.DiffuseColor)
		* (Irradiance * InSurface.AmbientOcclusion);
}
#endif	//SHADER_INDIRECT_DIFFUSE_HLSL
