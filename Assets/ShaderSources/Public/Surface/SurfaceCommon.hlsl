#ifndef SURFACE_COMMON_HLSL
#define SURFACE_COMMON_HLSL

// ---------------------------------------------------------------------------
// Small helpers the domains and the passes share when they assemble shading
// input. Nothing here owns state; every function is pure.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"

// View direction for a surface point, pointing from the surface towards the eye.
float3 ComputeViewDirection(in float3 InWorldPosition, in float4 InViewPosition)
{
	return SafeNormalize(InViewPosition.xyz - InWorldPosition);
}

// Half vector between the view and the light direction.
float3 ComputeHalfVector(in float3 InViewDirection, in float3 InLightDirection)
{
	return SafeNormalize(InViewDirection + InLightDirection);
}

// The mask surface mode discards on this. Kept as a predicate rather than a
// call to discard() so a pass can decide where the discard happens, and so the
// result can be reused by a depth prepass that needs the same test.
bool IsAlphaDiscarded(in float InOpacity, in float InAlphaCutoff)
{
	return InOpacity < InAlphaCutoff;
}

// Effective coverage of a surface: the material may reduce Opacity on top of
// the alpha channel of BaseColor.
float ComputeSurfaceOpacity(in float InBaseColorAlpha, in float InOpacity)
{
	return saturate(InBaseColorAlpha * InOpacity);
}

// Guards the specular denominator. At grazing angles N dot V goes to zero and
// the specular lobe would divide by it, so it is clamped just above zero.
float ClampNdotV(in float InNdotV)
{
	return max(InNdotV, PE_SHADER_MIN_N_DOT_V);
}
#endif	//SURFACE_COMMON_HLSL
