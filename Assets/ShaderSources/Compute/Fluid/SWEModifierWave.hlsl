#ifndef SWE_MODIFIER_WAVE_HLSL
#define SWE_MODIFIER_WAVE_HLSL

// ---------------------------------------------------------------------------
// Shallow water equation, modifier wave.
//
// Provenance: SWE_ModifierWave.usf, a Gerstner wave generator that injects wave
// driven velocity and depth over the whole simulation domain, faded towards the
// domain's borders. Four cascades of the same wave are superposed, each with the
// legacy's own scale, amplitude and direction offset.
//
// This port is deliberately not a layout compatible port. The legacy pass owned
// two textures of its own: VelocityTex (.rg velocity) and HeightFoamTex (.r
// height, .g foam). The rest of the family packs velocity, volume and foam into
// one float4 state, so the legacy pass could not share the ping pong pipeline
// and could not see the state the integrators write. The rewrite below reads
// and writes the packed state instead:
//
//   state.xy  plane velocity, world X and world Z
//   state.z   water volume/depth along world Y
//   state.w   foam
//
// The semantic adjustment this makes is explicit: the legacy added its height
// delta to an absolute height texture and clamped that height to SurfaceHeight.
// The packed state stores a depth, so the same delta is applied to state.z and
// clamped to WaveSurfaceHeight, which is the maximum depth this pass may add.
// CaptureY keeps the legacy fill gate: when the reference plane is above zero,
// the delta is faded by how far the current depth is below that level.
//
// The engine world is left handed with X right, Z forward and Y up. The
// simulation plane is world XZ and the height axis is world Y, so the texture V
// axis maps to world Z; the border fields name that axis Z instead of the
// legacy's Y.
//
// The magic numbers of the source are kept: 6.28318548f, which turns the wave
// length into the wave number, and the four cascade scale and direction offset
// literals in the calls below, which are the ratios the pass was tuned with.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"

// One Gerstner cascade. K is the wave number, Phase advances with the direction
// and the clock, and the result is ( velocity xz, depth delta ). Texcoord is the
// simulation plane coordinate: its x axis is world X and its y axis is world Z.
float3 GerstnerCascade(
	in float2	InTexcoord,
	in float	InWaveLength,
	in float	InWaveSpeed,
	in float	InAmplitude,
	in float	InSteepness,
	in float	InDirectionAngle,
	in float	InTime)
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
	int2 Resolution = int2( (int)(ResolutionX), (int)(ResolutionZ) );
	if (any( Coord >= Resolution ))
	{
		return;
	}

	float2 Texcoord = (float2( Coord ) + 0.5f) * float2( (InverseResolutionX), (InverseResolutionZ) );

	// Border edge fade: the wave is at full strength in the interior and fades
	// toward each edge. The V axis names world Z.
	float BorderXNegative	= saturate( Texcoord.x		* (WaveBorderXNegative) );
	float BorderZNegative	= saturate( Texcoord.y		* (WaveBorderZNegative) );
	float BorderXPositive	= saturate( (1.f - Texcoord.x) * (WaveBorderXPositive) );
	float BorderZPositive	= saturate( (1.f - Texcoord.y) * (WaveBorderZPositive) );
	float EdgeFade = min( min( BorderXNegative, BorderXPositive ), min( BorderZNegative, BorderZPositive ) );

	// Four cascade Gerstner superposition, with the legacy harmonic ratios and
	// direction offsets.
	float3 W0 = GerstnerCascade( Texcoord, (WaveLength) * 1.0f, (WaveSpeed), (WaveAmplitude) * 1.0f, (WaveSteepness), (WaveDirectionAngle) + 0.0000f, (WaveActiveTime) );
	float3 W1 = GerstnerCascade( Texcoord, (WaveLength) * 0.8f, (WaveSpeed), (WaveAmplitude) * 0.8f, (WaveSteepness), (WaveDirectionAngle) + 0.1250f, (WaveActiveTime) );
	float3 W2 = GerstnerCascade( Texcoord, (WaveLength) * 0.7f, (WaveSpeed), (WaveAmplitude) * 0.7f, (WaveSteepness), (WaveDirectionAngle) + 0.2494f, (WaveActiveTime) );
	float3 W3 = GerstnerCascade( Texcoord, (WaveLength) * 0.6f, (WaveSpeed), (WaveAmplitude) * 0.6f, (WaveSteepness), (WaveDirectionAngle) + 0.5225f, (WaveActiveTime) );

	float2 WaveVelocity = (W0.xy + W1.xy + W2.xy + W3.xy) * (WaveVelocityScale) * EdgeFade;
	float WaveDepthDelta = (W0.z + W1.z + W2.z + W3.z) * EdgeFade;

	float4 State = PE_LOAD_TEX2D( VelocityHeightFoamTex, int3( Coord, 0 ) );

	// Depth write. The state stores depth along world Y; the legacy wrote an
	// absolute height, so the delta is the same and the clamp is a depth clamp.
	float DepthDelta = WaveDepthDelta * (SimulationDeltaTime);
	if ((CaptureY) > 0.f)
	{
		float FillBlend = saturate( ((CaptureY) - State.z) / max( (CaptureY), 0.0001f ) );
		DepthDelta *= FillBlend;
	}
	State.z = clamp( State.z + DepthDelta, 0.f, (WaveSurfaceHeight) );

	// Velocity write: state.xy is (velocityX, velocityZ).
	State.xy += WaveVelocity * (SimulationDeltaTime);
	State.xy *= saturate( 1.f - (WaveCalming) * (SimulationDeltaTime) );

	OutVelocityHeightFoamTex[ Coord ] = State;
}

#endif	//SWE_MODIFIER_WAVE_HLSL
