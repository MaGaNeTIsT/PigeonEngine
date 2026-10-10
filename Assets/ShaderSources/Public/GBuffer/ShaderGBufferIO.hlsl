#ifndef SHADER_GBUFFER_IO_HLSL
#define SHADER_GBUFFER_IO_HLSL

// ---------------------------------------------------------------------------
// The GBuffer contract: render target slots, the transparent structures the
// encoders and decoders pass around, the identity packing and the light channel
// mask bits.
//
// This is the single definition point for the light channel mask (K18) and the
// reference for the render target formats. Changing a format or a channel
// meaning here means changing ShaderGBufferEncode.hlsl and
// ShaderGBufferDecode.hlsl in the same commit (K5, design section 13.2).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Render target table
//
//   RT  Name         Format                  Payload
//   --  -----------  ----------------------  ----------------------------------
//   0   SceneColor   R16G16B16A16_FLOAT      Linear scene luminance in cd/m2: the
//                                            term the lighting pass adds to, the
//                                            sky's own radiance, and the emissive
//                                            a base pass surface wrote. Exposure
//                                            and a tone map are applied at the
//                                            display boundary, so nothing here is
//                                            clamped before that. Alpha is a free
//                                            general purpose channel - the lighting
//                                            pass blends ZERO/ONE on it, so it
//                                            preserves what the base pass put
//                                            there instead of overwriting it.
//   1   GBufferA     R16G16B16A16_FLOAT      HDR BaseColor.rgb (alpha free)
//   2   GBufferB     R16G16B16A16_UNORM      rg octahedral normal, b roughness,
//                                            a metallic
//   3   GBufferC     R8G8B8A8_UNORM          r reflectance, g ambient occlusion,
//                                            b/a domain extension (free today)
//   4   GBufferD     R16G16B16A16_UINT       integer identity and light mask
//   5   GBufferE     R11G11B10_FLOAT         HDR Emissive.rgb (alpha free)
//   6   GBufferF     R8_UNORM                reserved, allocated but unused
//
// UNORM cannot represent HDR: its range is clamped to [0, 1], so any channel
// that carries HDR data uses a FLOAT format. SceneColor carries the widest range
// in the frame, because scene luminance is physically scaled: sunlight is
// 100000 lux and a mirror reflecting it returns 1.5e9 cd/m2, past the ceiling of
// every sixteen bit format. The direct specular term carries its own guard for
// exactly that reason; see PE_LIGHTING_SPECULAR_MAX.
//
// The identity data sits in the integer target rather than in GBufferA's alpha
// because that target exists precisely to carry integers, and it keeps the light
// mask next to it. The channels free for whoever needs the next one: SceneColor.a,
// GBufferA.a, GBufferC.b, GBufferC.a, GBufferE.a, and all of GBufferF.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Light channel mask
//
// Written into GBufferD by the base pass, read by the deferred lighting pass to
// skip light contributions a surface cannot receive at all.
// ---------------------------------------------------------------------------

#define LIGHT_CHANNEL_DIRECT				(1u << 0)
#define LIGHT_CHANNEL_LOCAL					(1u << 1)
#define LIGHT_CHANNEL_INDIRECT				(1u << 2)
#define LIGHT_CHANNEL_EMISSIVE				(1u << 3)
#define LIGHT_CHANNEL_ALL					(LIGHT_CHANNEL_DIRECT | LIGHT_CHANNEL_LOCAL | LIGHT_CHANNEL_INDIRECT | LIGHT_CHANNEL_EMISSIVE)

// Bits from here up belong to the domain; the base pass must not claim them.
#define LIGHT_CHANNEL_DOMAIN_FIRST			(1u << 4)

// ---------------------------------------------------------------------------
// Structures
// ---------------------------------------------------------------------------

// One member per render target, filled by EncodeGBuffer and written by the
// program template. GBufferD is unsigned integer because its target is an
// integer format.
struct GBufferOutput
{
	float4	SceneColor;
	float4	GBufferA;
	float4	GBufferB;
	float4	GBufferC;
	uint4	GBufferD;
	float4	GBufferE;
	float4	GBufferF;
};

// The identity fields unpacked out of GBufferD.
struct GBufferIds
{
	uint	DomainId;
	uint	SurfaceMode;
	uint	Flags;
};

// What the deferred lighting pass reads back. The identity is carried both raw
// (GBufferD) and unpacked, because the dispatch switch runs on DomainId before
// any domain specific decoding happens.
struct GBufferInput
{
	float4	SceneColor;
	float4	GBufferA;
	float4	GBufferB;
	float4	GBufferC;
	uint4	GBufferD;
	float4	GBufferE;
	uint	DomainId;
	uint	SurfaceMode;
	uint	Flags;
	uint	LightChannelMask;
};

// ---------------------------------------------------------------------------
// Identity packing
//
// GBufferD is R16G16B16A16_UINT, one field per 16 bit lane. Keeping the four
// fields in separate lanes means a decode never has to mask or shift, and the
// layout stays readable from both HLSL and C++.
// ---------------------------------------------------------------------------

uint4 PackIdentityAndLightMask(in uint InDomainId, in uint InSurfaceMode, in uint InFlags, in uint InLightChannelMask)
{
	return uint4(
		InDomainId & 0xFFFFu,
		InSurfaceMode & 0xFFFFu,
		InFlags & 0xFFFFu,
		InLightChannelMask & 0xFFFFu);
}

void UnpackIdentity(in uint4 InPacked, out GBufferIds OutIds, out uint OutLightChannelMask)
{
	OutIds.DomainId = InPacked.x;
	OutIds.SurfaceMode = InPacked.y;
	OutIds.Flags = InPacked.z;
	OutLightChannelMask = InPacked.w;
}

// Convenience for the decode side, which works with GBufferInput.
void UnpackGBufferInputIdentity(inout GBufferInput InOutGBuffer)
{
	GBufferIds Ids = (GBufferIds)0;
	uint LightChannelMask = 0u;
	UnpackIdentity(InOutGBuffer.GBufferD, Ids, LightChannelMask);
	InOutGBuffer.DomainId = Ids.DomainId;
	InOutGBuffer.SurfaceMode = Ids.SurfaceMode;
	InOutGBuffer.Flags = Ids.Flags;
	InOutGBuffer.LightChannelMask = LightChannelMask;
}
#endif	//SHADER_GBUFFER_IO_HLSL
