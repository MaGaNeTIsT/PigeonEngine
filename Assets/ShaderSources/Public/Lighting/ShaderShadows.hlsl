#ifndef SHADER_SHADOWS_HLSL
#define SHADER_SHADOWS_HLSL

// ---------------------------------------------------------------------------
// Shadow terms: the atlas sampling core, and the screen space mask the lighting pass reads.
//
// Three pieces live here, and which of them a compilation gets depends on what it declares:
//
//   the atlas core        (SHADER_SHADOW_RECEIVER) reads ShadowMapAtlas and ShadowSlots. It is
//                         compiled only into a program that declares Components/C_Shadows.json,
//                         because the bindings are globals the component emits: a program that
//                         does not declare it would be compiled against names it does not have.
//                         The base pass is exactly that program - it includes the direct
//                         lighting header to derive a surface, and never samples a shadow.
//
//   the mask read         (SHADER_LIGHTING_PASS) reads the directional mask target. Only the
//                         deferred lighting pass declares that target and sets the define.
//
//   the local light term  uses the same atlas core and is guarded by SHADER_SHADOW_RECEIVER, so
//                         a program without the shadow bindings compiles it to one. Both the
//                         deferred lighting pass and the mask pass declare those bindings; the
//                         base pass does not, and never shades.
//
// ---------------------------------------------------------------------------
// Why the depth test is written out instead of using a comparison sampler
//
// The engine has no comparison sampler. Components/C_EngineSystem.json declares SamplerShadow,
// but the engine creates it with ComparisonFunc NEVER (see the sampler table in SceneRenderer),
// and the HLSL generator emits every sampler as a plain SamplerState. SampleCmpLevelZero would
// therefore either fail to compile or compare against a state that never passes.
//
// So a shadow lookup loads the stored depth and compares it in the shader. That is the same
// arithmetic a hardware compare would do, and it buys two things the hardware path cannot give:
// the tap offsets are explicit integers clamped to the tile, and the bias is a value the
// renderer can tune per cascade without touching a rasterizer state.
//
// ---------------------------------------------------------------------------
// Conventions
//
//   - A slot's matrix is four rows. The engine stores a row major matrix and applies it as
//     mul(Vector, Matrix), so float4x4(Row0..Row3) is the matrix and the product is the same
//     one the cpu side states.
//   - Clip space y grows up; the atlas' rows grow down. Every projection to texture space
//     flips y before it becomes a texel coordinate. Getting this wrong turns the shadow upside
//     down while leaving the picture looking almost plausible, which is why it is stated here.
//   - AtlasTile is (tile U scale, tile V scale, tile U offset, tile V offset) in atlas UV, so
//     the tile's pixel origin is AtlasTile.zw * atlas size and its edge is the atlas size times
//     the scale.
//   - A slot's w in ShadowParams is its validity. Slot zero is the "casts no shadow" sentinel
//     and is never sampled: a light that casts no shadow carries ShadowMapIndex 0, which the
//     light decoding and every entry point here test before they touch the table.
//   - One means fully lit. The shading side multiplies by the term unconditionally, so a light
//     without a shadow has to produce one rather than a branch at every call site.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/System/ShaderBindless.hlsl"
#include "Public/Lighting/ShaderLightData.hlsl"

// One mask target carries four directional lights. The count is a compile time value because it
// shapes the declarations and the branch the lighting pass compiles; it is stated by the program
// descriptors (P_ShadowMask.json and P_SceneLighting.json) and defaults here so a shader that
// only includes this file still compiles.
#ifndef PE_SHADOW_MASK_SET_NUM
#define PE_SHADOW_MASK_SET_NUM	1
#endif	//PE_SHADOW_MASK_SET_NUM

#define PE_SHADOW_MASK_LIGHT_NUM_MAX	(PE_SHADOW_MASK_SET_NUM * 4)

// RenderFlags bit 0: the shadow mask resolver exists this frame, so a light whose shadow address
// is not zero has a mask the pass may sample. It is the frame wide half of a two level contract -
// whether this particular view produced a mask travels in the light list the view was shaded
// with, where a light the resolver could not serve keeps the "casts no shadow" address and never
// reaches the mask read at all. That split is what lets any number of views share one frame wide
// block: the bit cannot be wrong per view because it says nothing about a view.
#define PE_SHADOW_MASK_VALID_BIT	1u

// PCF radius in texels. The quality preset picks it, and it is compile time on purpose: the tap
// count is the loop bound this file compiles, so it belongs to the assembly and not to a runtime
// value. A program that declares the shadow component states it; the fallback keeps this file
// self contained.
#ifndef PE_SHADOW_PCF_RADIUS
#define PE_SHADOW_PCF_RADIUS	1
#endif	//PE_SHADOW_PCF_RADIUS

#define PE_SHADOW_PCF_TAP_NUM	((PE_SHADOW_PCF_RADIUS * 2) + 1)

// A directional light has at most this many cascades. The engine caps the cascade count at the
// same number, and the loop below never runs past the light's own count.
#define PE_SHADOW_CASCADE_NUM_MAX	4u

#if SHADER_SHADOW_RECEIVER

// One element of the slot table, in the readable shape the sampling code wants. The storage
// layout is Components/C_Shadows.json; this is the only place that knows how it is stored.
struct ShadowSlotData
{
	float4x4	ViewProjection;
	float4		AtlasTile;
	float4		ShadowParams;
	float4		BiasParams;
};

ShadowSlotData DecodeShadowSlot(in uint InSlot)
{
	ShadowSlots_Struct Stored = ShadowSlots[InSlot];
	ShadowSlotData Result;
	Result.ViewProjection = float4x4(Stored.ViewProjectionRow0, Stored.ViewProjectionRow1
		, Stored.ViewProjectionRow2, Stored.ViewProjectionRow3);
	Result.AtlasTile = Stored.AtlasTile;
	Result.ShadowParams = Stored.ShadowParams;
	Result.BiasParams = Stored.BiasParams;
	return Result;
}

// Averages the depth test over one tile. The taps are clamped into the tile rather than into the
// atlas: neighbouring tiles hold unrelated geometry, so a filter that reached past the tile edge
// would blend a caster's depth into a completely different light's shadow.
float SampleShadowTile(in ShadowSlotData InSlot, in float2 InTileTexel, in float InCompareDepth)
{
	const float TileSize = 1.0 / max(InSlot.ShadowParams.y, 1e-6);
	const float AtlasSize = TileSize / max(InSlot.AtlasTile.x, 1e-6);
	const int TileSizeInt = (int)round(TileSize);
	const int2 TileMin = int2(round(InSlot.AtlasTile.zw * AtlasSize));
	const int2 TileMax = (TileMin + int2(TileSizeInt, TileSizeInt)) - 1;
	const int2 BaseTexel = int2(floor(InTileTexel));

	float LitNum = 0.0;
	for (int OffsetY = -PE_SHADOW_PCF_RADIUS; OffsetY <= PE_SHADOW_PCF_RADIUS; OffsetY++)
	{
		for (int OffsetX = -PE_SHADOW_PCF_RADIUS; OffsetX <= PE_SHADOW_PCF_RADIUS; OffsetX++)
		{
			const int2 AtlasTexel = clamp(BaseTexel + int2(OffsetX, OffsetY), TileMin, TileMax);
			const float StoredDepth = PE_LOAD_TEX2D(ShadowMapAtlas, int3(AtlasTexel, 0)).r;
			LitNum += (InCompareDepth <= StoredDepth) ? 1.0 : 0.0;
		}
	}
	return LitNum / (float)(PE_SHADOW_PCF_TAP_NUM * PE_SHADOW_PCF_TAP_NUM);
}

// One shadow test against one slot: project, bias, and filter. The normal offset is applied to
// the world position before the projection, because it is a world space displacement; the
// constant bias is added to the normalized depth, which is the space the comparison happens in.
float SampleShadowSlot(in ShadowSlotData InSlot, in float3 InWorldPosition, in float3 InWorldNormal)
{
	const float3 OffsetPosition = InWorldPosition + (InWorldNormal * InSlot.BiasParams.z);
	const float4 ClipPosition = mul(float4(OffsetPosition, 1.0), InSlot.ViewProjection);
	if (abs(ClipPosition.w) < 1e-6)
	{
		return 1.0;
	}

	const float3 NormalizedPosition = ClipPosition.xyz / ClipPosition.w;
	if ((NormalizedPosition.z <= 0.0) || (NormalizedPosition.z >= 1.0))
	{
		return 1.0;
	}

	// Clip space to texture space, with the y flip stated above.
	float2 AtlasUv = (NormalizedPosition.xy * 0.5) + 0.5;
	AtlasUv.y = 1.0 - AtlasUv.y;
	if ((AtlasUv.x < 0.0) || (AtlasUv.x > 1.0) || (AtlasUv.y < 0.0) || (AtlasUv.y > 1.0))
	{
		return 1.0;
	}

	const float TileSize = 1.0 / max(InSlot.ShadowParams.y, 1e-6);
	const float CompareDepth = NormalizedPosition.z + InSlot.BiasParams.x;
	return SampleShadowTile(InSlot, AtlasUv * TileSize, CompareDepth);
}

// Selects the cascade a pixel belongs to and returns its shadow term, blending into the next
// cascade over the transition band the slot carries. A pixel past the last cascade, or one whose
// light casts no shadow, is fully lit.
float ComputeDirectionalShadow(in DirectionalLightData InLight, in float3 InWorldPosition, in float3 InWorldNormal)
{
	if ((InLight.ShadowMapIndex == 0u) || (InLight.CascadeCount == 0u))
	{
		return 1.0;
	}

	// The split a cascade states is a distance along the view's forward axis, so the pixel's
	// depth is measured the same way rather than as a straight line distance.
	const float ViewDepth = dot(InWorldPosition - ViewPosition.xyz, ViewDirection.xyz);
	const uint CascadeNum = min(InLight.CascadeCount, PE_SHADOW_CASCADE_NUM_MAX);
	for (uint CascadeIndex = 0u; CascadeIndex < CascadeNum; CascadeIndex++)
	{
		ShadowSlotData Slot = DecodeShadowSlot(InLight.ShadowMapIndex + CascadeIndex);
		if (Slot.ShadowParams.w < 0.5)
		{
			continue;
		}
		if (ViewDepth > Slot.ShadowParams.x)
		{
			continue;
		}

		float Shadow = SampleShadowSlot(Slot, InWorldPosition, InWorldNormal);

		// The band at the far end of a cascade fades into the next one. Only a cascade that has
		// a successor blends: the last one has nothing behind it to fade into, and its blend
		// fraction is zero because of that.
		const float BlendFraction = Slot.BiasParams.w;
		if ((BlendFraction > 0.0) && ((CascadeIndex + 1u) < CascadeNum))
		{
			const float BlendStart = Slot.ShadowParams.x * (1.0 - BlendFraction);
			if (ViewDepth > BlendStart)
			{
				ShadowSlotData NextSlot = DecodeShadowSlot(InLight.ShadowMapIndex + CascadeIndex + 1u);
				if (NextSlot.ShadowParams.w >= 0.5)
				{
					const float NextShadow = SampleShadowSlot(NextSlot, InWorldPosition, InWorldNormal);
					const float Blend = saturate((ViewDepth - BlendStart) / max(Slot.ShadowParams.x - BlendStart, 1e-6));
					Shadow = lerp(Shadow, NextShadow, Blend);
				}
			}
		}
		return Shadow;
	}
	return 1.0;
}

#endif	//SHADER_SHADOW_RECEIVER

#if SHADER_LIGHTING_PASS

// The directional shadow term as the deferred lighting pass sees it: one channel of the screen
// space mask the shadow mask pass resolved. The channel index is the light's position in the
// light list, which is the same index the mask pass wrote.
float ReadDirectionalShadowMask(in DirectionalLightData InLight, in uint InLightIndex, in float2 InScreenTexel)
{
	if ((InLight.ShadowMapIndex == 0u) || (InLight.CascadeCount == 0u))
	{
		return 1.0;
	}
	if (InLightIndex >= (uint)(PE_SHADOW_MASK_LIGHT_NUM_MAX))
	{
		return 1.0;
	}
	if ((RenderFlags.x & PE_SHADOW_MASK_VALID_BIT) == 0u)
	{
		return 1.0;
	}

	// ScreenPosition is SV_Position: xy is the pixel centre in pixels, so the mask's texture
	// coordinate is that value normalized and nothing more. Adding another half texel here would
	// sample half a pixel to the right and down, which is a one pixel shadow offset rather than a
	// visible error - which is exactly the kind of mistake that survives a visual check.
	const float2 MaskUv = InScreenTexel * InvScreenSize;
#if PE_SHADOW_MASK_SET_NUM == 1
	float4 ShadowMask = PE_SAMPLE_TEX2D(DirectionalShadowMask0, SamplerLinearClamp, MaskUv);
	// Channel selection written out instead of a dynamic subscript: the index is uniform across
	// the draw in practice, and a compared chain is what every compiler handles unambiguously.
	if (InLightIndex == 0u)
	{
		return ShadowMask.x;
	}
	if (InLightIndex == 1u)
	{
		return ShadowMask.y;
	}
	if (InLightIndex == 2u)
	{
		return ShadowMask.z;
	}
	return ShadowMask.w;
#else
	// More than one mask set: every target carries four lights. The extra targets have to be
	// declared by the lighting program before this branch can be compiled, which is part of the
	// configuration step that raises PE_SHADOW_MASK_SET_NUM. Nothing here is compiled in the
	// default build.
	const uint SetIndex = InLightIndex / 4u;
	const uint ChannelIndex = InLightIndex % 4u;
	float4 ShadowMask = PE_SAMPLE_TEX2D(DirectionalShadowMask0, SamplerLinearClamp, MaskUv);
	if (SetIndex == 1u)
	{
		ShadowMask = PE_SAMPLE_TEX2D(DirectionalShadowMask1, SamplerLinearClamp, MaskUv);
	}
	else if (SetIndex == 2u)
	{
		ShadowMask = PE_SAMPLE_TEX2D(DirectionalShadowMask2, SamplerLinearClamp, MaskUv);
	}
	else if (SetIndex == 3u)
	{
		ShadowMask = PE_SAMPLE_TEX2D(DirectionalShadowMask3, SamplerLinearClamp, MaskUv);
	}
	if (ChannelIndex == 0u)
	{
		return ShadowMask.x;
	}
	if (ChannelIndex == 1u)
	{
		return ShadowMask.y;
	}
	if (ChannelIndex == 2u)
	{
		return ShadowMask.z;
	}
	return ShadowMask.w;
#endif
}

#endif	//SHADER_LIGHTING_PASS

// A local light's shadow: one slot for a spot, six consecutive ones for a point. Which face of a
// point light's cube a pixel is in is the axis of the vector from the light to the surface that
// has the largest magnitude, with the positive axis winning a tie. That order has to match
// FaceDirections in RLocalLightSceneProxy::BuildShadowViewDomain - the renderer built the six
// views in it, so the receiver has to name the same one.
float SampleLocalLightShadow(in LocalLightData InLight, in float3 InWorldPosition, in float3 InWorldNormal)
{
#if SHADER_SHADOW_RECEIVER
	if (InLight.ShadowMapIndex == 0u)
	{
		return 1.0;
	}

	uint SlotIndex = InLight.ShadowMapIndex;
	if (InLight.LightType != LIGHT_TYPE_SPOT)
	{
		const float3 ToSurface = InWorldPosition - InLight.WorldPosition;
		const float3 AbsDirection = abs(ToSurface);
		if ((AbsDirection.x >= AbsDirection.y) && (AbsDirection.x >= AbsDirection.z))
		{
			SlotIndex += (ToSurface.x >= 0.0) ? 0u : 1u;
		}
		else if (AbsDirection.y >= AbsDirection.z)
		{
			SlotIndex += (ToSurface.y >= 0.0) ? 2u : 3u;
		}
		else
		{
			SlotIndex += (ToSurface.z >= 0.0) ? 4u : 5u;
		}
	}

	ShadowSlotData Slot = DecodeShadowSlot(SlotIndex);
	if (Slot.ShadowParams.w < 0.5)
	{
		return 1.0;
	}
	return SampleShadowSlot(Slot, InWorldPosition, InWorldNormal);
#else
	return 1.0;
#endif
}

#endif	//SHADER_SHADOWS_HLSL
