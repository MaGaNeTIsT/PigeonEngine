#ifndef SHALLOW_WATER_EQUATION_COMMON_HLSL
#define SHALLOW_WATER_EQUATION_COMMON_HLSL

// ---------------------------------------------------------------------------
// Shallow water equation, the shared algorithm.
//
// Provenance: FluxWater/ShallowWaterEquationCommon.hlsl, plus the per step
// functions the legacy FluxWater/CS_SWEIntegrateHeight/SWEIntegrateHeight.hlsl
// kept to itself. Those steps live here now because the height pass and the
// velocity pass both start from the same advection, and neither of them should
// own a copy of it. Every expression below is the legacy one, character for
// character; what changed is the parameter path and the styling.
//
// What the port changed:
//
//   old cbuffer ConstantBufferSWECommon : register(b1), declared in HLSL
//       new SWEParams, declared by the component JSON of each pass, whose fields
//       are plain globals here. HLSL never declares a binding, so nothing in
//       this file may say cbuffer, Texture2D or SamplerState
//   old SWE_DeltaTime                    new SimulationDeltaTime
//       renamed because the system layer already declares a DeltaTime in
//       GlobalParams, and two globals of one compilation may not share a name
//   old UniversalVectorParams[0..2] with the FoamFadeSpeed and the other nine
//   #define aliases into a block every material shared
//       new the same ten scalars as named fields of SWEParams, so a simulation
//       parameter is named instead of being an index into a generic block
//   old SWE_InvResX and the rest, #define aliases of the same block
//       new named SWEParams fields with the SWE_ prefix dropped, the block name
//       already says shallow water equation
//   old VelocityHeightFoamTex.SampleLevel(UCGSampler, UV, 0) and the same for
//   GroundTex
//       new PE_SAMPLE_TEX2D_LEVEL(VelocityHeightFoamTex, SamplerLinearClamp, UV, 0)
//       UCGSampler belonged to the retired material parameter system. The engine
//       sampler that replaces it is linear because the taps are placed on half
//       texels on purpose (FluxAdvectFoam averages two neighbours that way),
//       and clamp because FluxSample masks the velocity on the negative side of
//       the area, which only means something if a sample outside it cannot wrap
//   old the SWE_USE_FLUX_SAMPLE_FUNCTION switch around FluxSample
//       new FluxSample is unconditional: every step in this file samples through
//       it, so a switch that can take the sampler away from the library's own
//       advection has no use
//   old ../ShallowWaterEquationCommon.ush
//       new Public/Fluid/ShallowWaterEquationCommon.hlsl. The .ush extension was
//       wrong for a file with this content, and the path climbed out of the
//       shader root, which no include does any more
//   the FluxOutline switch is kept, still off by default
//       its inner #if 0 FluxSobelOffsets stub and the empty scope in its body
//       were dropped; nothing else about it was touched. Neither pass calls it
//       today, but it is part of the algorithm and its home is this file
//   the early exit switches, which the legacy declared in this same header
//       SWE_ENABLE_EARLY_EXIT and the condition derived from it are added at the
//       foot of this file, because the state injection passes the port migrated
//       after this file are their only users and every one of those includes
//       this header. Nothing above them changed
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"

// The legacy handed every simulation constant out through this one accessor.
// The name is the legacy one and it is a poor description of what it returns,
// but renaming it would be churn in a file whose whole job is to be the same
// algorithm in a new place.
void FluxSimulationUV(
	out float2	OutLocation,
	out float2	OutScale,
	out float2	OutInvScale,
	out float	OutHeight,
	out float2	OutTexelSize,
	out float2	OutResolution)
{
	OutLocation = float2( AreaLocX, AreaLocY );
	OutScale = float2( AreaSizeX, AreaSizeY );
	OutInvScale = float2( 1.0f / AreaSizeX, 1.0f / AreaSizeY );
	OutHeight = CaptureHeight;
	OutTexelSize = float2( InvResX, InvResY );
	OutResolution = float2( ResX, ResY );
}

// One state tap. bCleanVelocity became InIsCleanVelocity: the legacy bool had no
// In/Out prefix and the house style gives booleans InIs.
void FluxSample(
	in float2	InTexcoord,
	in float2	InOffset,
	in bool		InIsCleanVelocity,
	out float2	OutVelocity,
	out float	OutVolume,
	out float	OutFoam,
	out float	OutGroundHeight,
	out float	OutGroundHeightLocal,
	out float	OutSurfaceHeight)
{
	float2 Location; float2 Scale; float2 InvScale; float Height; float2 TexelSize; float2 Resolution;
	FluxSimulationUV( Location, Scale, InvScale, Height, TexelSize, Resolution );

	float2 UV = TexelSize.xy * InOffset.xy + InTexcoord.xy;
	float4 VelocityHeightFoamValue = PE_SAMPLE_TEX2D_LEVEL( VelocityHeightFoamTex, SamplerLinearClamp, UV, 0 ).rgba;
	float GroundValue = PE_SAMPLE_TEX2D_LEVEL( GroundTex, SamplerLinearClamp, UV, 0 ).r;

	OutVelocity = InIsCleanVelocity ? (VelocityHeightFoamValue.xy * step( 0, UV.xy )) : (VelocityHeightFoamValue.xy);
	OutVolume = VelocityHeightFoamValue.z;
	OutFoam = VelocityHeightFoamValue.w;
	OutGroundHeight = Height - GroundValue.x;
	OutGroundHeightLocal = -(GroundValue.x);
	OutSurfaceHeight = max( 0, VelocityHeightFoamValue.z ) + OutGroundHeight;
}

#ifndef SWE_USE_FLUX_OUTLINE_FUNCTION
	#define SWE_USE_FLUX_OUTLINE_FUNCTION	0
#endif	//SWE_USE_FLUX_OUTLINE_FUNCTION

#if SWE_USE_FLUX_OUTLINE_FUNCTION
float FluxOutline(
	in float2	InTexcoord,
	in float	InSmoothBlend,
	in float	InMinOutline,
	in float	InOutlineThreshold)
{
	float2 Velocity; float Volume; float Foam; float GroundHeight; float GroundHeightLocal; float SurfaceHeight;
	FluxSample( InTexcoord, float2( 0, 0 ), false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );

	float CenterVolume = Volume;
	float CenterVolumePos = max( CenterVolume, 0 );
	float CenterGroundHeight = GroundHeight;

	float NeighborVolume[8];
	float NeighborSurfaceHeight[8];
	float2 SobelOffset[ 8 ];

	FluxSample( InTexcoord, float2( -1, 0 ), false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[0] = Volume; NeighborSurfaceHeight[0] = SurfaceHeight;
	FluxSample( InTexcoord, float2( 1, 0 ), false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[1] = Volume; NeighborSurfaceHeight[1] = SurfaceHeight;
	FluxSample( InTexcoord, float2( 0, -1 ), false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[2] = Volume; NeighborSurfaceHeight[2] = SurfaceHeight;
	FluxSample( InTexcoord, float2( 0, 1 ), false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[3] = Volume; NeighborSurfaceHeight[3] = SurfaceHeight;
	FluxSample( InTexcoord, float2( -1, -1 ), false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[4] = Volume; NeighborSurfaceHeight[4] = SurfaceHeight;
	FluxSample( InTexcoord, float2( 1, -1 ), false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[5] = Volume; NeighborSurfaceHeight[5] = SurfaceHeight;
	FluxSample( InTexcoord, float2( -1, 1 ), false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[6] = Volume; NeighborSurfaceHeight[6] = SurfaceHeight;
	FluxSample( InTexcoord, float2( 1, 1 ), false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[7] = Volume; NeighborSurfaceHeight[7] = SurfaceHeight;

	float4 VolumeA = float4( NeighborVolume[0], NeighborVolume[1], NeighborVolume[2], NeighborVolume[3] );
	float4 VolumeB = float4( NeighborVolume[4], NeighborVolume[5], NeighborVolume[6], NeighborVolume[7] );
	float4 SurfaceA = float4( NeighborSurfaceHeight[0], NeighborSurfaceHeight[1], NeighborSurfaceHeight[2], NeighborSurfaceHeight[3] );
	float4 SurfaceB = float4( NeighborSurfaceHeight[4], NeighborSurfaceHeight[5], NeighborSurfaceHeight[6], NeighborSurfaceHeight[7] );

	float4 MaxVolume = max( VolumeA, VolumeB );
	float2 MaxVolumePair = max( MaxVolume.rg, MaxVolume.ba );
	float MaxNeighborVolume = max( MaxVolumePair.r, MaxVolumePair.g );
	float MaxAllVolume = max( CenterVolume, MaxNeighborVolume );

	float4 WeightsA = saturate( VolumeA );
	float4 WeightsB = saturate( VolumeB );
	float4 WeightedSurfaceA = WeightsA * SurfaceA;
	float4 WeightedSurfaceB = WeightsB * SurfaceB;
	float WeightedSurfaceSum = dot( WeightedSurfaceA + WeightedSurfaceB, 1.0f );
	float TotalWeight = max( dot( WeightsA + WeightsB, 1.0f ), 0.00001f );
	float AvgNeighborSurfaceHeight = WeightedSurfaceSum / TotalWeight;
	float SurfaceDiff = AvgNeighborSurfaceHeight - CenterGroundHeight;

	float OutlineSharp = min( min( CenterVolumePos, SurfaceDiff ), 0 );

	float NeighborSurfaceSum = dot( SurfaceA + SurfaceB, 1.0f );
	float AvgAllSurface = ( CenterVolumePos + CenterGroundHeight + NeighborSurfaceSum ) / 9.0f;
	float SmoothDiff = AvgAllSurface - CenterGroundHeight;
	float SmoothedValue = lerp( SmoothDiff, CenterVolume, InSmoothBlend );
	float OutlineSmooth = min( max( SmoothedValue, InMinOutline ), 0 );

	float Outline = ( abs( MaxAllVolume - InOutlineThreshold ) > 0.00001f )
		? ( ( MaxAllVolume >= InOutlineThreshold ) ? OutlineSharp : OutlineSmooth )
		: OutlineSmooth;

	return ( CenterVolume >= InOutlineThreshold ) ? CenterVolume : Outline;
}
#endif	//SWE_USE_FLUX_OUTLINE_FUNCTION

// Semi-Lagrangian advection of the state one tap away. This is the legacy
// FluxAdvectSimulation, kept as it was: the tap is displaced by the centre
// velocity, and the result is masked by abs( sign() ) of that same velocity so a
// cell with no velocity keeps whatever it sampled.
void FluxAdvectSimulation(
	in float2	InTexcoord,
	in float2	InOffset,
	out float2	OutVelocity,
	out float	OutVolume)
{
	float2 Velocity; float Volume; float Foam; float GroundHeight; float GroundHeightLocal; float SurfaceHeight;
	FluxSample( InTexcoord.xy, InOffset.xy, true, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );

	float2 Location; float2 Scale; float2 InvScale; float Height; float2 TexelSize; float2 Resolution;
	FluxSimulationUV( Location, Scale, InvScale, Height, TexelSize, Resolution );

	float2 UV = ((InOffset.xy - (Velocity.xy * (SimulationDeltaTime))) * TexelSize.xy) + InTexcoord.xy;
	float4 VelocityHeightFoamValue = PE_SAMPLE_TEX2D_LEVEL( VelocityHeightFoamTex, SamplerLinearClamp, UV, 0 ).rgba;

	OutVelocity = VelocityHeightFoamValue.xy * abs( sign( Velocity.xy ) );
	OutVolume = Volume;
}

// The same displacement applied to the foam, but each component is taken from
// the tap half a texel in its own direction, which is why the sampler has to be
// a linear one.
void FluxAdvectFoam(
	in float2	InTexcoord,
	in float2	InOffset,
	out float2	OutVelocity,
	out float	OutFoam)
{
	float VelocityX; float VelocityY;

	float2 Velocity; float Volume; float Foam; float GroundHeight; float GroundHeightLocal; float SurfaceHeight;
	FluxSample( InTexcoord.xy, InOffset.xy + float2( -0.5f, 0.0f ), true, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	VelocityX = Velocity.x;
	FluxSample( InTexcoord.xy, InOffset.xy + float2( 0.0f, -0.5f ), true, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	VelocityY = Velocity.y;

	float2 Location; float2 Scale; float2 InvScale; float Height; float2 TexelSize; float2 Resolution;
	FluxSimulationUV( Location, Scale, InvScale, Height, TexelSize, Resolution );

	float2 UV = ((InOffset.xy - (float2( VelocityX, VelocityY ) * (SimulationDeltaTime))) * TexelSize.xy) + InTexcoord.xy;
	float4 VelocityHeightFoamValue = PE_SAMPLE_TEX2D_LEVEL( VelocityHeightFoamTex, SamplerLinearClamp, UV, 0 ).rgba;

	OutVelocity = VelocityHeightFoamValue.xy;
	OutFoam = VelocityHeightFoamValue.w;
}

// OutIsWater is computed and never read by either pass. That is the legacy
// behaviour and it stays: the value is part of the step's signature.
void FluxHeight(
	in float2	InTexcoord,
	in float4	InVelocity,
	out float	OutFlux,
	out float	OutIsWater)
{
	float Vc; float Vl; float Vr; float Vu; float Vd;

	float2 Velocity; float Volume; float Foam; float GroundHeight; float GroundHeightLocal; float SurfaceHeight;
	FluxSample( InTexcoord.xy, float2( 0, 0 ), true, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	Vc = Volume;
	FluxSample( InTexcoord.xy, float2( 1, 0 ), true, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	Vr = Volume;
	FluxSample( InTexcoord.xy, float2( 0, 1 ), true, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	Vd = Volume;
	FluxSample( InTexcoord.xy, float2( -1, 0 ), true, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	Vl = Volume;
	FluxSample( InTexcoord.xy, float2( 0, -1 ), true, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	Vu = Volume;

	float4 Transport = clamp( lerp( Vc.xxxx, float4( Vr, Vd, Vl, Vu ), step( InVelocity.xyzw, 0 ).xyzw ), 0, (FluidTransportMax) );

	OutFlux = dot( Transport.xyzw, InVelocity.xyzw );
	OutIsWater = max( 0, min( min( min( min( Vc, Vl ), Vr ), Vu ), Vd ) );
}

// InFlux is not read by this step, in the legacy or here.
void FluxFoam(
	in float	InFoam,
	in float	InFlux,
	in float	InVolume,
	in float4	InVelocity,
	out float	OutFoam)
{
	float InputFoam = saturate( InFoam );
	float Dv = dot( InVelocity.xyzw, float4( 0.5f, 0.5f, 0.5f, 0.5f ) );
	float Foam = max( 0, Dv * (FoamShallowGenerate) ) * saturate( 1 - (InVolume - (FoamShallowDepth)) );
	Foam = (Dv * (AdvectFoam) * InputFoam + Foam - (FoamFadeSpeed)) * (SimulationDeltaTime) + InputFoam;

	OutFoam = saturate( max( Dv * (-(FoamWaveGenerate)), Foam ) );
}

// ---------------------------------------------------------------------------
// Early exit switches
//
// The legacy declared these here, in the shared SWECommon.ush, and the three
// state injection passes read them: a dispatch tile whose water mask is empty
// everywhere is skipped whole. The default is the legacy default, off, so a pass
// that does not set the define behaves exactly as it did before the switch
// existed.
//
// What the port changed:
//
//   old SWE_EARLY_EXIT_USE_WAVE_FUNCTION was
//       (SM6_PROFILE && PLATFORM_SUPPORTS_SM6_0_WAVE_OPERATIONS)
//       new 0 by default. Those are UE platform macros with no equivalent here.
//       0 keeps the groupshared tile mask, which needs no platform feature; a
//       pass that sets the define to 1 in its component JSON Defines takes the
//       wave branch instead. The two are the same decision, taken by the wave or
//       by the group, and both are correct at the 8 x 8 group the passes declare
//   old the condition was written out in full in every pass that used it
//       new derived once below, so three passes cannot drift apart. The tile mask
//       itself stays in the passes: it owns a groupshared variable and a group
//       barrier, and only the entry point file has the group index both need
//
// One invariant holds over the block that reads the tile mask, and it is why the
// passes evaluate their bounds test instead of returning from it: every thread of
// a group has to reach the group's barriers. A barrier that some threads of the
// group never reach is undefined behaviour, it can hang the GPU, and the tile
// mask is exactly the state it corrupts. So a pass may not return from the entry
// point, or from the step that holds the barrier, between the reset of the mask
// and the barrier that reads it; the thread that has nothing to do has to arrive
// at the barrier before it leaves. Nothing in this file holds a barrier, so
// nothing here is bound by that rule.
// ---------------------------------------------------------------------------

#ifndef SWE_ENABLE_EARLY_EXIT
	#define SWE_ENABLE_EARLY_EXIT				0
#endif	//SWE_ENABLE_EARLY_EXIT

#ifndef SWE_EARLY_EXIT_USE_WAVE_FUNCTION
	#define SWE_EARLY_EXIT_USE_WAVE_FUNCTION	0
#endif	//SWE_EARLY_EXIT_USE_WAVE_FUNCTION

#if SWE_ENABLE_EARLY_EXIT && (!(SWE_EARLY_EXIT_USE_WAVE_FUNCTION))
	#define SWE_EARLY_EXIT_USES_TILE_MASK	1
#else
	#define SWE_EARLY_EXIT_USES_TILE_MASK	0
#endif	//SWE_ENABLE_EARLY_EXIT && !SWE_EARLY_EXIT_USE_WAVE_FUNCTION

#endif	//SHALLOW_WATER_EQUATION_COMMON_HLSL
