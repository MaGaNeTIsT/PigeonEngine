#ifndef M_SPONZA_MASK_HLSL
#define M_SPONZA_MASK_HLSL

// ---------------------------------------------------------------------------
// Sponza masked material: the opaque scene material plus a coverage map.
//
// The surface mode is Mask (PEMaterial.SurfaceMode beside this file), so the engine
// compiles the discard paths that the opaque variant does not contain. The coverage
// itself is the material's: MaskMap's alpha, thresholded by MaskThreshold. The scene
// binds the albedo map as the coverage map because this asset's separate *_mask.tga
// files are missing and the albedo alpha carries the same cutout.
//
// Two entry points answer the same coverage, which is the contract the mask mode
// relies on: CalcMaterialSurface feeds the base pass through Surface.Opacity and
// Surface.AlphaCutoff, and CalcMaterialOpacity feeds the depth/shadow pass directly.
// They share SampleMaskCoverage so the two passes cannot disagree about which pixels
// survive.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"

#if SHADER_STAGE_PIXEL
float SampleMaskCoverage(in MaterialPixelInput InInput)
{
	// The coverage map's alpha is the cutout. A material with no map (HasMaskMap 0) is fully
	// covered, which keeps an unbound default from discarding the whole surface.
	if (HasMaskMap <= 0.5)
	{
		return 1.0;
	}
	return PE_SAMPLE_TEX2D(MaskMap, SamplerMaterial2D, InInput.UV0).a;
}

void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);

	const float4 BaseColorSample = PE_SAMPLE_TEX2D(BaseColorMap, SamplerMaterial2D, InInput.UV0);
	const float3 BaseColor = ((HasBaseColorMap > 0.5) ? (BaseColorSample.rgb * BaseColorTint.rgb) : BaseColorTint.rgb);

	const float3 NormalSample = PE_SAMPLE_TEX2D(NormalMap, SamplerMaterial2D, InInput.UV0).rgb;
	const float3 TangentSpaceNormal = ((HasNormalMap > 0.5) ? ((NormalSample * 2.0) - 1.0) : float3(0.0, 0.0, 1.0));

	OutSurface.BaseColor = float4(BaseColor, 1.0);
	OutSurface.Normal = TangentSpaceNormal;
	OutSurface.Roughness = Roughness;
	OutSurface.Metallic = Metallic;
	OutSurface.Reflectance = Reflectance;
	OutSurface.AmbientOcclusion = AmbientOcclusion;
	OutSurface.EmissiveColor = float3(0.0, 0.0, 0.0);
	// Coverage is the surface's, and the cutoff is the material's; the engine compares the two
	// under its mask mode macro and discards below the threshold.
	OutSurface.Opacity = SampleMaskCoverage(InInput);
	OutSurface.AlphaCutoff = MaskThreshold;
}

// Coverage only, for a pass that clips but does not shade: no albedo, no normal, one fetch.
void CalcMaterialOpacity(in MaterialPixelInput InInput, out float OutOpacity, out float OutAlphaCutoff)
{
	OutOpacity = SampleMaskCoverage(InInput);
	OutAlphaCutoff = MaskThreshold;
}
#endif	//SHADER_STAGE_PIXEL

#endif	//M_SPONZA_MASK_HLSL
