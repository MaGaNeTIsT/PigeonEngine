#ifndef _SHALLOW_WATERE_QUATION_MODIFIER_PE_CS_HLSL
#define _SHALLOW_WATERE_QUATION_MODIFIER_PE_CS_HLSL

#if 0

#include "ShallowWaterEquationCommon.ush"

#define GroundTex						UCGInput0
#define OutVelocityHeightFoamTex		UCGOutput0

#define Velocity						UCGVectorParams[(SWE_VectorParamOffset) + 0].xy
#define VelocityX						UCGVectorParams[(SWE_VectorParamOffset) + 0].x
#define VelocityY						UCGVectorParams[(SWE_VectorParamOffset) + 0].y
#define Volume							UCGVectorParams[(SWE_VectorParamOffset) + 0].z
#define Mode							UCGVectorParams[(SWE_VectorParamOffset) + 0].w
#define ModLoc							UCGVectorParams[(SWE_VectorParamOffset) + 1].xy
#define ModLocX							UCGVectorParams[(SWE_VectorParamOffset) + 1].x
#define ModLocY							UCGVectorParams[(SWE_VectorParamOffset) + 1].y
#define ModSize							UCGVectorParams[(SWE_VectorParamOffset) + 1].zw
#define ModSizeX						UCGVectorParams[(SWE_VectorParamOffset) + 1].z
#define ModSizeY						UCGVectorParams[(SWE_VectorParamOffset) + 1].w
#define ForwardVector					UCGVectorParams[(SWE_VectorParamOffset) + 2].xyz
#define ForwardVectorX					UCGVectorParams[(SWE_VectorParamOffset) + 2].x
#define ForwardVectorY					UCGVectorParams[(SWE_VectorParamOffset) + 2].y
#define ForwardVectorZ					UCGVectorParams[(SWE_VectorParamOffset) + 2].z
#define ZOffset							UCGVectorParams[(SWE_VectorParamOffset) + 2].w
#define PixelAnchor						UCGVectorParams[(SWE_VectorParamOffset) + 3].xy
#define PixelSize						UCGVectorParams[(SWE_VectorParamOffset) + 3].zw
#define Hardness						UCGVectorParams[(SWE_VectorParamOffset) + 4].x
#define Intensity						UCGVectorParams[(SWE_VectorParamOffset) + 4].y
#define Radius							UCGVectorParams[(SWE_VectorParamOffset) + 4].z

float FluxModifierShape( float2 InTexcoord, float InRadius, float InHardness )
{
	float2 UV = abs( InTexcoord * 2 - 1 ) - (1 - InRadius);
	float Dist = InRadius - (distance( 0, max( 0, UV ) ) + min( 0, max( UV.x, UV.y ) ));
	return saturate( Dist * InHardness );
}

float4 FluxModifierMode( float InWeight, float2 InTexcoord, float InSimulationHeight )
{
	float4 Result;
	
	float GroundValue = GroundTex.SampleLevel( UCGSampler, InTexcoord, 0 ).r;
	float GroundHeight = InSimulationHeight - GroundValue;
	float Offset = min( (ZOffset), (Volume) + GroundHeight ) - GroundHeight;
	float Height = max( 0, (Volume) + (ZOffset) - GroundHeight );
	Result = float4( (VelocityX), (VelocityY), lerp( Offset, Height, saturate( (Mode) - 1 ) ), saturate( Mode ) ) * InWeight;
	
	return Result;
}

bool ComputeModifier( in int2 Coord, out float4 OutValue )
{
	float2 Location; float2 Scale; float2 InvScale; float Height; float2 TexelSize; float2 Resolution;
	FluxSimulationUV( Location, Scale, InvScale, Height, TexelSize, Resolution );
	
	int2 LocalCoord = Coord - asint( PixelAnchor );
	int2 LocalRes = asint( PixelSize );
#if SWE_COMPUTE_PATH
	int2 Res = int2( (int)(Resolution.x), (int)(Resolution.y) );
	if (any( Coord >= Res ) || any( LocalCoord >= LocalRes ))
	{
		OutValue = float4( 0, 0, 0, 0 );
		return false;
	}
#endif
	
	float2 Texcoord0 = saturate( (LocalCoord.xy + 0.5) / LocalRes.xy );
	float Weight = FluxModifierShape( Texcoord0.xy, (Radius), (Hardness) );
	
	float2 SimulationAspect = float2( Scale.y / Scale.x, 1 );
	float2 Texcoord1 = (Texcoord0.xy - 0.5) * ((ModSize) / (Scale.xy * SimulationAspect.xy));
	float2 ForwardVec = float2( (ForwardVectorX), (ForwardVectorY) );
	Texcoord1 = (ForwardVec.xy * Texcoord1.x) + (float2( -ForwardVec.y, ForwardVec.x ) * Texcoord1.y);
	Texcoord1 = Texcoord1.xy * SimulationAspect.xy + (((ModLoc) - Location.xy) / Scale.xy);
	float4 SrcValue = FluxModifierMode( Weight, Texcoord1.xy, Height );
	
#if SWE_COMPUTE_PATH
	float4 DstValue = OutVelocityHeightFoamTex[ Coord ];
	OutValue = float4( DstValue.xyz * (1 - SrcValue.w) + SrcValue.xyz, DstValue.w * (1 - SrcValue.w));
#else
	OutValue = SrcValue;
#endif
	return true;
}

[numthreads( 8, 8, 1 )]
void MainCS( uint3 InDispatchThreadID : SV_DispatchThreadID )
{
	int2 Coord = int2( InDispatchThreadID.xy ) + asint( PixelAnchor );
	float4 Value;
	ComputeModifier( Coord, Value );
	OutVelocityHeightFoamTex[ Coord ] = Value;
}

// Mobile VS+PS fallback
void MainVS( in uint VertexID : SV_VertexID, out float4 OutPosition : SV_POSITION, out float2 OutUV : TEXCOORD0 )
{
	OutUV = float2( (VertexID << 1) & 2, VertexID & 2 );
	OutPosition = float4( OutUV * float2( 2, -2 ) + float2( -1, 1 ), 0, 1 );
}

void MainPS( in float4 Position : SV_POSITION, in float2 UV : TEXCOORD0, out float4 OutColor : SV_Target0 )
{
	ComputeModifier( int2( Position.xy ), OutColor );
}
#else
[numthreads(8, 1, 1)]
void main()
{
    
}
#endif	// 0
#endif	// _SHALLOW_WATERE_QUATION_MODIFIER_PE_CS_HLSL