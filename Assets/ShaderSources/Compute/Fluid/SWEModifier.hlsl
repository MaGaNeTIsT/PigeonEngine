#ifndef SWE_MODIFIER_HLSL
#define SWE_MODIFIER_HLSL

// ---------------------------------------------------------------------------
// Shallow water equation, modifier.
//
// Provenance: SWE_Modifier.usf, the unified source and force injection pass. It
// places a shaped footprint over the simulation, samples the terrain and the
// injected values at the footprint's own frame, and blends them into the state
// the pass reads back: the new state is the old one scaled by one minus the
// footprint weight plus the injected value times that weight. Every expression
// below is the legacy one, character for character; what changed is the
// parameter path, the entry point name, the pixel branch and the four renames
// listed below.
//
// What the port changed:
//
//   old Texture2D GroundTex : register(t0), WaterMask : register(t1) and
//   RWTexture2D OutVelocityHeightFoamTex : register(u0), plus the #define
//   aliases that named them
//       new the same three names declared in Compute/Fluid/SWEModifier.json,
//       which is where every binding belongs now, plus VelocityHeightFoamTex.
//       That fourth one has no reader in this pass: it is declared because this
//       file includes Public/Fluid/ShallowWaterEquationCommon.hlsl, whose
//       FluxSample and the steps around it read the state through that name, and
//       an HLSL function the pass never calls still has to see its globals
//       declared. Every pass that includes the library declares the state
//       texture for that reason, and the compiler strips the binding again when
//       nothing in the compiled unit uses it
//   old UCGSampler
//       new SamplerLinearClamp, the engine sampler, referenced by name and not
//       declared here. Linear with clamp is what the already migrated SWE passes
//       use; the legacy sampler object's own settings are not in these files
//   old the SWE_ parameters and the VelocityX, Volume, Mode, ModLoc, ModSize,
//   ForwardVector and the rest of the UCGVectorParams #defines
//       new named SWEParams fields, declared in the component JSON: the same
//       twenty fields the two already migrated SWE passes declare, then the
//       pass's own fields in the legacy order. Two things about the names:
//
//       - the legacy split one float4 of UCGVectorParams into the aliases
//         Velocity ( .xy ), VelocityX ( .x ), VelocityY ( .y ), Volume ( .z )
//         and Mode ( .w ). Here the three the body reads are three fields, and
//         they are named SourceVelocityX, SourceVelocityY and SourceVolume with
//         SourceMode for the mode. They are renamed rather than kept because a
//         constant buffer field named VelocityX or Volume would be shadowed by
//         the local of that name inside the shared library's FluxAdvectFoam and
//         FluxSample, which this pass compiles along with everything else in
//         that header. Source is the legacy's own word for what the modifier
//         injects: the body already calls the injected value SrcValue
//       - the aliases the body never reads are not declared at all: Velocity,
//         ModLocX, ModLocY, ModSizeX, ModSizeY, ForwardVectorZ. A field with no
//         reader is a field a dispatcher has to fill for nothing. ForwardVector
//         is kept as the float3 the legacy group was, and the body reads its
//         .xy, which is what the legacy's ForwardVectorX and ForwardVectorY
//         aliases were
//   old the four lines that moved the texel into the modifier's frame, written
//   out inline here
//       new a call to FluxModifierTransform in
//       Public/Fluid/ShallowWaterEquationModifier.hlsl, which is the same
//       arithmetic in the same order with this pass's ModLoc, ModSize,
//       ForwardVector.xy, Location and Scale as its arguments. The whirlpool
//       pass computes the same transform, so the two passes now share one copy
//       instead of keeping two
//   old #if SWE_COMPUTE_PATH around the read modify write of the state, with the
//   pixel branch behind its #else that wrote the injected value alone, and the
//   MainVS / MainPS fallback pair
//       new the compute branch only. The new system's compute pass has no pixel
//       path. The compute branch is the one that reads the state texture back and
//       blends into it, so the pass keeps its read, its blend and its write
//   old MainCS
//       new CSMain, the Entry the program description names
//   old BRANCH, bMasked and the early exit's UE platform macros
//       new [branch], IsMasked, and SWE_EARLY_EXIT_USE_WAVE_FUNCTION defaulted to
//       0 in the shared library. The early exit block is otherwise the legacy's,
//       threshold literal included, and off by default as the legacy had it
//       one thing about it is not: the legacy put the block after a bounds test
//       that returned, so a group whose last row fell out of the dispatch
//       reached the barrier with fewer threads than the group has, and a group
//       barrier that part of a group never reaches is undefined behaviour. The
//       test is a predicate here and the return sits below the barrier, which
//       every thread of the group now reaches; a thread outside the simulation
//       does not touch the tile mask
// ---------------------------------------------------------------------------

#include "Public/Fluid/ShallowWaterEquationCommon.hlsl"
#include "Public/Fluid/ShallowWaterEquationModifier.hlsl"

// Groupshared state of this entry point, so it lives in this file; see the note
// in the shared library.
#if SWE_EARLY_EXIT_USES_TILE_MASK
groupshared uint SharedTileBitmask;
#endif	//SWE_EARLY_EXIT_USES_TILE_MASK

// The footprint: a rounded square of the given radius, its edge softened by the
// hardness. InRadius is a fraction of the half extent, InHardness the reciprocal
// width of the falloff.
float FluxModifierShape( in float2 InTexcoord, in float InRadius, in float InHardness )
{
	float2 UV = abs( InTexcoord * 2 - 1 ) - (1 - InRadius);
	float Dist = InRadius - (distance( 0, max( 0, UV ) ) + min( 0, max( UV.x, UV.y ) ));
	return saturate( Dist * InHardness );
}

// The value the modifier injects, before the footprint weight is applied. The
// two branches are the two ways of expressing the same target: Offset is the
// water level moved by ZOffset, Height is the water level driven to ZOffset
// above the ground, and Mode selects and blends between them.
float4 FluxModifierMode( in float InWeight, in float2 InTexcoord, in float InSimulationHeight )
{
	float4 Result;

	float GroundValue = PE_SAMPLE_TEX2D_LEVEL( GroundTex, SamplerLinearClamp, InTexcoord, 0 ).r;
	// GroundRT.R holds the ground's depth RELATIVE to the reference plane
	// (reference-plane Z - ground world Z), matching the original FluidFlux
	// ground-capture contract so this subsystem stays data-compatible with the
	// original plugin. InSimulationHeight is the reference-plane Z (the solver
	// domain top = component Z), so GroundHeight = InSimulationHeight -
	// GroundValue converts the relative depth back to the ground's absolute
	// world Z. If the ground RT ever holds the ABSOLUTE ground world Z instead
	// (non-original layout), switch to GroundHeight = GroundValue; the formula
	// below would then fold the ground height in twice and lift the surface by
	// 2*GroundZ - InSimulationHeight.
	float GroundHeight = InSimulationHeight - GroundValue;
	float Offset = min( (ZOffset), (SourceVolume) + GroundHeight ) - GroundHeight;
	float Height = max( 0, (SourceVolume) + (ZOffset) - GroundHeight );
	Result = float4( (SourceVelocityX), (SourceVelocityY), lerp( Offset, Height, saturate( (SourceMode) - 1 ) ), saturate( SourceMode ) ) * InWeight;

	return Result;
}

// The per texel step. Note where the footprint is sampled: Texcoord0 is the
// texel in the pass's own frame, and Texcoord1 is that texel moved into the
// modifier's frame, which is where the terrain and the injected value are read.
// The footprint weight itself is evaluated in Texcoord0, not Texcoord1, as the
// legacy evaluates it.
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

	float2 Texcoord0 = saturate( (LocalCoord.xy + 0.5) / LocalRes.xy );
	float Weight = FluxModifierShape( Texcoord0.xy, (Radius), (Hardness) );

	float2 Texcoord1 = FluxModifierTransform( Texcoord0.xy, (ModLoc), (ModSize), ForwardVector.xy, Location.xy, Scale.xy );
	float4 SrcValue = FluxModifierMode( Weight, Texcoord1.xy, Height );
	float3 SrcXYZ = SrcValue.xyz * (Intensity);
	float SrcW = SrcValue.w;

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

#endif	//SWE_MODIFIER_HLSL
