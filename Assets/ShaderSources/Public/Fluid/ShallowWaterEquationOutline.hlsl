#ifndef SHALLOW_WATER_EQUATION_OUTLINE_HLSL
#define SHALLOW_WATER_EQUATION_OUTLINE_HLSL

// ---------------------------------------------------------------------------
// Shallow water equation, the outline state.
//
// Provenance: FluxOutline of the legacy shared header SWECommon.ush, the one the
// wet state pass is the only user of and which the legacy kept behind
// SWE_USE_FLUX_OUTLINE_FUNCTION, off by default. It samples the centre and the
// eight neighbours of a texel and fills the nine values the wet state pass turns
// into the .rg of its water state texture: the averaged surface height, the
// ground slope, the water height, the border water and border height, the water
// and ground min and max. Every expression below is the legacy one, character
// for character; what changed is the parameter path and the name.
//
// This is NOT the FluxOutline that Public/Fluid/ShallowWaterEquationCommon.hlsl
// already carries, and the two cannot share the name: that one is the legacy
// height pass's local outline, returns one water height and takes four
// parameters, while this one fills nine. A pass that reads the outline state
// includes both files, so a second function called FluxOutline would not
// compile. The name here is the legacy name plus the thing it returns.
//
// What the port changed:
//
//   old VelocityHeightFoamTex.SampleLevel(UCGSampler, UV, 0) and the same for
//   GroundTex, both inside FluxSample
//       new PE_SAMPLE_TEX2D_LEVEL(..., SamplerLinearClamp, UV, 0), through the
//       shared FluxSample. Nothing in this file names a texture, a sampler or a
//       register: the state pass declares those and this library reads them as
//       plain globals, exactly as the legacy did with its #define aliases
//   old bFullPrecision and bInputAverage
//       new InIsFullPrecision and InIsInputAverage, the house InIs form for a
//       bool. The wet state pass passes the same true and false the legacy did
//   old OutBorderHeight, OutBorderWater, OutHeightAvg, OutWaterMin, OutWaterMax,
//   OutWaterHeight, OutSlope, OutGroundMin and OutGroundMax were assigned
//   directly from the body, and some of them are read back by the statements
//   that follow
//       new unchanged, in the legacy order, including the reads of OutWaterMax
//       and OutBorderWater by the two OutWaterHeight statements. Only the
//       comments were dropped
//   old the "9 neighbours" #define of the sample offsets
//       new InOffset, which stays an argument: the wet state pass passes the
//       float2(1, 1) the legacy passed at its one call site
//   old the FluxOutline of the legacy shared header is off in every migrated
//   pass
//       new unconditional, included only by the pass that calls it
// ---------------------------------------------------------------------------

#include "Public/Fluid/ShallowWaterEquationCommon.hlsl"

// One outline tap set. The two bool parameters are the legacy's bFullPrecision
// and bInputAverage, which no migrated pass sets to anything but true and false.
void FluxOutlineState(
	in float2	InTexcoord,
	in float2	InOffset,
	in float	InMinWater,
	in float	InFallingSpeed,
	in float	InMinOutline,
	in float	InAverage,
	in bool		InIsFullPrecision,
	in bool		InIsInputAverage,
	out float	OutHeightAvg,
	out float2	OutSlope,
	out float	OutWaterHeight,
	out float	OutBorderWater,
	out float	OutBorderHeight,
	out float	OutWaterMin,
	out float	OutWaterMax,
	out float	OutGroundMin,
	out float	OutGroundMax)
{
	float2 Velocity; float Volume; float Foam; float GroundHeight; float GroundHeightLocal; float SurfaceHeight;
	FluxSample( InTexcoord, float2( 0, 0 ), false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );

	float CenterVolume = Volume;
	float CenterVolumePos = max( CenterVolume, 0 );
	float CenterGroundHeight = GroundHeight;
	float CenterSurfaceHeight = SurfaceHeight;

	float NeighborVolume[8];
	float NeighborGroundHeight[8];
	float NeighborSurfaceHeight[8];

	FluxSample( InTexcoord, float2( 1, 1 ) * InOffset, false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[0] = Volume; NeighborGroundHeight[0] = GroundHeight; NeighborSurfaceHeight[0] = SurfaceHeight;

	FluxSample( InTexcoord, float2( -1, 1 ) * InOffset, false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[1] = Volume; NeighborGroundHeight[1] = GroundHeight; NeighborSurfaceHeight[1] = SurfaceHeight;

	FluxSample( InTexcoord, float2( 1, -1 ) * InOffset, false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[2] = Volume; NeighborGroundHeight[2] = GroundHeight; NeighborSurfaceHeight[2] = SurfaceHeight;

	FluxSample( InTexcoord, float2( -1, -1 ) * InOffset, false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[3] = Volume; NeighborGroundHeight[3] = GroundHeight; NeighborSurfaceHeight[3] = SurfaceHeight;

	FluxSample( InTexcoord, float2( -1, 0 ) * InOffset, false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[4] = Volume; NeighborGroundHeight[4] = GroundHeight; NeighborSurfaceHeight[4] = SurfaceHeight;

	FluxSample( InTexcoord, float2( 0, 1 ) * InOffset, false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[5] = Volume; NeighborGroundHeight[5] = GroundHeight; NeighborSurfaceHeight[5] = SurfaceHeight;

	FluxSample( InTexcoord, float2( 0, -1 ) * InOffset, false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[6] = Volume; NeighborGroundHeight[6] = GroundHeight; NeighborSurfaceHeight[6] = SurfaceHeight;

	FluxSample( InTexcoord, float2( 1, 0 ) * InOffset, false, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	NeighborVolume[7] = Volume; NeighborGroundHeight[7] = GroundHeight; NeighborSurfaceHeight[7] = SurfaceHeight;

	float4 VolumeA = float4( NeighborVolume[0], NeighborVolume[1], NeighborVolume[2], NeighborVolume[3] );
	float4 VolumeB = float4( NeighborVolume[4], NeighborVolume[5], NeighborVolume[6], NeighborVolume[7] );
	float4 SurfaceA = float4( NeighborSurfaceHeight[0], NeighborSurfaceHeight[1], NeighborSurfaceHeight[2], NeighborSurfaceHeight[3] );
	float4 SurfaceB = float4( NeighborSurfaceHeight[4], NeighborSurfaceHeight[5], NeighborSurfaceHeight[6], NeighborSurfaceHeight[7] );

	float4 WeightsA = saturate( VolumeA );
	float4 WeightsB = saturate( VolumeB );
	float4 WeightedSurfaceA = WeightsA * SurfaceA;
	float4 WeightedSurfaceB = WeightsB * SurfaceB;
	float WeightedSurfaceSum = dot( WeightedSurfaceA + WeightedSurfaceB, 1.0f );
	float TotalWeight = max( dot( WeightsA + WeightsB, 1.0f ), 0.00001f );
	OutBorderHeight = WeightedSurfaceSum / TotalWeight;
	float SurfaceDiff = OutBorderHeight - CenterGroundHeight;
	OutBorderWater = min( min( CenterVolumePos, SurfaceDiff ), 0 );

	float NeighborSurfaceSum = dot( SurfaceA + SurfaceB, 1.0f );
	OutHeightAvg = ( CenterSurfaceHeight + NeighborSurfaceSum ) / 9.0f;

	float Average;
	if (InIsInputAverage)
	{
		Average = InAverage;
	}
	else
	{
		float SmoothDiff = OutHeightAvg - CenterGroundHeight;
		Average = lerp( SmoothDiff, CenterVolume, InFallingSpeed );
	}

	float OutlineSmooth = clamp( Average, InMinOutline, 0.0f );
	float4 MinVolume = min( VolumeA, VolumeB );
	OutWaterMin = min( min( min( min( MinVolume.x, MinVolume.y ), MinVolume.z ), MinVolume.w ), CenterVolume );
	float4 MaxVolume = max( VolumeA, VolumeB );
	OutWaterMax = max( max( max( max( MaxVolume.x, MaxVolume.y ), MaxVolume.z ), MaxVolume.w ), CenterVolume );

	OutWaterHeight = ( OutWaterMax > InMinWater ) ? OutBorderWater : OutlineSmooth;
	OutWaterHeight = ( CenterVolume > InMinWater ) ? CenterVolume : OutWaterHeight;

	float4 MinGround = float4( NeighborGroundHeight[0], NeighborGroundHeight[1], NeighborGroundHeight[2], NeighborGroundHeight[3] );
	float4 MaxGround = float4( NeighborGroundHeight[4], NeighborGroundHeight[5], NeighborGroundHeight[6], NeighborGroundHeight[7] );
	float4 GroundSlope = MinGround - MaxGround;
	OutSlope = float2( dot( float4( 1, 1, 2, 0 ), GroundSlope ), dot( float4( 1, -1, 0, 2 ), GroundSlope ) );

	if (InIsFullPrecision)
	{
		float4 GroundA = MinGround;
		MinGround = min( GroundA, MaxGround );
		MaxGround = max( GroundA, MaxGround );
	}
	OutGroundMin = min( min( min( MinGround.x, MinGround.y ), MinGround.z ), MinGround.w );
	OutGroundMax = max( max( max( MaxGround.x, MaxGround.y ), MaxGround.z ), MaxGround.w );
}

#endif	//SHALLOW_WATER_EQUATION_OUTLINE_HLSL
