#ifndef SWE_INITIALIZE_HLSL
#define SWE_INITIALIZE_HLSL

// ---------------------------------------------------------------------------
// Shallow water equation, initialize.
//
// Fills the packed state and the height/wet state from the ground and the
// initial depth. It runs once when the simulation resources are created and
// again on a reset or a resolution change.
//
// The engine world is left handed with X right, Z forward and Y up. The
// simulation plane is world XZ and the height axis is world Y:
//
//   OutState.xy  plane velocity, world X and world Z; initialized to zero
//   OutState.z   water volume/depth along world Y
//   OutState.w   foam; initialized to zero
//   OutHeightWet.r  surface height in world Y
//   OutHeightWet.g  wetness
//
// GroundTex stores referenceY - groundWorldY, so the ground's world Y is
// CaptureY - GroundValue.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"

[numthreads(SHADER_THREAD_GROUP_X, SHADER_THREAD_GROUP_Y, 1)]
void CSMain(uint3 InDispatchThreadID : SV_DispatchThreadID)
{
	int2 Coord = int2( InDispatchThreadID.xy ) + PixelAnchor;
	int2 Resolution = int2( (int)(ResolutionX), (int)(ResolutionZ) );
	if (any( Coord >= Resolution ))
	{
		return;
	}

	float2 Texcoord = (float2( Coord ) + 0.5f) * float2( (InverseResolutionX), (InverseResolutionZ) );
	float GroundValue = PE_SAMPLE_TEX2D_LEVEL( GroundTex, SamplerLinearClamp, Texcoord, 0 ).r;
	float GroundY = (CaptureY) - GroundValue;

	float InitialVolume = max( 0.f, (InitialDepth) );
	OutState[ Coord ] = float4( 0.f, 0.f, InitialVolume, 0.f );
	OutHeightWet[ Coord ] = float2( GroundY + InitialVolume, ((InitialVolume > 0.f) ? 1.f : 0.f) );
}

#endif	//SWE_INITIALIZE_HLSL
