#ifndef SHADER_SHADOWS_HLSL
#define SHADER_SHADOWS_HLSL

// ---------------------------------------------------------------------------
// Shadow terms.
//
// The design defers shadow algorithms to the second stage, so what lives here
// now is the interface plus an unshadowed implementation. It exists this early
// because the direct lighting loop calls it once per light, and having the call
// site in place means adding real shadows later changes this file and the
// declarations in C_Lighting.json, and touches nothing else.
//
// What the second stage will add, in this file:
//
//   - a shadow map array sampled through SamplerShadow (already declared by the
//     system layer) with the compare sampler doing the depth test for free
//   - cascade selection for directional lights, from ShadowMapIndex and
//     CascadeCount, which is all the light struct carries
//   - percentage closer filtering, and slope scaled depth bias
//
// The light struct deliberately carries only the *address* of a shadow: an index
// and a cascade count, no matrix. A cascade's view projection is per view, and
// the light list is per frame and shared by every view, so the matrices cannot
// live in it. They go into a per view array indexed by ShadowMapIndex when the
// algorithms land, which also keeps the two light buffers view independent and
// therefore uploadable once per frame.
//
// Returning 1.0 rather than skipping the multiply keeps the shader free of a
// branch: an unshadowed light multiplies by one.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Lighting/ShaderLightData.hlsl"

// 1 = fully lit. The second stage replaces the body with the shadow test.
float SampleDirectionalLightShadow(in DirectionalLightData InLight, in float3 InWorldPosition)
{
	return 1.0;
}

float SampleLocalLightShadow(in LocalLightData InLight, in float3 InWorldPosition)
{
	return 1.0;
}
#endif	//SHADER_SHADOWS_HLSL
