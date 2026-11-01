#ifndef SHADER_INDIRECT_SPECULAR_HLSL
#define SHADER_INDIRECT_SPECULAR_HLSL

// ---------------------------------------------------------------------------
// The indirect specular term: the environment reflected by the split sum.
//
// The split sum factors the reflection integral into two precomputed terms:
//   - a prefiltered radiance cubemap, sampled with the reflection direction and
//     a mip selected by perceptual roughness;
//   - a DFG lookup table, sampled with (N dot V, perceptual roughness).
//
// The prefiltered chain is normalized to mean one, so the sky light's mean
// luminance and tint are applied here exactly as the diffuse term applies them.
// That is what keeps a brightness or tint edit free: neither term is baked with
// either value.
//
// This file is compiled by the deferred lighting pass only. The base pass
// compiles the same domain to derive a surface and must not declare the
// prefiltered cube or the LUT, so D_DefaultLit includes this under
// SHADER_LIGHTING_PASS.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"
#include "Public/Surface/SurfaceData.hlsl"
#include "Public/Surface/SurfaceCommon.hlsl"
#include "Public/Surface/SurfaceDerive.hlsl"
#include "Public/System/ShaderBindless.hlsl"

// Environment radiance for one reflection direction, in the shape-normalized form the
// prefilter produced. Perceptual roughness maps linearly onto the mip chain, which is why
// the prefilter builds its levels at that same spacing.
//
// The read side is a cube, so the direction goes to the hardware as it is: the face selection
// and the per face UVs are the API's own cube convention, and filtering across a face border
// is seamless. The bake writes through a 2D array UAV because no D3D version has a cube UAV,
// but the texels it writes are the very ones this view reads - the prefilter's face table IS
// the cube convention, which is what makes the two halves agree.
float3 SamplePrefilteredEnvironmentRadiance(in float3 InReflectionDirection, in float InPerceptualRoughness)
{
	const float MipCount = max(PrefilterMipCount, 1.0);
	const float Level = InPerceptualRoughness * (MipCount - 1.0);

	return PE_SAMPLE_TEXCUBE_LEVEL(PrefilteredEnvironmentCube, SamplerMaterialCube, InReflectionDirection, Level).rgb;
}

// InViewDirection is the unit vector from the surface towards the eye, and it arrives as an
// argument rather than as a SurfaceData field because SurfaceData is the BRDF parameter bundle
// the GBuffer round trip reconstructs: the view direction belongs to the shading call, which is
// why the direct lights take it the same way.
void ShadeIndirectSpecular(in SurfaceData InSurface, in float3 InViewDirection, inout ShadingResult InOutResult)
{
	// The same bit test the diffuse term and the light loops make: a surface that does not
	// take part in indirect light is skipped before the work, not multiplied by zero after it.
	if ((InSurface.LightChannelMask & LIGHT_CHANNEL_INDIRECT) == 0u)
	{
		return;
	}

	const float NdotV = ClampNdotV(dot(InSurface.Normal, InViewDirection));
	const float3 ReflectionDirection = reflect(-InViewDirection, InSurface.Normal);
	const float3 PrefilteredRadiance = SamplePrefilteredEnvironmentRadiance(ReflectionDirection, InSurface.PerceptualRoughness);

	// The LUT stores the DFG integral: F0 * x + y. The same (NdotV, perceptual roughness)
	// pair the prefilter used for its mip selection.
	const float2 EnvironmentBRDF = EnvironmentBRDFLUT.SampleLevel(
		SamplerLinearClamp, float2(NdotV, InSurface.PerceptualRoughness), 0.0).rg;

	const float3 EnvironmentScale = SkyLightColorLuminance.rgb * SkyLightColorLuminance.w;
	const float SpecularOcclusion = ComputeSpecularOcclusion(NdotV, InSurface.AmbientOcclusion, InSurface.PerceptualRoughness);

	InOutResult.Specular += PrefilteredRadiance
		* ((InSurface.F0 * EnvironmentBRDF.x) + EnvironmentBRDF.y)
		* EnvironmentScale
		* SpecularOcclusion;
}

#endif	//SHADER_INDIRECT_SPECULAR_HLSL
