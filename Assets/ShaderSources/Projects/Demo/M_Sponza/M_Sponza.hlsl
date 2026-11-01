#ifndef M_SPONZA_HLSL
#define M_SPONZA_HLSL

// ---------------------------------------------------------------------------
// Sponza scene material: an albedo map, a tangent space normal map, and the four
// DefaultLit scalars.
//
// One material serves the whole scene. The source file names 25 materials, and what
// separates them is which texture files they point at - the scalars here are shared -
// so the scene builder clones one material instance per source material and binds
// that material's own maps into it, rather than compiling 25 near identical shaders.
//
// Two switches, HasBaseColorMap and HasNormalMap, exist for the two states the data
// actually reaches:
//
//   - a source material that names no albedo map. Sampling the engine's fallback
//     would make it black, while the material's own Kd is a perfectly good base
//     colour, so the switch picks the tint instead of an absent map.
//   - a source material that names no normal map, or whose normal map has not been
//     imported yet. The engine's fallback texture is black, and a black normal
//     decodes to (-1, -1, -1) - every surface would be lit as if it faced away from
//     the light. Flat (0, 0, 1) is the value that leaves the geometric normal alone.
//
// Both mappings are sRGB questions the importer answered, not this file: the albedo
// map is sampled as colour and the normal map as data, and the sampler's own sRGB
// decode is what tells them apart.
//
// VertexOps is false in the JSON beside this file, so the vertex stage is the shared
// neutral one and the material's textures exist only in the pixel stage.
//
// The bindless header is included rather than assumed: PE_SAMPLE_TEX2D is defined
// there, and which path it resolves to is the compiler's decision, not this file's.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"

#if SHADER_STAGE_PIXEL
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
}

// Coverage only, for a pass that clips but does not shade. The mode is Opaque, so this answers
// with the default fully opaque pair - but it still runs the material, because a depth pass and
// the base pass have to agree about where the surface is.
void CalcMaterialOpacity(in MaterialPixelInput InInput, out float OutOpacity, out float OutAlphaCutoff)
{
	MaterialSurface TempSurface;
	InitMaterialSurface(TempSurface);
	CalcMaterialSurface(InInput, TempSurface);
	OutOpacity = TempSurface.Opacity;
	OutAlphaCutoff = TempSurface.AlphaCutoff;
}
#endif	//SHADER_STAGE_PIXEL

#endif	//M_SPONZA_HLSL
