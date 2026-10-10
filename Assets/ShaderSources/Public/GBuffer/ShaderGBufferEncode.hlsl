#ifndef SHADER_GBUFFER_ENCODE_HLSL
#define SHADER_GBUFFER_ENCODE_HLSL

// ---------------------------------------------------------------------------
// Writing the GBuffer.
//
// Engine owned. A domain does not write its own render targets: it fills
// SurfaceData, and the encoder here decides how that surface maps onto the seven
// targets. Adding a domain means adding one EncodeGBuffer_<Domain> below and one
// #elif branch, and no artist facing file changes at all.
//
// The dispatch is compile time. Each pixel stage is pinned to exactly one
// MATERIAL_DOMAIN by its variant defines, so the branch chain collapses to a
// single call with no runtime cost.
//
// Encode and decode are a pair: changing a channel meaning here means changing
// ShaderGBufferDecode.hlsl in the same commit, and the reverse.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderPacking.hlsl"
#include "Public/GBuffer/ShaderGBufferIO.hlsl"
#include "Public/Material/MaterialDomainTypes.hlsl"
#include "Public/Surface/SurfaceData.hlsl"

// ---------------------------------------------------------------------------
// Shared channel packing
//
// Kept as small named functions so the layout is stated once and both the
// encoders below read the same way.
// ---------------------------------------------------------------------------

// GBufferC: r = reflectance, g = ambient occlusion, b/a belong to the domain.
float4 EncodeGBufferCCommon(in SurfaceData InSurface, in float InDomainChannelB, in float InDomainChannelA)
{
	return float4(InSurface.Reflectance, InSurface.AmbientOcclusion, InDomainChannelB, InDomainChannelA);
}

// ---------------------------------------------------------------------------
// Unlit
//
// Carries colour and coverage and nothing else: no normal, no roughness, no
// metallicity. The light channel mask is zero, which is what tells the deferred
// lighting pass to skip this pixel entirely rather than shade it with a default
// material. That is the whole point of the unlit domain in a deferred renderer.
// ---------------------------------------------------------------------------

static void EncodeGBuffer_Unlit(in SurfaceData InSurface, in uint InSurfaceMode, out GBufferOutput OutGBuffer)
{
	// The surface's own light, which for an unlit material is the whole result: the derive
	// step folds the base colour into emission, because a surface that takes part in no
	// lighting term is exactly a surface that emits its own colour.
	OutGBuffer.SceneColor = float4(InSurface.EmissiveColor, 0.0);
	OutGBuffer.GBufferA = float4(InSurface.BaseColor, 0.0);
	OutGBuffer.GBufferB = float4(0.0, 0.0, 1.0, 0.0);		// flat normal, fully rough, dielectric
	OutGBuffer.GBufferC = EncodeGBufferCCommon(InSurface, 0.0, 0.0);
	OutGBuffer.GBufferD = PackIdentityAndLightMask(MATERIAL_DOMAIN_UNLIT, InSurfaceMode, 0u, InSurface.LightChannelMask);
	OutGBuffer.GBufferE = float4(0.0, 0.0, 0.0, 0.0);
	OutGBuffer.GBufferF = float4(0.0, 0.0, 0.0, 1.0);
}

// ---------------------------------------------------------------------------
// DefaultLit
//
// The full set: colour, octahedral normal, roughness, metallicity, reflectance,
// occlusion. This is the layout the channel table in ShaderGBufferIO.hlsl
// describes.
// ---------------------------------------------------------------------------

static void EncodeGBuffer_DefaultLit(in SurfaceData InSurface, in uint InSurfaceMode, out GBufferOutput OutGBuffer)
{
	// Scene colour starts as the surface's own emission, and it is the only thing the base
	// pass puts there. The deferred lighting pass adds its direct term on top, which is why a
	// pixel that receives no light at all still keeps whatever it emits.
	//
	// Emission in cd/m2 writes in as it is: it is already the same quantity the lighting pass
	// adds to the target, so no conversion belongs here, and a pi factor would put the
	// exposure calibration out by that much.
	OutGBuffer.SceneColor = float4(InSurface.EmissiveColor, 0.0);

	// GBufferA is BaseColor, not DiffuseColor: a metal has no diffuse term, so
	// storing the diffuse colour would lose its albedo and it would decode black.
	// See the note in SurfaceData.hlsl.
	OutGBuffer.GBufferA = float4(InSurface.BaseColor, 0.0);

	OutGBuffer.GBufferB = float4(EncodeOctahedron(InSurface.Normal), InSurface.Roughness, InSurface.Metallic);
	OutGBuffer.GBufferC = EncodeGBufferCCommon(InSurface, 0.0, 0.0);
	OutGBuffer.GBufferD = PackIdentityAndLightMask(MATERIAL_DOMAIN_DEFAULT_LIT, InSurfaceMode, 0u, InSurface.LightChannelMask);
	// Free since emission moved into the scene colour target: nothing reads this target any
	// more, and the allocation is kept because a GBuffer parameter that wants a HDR channel
	// pair is a more likely next use than anything else.
	OutGBuffer.GBufferE = float4(0.0, 0.0, 0.0, 0.0);
	OutGBuffer.GBufferF = float4(0.0, 0.0, 0.0, 1.0);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

// A compilation with no domain at all is a mistake worth stopping on: without
// MATERIAL_DOMAIN the branch chain below falls through to Unlit silently, and a
// silently wrong channel layout only shows up as a wrong looking frame.
#ifndef MATERIAL_DOMAIN
	#error MATERIAL_DOMAIN is not defined: a pixel stage that encodes the GBuffer must be pinned to a domain
#endif	//MATERIAL_DOMAIN

// The only name a program template writes. MATERIAL_DOMAIN is a variant define,
// so exactly one branch below survives preprocessing.
void EncodeGBuffer(in SurfaceData InSurface, in uint InDomainId, in uint InSurfaceMode, out GBufferOutput OutGBuffer)
{
#if MATERIAL_DOMAIN == MATERIAL_DOMAIN_UNLIT
	EncodeGBuffer_Unlit(InSurface, InSurfaceMode, OutGBuffer);
#elif MATERIAL_DOMAIN == MATERIAL_DOMAIN_DEFAULT_LIT
	EncodeGBuffer_DefaultLit(InSurface, InSurfaceMode, OutGBuffer);
#else
	#error Unsupported MATERIAL_DOMAIN: add an EncodeGBuffer_<Domain> branch in Public/GBuffer/ShaderGBufferEncode.hlsl
#endif	//MATERIAL_DOMAIN
}
#endif	//SHADER_GBUFFER_ENCODE_HLSL
