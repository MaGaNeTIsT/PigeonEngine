#ifndef SHADOW_MASK_PS_TMPL_HLSL
#define SHADOW_MASK_PS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Directional shadow mask pass, pixel stage.
//
// One full screen pass resolves every directional light's shadow into a screen space target:
// channel i holds the shadow term of directional light i, one means fully lit. The lighting
// pass then reads a channel per light instead of walking cascades per light, which is what
// keeps the expensive part - cascade selection, projection, PCF - out of the shading path that
// runs once per light and per pixel.
//
// The target is written for every pixel the pass covers, including the pixels the lighting
// pass will skip. A channel no directional light occupies stays at one, and so does the channel
// of a light that casts no shadow: the sampler side multiplies the light by the channel
// unconditionally, so "no shadow" has to be the identity in the data rather than a branch every
// reader has to repeat.
//
// Two things are deliberately not done here:
//
//   - no depth test against the view's depth buffer. The pass is a full screen triangle that
//     writes a new target, and the scene's depth is read as a texture for world position
//     reconstruction. Ranking the mask against the depth it came from would reject nothing:
//     every pixel it covers is a pixel the mask is meant to describe.
//
//   - no surface shading. The only surface values a shadow test needs are the world position
//     and the world normal, so the GBuffer is decoded for exactly those two and the domain
//     machinery is not compiled at all.
// ---------------------------------------------------------------------------

#include "Public/GBuffer/ShaderGBufferDecode.hlsl"
#include "Public/Lighting/ShaderShadows.hlsl"

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}
{VF_VERTEX_FACTORY}
{LIGHTING}
{SHADOWS}
{SHADOW_MASK}

void PSMain(in VFVarying InVarying, out float4 OutShadowMask : SV_Target0)
{
	// This pass runs at the mask's resolution, which is the view's scaled by the shadow mask
	// setting, so the pixel coordinate it was given is a mask pixel and not a view texel. The
	// mask's reciprocal size turns it into the view's normalized coordinate, and the view's own
	// size turns that back into the depth and GBuffer texel the surface values live at. At the
	// default scale of one the two sizes are equal and the second step is the identity.
	const float2 MaskUv = InVarying.Position.xy * InvMaskSize;
	const uint2 ViewTexel = uint2(MaskUv * ScreenSize);

	// World position comes back from the depth buffer, the same reconstruction the deferred
	// lighting pass uses: the GBuffer does not store it, and a pixel that was never written has
	// a meaningless position there - which is harmless, because the lighting pass skips that
	// pixel on its light channel mask and never reads the channel this pass wrote for it.
	float DeviceZ = PE_LOAD_TEX2D(DepthTexture, uint3(ViewTexel, 0u)).r;
	float4 ClipPosition = float4(MaskUv * 2.0 - 1.0, DeviceZ, 1.0);
	ClipPosition.y = -ClipPosition.y;
	float4 WorldPosition = mul(ViewProjectionInvMatrix, ClipPosition);
	WorldPosition /= max(abs(WorldPosition.w), 1e-6) * sign(WorldPosition.w + 1e-9);

	// The normal is the GBuffer's octahedral pair. A domain that stores a different normal is
	// not supported by this pass: the mask needs the world normal for the normal offset, and the
	// deferred contract already states that GBufferB holds it.
	float3 WorldNormal = DecodeOctahedron(PE_LOAD_TEX2D(GBufferBTexture, uint3(ViewTexel, 0u)).rg);

	// One means lit, and it stays the value of every channel no directional light claims. Four
	// channels per target: this pass writes set zero, and the multi set configuration (P6) is the
	// one that adds a target per set and loops here.
	float ShadowChannels[4] = { 1.0, 1.0, 1.0, 1.0 };
	const uint ShadowLightNum = min(DirectionalLightCount, 4u);
	for (uint LightIndex = 0u; LightIndex < ShadowLightNum; LightIndex++)
	{
		DirectionalLightData StoredLight = DecodeDirectionalLight(DirectionalLights[LightIndex]);
		ShadowChannels[LightIndex] = ComputeDirectionalShadow(StoredLight, WorldPosition.xyz, WorldNormal);
	}
	OutShadowMask = float4(ShadowChannels[0], ShadowChannels[1], ShadowChannels[2], ShadowChannels[3]);
}

#endif	//SHADOW_MASK_PS_TMPL_HLSL
