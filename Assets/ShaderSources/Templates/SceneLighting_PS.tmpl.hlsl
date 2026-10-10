#ifndef SCENE_LIGHTING_PS_TMPL_HLSL
#define SCENE_LIGHTING_PS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Deferred lighting pass, pixel stage.
//
// One fullscreen pass lights the whole frame, and it handles every domain at
// once: the GBuffer stores a domain id per pixel and the decoder dispatches on
// it at runtime. That is what keeps the number of lighting variants independent
// of the number of domains, and it is why adding a domain costs one case in
// ShaderGBufferDecode.hlsl and nothing here.
//
// The GBuffer targets are program owned bindings, not a material's: this pass
// reads what the base pass wrote, so they are declared in the program
// description's own Textures block.
//
// The scene colour target is deliberately not among them, even though the base
// pass writes the surface emission there. This pass writes that same target and a
// resource cannot be sampled by the draw that writes it, so a declared binding
// for it could only ever hold a null view. The emission still reaches the result,
// through this pass's additive blend rather than through a read, which is why the
// sum at the bottom of this file carries no scene colour term.
//
// World position comes back from the depth buffer, because the GBuffer does not
// store it. The rest of MaterialPixelInput is left at zero: a domain whose
// ShadeSurface needs more than the position and the view direction has to take
// it from the surface it decoded, which is the honest description of what a
// deferred pass can know.
// ---------------------------------------------------------------------------

#include "Public/GBuffer/ShaderGBufferDecode.hlsl"
#include "Public/Lighting/ShaderDirectLighting.hlsl"

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}
{VF_VERTEX_FACTORY}
{PROGRAMMABLE}
{LIGHTING}

MaterialPixelInput BuildDeferredPixelInput(in VFVarying InVarying)
{
	uint2 Texel = uint2(InVarying.Position.xy);

	float DeviceZ = PE_LOAD_TEX2D(DepthTexture, uint3(Texel, 0u)).r;

	// The near plane is the far one for a pixel that was never written, and the
	// reconstructed position is meaningless there. The lighting contribution of
	// such a pixel is skipped by the light channel mask instead, which the sky
	// or the clear wrote as zero.
	float4 ClipPosition = float4(InVarying.Position.xy * InvScreenSize * 2.0 - 1.0, DeviceZ, 1.0);
	ClipPosition.y = -ClipPosition.y;
	float4 WorldPosition = mul(ViewProjectionInvMatrix, ClipPosition);
	WorldPosition /= max(abs(WorldPosition.w), 1e-6) * sign(WorldPosition.w + 1e-9);

	MaterialPixelInput Result;
	Result.WorldPosition = WorldPosition.xyz;
	Result.WorldNormal = float3(0.0, 0.0, 1.0);
	Result.WorldTangent = float3(1.0, 0.0, 0.0);
	Result.WorldBitangent = float3(0.0, 1.0, 0.0);
	Result.UV0 = float2(0.0, 0.0);
	Result.UV1 = float2(0.0, 0.0);
	Result.VertexColor = float4(1.0, 1.0, 1.0, 1.0);
	Result.ViewDirection = ComputeViewDirection(WorldPosition.xyz, ViewPosition);
	Result.ScreenPosition = InVarying.Position;
	Result.ScreenUV = InVarying.Position.xy * InvScreenSize;
	Result.PrimitiveID = 0u;
	Result.IsFrontFace = true;
	return Result;
}

void PSMain(in VFVarying InVarying,
	out float4 OutSceneColor : SV_Target0)
{
	GBufferInput GBuffer;
	// Defined as zero rather than read: the target this pass writes cannot be sampled in the same
	// draw. The field stays in the structure because it is part of the GBuffer contract the
	// encoders write and a pass that may legally read it fills it from its own binding.
	GBuffer.SceneColor = float4(0.0, 0.0, 0.0, 0.0);
	GBuffer.GBufferA = PE_LOAD_TEX2D(GBufferATexture, uint3(InVarying.Position.xy, 0u));
	GBuffer.GBufferB = PE_LOAD_TEX2D(GBufferBTexture, uint3(InVarying.Position.xy, 0u));
	GBuffer.GBufferC = PE_LOAD_TEX2D(GBufferCTexture, uint3(InVarying.Position.xy, 0u));
	GBuffer.GBufferD = PE_LOAD_TEX2D(GBufferDTexture, uint3(InVarying.Position.xy, 0u));
	GBuffer.GBufferE = PE_LOAD_TEX2D(GBufferETexture, uint3(InVarying.Position.xy, 0u));
	UnpackGBufferInputIdentity(GBuffer);

	SurfaceData Surface;
	GBufferIds Ids;
	DecodeGBufferDispatch(GBuffer, Surface, Ids);

	// A pixel with no lighting channel claimed is one the base pass already
	// declared out of the lighting system: an unlit surface, or a cleared pixel.
	// Its colour is the emission the base pass wrote into the target, and this
	// pass blends additively, so adding anything here would add it twice.
	if (Surface.LightChannelMask == 0u)
	{
		return;
	}

	MaterialPixelInput MatInput = BuildDeferredPixelInput(InVarying);

	ShadingResult Result;
	ShadeSurface(Surface, MatInput, Result);

	// The base pass's contribution - the surface's own emission - is already in the target and
	// arrives through the additive blend, not through this write. This pass adds the lighting
	// term and nothing else, which is why the sum has no scene colour term in it: there is no
	// legal way for this draw to have read one.
	OutSceneColor = float4(Result.Diffuse + Result.Specular, 1.0);
}

#endif	//SCENE_LIGHTING_PS_TMPL_HLSL
