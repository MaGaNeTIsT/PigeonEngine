#ifndef SURFACE_DERIVE_HLSL
#define SURFACE_DERIVE_HLSL

// ---------------------------------------------------------------------------
// Deriving BRDF parameters from what the material wrote.
//
// These are the Filament conversions, kept as their own functions so every
// domain derives its parameters the same way and the GBuffer stores values that
// mean the same thing no matter which domain produced them.
//
// The naming follows Filament's own, which is also the vocabulary the design
// uses: DiffuseColor, F0, Reflectance, perceptual versus true roughness.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"

// ---------------------------------------------------------------------------
// Roughness
// ---------------------------------------------------------------------------

// Clamps the artist facing roughness away from zero. A value below this floor
// makes the specular lobe narrower than a pixel can represent and pushes the
// fourth power of roughness into denormals on half precision targets.
float ClampPerceptualRoughness(in float InRoughness)
{
	return max(InRoughness, MIN_PERCEPTUAL_ROUGHNESS);
}

// perceptualRoughness = sqrt(roughness). This is the direction the artist
// facing value moves, so it is the one stored in the GBuffer.
float ComputePerceptualRoughnessToRoughness(in float InPerceptualRoughness)
{
	return InPerceptualRoughness * InPerceptualRoughness;
}

// The inverse, for the decode side which reads the true value back out.
float ComputeRoughnessToPerceptualRoughness(in float InRoughness)
{
	return sqrt(InRoughness);
}

// ---------------------------------------------------------------------------
// Diffuse and specular
// ---------------------------------------------------------------------------

// Filament's metallic workflow: a metal has no diffuse lobe, a dielectric keeps
// its base colour as diffuse. The result is HDR and must not be clamped before
// it reaches a float render target.
float3 ComputeDiffuseColor(in float3 InBaseColor, in float InMetallic)
{
	return InBaseColor * (1.0 - InMetallic);
}

// Normal incidence specular reflectance.
//
// A dielectric reflects a fixed fraction of the light, taken from Reflectance
// and remapped from the [0, 1] artist range to F0's [0, 0.16] physical range.
// A metal has no transmission at all, so its F0 is simply its base colour.
float3 ComputeFresnelF0(in float InReflectance, in float InMetallic, in float3 InBaseColor)
{
	float DielectricF0 = 0.16 * InReflectance * InReflectance;
	return lerp(DielectricF0.xxx, InBaseColor, InMetallic);
}

// Specular occlusion: the ambient specular term has to be dimmed by the same
// occlusion the diffuse term suffers, or creases show a specular sheen that
// nothing occludes. Filament's Horizon Occlusion approximation.
float ComputeSpecularOcclusion(in float InNdotV, in float InAmbientOcclusion, in float InRoughness)
{
	return saturate(pow(InNdotV + InAmbientOcclusion, exp2(-16.0 * InRoughness - 1.0)) - 1.0 + InAmbientOcclusion);
}

// ---------------------------------------------------------------------------
// Tangent space to world
// ---------------------------------------------------------------------------

// InTangentSpaceNormal is the normal the material produced, with +Z along the
// surface normal. InTangentToWorld must have the frame axes as its columns,
// which is what CreateTangentMatrix in Public/Core/ShaderSpaceTransform.hlsl
// produces.
float3 TransformTangentToWorld(in float3 InTangentSpaceNormal, in float3x3 InTangentToWorld, in bool InIsNormalize)
{
	float3 NormalWS = mul(InTangentToWorld, InTangentSpaceNormal);
	if (InIsNormalize)
	{
		NormalWS = SafeNormalize(NormalWS);
	}
	return NormalWS;
}

// Builds the tangent to world frame from an already orthonormal set of axes.
//
// This is the path the pixel stage uses: the engine has already applied the
// tangent's handedness when it built WorldBitangent, so the frame is complete
// and recomputing the binormal with a cross product would throw that away.
// CreateTangentMatrix in ShaderSpaceTransform.hlsl is the other path, for a
// caller that only has a normal and a tangent.
//
// The axes are the columns, which is what makes mul(Frame, tangentSpaceVector)
// the world space vector.
float3x3 CreateTangentToWorldMatrix(in float3 InNormal, in float3 InTangent, in float3 InBitangent)
{
	return float3x3(
		InTangent.x, InBitangent.x, InNormal.x,
		InTangent.y, InBitangent.y, InNormal.y,
		InTangent.z, InBitangent.z, InNormal.z);
}

// Fallback for a mesh with no tangent stream: any perpendicular of the normal
// is a valid frame, so the tangent space normal is still usable, it just is not
// oriented the way the artist expected.
float3x3 BuildTangentToWorldFromNormal(in float3 InNormal, out float3 OutTangent, out float3 OutBitangent)
{
	float3 Normal = SafeNormalize(InNormal);
	float3 Tangent = (abs(Normal.z) < 0.999) ? SafeNormalize(cross(float3(0.0, 0.0, 1.0), Normal)) : float3(1.0, 0.0, 0.0);
	OutTangent = Tangent;
	OutBitangent = cross(Normal, Tangent);
	return CreateTangentToWorldMatrix(Normal, OutTangent, OutBitangent);
}
#endif	//SURFACE_DERIVE_HLSL
