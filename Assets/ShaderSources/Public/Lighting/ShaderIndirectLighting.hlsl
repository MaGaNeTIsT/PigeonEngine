#ifndef SHADER_INDIRECT_LIGHTING_HLSL
#define SHADER_INDIRECT_LIGHTING_HLSL

// ---------------------------------------------------------------------------
// Environment sampling.
//
// This file is the cube read and nothing else: it is what the sky pass uses for
// its own background, and what a prefiltered chain will sample once the specular
// half lands. A surface's indirect *diffuse* term is the other direction of the
// same feature and lives in Public/Lighting/ShaderIndirectDiffuse.hlsl - the two
// are separate files because they have separate bindings, and a pass that only
// draws a sky must not be made to declare what a shading pass reads.
//
// What the second stage adds here: a prefiltered mip chain plus a DFG lookup
// table, which turns the split sum into a single fetch. The interface does not
// change.
//
// The environment cube is declared by C_Lighting.json. A pass that never samples
// it culls it.
//
// The helpers in Public/BRDF/ShaderIndirectBxDF.hlsl (the diffuse environment
// weight, the environment BRDF and specular occlusion) are shared by both halves
// and stay where they are.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/System/ShaderBindless.hlsl"
#include "Public/Core/ShaderMath.hlsl"
#include "Public/Core/ShaderPacking.hlsl"
#include "Public/Surface/SurfaceData.hlsl"
#include "Public/Surface/SurfaceCommon.hlsl"
#include "Public/Surface/SurfaceDerive.hlsl"
#include "Public/BRDF/ShaderIndirectBxDF.hlsl"

// Environment radiance in a direction, selected by roughness.
//
// The first stage samples the cube's own mip chain, so roughness maps to a
// level rather than to a prefiltered level of a specific size. That is a coarse
// approximation and it is expected to look soft: it exists so the chain has
// something correctly shaped to run through, not to be the final quality bar.
//
// The result is radiance in cd/m2. The cube carries the shape of the sky and not
// its brightness, so whoever samples it decides the scale - the sky pass uses
// PSkyLightComponent's luminance for that.
//
// InMipCount is passed in rather than read from a constant buffer, because
// C_Lighting declares bindings only and has no constant buffer of its own. The
// pass that owns the environment cube is the one that knows its mip count.
float3 SampleEnvironmentRadiance(in float3 InReflectionDirection, in float InRoughness, in float InMipCount)
{
	// Roughness to cube mip level. Using the cube's own level count means a mip
	// chain of any depth works without a second constant to keep in step.
	float Level = InRoughness * InRoughness * max(InMipCount - 1.0, 0.0);
	return PE_SAMPLE_TEXCUBE_LEVEL(EnvironmentCube, SamplerMaterialCube, InReflectionDirection, Level).rgb;
}
#endif	//SHADER_INDIRECT_LIGHTING_HLSL
