#define VelocityHeightFoamTex			UCGInput0
#define GroundTex						UCGInput1
#define OutIntegrateVelocity			UCGOutput0

#define SWE_USE_FLUX_SAMPLE_FUNCTION	1
#include "ShallowWaterEquationCommon.ush"

#define Gravity							UCGVectorParams[(SWE_VectorParamOffset) + 0].x
#define VelocityClamp					UCGVectorParams[(SWE_VectorParamOffset) + 0].y
#define ScaleZ							UCGVectorParams[(SWE_VectorParamOffset) + 0].z
#define Friction						UCGVectorParams[(SWE_VectorParamOffset) + 0].w
#define Damping							UCGVectorParams[(SWE_VectorParamOffset) + 1].x
#define AccelerationClamp				UCGVectorParams[(SWE_VectorParamOffset) + 1].y
#define OvershootingBlend				UCGVectorParams[(SWE_VectorParamOffset) + 1].z
#define OvershootingLimit				UCGVectorParams[(SWE_VectorParamOffset) + 1].w
#define MinWater						UCGVectorParams[(SWE_VectorParamOffset) + 2].x	// 0.000001
#define PixelAnchor						UCGVectorParams[(SWE_VectorParamOffset) + 3].xy
#define PixelSize						UCGVectorParams[(SWE_VectorParamOffset) + 3].zw

float2 FluxClampMax2( in float2 InVector, in float InMaxScalar )
{
	float Len = distance( 0, InVector.xy );
	return (InVector * min( 1, InMaxScalar / Len ));
}

float FluxRemoveFluid( in float InW0, in float InW1, in float InSlope )
{
	return ((((InSlope > 0) ? InW0 : InW1) > (MinWater)) ? 1 : 0);
}

float2 SelectVector2( in float InCon, in float2 InVector2 )
{
	return ((InCon > 0) ? (InVector2.xy) : 0);
}

float Overshooting( float HC, float VC, float HL, float VL, float HR, float VR, float HB, float VB, float HT, float VT, float Blend, float Limit )
{
	float Change = 0;

	BRANCH
	if (Blend > 0)
	{
		if (HC > 0)
		{
			if ((HC - HL > Limit) && (HC > HR))
			{
				float Remove = Blend * (max( 0, 0.5 * (VC + VR) ) - VC);
				Change += Remove;
			}
			if ((HC - HR > Limit) && (HC > HL))
			{
				float Remove = Blend * (max( 0, 0.5 * (VC + VL) ) - VC);
				Change += Remove;
			}
			if ((HC - HB > Limit) && (HC > HT))
			{
				float Remove = Blend * (max( 0, 0.5 * (VC + VT) ) - VC);
				Change += Remove;
			}
			if ((HC - HT > Limit) && (HC > HB))
			{
				float Remove = Blend * (max( 0, 0.5 * (VC + VB) ) - VC);
				Change += Remove;
			}
		}
		return Change;
	}
	return 0;
}

bool ComputeIntegrateVelocity( in int2 Coord, out float4 OutValue )
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
		OutValue = float4(0, 0, 0, 0);
		return false;
	}
#endif

	float2 Texcoord = (Coord.xy + 0.5) / Resolution.xy;
	
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
	
	float2 Velocity = FluxClampMax2( Acceleration.xy * (SWE_DeltaTime) * (Gravity), (AccelerationClamp) ) + Velc.xy;
	
	float Ac = Vc * InvScaleZ;
	float Ar = Vr * InvScaleZ;
	float Ab = Vb * InvScaleZ;
	
	float2 Wet = float2( FluxRemoveFluid( Ac, Ar, Acceleration.x ), FluxRemoveFluid( Ac, Ab, Acceleration.y ) );
	
	Velocity = Velocity.xy * Wet.xy;
	
	float FrictionFactor = distance( 0, Velocity.xy ) * (SWE_DeltaTime) * (Friction);
	FrictionFactor = FrictionFactor / max( 0.0001, (Ac * 2 + Ar + Ab) * 0.25 );
	
	Velocity = Velocity * saturate( 1 - FrictionFactor );
	Velocity = FluxClampMax2( Velocity.xy - (clamp( Velocity.xy, -0.01, 0.01 ) * (Damping) * (SWE_DeltaTime)), (VelocityClamp) );
	
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

[numthreads( 8, 8, 1 )]
void MainCS( uint3 InDispatchThreadID : SV_DispatchThreadID )
{
	int2 Coord = int2( InDispatchThreadID.xy ) + asint( PixelAnchor );
	float4 Value;
	const bool bOutSide = ComputeIntegrateVelocity( Coord, Value );
	if (bOutSide)
	{
		OutIntegrateVelocity[ Coord ] = Value;
	}
}

void MainVS(in uint VertexID : SV_VertexID, out float4 OutPosition : SV_POSITION, out float2 OutUV : TEXCOORD0)
{
	OutUV = float2((VertexID << 1) & 2, VertexID & 2);
	OutPosition = float4(OutUV * float2(2, -2) + float2(-1, 1), 0, 1);
}

void MainPS(in float4 Position : SV_POSITION, in float2 UV : TEXCOORD0, out float4 OutColor : SV_Target0)
{
	ComputeIntegrateVelocity( int2( Position.xy ), OutColor );
}
