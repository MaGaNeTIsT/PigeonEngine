#ifndef _ENGINE_BEZIER_GRASS_COMMON_HLSL
#define _ENGINE_BEZIER_GRASS_COMMON_HLSL

//struct Attribute
//{
//	float4	PositionOS	: POSITION0;
//};

struct Varying
{
	float4	PositionCS	: SV_POSITION;
	float4	NormalWS	: NORMAL0;
};

struct GrassPropertyInfo
{
	uint	SplitNum;
	float	StepLength;
	float	LeafWidth;
};

struct GrassInstanceInfo
{
	float3		Origin;
	float3		Direction;
	float2		Tip;
	float		Tilt;
	float		Bend;
};

struct GrassInstanceParams
{
	float4	Params0;
	float4	Params1;
	float2	Params2;
};

struct ShadingLightParams
{
	float4	ColorIntensity;
	float3	Direction;
	float	Attenuation;
};

struct LightParams
{
	float4	Color;
	float4	Params0;
	float4	Params1;
};

cbuffer ConstantBufferPerFrame : register(b0)
{
	float4x4		_ViewMatrix;
	float4x4		_ViewInvMatrix;
	float4x4		_ProjectionMatrix;
	float4x4		_ProjectionInvMatrix;
	float4x4		_ViewProjectionMatrix;
	float4x4		_ViewProjectionInvMatrix;
	float4x4		_LightViewProjectionMatrix;
	float4			_TimeParams;
	float4			_DepthMultiAdd;
	float4			_ScreenToViewSpaceParams;
	float4			_CameraViewportMinSizeAndInvBufferSize;
	float4			_CameraViewportSizeAndInvSize;
	float4			_CameraViewportRect;
	float4			_CameraWorldPosition;
};

cbuffer ConstantBufferLightData : register(b1)
{
	uint4			_LightCount;
	LightParams		_LightData[16];
}

GrassPropertyInfo GetGrassPropertyInfo(in float4 Input)
{
	GrassPropertyInfo Output;
	Output.SplitNum = (uint)(Input.x);
	Output.StepLength = Input.y;
	Output.LeafWidth = Input.z;
	return Output;
}

GrassInstanceInfo GetGrassInstanceInfo(in GrassInstanceParams Input)
{
	GrassInstanceInfo Output;
	Output.Origin = float3(Input.Params0.x, Input.Params0.y, Input.Params0.z);
	Output.Direction = float3(Input.Params1.x, Input.Params1.y, Input.Params1.z);
	Output.Tip = float2(Input.Params2.xy);
	Output.Tilt = Input.Params0.w;
	Output.Bend = Input.Params1.w;
	return Output;
}

float3 SafeNormalize(const float3 Input)
{
	float3 Output = normalize(Input);
	bool i = any(isinf(Output));
	Output = i ? float3(0.0, 0.0, 1.0) : Output;
	return Output;
}

float RandomVector3(in float3 vec)
{
	return frac(sin(dot(vec.xyz, float3(12.9898, 78.233, 47.2311))) * 43758.5453123);
}

float4 TransformWorldToClip(const float3 Position)
{
	return mul(float4(Position, 1.0), _ViewProjectionMatrix);
}

float2 QuadraticBezierCurve(in float2 P0, in float2 P1, in float2 P2, in float T)
{
	float TSq = T * T;
	float OneMinusT = 1.0 - T;
	float OneMinusTSq = OneMinusT * OneMinusT;
	return (P0 * OneMinusTSq + P1 * (2.0 * OneMinusT * T) + P2 * TSq);
}

float2 DerivativeQuadraticBezierCurve(in float2 P0, in float2 P1, in float2 P2, in float T)
{
	return ((P1 - P0) * (2.0 * (1.0 - T)) + (P2 - P1) * (2.0 * T));
}

float2 CubicBezierCurve(in float2 P0, in float2 P1, in float2 P2, in float2 P3, in float T)
{
	float TSq = T * T;
	float TCu = TSq * T;
	float OneMinusT = 1.0 - T;
	float OneMinusTSq = OneMinusT * OneMinusT;
	float OneMinusTCu = OneMinusTSq * OneMinusT;
	return (P0 * OneMinusTCu + P1 * (3.0 * OneMinusTSq * T) + P2 * (3.0 * OneMinusT * TSq) + P3 * TCu);
}

float2 DerivativeCubicBezierCurve(in float2 P0, in float2 P1, in float2 P2, in float2 P3, in float T)
{
	float TSq = T * T;
	float OneMinusT = 1.0 - T;
	float OneMinusTSq = OneMinusT * OneMinusT;
	return ((P1 - P0) * (3.0 * OneMinusTSq) + (P2 - P1) * (6.0 * OneMinusT * T) + (P3 - P2) * (3.0 * TSq));
}

#endif