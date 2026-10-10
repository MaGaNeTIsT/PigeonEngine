#ifndef SWE_INTEGRATE_MOMENTUM_HLSL
#define SWE_INTEGRATE_MOMENTUM_HLSL

// ---------------------------------------------------------------------------
// Shallow water equation, integrate momentum.
//
// Provenance: SWE_IntegrateVelocity.usf, the velocity step of the plugin the
// author kept and never brought over. It is named IntegrateMomentum here because
// The velocity integrator, ported from SWE_IntegrateVelocity.usf. The legacy tree
// appeared to have one under FluxWater/CS_SWEIntegrateVelocity, but that directory
// is a verbatim copy of the colour grade pass, so this is the only velocity
// integrator in the tree.
// was written before this source was available, it integrates nothing, and it is
// a documented reconstruction of the advection half of the height pass that its
// own header says is an addition rather than a port. This file is the real
// velocity step. The two are not alternates of one algorithm and they are not
// ordered against each other here: this one takes the surface height gradient,
// adds gravity, friction and damping, removes the fluid that pours out of a cell,
// writes the resulting velocity back into the state and corrects the volume with
// the overshoot term. Every expression below is the legacy one, character for
// character; what changed is the parameter path, the entry point name, the pixel
// branch and the two renames listed below.
//
// What the port changed:
//
//   old Texture2D VelocityHeightFoamTex : register(t0) and the rest of the
//   binding declarations, plus the #define aliases that named them
//       new VelocityHeightFoamTex, GroundTex, WaterMask and OutIntegrateVelocity
//       declared in Compute/Fluid/SWEIntegrateMomentum.json, which is where
//       every binding belongs now. The names are the legacy ones, so the taps
//       below read exactly as they did
//   old UCGSampler, shared by every sample of the plugin
//       new SamplerLinearClamp, the engine sampler, referenced by name and not
//       declared here. The legacy sampler object's filter and address mode are
//       not recoverable from these files; linear with clamp is the engine's
//       closest and it is the one the already migrated SWE passes use
//   old the SWE_ parameters and the Gravity and VelocityClamp #defines of
//   UCGVectorParams
//       new named SWEParams fields, declared in the component JSON: the same
//       twenty fields the two already migrated SWE passes declare, then Gravity,
//       VelocityClamp, ScaleZ, Friction, Damping, AccelerationClamp,
//       OvershootingBlend, OvershootingLimit and MinWater in the legacy order
//   old #if SWE_COMPUTE_PATH, with the pixel branch of ComputeIntegrateVelocity
//   and the MainVS / MainPS fallback pair behind its #else
//       new the compute branch only. The new system's compute pass has no pixel
//       path, so the branch and the two fallback entry points are gone. Nothing
//       in the pixel branch is lost: it differs only in that it takes the
//       texcoord from the interpolated UV instead of deriving it, and it has no
//       bounds test
//   old MainCS
//       new CSMain, the Entry the program description names, here and everywhere
//       else in the new system
//   old BRANCH, bMasked, bEarlyExit and the two UE platform macros behind the
//   early exit
//       new [branch], IsMasked, IsEarlyExit, and SWE_EARLY_EXIT_USE_WAVE_FUNCTION,
//       which the shared library now defaults to 0 because SM6_PROFILE and
//       PLATFORM_SUPPORTS_SM6_0_WAVE_OPERATIONS have no equivalent here. See
//       Public/Fluid/ShallowWaterEquationCommon.hlsl
//   the early exit block, its groupshared bitmask and its barrier are kept,
//   behind the same off by default switch, with the legacy's threshold literal
//       one thing about it is not verbatim. The legacy put the block after a
//       bounds test that returned, so a group whose last row fell out of the
//       dispatch reached the barrier with fewer threads than the group has, and
//       a group barrier that part of a group never reaches is undefined
//       behaviour. The test is a predicate here and the return sits below the
//       barrier, which every thread of the group now reaches; a thread outside
//       the simulation does not touch the tile mask
// ---------------------------------------------------------------------------

#include "Public/Fluid/ShallowWaterEquationCommon.hlsl"

// The tile bitmask of the early exit, which the legacy declared per pass. Its
// home is here rather than in the shared library because it is groupshared
// state of this entry point and the group index that resets it is a parameter of
// this entry point.
#if SWE_EARLY_EXIT_USES_TILE_MASK
groupshared uint SharedTileBitmask;
#endif	//SWE_EARLY_EXIT_USES_TILE_MASK

// Clamps a vector to a length. Zero length stays zero, which is what the legacy
// min( 1, Max / Len ) does with a Len of zero as well.
float2 FluxClampMax2( in float2 InVector, in float InMaxScalar )
{
	float Len = distance( 0, InVector.xy );
	return (InVector * min( 1, InMaxScalar / Len ));
}

// One when the uphill neighbour of the pair holds water, zero otherwise. InSlope
// is the surface height difference the acceleration comes from, so its sign says
// which of the two neighbours the fluid is leaving.
float FluxRemoveFluid( in float InW0, in float InW1, in float InSlope )
{
	return ((((InSlope > 0) ? InW0 : InW1) > (MinWater)) ? 1 : 0);
}

float2 SelectVector2( in float InCon, in float2 InVector2 )
{
	return ((InCon > 0) ? (InVector2.xy) : 0);
}

// The overshoot correction: a cell that stands more than Limit above a neighbour
// and above the opposite one gives up part of its velocity towards that
// neighbour, blended by InBlend.
//
// The call site below feeds the ( 0, -1 ) tap into the B slots and the ( 0, 1 )
// tap into the T slots, so the parameter letters are the reverse of the offsets
// the taps were sampled at. That is the legacy's argument order and it is kept:
// the four conditions are written in terms of these slots, and swapping the
// arguments to match their letters would swap two of the four corrections.
float Overshooting(
	in float	InHeightC,
	in float	InVolumeC,
	in float	InHeightL,
	in float	InVolumeL,
	in float	InHeightR,
	in float	InVolumeR,
	in float	InHeightB,
	in float	InVolumeB,
	in float	InHeightT,
	in float	InVolumeT,
	in float	InBlend,
	in float	InLimit)
{
	float Change = 0;

	[branch] if (InBlend > 0)
	{
		if (InHeightC > 0)
		{
			if ((InHeightC - InHeightL > InLimit) && (InHeightC > InHeightR))
			{
				float Remove = InBlend * (max( 0, 0.5 * (InVolumeC + InVolumeR) ) - InVolumeC);
				Change += Remove;
			}
			if ((InHeightC - InHeightR > InLimit) && (InHeightC > InHeightL))
			{
				float Remove = InBlend * (max( 0, 0.5 * (InVolumeC + InVolumeL) ) - InVolumeC);
				Change += Remove;
			}
			if ((InHeightC - InHeightB > InLimit) && (InHeightC > InHeightT))
			{
				float Remove = InBlend * (max( 0, 0.5 * (InVolumeC + InVolumeT) ) - InVolumeC);
				Change += Remove;
			}
			if ((InHeightC - InHeightT > InLimit) && (InHeightC > InHeightB))
			{
				float Remove = InBlend * (max( 0, 0.5 * (InVolumeC + InVolumeB) ) - InVolumeC);
				Change += Remove;
			}
		}
		return Change;
	}
	return 0;
}

// The per texel step: the momentum half of the shallow water update. Hc, Hr and
// Hb are the local surface heights of the centre and the two neighbours the
// gradient is taken against, and the acceleration is that gradient over ScaleZ.
bool ComputeIntegrateMomentum(
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

	float2 Texcoord = (InCoord.xy + 0.5) / Resolution.xy;

	bool IsEarlyExit = false;
#if SWE_ENABLE_EARLY_EXIT
	// A thread outside the simulation counts as one with nothing to do, so its
	// tile bit stays clear and the mask is never sampled out of the area.
	const bool IsMasked = IsOutside || ((PE_SAMPLE_TEX2D_LEVEL( WaterMask, SamplerLinearClamp, Texcoord, 0 ).r) <= 0.007844);

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

	float2 Velc; float Volc; float Fc; float GroundHc; float GroundHLc; float SurfaceHc;
	FluxSample( Texcoord.xy, float2( 0, 0 ), true, Velc, Volc, Fc, GroundHc, GroundHLc, SurfaceHc );

	float2 Velr; float Volr; float Fr; float GroundHr; float GroundHLr; float SurfaceHr;
	FluxSample( Texcoord.xy, float2( 1, 0 ), true, Velr, Volr, Fr, GroundHr, GroundHLr, SurfaceHr );

	float2 Velb; float Volb; float Fb; float GroundHb; float GroundHLb; float SurfaceHb;
	FluxSample( Texcoord.xy, float2( 0, 1 ), true, Velb, Volb, Fb, GroundHb, GroundHLb, SurfaceHb );

	float Vc = max( 0, Volc );
	float Vr = max( 0, Volr );
	float Vb = max( 0, Volb );
	float Hc = GroundHLc + Vc;
	float Hr = GroundHLr + Vr;
	float Hb = GroundHLb + Vb;

	float InvScaleZ = 1.0 / (ScaleZ);

	float2 Acceleration = (Hc.xx - float2( Hr, Hb )) * InvScaleZ;

	float2 Velocity = FluxClampMax2( Acceleration.xy * (SimulationDeltaTime) * (Gravity), (AccelerationClamp) ) + Velc.xy;

	float Ac = Vc * InvScaleZ;
	float Ar = Vr * InvScaleZ;
	float Ab = Vb * InvScaleZ;

	float2 Wet = float2( FluxRemoveFluid( Ac, Ar, Acceleration.x ), FluxRemoveFluid( Ac, Ab, Acceleration.y ) );

	Velocity = Velocity.xy * Wet.xy;

	float FrictionFactor = distance( 0, Velocity.xy ) * (SimulationDeltaTime) * (Friction);
	FrictionFactor = FrictionFactor / max( 0.0001, (Ac * 2 + Ar + Ab) * 0.25 );

	Velocity = Velocity * saturate( 1 - FrictionFactor );
	Velocity = FluxClampMax2( Velocity.xy - (clamp( Velocity.xy, -0.01, 0.01 ) * (Damping) * (SimulationDeltaTime)), (VelocityClamp) );

	float MaxVol = max( max( Volc, Volr ), Volb );
	Velocity = SelectVector2( MaxVol, Velocity.xy );

	Velocity = Velocity.xy * step( Texcoord.xy * Resolution.xy, Resolution.xy - 0.9 );	// Resolution == VelocityHeightFoamTex Dimension

	float ModVolume;
	{
		float2 Vell; float Voll; float Fl; float GroundHl; float GroundHLl; float SurfaceHl;
		FluxSample( Texcoord.xy, float2( -1, 0 ), true, Vell, Voll, Fl, GroundHl, GroundHLl, SurfaceHl );

		float2 Velt; float Volt; float Ft; float GroundHt; float GroundHLt; float SurfaceHt;
		FluxSample( Texcoord.xy, float2( 0, -1 ), true, Velt, Volt, Ft, GroundHt, GroundHLt, SurfaceHt );

		float VL = max(0, Voll);
		float VT = max(0, Volt);

		ModVolume = Overshooting(
			Hc, Vc,
			GroundHLl + VL, VL,
			Hr, Vr,
			GroundHLt + VT, VT,
			Hb, Vb,
			(OvershootingBlend), (OvershootingLimit)
		);
	}

	OutValue = float4( Velocity.x, Velocity.y, Volc + ModVolume, Fc );
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
	if (!(ComputeIntegrateMomentum( Coord, Value )))
	{
		return;
	}
	OutIntegrateVelocity[ Coord ] = Value;
}

#endif	//SWE_INTEGRATE_MOMENTUM_HLSL
