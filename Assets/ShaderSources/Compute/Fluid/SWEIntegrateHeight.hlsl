#ifndef SWE_INTEGRATE_HEIGHT_HLSL
#define SWE_INTEGRATE_HEIGHT_HLSL

// ---------------------------------------------------------------------------
// Shallow water equation, integrate height.
//
// Provenance: FluxWater/CS_SWEIntegrateHeight/SWEIntegrateHeight.hlsl. This file
// is the legacy per pixel step plus the entry point; the taps it calls moved into
// Public/Fluid/ShallowWaterEquationCommon.hlsl because the velocity pass needs
// the same advection. The maths is not touched: the update order, the three
// advection taps, the flux, the foam and the final state write are all the
// legacy ones. One dispatch still writes velocity, volume and foam together, as
// it did before.
//
// What the port changed, and it is only the parameter path:
//
//   old Texture2D / RWTexture2D declared in the HLSL
//       new VelocityHeightFoamTex, GroundTex and OutIntegrateHeight declared in
//       Compute/Fluid/SWEIntegrateHeight.json, which is where every binding
//       belongs now
//   old [numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_X, 1)]
//       new [numthreads(SHADER_THREAD_GROUP_X, SHADER_THREAD_GROUP_Y, 1)]
//       the legacy passed THREAD_GROUP_SIZE_X twice, a copy and paste slip that
//       happened to be invisible because both defines were 8
//   old the SWE_ parameters and the FoamFadeSpeed #define aliases
//       new the named SWEParams fields, see the shared header for the mapping
//   old ../ShallowWaterEquationCommon.ush with SWE_USE_FLUX_SAMPLE_FUNCTION
//   switched on before it
//       new an include of the shader root relative library, which is where that
//       switch now lives and is no longer needed
//   the legacy disabled evaporation branch is kept verbatim
//       if (false) // bHeightInNegativeAmount still folds the same way. It is
//       dead, and rewriting dead code while porting is how live code changes by
//       accident
// ---------------------------------------------------------------------------

#include "Public/Fluid/ShallowWaterEquationCommon.hlsl"

bool ComputeIntegrateHeight(
	in int2		InCoord,
	out float4	OutValue)
{
	float2 Location; float2 Scale; float2 InvScale; float Height; float2 TexelSize; float2 Resolution;
	FluxSimulationUV( Location, Scale, InvScale, Height, TexelSize, Resolution );

	int2 LocalCoord = InCoord - asint( PixelAnchor );
	int2 LocalRes = asint( PixelSize );
	int2 Res = int2( (int)(Resolution.x), (int)(Resolution.y) );

	[branch] if (any(InCoord >= Res) || any(LocalCoord >= LocalRes))
	{
		OutValue = float4( 0, 0, 0, 0 );
		return false;
	}

	float2 Texcoord = (InCoord.xy + 0.5) / Resolution.xy;

	float2 Vc; float Dc;
	FluxAdvectSimulation( Texcoord.xy, float2( 0, 0 ), Vc, Dc );
	float2 Vl; float Dl;
	FluxAdvectSimulation( Texcoord.xy, float2( -1, 0 ), Vl, Dl );
	float2 Vu; float Du;
	FluxAdvectSimulation( Texcoord.xy, float2( 0, -1 ), Vu, Du );

	float4 Velocity = float4( Vc.x, Vc.y, -(Vl.x), -(Vu.y) );
	float Volume = max( 0, Dc );

	float Flux;
	{
		float IsWater;
		FluxHeight( Texcoord.xy, Velocity.xyzw, Flux, IsWater );
	}

	float Foam;
	{
		float2 TempV;
		FluxAdvectFoam( Texcoord.xy, float2( 0, 0 ), TempV, Foam );
	}

	float FinalFoam;
	FluxFoam( Foam, Flux, Volume, Velocity.xyzw, FinalFoam );

	Volume = Volume - (Flux * (FluxEnergy) * (SimulationDeltaTime));

	float NoVolumeValue = 0;
	if (false)	// bHeightInNegativeAmount
	{
		float2 TempVel; float TempV; float TempF; float GroundHeight; float GroundHeightLocal; float SurfaceHeight;
		FluxSample( Texcoord.xy, float2( 0, 0 ), true, TempVel, TempV, TempF, GroundHeight, GroundHeightLocal, SurfaceHeight );
		NoVolumeValue = (TempV >= (EvaporationHeight)) ? 0 : TempV;
	}

	Volume = (Volume > (EvaporationHeight)) ? Volume : NoVolumeValue;
	OutValue = float4( Vc.x, Vc.y, Volume, FinalFoam );
	return true;
}

[numthreads(SHADER_THREAD_GROUP_X, SHADER_THREAD_GROUP_Y, 1)]
void CSMain(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	int2 Coord = int2( DispatchThreadID.xy ) + asint( PixelAnchor );
	float4 Value;
	const bool IsOutSide = ComputeIntegrateHeight( Coord, Value );
	if (IsOutSide)
	{
		OutIntegrateHeight[ Coord ] = Value;
	}
}

#endif	//SWE_INTEGRATE_HEIGHT_HLSL
