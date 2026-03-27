#define VelocityHeightFoamTex			UCGInput0
#define GroundTex						UCGInput1
#define OutIntegrateHeight				UCGOutput0

#define SWE_USE_FLUX_SAMPLE_FUNCTION	1
#include "ShallowWaterEquationCommon.ush"

#define FoamFadeSpeed					UCGVectorParams[(SWE_VectorParamOffset) + 0].x
#define FoamShallowDepth				UCGVectorParams[(SWE_VectorParamOffset) + 0].y
#define FoamShallowGenerate				UCGVectorParams[(SWE_VectorParamOffset) + 0].z
#define FoamWaveGenerate				UCGVectorParams[(SWE_VectorParamOffset) + 0].w
#define FluidTransportMax				UCGVectorParams[(SWE_VectorParamOffset) + 1].x
#define AdvectFoam						UCGVectorParams[(SWE_VectorParamOffset) + 1].y
#define EvaporationHeight				UCGVectorParams[(SWE_VectorParamOffset) + 1].z
#define FluxEnergy						UCGVectorParams[(SWE_VectorParamOffset) + 1].w
#define PixelAnchor						UCGVectorParams[(SWE_VectorParamOffset) + 2].xy
#define PixelSize						UCGVectorParams[(SWE_VectorParamOffset) + 2].zw

void FluxAdvectSimulation( in float2 InTexcoord, in float2 InOffset, out float2 OutVelocity, out float OutVolume )
{
	float2 Velocity; float Volume; float Foam; float GroundHeight; float GroundHeightLocal; float SurfaceHeight;
	FluxSample( InTexcoord.xy, InOffset.xy, true, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	
	float2 Location; float2 Scale; float2 InvScale; float Height; float2 TexelSize; float2 Resolution;
	FluxSimulationUV( Location, Scale, InvScale, Height, TexelSize, Resolution );
	
	float2 UV = ((InOffset.xy - (Velocity.xy * (SWE_DeltaTime))) * TexelSize.xy) + InTexcoord.xy;
	float4 VelocityHeightFoamValue = VelocityHeightFoamTex.SampleLevel( UCGSampler, UV, 0 ).rgba;
	
	OutVelocity = VelocityHeightFoamValue.xy * abs( sign( Velocity.xy ) );
	OutVolume = Volume;
}

void FluxAdvectFoam( in float2 InTexcoord, in float2 InOffset, out float2 OutVelocity, out float OutFoam )
{
	float VelocityX; float VelocityY;
	
	float2 Velocity; float Volume; float Foam; float GroundHeight; float GroundHeightLocal; float SurfaceHeight;
	FluxSample( InTexcoord.xy, InOffset.xy + float2( -0.5f, 0.0f ), true, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	VelocityX = Velocity.x;
	FluxSample( InTexcoord.xy, InOffset.xy + float2( 0.0f, -0.5f ), true, Velocity, Volume, Foam, GroundHeight, GroundHeightLocal, SurfaceHeight );
	VelocityY = Velocity.y;
	
	float2 Location; float2 Scale; float2 InvScale; float Height; float2 TexelSize; float2 Resolution;
	FluxSimulationUV( Location, Scale, InvScale, Height, TexelSize, Resolution );
	
	float2 UV = ((InOffset.xy - (float2( VelocityX, VelocityY ) * (SWE_DeltaTime))) * TexelSize.xy) + InTexcoord.xy;
	float4 VelocityHeightFoamValue = VelocityHeightFoamTex.SampleLevel( UCGSampler, UV, 0 ).rgba;

	OutVelocity = VelocityHeightFoamValue.xy;
	OutFoam = VelocityHeightFoamValue.w;
}

void FluxHeight( in float2 InTexcoord, in float4 InVelocity, out float OutFlux, out float OutIsWater )
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

void FluxFoam( in float InFoam, in float InFlux, in float InVolume, in float4 InVelocity, out float OutFoam )
{
	float InputFoam = saturate( InFoam );
	float Dv = dot( InVelocity.xyzw, float4( 0.5f, 0.5f, 0.5f, 0.5f ) );
	float Foam = max( 0, Dv * (FoamShallowGenerate) ) * saturate( 1 - (InVolume - (FoamShallowDepth)) );
	Foam = (Dv * (AdvectFoam) * InputFoam + Foam - (FoamFadeSpeed)) * (SWE_DeltaTime) + InputFoam;
	
	OutFoam = saturate( max( Dv * (-(FoamWaveGenerate)), Foam ) );
}

bool ComputeIntegrateHeight( in int2 Coord, out float4 OutValue )
{
	float2 Location; float2 Scale; float2 InvScale; float Height; float2 TexelSize; float2 Resolution;
	FluxSimulationUV( Location, Scale, InvScale, Height, TexelSize, Resolution );

#if SWE_COMPUTE_PATH
	int2 LocalCoord = Coord - asint( PixelAnchor );
	int2 LocalRes = asint( PixelSize );
	int2 Res = int2( (int)(Resolution.x), (int)(Resolution.y) );
	
	BRANCH
	if (any( Coord >= Res ) || any( LocalCoord >= LocalRes ))
	{
		OutValue = float4( 0, 0, 0, 0 );
		return false;
	}
#endif

	float2 Texcoord = (Coord.xy + 0.5) / Resolution.xy;

	float2 Vc; float Dc;
	FluxAdvectSimulation( Texcoord.xy, float2( 0, 0 ), Vc, Dc );
	float2 Vl; float Dl;
	FluxAdvectSimulation( Texcoord.xy, float2( -1, 0 ), Vl, Dl );
	float2 Vu; float Du;
	FluxAdvectSimulation( Texcoord.xy, float2( 0, -1 ), Vu, Du );

	float4 Velocity = float4( Vc.x, Vc.y, -(Vl.x), -(Vu.y) );
	float Volume = max(0, Dc);

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

	Volume = Volume - (Flux * (FluxEnergy) * (SWE_DeltaTime));

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

[numthreads( 8, 8, 1 )]
void MainCS( uint3 InDispatchThreadID : SV_DispatchThreadID )
{
	int2 Coord = int2( InDispatchThreadID.xy ) + asint( PixelAnchor );
	float4 Value;
	const bool bOutSide = ComputeIntegrateHeight( Coord, Value );
	if (bOutSide)
	{
		OutIntegrateHeight[ Coord ] = Value;
	}
}

// VS+PS
void MainVS(in uint VertexID : SV_VertexID, out float4 OutPosition : SV_POSITION, out float2 OutUV : TEXCOORD0)
{
	OutUV = float2((VertexID << 1) & 2, VertexID & 2);
	OutPosition = float4(OutUV * float2(2, -2) + float2(-1, 1), 0, 1);
}

void MainPS(in float4 Position : SV_POSITION, in float2 UV : TEXCOORD0, out float4 OutColor : SV_Target0)
{
	ComputeIntegrateHeight( int2( Position.xy ), OutColor );
}

