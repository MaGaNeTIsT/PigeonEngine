#ifndef SWE_MODIFIER_WAVE_HLSL
#define SWE_MODIFIER_WAVE_HLSL

// ---------------------------------------------------------------------------
// Shallow water equation, modifier wave.
//
// Provenance: SWE_ModifierWave.usf, a Gerstner wave generator that injects wave
// driven velocity and height over the whole simulation domain, faded towards the
// domain's borders. Four cascades of the same wave are superposed, each with the
// legacy's own scale, amplitude and direction offset, and the height is gated by
// the fill level before the velocity gets its wave push and its calming. Every
// expression below is the legacy one, character for character; what changed is
// the parameter path, the entry point name and the notes below.
//
// What the port changed:
//
//   old Texture2D VelocityTex : register(t0), HeightFoamTex : register(t1) and
//   RWTexture2D OutVelocity / OutHeight : register(u0) / (u1), plus the #define
//   aliases that named them
//       new the four names declared in Compute/Fluid/SWEModifierWave.json. The
//       layout is the legacy's and it is not the one the rest of the family
//       uses: this pass reads a velocity texture whose .rg is vx, vy and a height
//       and foam texture whose .r is the height and .g the foam, writes both back
//       whole, and never touches the packed velocity / volume / foam state the
//       other four passes share. That is the source's contract, so it is the
//       port's contract; a dispatcher has to bind these two the way this pass
//       expects them
//   old VelocityTex.Load( int3( Coord, 0 ) ) and the same for HeightFoamTex
//       new PE_LOAD_TEX2D( VelocityTex, int3( Coord, 0 ) ), which is why this file
//       includes Public/System/ShaderBindless.hlsl itself rather than relying on
//       the caller. The bindless switch stays a one file change
//   old the SWE_ aliases of UCGVectorParams, and the pass's own VelocityScale,
//   WaveCalming, SurfaceHeight, InvBorder*, WaveLength, WaveSpeed, Amplitude,
//   Steepness, ActiveTime and DirectionAngle #defines
//       new named SWEParams fields, declared in the component JSON: the same
//       twenty fields the two already migrated SWE passes declare, then the
//       pass's own fields in the legacy order. ActiveTime is the legacy's own
//       wave clock, not the engine's GlobalParams time
//   old the header comment of the source, which listed a UCGVectorParams layout
//       that does not match the file's own #defines: it puts DeltaTime at
//       offset + 0 and the border inverses at + 2, while the defines put
//       VelocityScale, WaveCalming and SurfaceHeight at + 0 and the border
//       inverses at + 0 and + 1
//       new the comment is rewritten to say what the code reads. The #defines are
//       what the algorithm uses, so the defines win; the port follows them
//       exactly and no expression moved
//   old MainCS
//       new CSMain, the Entry the program description names
//   this pass has no pixel branch, no early exit and no water mask in the
//   legacy, and none here
//
// The one thing the port could not carry over as a library include: the legacy
// included SWECommon.ush for the SWE_ parameter names, with the shared sample
// function switched off, so nothing from that header was ever compiled into this
// pass. It still is not: this pass neither samples a state texel nor reads the
// outline, it only reads its parameter block, so it includes only the bindless
// macros. It cannot include Public/Fluid/ShallowWaterEquationCommon.hlsl without
// renaming its own SurfaceHeight, which would be shadowed by the local of that
// name inside the library's FluxSample.
//
// The magic numbers of the source are kept: 6.28318548f, which turns the wave
// length into the wave number, and the four cascade scale and direction offset
// literals in the calls below, which are the ratios the pass was tuned with.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"

// One Gerstner cascade. K is the wave number, Phase advances with the direction
// and the clock, and the result is ( velocity xy, height delta ): the horizontal
// displacement of the crest times the amplitude, and the steepness times the
// amplitude as the vertical one.
float3 GerstnerCascade( in float2 InTexcoord, in float InWaveLength, in float InWaveSpeed, in float InAmplitude, in float InSteepness, in float InDirectionAngle, in float InTime )
{
	float K = 6.28318548f / max( InWaveLength, 0.0001f );
	float SinD; float CosD;
	sincos( InDirectionAngle, SinD, CosD );
	float2 Dir = float2( SinD, CosD );
	float Phase = dot( Dir, InTexcoord ) * K - InWaveSpeed * InTime * K;
	float CosP = cos( Phase );
	float SinP = sin( Phase );
	return float3( Dir * InAmplitude * CosP, InSteepness * InAmplitude * SinP );
}

[numthreads(SHADER_THREAD_GROUP_X, SHADER_THREAD_GROUP_Y, 1)]
void CSMain(uint3 InDispatchThreadID : SV_DispatchThreadID)
{
	int2 Coord = int2( InDispatchThreadID.xy );
	int2 Res = int2( (int)(ResX), (int)(ResY) );
	if (any( Coord >= Res ))
	{
		return;
	}

	float2 Texcoord = (float2( Coord ) + 0.5f) * float2( (InvResX), (InvResY) );

	// Border edge fade: wave is at full strength in the interior, fades toward each edge.
	float BXNeg	= saturate( Texcoord.x			* (InvBorderXNeg) );
	float BYNeg	= saturate( Texcoord.y			* (InvBorderYNeg) );
	float BXPos	= saturate( (1.f - Texcoord.x)	* (InvBorderXPos) );
	float BYPos	= saturate( (1.f - Texcoord.y)	* (InvBorderYPos) );
	float EdgeFade = min( min( BXNeg, BXPos ), min( BYNeg, BYPos ) );

	// 4-cascade Gerstner wave superposition (matching Flux harmonic ratios and direction offsets)
	float3 W0 = GerstnerCascade( Texcoord, (WaveLength) * 1.0f, (WaveSpeed), (Amplitude) * 1.0f, (Steepness), (DirectionAngle) + 0.0000f, (ActiveTime) );
	float3 W1 = GerstnerCascade( Texcoord, (WaveLength) * 0.8f, (WaveSpeed), (Amplitude) * 0.8f, (Steepness), (DirectionAngle) + 0.1250f, (ActiveTime) );
	float3 W2 = GerstnerCascade( Texcoord, (WaveLength) * 0.7f, (WaveSpeed), (Amplitude) * 0.7f, (Steepness), (DirectionAngle) + 0.2494f, (ActiveTime) );
	float3 W3 = GerstnerCascade( Texcoord, (WaveLength) * 0.6f, (WaveSpeed), (Amplitude) * 0.6f, (Steepness), (DirectionAngle) + 0.5225f, (ActiveTime) );

	float2 WaveVelocity = (W0.xy + W1.xy + W2.xy + W3.xy) * (VelocityScale) * EdgeFade;
	float WaveHeightDelta = (W0.z  + W1.z  + W2.z  + W3.z ) * EdgeFade;

	// Height write-back: gate by fill-to-level if CaptureHeight is set (> 0)
	float4 HeightFoamValue = PE_LOAD_TEX2D( HeightFoamTex, int3( Coord, 0 ) );
	float HeightDelta = WaveHeightDelta * (SimulationDeltaTime);
	if ((CaptureHeight) > 0.f)
	{
		float FillBlend = saturate( ((CaptureHeight) - HeightFoamValue.r) / max( (CaptureHeight), 0.0001f ) );
		HeightDelta *= FillBlend;
	}
	HeightFoamValue.r = clamp( HeightFoamValue.r + HeightDelta, 0.f, (SurfaceHeight) );
	OutHeight[ Coord ] = HeightFoamValue;

	// Velocity write-back: inject wave velocity then apply per-frame calming
	float4 VelValue = PE_LOAD_TEX2D( VelocityTex, int3( Coord, 0 ) );
	VelValue.rg += WaveVelocity * (SimulationDeltaTime);
	VelValue.rg *= saturate( 1.f - (WaveCalming) * (SimulationDeltaTime) );
	OutVelocity[ Coord ] = VelValue;
}

#endif	//SWE_MODIFIER_WAVE_HLSL
