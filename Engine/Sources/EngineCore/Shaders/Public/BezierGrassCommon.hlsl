#ifndef _BEZIER_GRASS_COMMON_HLSL
#define _BEZIER_GRASS_COMMON_HLSL

struct GrassPropertyInfo
{
    float LOD;
    float LeafWidth;
};

struct GrassInstanceInfo
{
    float3 Origin;
    float Tilt;
    float3 Direction;
    float Bend;
    float2 Tip;
};

struct BezierPropertyInfo
{
    float T;
    float NegOffset;
    float PosOffset;
    float PrevLODT;
};

GrassPropertyInfo GetGrassPropertyInfo(in float4 Input)
{
    GrassPropertyInfo Output;
    Output.LOD = Input.x;
    Output.LeafWidth = Input.y;
    return Output;
}

BezierPropertyInfo GetBezierPropertyInfo(in float4 Input)
{
    BezierPropertyInfo Output;
    Output.T = Input.x;
    Output.NegOffset = Input.y;
    Output.PosOffset = Input.z;
    Output.PrevLODT = Input.w;
    return Output;
}

float RandomVector3(in float3 vec)
{
    return frac(sin(dot(vec.xyz, float3(12.9898, 78.233, 47.2311))) * 43758.5453123);
}

float4 TransformWorldToClip(const float3 Position, in float4x4 InViewProjectionMatrix)
{
    return mul(float4(Position, 1.0), InViewProjectionMatrix);
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