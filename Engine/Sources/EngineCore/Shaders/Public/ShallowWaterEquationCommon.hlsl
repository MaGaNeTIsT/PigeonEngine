#define SWE_InvResX				UCGVectorParams[0].x
#define SWE_InvResY				UCGVectorParams[0].y
#define SWE_ResX				UCGVectorParams[0].z
#define SWE_ResY				UCGVectorParams[0].w

#define SWE_AreaLocX			UCGVectorParams[1].x
#define SWE_AreaLocY			UCGVectorParams[1].y
#define SWE_AreaSizeX			UCGVectorParams[1].z
#define SWE_AreaSizeY			UCGVectorParams[1].w

#define SWE_DeltaTime			UCGVectorParams[2].x
#define SWE_CaptureHeight		UCGVectorParams[2].y

#define SWE_VectorParamOffset				3

void FluxSimulationUV(
	out float2	OutLocation,
	out float2	OutScale,
	out float2	OutInvScale,
	out float	OutHeight,
	out float2	OutTexelSize,
	out float2	OutResolution)
{
	OutLocation = float2( (SWE_AreaLocX), (SWE_AreaLocY) );
	OutScale = float2( (SWE_AreaSizeX), (SWE_AreaSizeY) );
	OutInvScale = float2( 1.0f / (SWE_AreaSizeX), 1.0f / (SWE_AreaSizeY) );
	OutHeight = (SWE_CaptureHeight);
	OutTexelSize = float2( (SWE_InvResX), (SWE_InvResY) );
	OutResolution = float2( (SWE_ResX), (SWE_ResY) );
}

#ifndef SWE_USE_FLUX_SAMPLE_FUNCTION
	#define SWE_USE_FLUX_SAMPLE_FUNCTION	0
#endif	// SWE_USE_FLUX_SAMPLE_FUNCTION

#if SWE_USE_FLUX_SAMPLE_FUNCTION
void FluxSample(
	in float2	InTexcoord,
	in float2	InOffset,
	in bool		bCleanVelocity,
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
	float4 VelocityHeightFoamValue = VelocityHeightFoamTex.SampleLevel( UCGSampler, UV, 0 ).rgba;
	float GroundValue = GroundTex.SampleLevel( UCGSampler, UV, 0 ).r;
	
	OutVelocity = bCleanVelocity ? (VelocityHeightFoamValue.xy * step( 0, UV.xy )) : (VelocityHeightFoamValue.xy);
	OutVolume = VelocityHeightFoamValue.z;
	OutFoam = VelocityHeightFoamValue.w;
	OutGroundHeight = Height - GroundValue.x;
	OutGroundHeightLocal = -(GroundValue.x);
	OutSurfaceHeight = max( 0, VelocityHeightFoamValue.z ) + OutGroundHeight;
}
#endif	// SWE_USE_FLUX_SAMPLE_FUNCTION

#ifndef SWE_USE_FLUX_OUTLINE_FUNCTION
	#define SWE_USE_FLUX_OUTLINE_FUNCTION	0
#endif	// SWE_USE_FLUX_OUTLINE_FUNCTION

#if SWE_USE_FLUX_OUTLINE_FUNCTION
#if 0
void FluxSobelOffsets(
	in float2	InTexcoord,
	in float2	InOffsetAmount,
	out float2	OutOffsetXPYP,
	out float2	OutOffsetXNYP,
	out float2	OutOffsetXPYN,
	out float2	OutOffsetXNYN,
	out float2	OutOffsetXN,
	out float2	OutOffsetYP,
	out float2	OutOffsetYN,
	out float2	OutOffsetXP)
{
	OutOffsetXPYP = float2( 1, 1 );
}
#endif

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
	
	float2 SobelOffsetAmount;
	{
		
	}
	
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
#endif	// SWE_USE_FLUX_OUTLINE_FUNCTION
