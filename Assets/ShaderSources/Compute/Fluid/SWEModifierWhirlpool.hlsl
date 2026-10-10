#ifndef SWE_MODIFIER_WHIRLPOOL_HLSL
#define SWE_MODIFIER_WHIRLPOOL_HLSL

// ---------------------------------------------------------------------------
// Shallow water equation, modifier whirlpool.
//
// Provenance: SWE_ModifierWhirlpool.usf, the author's own addition to the plugin
// family, marked in the legacy with the //++[Bili]Begin marker it still carries
// below. It stirs the state: a swirl force around the modifier's location, taken
// at the right and bottom neighbours of the texel rather than at the texel
// itself, blended into the velocity the state already holds, with the water depth
// of the middle of the swirl pulled towards zero and the whole injection weighted
// by the distance to the modifier's centre and by the translucency. Every
// expression below is the legacy one, character for character; what changed is
// the parameter path, the entry point name, the pixel branch and the renames
// listed below.
//
// What the port changed:
//
//   old Texture2D VelocityHeightFoamTex : register(t0), WaterMask : register(t1)
//   and RWTexture2D OutVelocityHeightFoamTex : register(u0), plus the #define
//   aliases that named them
//       new WaterMask, VelocityHeightFoamTex, GroundTex and
//       OutVelocityHeightFoamTex declared in
//       Compute/Fluid/SWEModifierWhirlpool.json, which is where every binding
//       belongs now. This pass's own body samples nothing but the mask and takes
//       the state from the UAV it writes; VelocityHeightFoamTex and GroundTex are
//       declared because this file includes
//       Public/Fluid/ShallowWaterEquationCommon.hlsl, whose FluxSample and the
//       steps around it read the state through those names, and an HLSL function
//       the pass never calls still has to see its globals declared. The compiler
//       strips both bindings again when nothing in the compiled unit uses them.
//       The legacy read the state from VelocityHeightFoamTex only in its pixel
//       branch, which the port dropped
//   old UCGSampler
//       new SamplerLinearClamp, the engine sampler, referenced by name and not
//       declared here. The legacy's own sampler settings are not recoverable
//   old the SWE_ parameters and the ModLocation, ModRadius, ModRadial,
//   ModSuction, Translucent and Time #defines of UCGVectorParams
//       new named SWEParams fields, declared in the component JSON: the same
//       twenty fields the two already migrated SWE passes declare, then
//       ModLocation, ModRadius, ModRadial, ModSuction, Translucent and Time in
//       the legacy order, with the legacy names. Time is the legacy's own clock
//       for the swirl, not the engine's GlobalParams time, so it stays a field of
//       this block. ModLocationX and ModLocationY have no reader in the body and
//       are not declared
//   old FluxModifierTransform, a local function of this pass
//       new Public/Fluid/ShallowWaterEquationModifier.hlsl, which is the same
//       arithmetic. The modifier pass computes the same transform inline, so the
//       two now share one copy instead of two
//   old FluxSimulationUVToWorld, FluxVelocitySeparation, FluxSwirl and
//   FluxForceWhirlpool, the pass's own helpers
//       new kept here, with their arguments unchanged and their parameter names
//       given the In/Out prefixes. The one argument that changed shape is the
//       forward direction of FluxForceWhirlpool, which is still the literal
//       float2( 1, -1 ) the legacy passed
//   old FluxForceWhirlpool read the ModLocation, ModRadius, ModRadial and
//   ModSuction #defines straight out of the parameter block
//       new it still does, through the named fields of the same block. It is the
//       only helper in the family that reads the block directly, and it is left
//       that way because it was written that way
//   old #if SWE_COMPUTE_PATH around the read of the state, whose #else sampled
//   the input texture with UCGSampler instead, plus the MainVS / MainPS fallback
//       new the compute branch only, which reads the state through the same UAV
//       the pass writes. The new system's compute pass has no pixel path
//   old MainCS
//       new CSMain, the Entry the program description names
//   old BRANCH and the early exit's UE platform macros
//       new [branch] and SWE_EARLY_EXIT_USE_WAVE_FUNCTION defaulted to 0 in the
//       shared library, with bMasked renamed IsMasked. The block is otherwise
//       the legacy's and off by default as the legacy had it
//       one thing about it is not: the legacy put the block after a bounds test
//       that returned, so a group whose last row fell out of the dispatch
//       reached the barrier with fewer threads than the group has, and a group
//       barrier that part of a group never reaches is undefined behaviour. The
//       test is a predicate here and the return sits below the barrier, which
//       every thread of the group now reaches; a thread outside the simulation
//       does not touch the tile mask
//
// Two things the port kept although they do nothing, both the legacy's:
//
//   Texcoord1, the result of FluxModifierTransform, is computed and never read.
//   It is kept because the call is part of the pass's shape and because it shows
//   which transform the two modifier passes share
//
//   OutVelocityHeightFoamTex[ Coord ] is read twice, once into
//   VelocityHeightFoamValue and once into DstValue, with no write between them.
//   The second read is the legacy's and it is left in place rather than folded
//   into the first, because folding it is a change to live code in a pass whose
//   maths is supposed to arrive unchanged
// ---------------------------------------------------------------------------

#include "Public/Fluid/ShallowWaterEquationCommon.hlsl"
#include "Public/Fluid/ShallowWaterEquationModifier.hlsl"

// Groupshared state of this entry point, so it lives in this file; see the note
// in the shared library.
#if SWE_EARLY_EXIT_USES_TILE_MASK
groupshared uint SharedTileBitmask;
#endif	//SWE_EARLY_EXIT_USES_TILE_MASK

//++[Bili]Begin
//Modify by HengCan | Modifier Whirlpool Shader | 2026/04/07 14:12 |

// UV to world: the texcoord times the area size plus its world origin.
float2 FluxSimulationUVToWorld( in float2 InTexcoord, in float2 InWorldLocation, in float2 InWorldScale )
{
	return (InTexcoord.xy * InWorldScale.xy + InWorldLocation.xy);
}

// The world positions the two swirl forces are taken at: half a texel to the
// right and half a texel down, which is where a cell centred scheme puts the
// components of one velocity.
void FluxVelocitySeparation(
	in float2	InTexcoord,
	out float2	OutRightPosition,
	out float2	OutBottomPosition,
	in float2	InWorldLocation,
	in float2	InWorldScale,
	in float2	InTexelSize)
{
	OutRightPosition = FluxSimulationUVToWorld(InTexcoord.xy + (float2(0.5, 0.0) * InTexelSize.xy), InWorldLocation, InWorldScale);
	OutBottomPosition = FluxSimulationUVToWorld(InTexcoord.xy + (float2(0.0, 0.5) * InTexelSize.xy), InWorldLocation, InWorldScale);
}

// One swirl sample: the tangential part of InDirection, rotated by 90 degrees
// (Dir.yx), falls off with the distance to the centre and is scaled by
// InIntensity, and the radial part pulls towards the centre by InSuction. The
// 0.001 floor on the distance is the legacy's, and it is what keeps the radial
// term finite at the centre.
float2 FluxSwirl( in float2 InPosition, in float2 InCenter, in float2 InDirection, in float InRadius, in float InIntensity, in float InSuction )
{
	float2 Vec = (InPosition.xy - InCenter.xy);
	float2 Dir = (Vec.xy * InDirection.xy);
	float Dist = distance(InPosition.xy, InCenter.xy);
	return ((Dir.yx * saturate((1 - saturate(Dist / InRadius)) * InIntensity)) + ((Vec.xy / max(Dist, 0.001)) * InSuction));
}

float2 FluxForceWhirlpool( in float2 InWorldPosition )
{
	return FluxSwirl( InWorldPosition.xy, (ModLocation), float2(1, -1), (ModRadius), (ModRadial), (ModSuction) );
}

bool ComputeModifier(
	in int2		InCoord,
	out float4	OutValue)
{
	float2 Location; float2 Scale; float2 InvScale; float Height; float2 TexelSize; float2 Resolution;
	FluxSimulationUV( Location, Scale, InvScale, Height, TexelSize, Resolution );

	int2 LocalCoord = InCoord - asint( PixelAnchor );
	int2 LocalRes = asint( PixelSize );
	int2 Res = int2( (int)(Resolution.x), (int)(Resolution.y) );
	// The bounds test is a predicate now and not a return. The early exit below
	// owns a group barrier, every thread of the group has to reach it, and a
	// thread that left the function ahead of it would leave the group short at
	// that barrier. The return below the barrier takes this thread out instead,
	// and until it reaches that return it contributes nothing to the reduction.
	const bool IsOutside = any( InCoord >= Res ) || any( LocalCoord >= LocalRes );

	bool IsEarlyExit = false;
#if SWE_ENABLE_EARLY_EXIT
	// A thread outside the simulation counts as one with nothing to do, so its
	// tile bit stays clear and the mask is never sampled out of the area.
	const float2 Texcoord	= (InCoord.xy + 0.5) / Resolution.xy;
	const bool IsMasked		= IsOutside || ((PE_SAMPLE_TEX2D_LEVEL( WaterMask, SamplerLinearClamp, Texcoord, 0 ).r) <= 0.007844);

#if SWE_EARLY_EXIT_USE_WAVE_FUNCTION
	IsEarlyExit = WaveActiveBitAnd( IsMasked ? 1u : 0u );
#else	// !SWE_EARLY_EXIT_USE_WAVE_FUNCTION
	uint MaskBit = 0;
	if (!IsOutside)
	{
		InterlockedOr( SharedTileBitmask, (IsMasked ? 0 : 1), MaskBit );
	}
	GroupMemoryBarrierWithGroupSync();
	IsEarlyExit = SharedTileBitmask == 0;
#endif	//SWE_EARLY_EXIT_USE_WAVE_FUNCTION
#endif	//SWE_ENABLE_EARLY_EXIT

	[branch] if (IsOutside || IsEarlyExit)
	{
		OutValue = float4( 0, 0, 0, 0 );
		return false;
	}

	float2 Texcoord0 = saturate( (InCoord.xy + 0.5) / Resolution.xy );
	float2 Texcoord1 = FluxModifierTransform( Texcoord0.xy, (ModLocation), (ModRadius) * 2, float2(1, 0), Location.xy, Scale.xy );
	float ScaledTime = (Time) * 0.15;
	float2 TimeVec;
	sincos(ScaledTime, TimeVec.x, TimeVec.y);
	float Weight = 1 - saturate(distance((ModLocation), FluxSimulationUVToWorld( Texcoord0.xy, Location.xy, Scale.xy ) + (TimeVec.xy * 300)) / (ModRadius));
	float Weight2 = Weight * Weight;
	float Weight3 = Weight2 * Weight;
	float4 VelocityHeightFoamValue = OutVelocityHeightFoamTex[ InCoord ];
	float WaterDepth = VelocityHeightFoamValue.z * (1 - (saturate((Weight3 - 0.5) * 2.0) * 1.0));
	float2 Position0, Position1;
	FluxVelocitySeparation( Texcoord0.xy, Position0, Position1, Location.xy, Scale.xy, TexelSize.xy );
	float2 Velocity0 = FluxForceWhirlpool( Position0.xy );
	float2 Velocity1 = FluxForceWhirlpool( Position1.xy );
	float2 Velocity = lerp(float2(Velocity0.x, Velocity1.y) * Weight2, VelocityHeightFoamValue.xy, 0.7);

	float SrcW = Weight * (Translucent);
	float3 SrcXYZ = float3(Velocity.xy, WaterDepth) * SrcW;

	float4 DstValue = OutVelocityHeightFoamTex[ InCoord ];
	OutValue = float4( DstValue.xyz * (1 - SrcW) + SrcXYZ.xyz, DstValue.w * (1 - SrcW));
	return true;
}

[numthreads(SHADER_THREAD_GROUP_X, SHADER_THREAD_GROUP_Y, 1)]
void CSMain(uint3 InDispatchThreadID : SV_DispatchThreadID
#if SWE_EARLY_EXIT_USES_TILE_MASK
	, uint InGroupIndex : SV_GroupIndex
#endif	//SWE_EARLY_EXIT_USES_TILE_MASK
)
{
#if SWE_EARLY_EXIT_USES_TILE_MASK
	if (InGroupIndex == 0)
	{
		SharedTileBitmask = 0;
	}
	GroupMemoryBarrierWithGroupSync();
#endif	//SWE_EARLY_EXIT_USES_TILE_MASK
	int2 Coord = int2( InDispatchThreadID.xy ) + asint( PixelAnchor );
	float4 Value;
	if (!(ComputeModifier( Coord, Value )))
	{
		return;
	}
	OutVelocityHeightFoamTex[ Coord ] = Value;
}

//--[Bili]End

#endif	//SWE_MODIFIER_WHIRLPOOL_HLSL
