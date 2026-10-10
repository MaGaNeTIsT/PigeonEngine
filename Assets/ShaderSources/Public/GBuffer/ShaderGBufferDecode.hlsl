#ifndef SHADER_GBUFFER_DECODE_HLSL
#define SHADER_GBUFFER_DECODE_HLSL

// ---------------------------------------------------------------------------
// Reading the GBuffer back.
//
// Engine owned, and the mirror of ShaderGBufferEncode.hlsl. Change one, change
// the other in the same commit.
//
// The dispatch here is different in kind from the encoder's. Encode runs inside
// a pixel stage that is pinned to exactly one domain, so it can dispatch at
// compile time. Decode runs inside the deferred lighting pass, which shades one
// full screen and therefore sees pixels from every domain at once, so it has to
// dispatch at runtime on the DomainId stored in GBufferD.
//
// That asymmetry is deliberate: it is what keeps the number of deferred lighting
// variants independent of the number of domains. Adding a domain adds one case
// here; it does not multiply the lighting pass.
//
// Every decoder fills the same SurfaceData the material path fills, so the
// shading code cannot tell whether it is running forward or deferred. The derived
// values that a material would have computed -- DiffuseColor and F0 -- are
// rebuilt here by calling the same derive helpers the material path calls, which
// is why BaseColor is stored rather than DiffuseColor.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderPacking.hlsl"
#include "Public/GBuffer/ShaderGBufferIO.hlsl"
#include "Public/Material/MaterialDomainTypes.hlsl"
#include "Public/Surface/SurfaceData.hlsl"
#include "Public/Surface/SurfaceDerive.hlsl"

// ---------------------------------------------------------------------------
// Unlit
// ---------------------------------------------------------------------------

void DecodeGBuffer_Unlit(in GBufferInput InGBuffer, out SurfaceData OutSurface, out GBufferIds OutIds)
{
	uint LightChannelMask = 0u;
	UnpackIdentity(InGBuffer.GBufferD, OutIds, LightChannelMask);

	OutSurface.BaseColor = InGBuffer.GBufferA.rgb;
	OutSurface.DiffuseColor = OutSurface.BaseColor;
	OutSurface.F0 = float3(0.0, 0.0, 0.0);
	// The surface's emission already reached the scene colour target in the base pass, so it
	// is not decoded here: adding it again would double it.
	OutSurface.EmissiveColor = float3(0.0, 0.0, 0.0);
	OutSurface.Normal = float3(0.0, 0.0, 1.0);
	OutSurface.PerceptualRoughness = 1.0;
	OutSurface.Roughness = 1.0;
	OutSurface.Metallic = 0.0;
	OutSurface.Reflectance = 0.0;
	OutSurface.AmbientOcclusion = 1.0;
	OutSurface.Opacity = 1.0;
	OutSurface.AlphaCutoff = 0.5;
	OutSurface.LightChannelMask = LightChannelMask;
}

// ---------------------------------------------------------------------------
// DefaultLit
// ---------------------------------------------------------------------------

void DecodeGBuffer_DefaultLit(in GBufferInput InGBuffer, out SurfaceData OutSurface, out GBufferIds OutIds)
{
	uint LightChannelMask = 0u;
	UnpackIdentity(InGBuffer.GBufferD, OutIds, LightChannelMask);

	OutSurface.BaseColor = InGBuffer.GBufferA.rgb;

	OutSurface.Normal = DecodeOctahedron(InGBuffer.GBufferB.rg);
	OutSurface.Roughness = InGBuffer.GBufferB.b;
	OutSurface.PerceptualRoughness = ComputeRoughnessToPerceptualRoughness(OutSurface.Roughness);
	OutSurface.Metallic = InGBuffer.GBufferB.a;

	OutSurface.Reflectance = InGBuffer.GBufferC.r;
	OutSurface.AmbientOcclusion = InGBuffer.GBufferC.g;

	// The surface's emission already reached the scene colour target in the base pass, so the
	// lighting pass neither decodes nor re-adds it.
	OutSurface.EmissiveColor = float3(0.0, 0.0, 0.0);
	OutSurface.LightChannelMask = LightChannelMask;

	// The two values a material would have produced in DeriveSurface. Rebuilding
	// them with the same helpers is what keeps a deferred pixel indistinguishable
	// from a forward one downstream.
	OutSurface.DiffuseColor = ComputeDiffuseColor(OutSurface.BaseColor, OutSurface.Metallic);
	OutSurface.F0 = ComputeFresnelF0(OutSurface.Reflectance, OutSurface.Metallic, OutSurface.BaseColor);

	OutSurface.Opacity = 1.0;
	OutSurface.AlphaCutoff = 0.5;
}

// ---------------------------------------------------------------------------
// Runtime dispatch
// ---------------------------------------------------------------------------

void DecodeGBufferDispatch(in GBufferInput InGBuffer, out SurfaceData OutSurface, out GBufferIds OutIds)
{
	switch (InGBuffer.DomainId)
	{
		case MATERIAL_DOMAIN_UNLIT:
		{
			DecodeGBuffer_Unlit(InGBuffer, OutSurface, OutIds);
			break;
		}
		case MATERIAL_DOMAIN_DEFAULT_LIT:
		default:
		{
			DecodeGBuffer_DefaultLit(InGBuffer, OutSurface, OutIds);
			break;
		}
	}
}
#endif	//SHADER_GBUFFER_DECODE_HLSL
