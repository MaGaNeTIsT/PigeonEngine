#ifndef SKY_PS_TMPL_HLSL
#define SKY_PS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Sky pass, pixel stage.
//
// Draws the environment where nothing was drawn, and clears the GBuffer so the
// deferred lighting pass can skip those pixels instead of decoding whatever
// happened to be in the targets.
//
// The light channel mask is written as zero for exactly that reason. Emissive
// carries the sky radiance, which the lighting pass adds without shading
// anything, so the sky needs no special case downstream.
// ---------------------------------------------------------------------------

#include "Public/GBuffer/ShaderGBufferEncode.hlsl"
#include "Public/Lighting/ShaderIndirectLighting.hlsl"

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}
{VF_VERTEX_FACTORY}
{PROGRAMMABLE}
{LIGHTING}
{SKY_LIGHT}

void PSMain(in VFVarying InVarying,
	out float4 OutSceneColor : SV_Target0,
	out float4 OutGBufferA : SV_Target1,
	out float4 OutGBufferB : SV_Target2,
	out float4 OutGBufferC : SV_Target3,
	out uint4 OutGBufferD : SV_Target4,
	out float4 OutGBufferE : SV_Target5,
	out float4 OutGBufferF : SV_Target6)
{
	// The view direction through this pixel, from the inverse view projection.
	//
	// The clip position is a point on the far plane, so the inverse projection answers a
	// homogeneous point whose w is one over the far distance rather than one, and the divide
	// is what turns it into a world position. Without it the point lands a whole far distance
	// too far away, and the subtraction below then mixes the camera's own position into the
	// direction: the sky reads as a still frame only while the camera stands at the world
	// origin, and slides by roughly the camera's distance over the far plane as it moves.
	// The lighting pass reconstructs its world position the same way for the same reason.
	float4 ClipPosition = float4(InVarying.UV0 * 2.0 - 1.0, 1.0, 1.0);
	ClipPosition.y = -ClipPosition.y;
	float4 WorldPosition = mul(ViewProjectionInvMatrix, ClipPosition);
	WorldPosition /= max(abs(WorldPosition.w), 1e-6) * sign(WorldPosition.w + 1e-9);
	float3 SkyDirection = SafeNormalize(WorldPosition.xyz - ViewPosition.xyz);

	// Roughness zero and a mip count of zero: the sky is the sharpest reflection
	// the environment has, so it samples the top level. A prefiltered chain in the
	// second stage would pass its own level count here.
	float3 SkyRadiance = SampleEnvironmentRadiance(SkyDirection, 0.0, 0.0);

	// The cube is sampled as radiance, so its texels carry the shape of the sky and not its
	// brightness. This is what puts them on the same physical scale as the lights: the scene
	// colour target holds luminance in cd/m2, and an unscaled cube would land around 1 cd/m2,
	// which any daylight exposure turns black.
	SkyRadiance *= LightAdjustColor.rgb * LightAdjustIntensity;

	OutSceneColor = float4(SkyRadiance, 1.0);

	// A cleared GBuffer. The values are the neutral ones a decode would produce
	// for an all default material, so a pixel that the geometry pass did not
	// write still decodes into something well defined if anything ever reads it.
	//
	// The sky's own radiance is in the scene colour target above and nowhere else:
	// GBufferE no longer carries emission, because emission travels through the
	// scene colour target now and this pass already writes the sky straight there.
	OutGBufferA = float4(0.0, 0.0, 0.0, 0.0);
	OutGBufferB = float4(EncodeOctahedron(float3(0.0, 0.0, 1.0)), 1.0, 0.0);
	OutGBufferC = float4(0.0, 1.0, 0.0, 0.0);
	OutGBufferD = PackIdentityAndLightMask(MATERIAL_DOMAIN_UNLIT, MATERIAL_SURFACE_MODE_OPAQUE, 0u, 0u);
	OutGBufferE = float4(0.0, 0.0, 0.0, 0.0);
	OutGBufferF = float4(0.0, 0.0, 0.0, 1.0);
}

#endif	//SKY_PS_TMPL_HLSL
