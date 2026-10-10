#ifndef SHADER_MATH_HLSL
#define SHADER_MATH_HLSL

// ---------------------------------------------------------------------------
// General purpose shader math helpers.
//
// Merged from the legacy Public/ShaderFunctions.hlsl, Public/ShaderFastMath.hlsl
// and Public/ShaderSelectFunctions.hlsl. Only the parts every stage needs live
// here; domain specific math stays in its own library file.
//
// Everything named *Fast is an approximation. Use it where the error budget is
// known to be acceptable, never for geometry that feeds a depth or shadow test.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"

// ---------------------------------------------------------------------------
// Component-wise select
//
// The legacy library spelled out 75 hand written overloads; the HLSL select()
// intrinsic is the same operation, so the overloads below are one line each and
// keep the original call shape (result is A where the condition holds).
// ---------------------------------------------------------------------------

float	SelectInternal(bool InCondition, float InTrueValue, float InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

float2	SelectInternal(bool InCondition, float2 InTrueValue, float2 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

float3	SelectInternal(bool InCondition, float3 InTrueValue, float3 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

float4	SelectInternal(bool InCondition, float4 InTrueValue, float4 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

float2	SelectInternal(bool2 InCondition, float2 InTrueValue, float2 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

float3	SelectInternal(bool3 InCondition, float3 InTrueValue, float3 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

float4	SelectInternal(bool4 InCondition, float4 InTrueValue, float4 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

float2	SelectInternal(bool2 InCondition, float InTrueValue, float InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

float3	SelectInternal(bool3 InCondition, float InTrueValue, float InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

float4	SelectInternal(bool4 InCondition, float InTrueValue, float InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

int		SelectInternal(bool InCondition, int InTrueValue, int InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

int2	SelectInternal(bool2 InCondition, int2 InTrueValue, int2 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

int3	SelectInternal(bool3 InCondition, int3 InTrueValue, int3 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

int4	SelectInternal(bool4 InCondition, int4 InTrueValue, int4 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

uint	SelectInternal(bool InCondition, uint InTrueValue, uint InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

uint2	SelectInternal(bool2 InCondition, uint2 InTrueValue, uint2 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

uint3	SelectInternal(bool3 InCondition, uint3 InTrueValue, uint3 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

uint4	SelectInternal(bool4 InCondition, uint4 InTrueValue, uint4 InFalseValue)
{
	return select(InCondition, InTrueValue, InFalseValue);
}

// ---------------------------------------------------------------------------
// Robust helpers
// ---------------------------------------------------------------------------

// normalize() but never returns a NaN vector.
float3 SafeNormalize(in float3 InValue)
{
	float3 Result = normalize(InValue);
	bool bIsInfinite = any(isinf(Result));
	Result = bIsInfinite ? float3(0.0, 0.0, 1.0) : Result;
	return Result;
}

// Picks whichever neighbour is closer to the middle sample; used by the
// derivative based edge detection in post process.
float TakeSmallerAbsDelta(in float InLeft, in float InMid, in float InRight)
{
	float DeltaL = InMid - InLeft;
	float DeltaR = InRight - InMid;
	return (abs(DeltaL) < abs(DeltaR)) ? DeltaL : DeltaR;
}

float Power2(in float InValue)
{
	return (InValue * InValue);
}

float Power5(in float InValue)
{
	float Value2 = InValue * InValue;
	return (Value2 * Value2 * InValue);
}

float Max3(in float3 InValue)
{
	return max(max(InValue.x, InValue.y), InValue.z);
}

float Min3(in float3 InValue)
{
	return min(min(InValue.x, InValue.y), InValue.z);
}

// Keeps a value representable by fp16 arithmetic.
float ClampMediumPrecision(in float InValue)
{
	return min(InValue, PE_SHADER_MEDIUM_PRECISION_FLT_MAX);
}

// ---------------------------------------------------------------------------
// Fast approximations
// ---------------------------------------------------------------------------

// Relative error about 3.4% over the full range. 2 ALU.
float RsqrtFast(in float InValue)
{
	int ValueBits = asint(InValue);
	ValueBits = 0x5f3759df - (ValueBits >> 1);
	return asfloat(ValueBits);
}

// Relative error below 0.7% over the full range. 1 ALU.
float SqrtFast(in float InValue)
{
	int ValueBits = asint(InValue);
	ValueBits = 0x1FBD1DF5 + (ValueBits >> 1);
	return asfloat(ValueBits);
}

// Relative error below 0.4% over the full range. 1 ALU.
float RcpFast(in float InValue)
{
	int ValueBits = asint(InValue);
	ValueBits = 0x7EF311C2 - ValueBits;
	return asfloat(ValueBits);
}

// One Newton-Raphson refinement: relative error below 0.02%. 3 ALU.
float RcpFastNR1(in float InValue)
{
	int ValueBits = asint(InValue);
	ValueBits = 0x7EF311C3 - ValueBits;
	float Result = asfloat(ValueBits);
	Result = Result * (-Result * InValue + 2.0);
	return Result;
}

float LengthFast(in float3 InValue)
{
	float LengthSqr = dot(InValue, InValue);
	return SqrtFast(LengthSqr);
}

float3 NormalizeFast(in float3 InValue)
{
	float LengthSqr = dot(InValue, InValue);
	return InValue * RsqrtFast(LengthSqr);
}

// Max absolute error 9.0e-3. Input [-1, 1], output [0, PI].
float AcosFast(in float InValue)
{
	float X = abs(InValue);
	float Result = -0.156583 * X + (0.5 * PE_SHADER_PI);
	Result *= sqrt(1.0 - X);
	return (InValue >= 0.0) ? Result : PE_SHADER_PI - Result;
}

float2 AcosFast(in float2 InValue)
{
	return float2(AcosFast(InValue.x), AcosFast(InValue.y));
}

float3 AcosFast(in float3 InValue)
{
	return float3(AcosFast(InValue.x), AcosFast(InValue.y), AcosFast(InValue.z));
}

float4 AcosFast(in float4 InValue)
{
	return float4(AcosFast(InValue.x), AcosFast(InValue.y), AcosFast(InValue.z), AcosFast(InValue.w));
}

// Same cost and error as AcosFast. Input [-1, 1], output [-PI/2, PI/2].
float AsinFast(in float InValue)
{
	return (0.5 * PE_SHADER_PI) - AcosFast(InValue);
}

float2 AsinFast(in float2 InValue)
{
	return float2(AsinFast(InValue.x), AsinFast(InValue.y));
}

float3 AsinFast(in float3 InValue)
{
	return float3(AsinFast(InValue.x), AsinFast(InValue.y), AsinFast(InValue.z));
}

float4 AsinFast(in float4 InValue)
{
	return float4(AsinFast(InValue.x), AsinFast(InValue.y), AsinFast(InValue.z), AsinFast(InValue.w));
}

// Max absolute error 1.3e-3. Input [0, infinity), output [0, PI/2].
float AtanFastPos(in float InValue)
{
	float X = (InValue < 1.0) ? InValue : 1.0 / InValue;
	float X2 = X * X;
	float Poly = 0.0872929;
	Poly = -0.301895 + Poly * X2;
	Poly = 1.0 + Poly * X2;
	Poly = Poly * X;
	return (InValue < 1.0) ? Poly : (0.5 * PE_SHADER_PI) - Poly;
}

// Input (-infinity, infinity), output (-PI/2, PI/2).
float AtanFast(in float InValue)
{
	float Result = AtanFastPos(abs(InValue));
	return (InValue < 0.0) ? -Result : Result;
}

float2 AtanFast(in float2 InValue)
{
	return float2(AtanFast(InValue.x), AtanFast(InValue.y));
}

float3 AtanFast(in float3 InValue)
{
	return float3(AtanFast(InValue.x), AtanFast(InValue.y), AtanFast(InValue.z));
}

float4 AtanFast(in float4 InValue)
{
	return float4(AtanFast(InValue.x), AtanFast(InValue.y), AtanFast(InValue.z), AtanFast(InValue.w));
}

float Atan2Fast(in float InY, in float InX)
{
	float MaxAbs = max(abs(InX), abs(InY));
	float MinAbs = min(abs(InX), abs(InY));
	float Ratio = MinAbs / MaxAbs;
	float Ratio2 = Ratio * Ratio;

	float Poly = 0.0872929;
	Poly = Poly * Ratio2 - 0.301895;
	Poly = Poly * Ratio2 + 1.0;
	Ratio = Poly * Ratio;

	Ratio = (abs(InY) > abs(InX)) ? (0.5 * PE_SHADER_PI) - Ratio : Ratio;
	Ratio = (InX < 0.0) ? PE_SHADER_PI - Ratio : Ratio;
	Ratio = (InY < 0.0) ? -Ratio : Ratio;

	return Ratio;
}

float2 Atan2Fast(in float2 InY, in float2 InX)
{
	return float2(Atan2Fast(InY.x, InX.x), Atan2Fast(InY.y, InX.y));
}

float3 Atan2Fast(in float3 InY, in float3 InX)
{
	return float3(Atan2Fast(InY.x, InX.x), Atan2Fast(InY.y, InX.y), Atan2Fast(InY.z, InX.z));
}

float4 Atan2Fast(in float4 InY, in float4 InX)
{
	return float4(Atan2Fast(InY.x, InX.x), Atan2Fast(InY.y, InX.y), Atan2Fast(InY.z, InX.z), Atan2Fast(InY.w, InX.w));
}

// Degree four polynomial fit, max absolute error 7.0e-5.
float AcosFast4(in float InValue)
{
	float X1 = abs(InValue);
	float X2 = X1 * X1;
	float X3 = X2 * X1;

	float Result = -0.2121144 * X1 + 1.5707288;
	Result = 0.0742610 * X2 + Result;
	Result = -0.0187293 * X3 + Result;
	Result = sqrt(1.0 - X1) * Result;

	return (InValue >= 0.0) ? Result : PE_SHADER_PI - Result;
}

// Degree four polynomial fit, max absolute error 7.0e-5.
float AsinFast4(in float InValue)
{
	return (0.5 * PE_SHADER_PI) - AcosFast4(InValue);
}

// Neither input has to be normalized. Returns a value ready to be fed into
// AcosFast; it can be NaN when either input is near zero length.
float CosBetweenVectors(in float3 InA, in float3 InB)
{
	return dot(InA, InB) * rsqrt(dot(InA, InA) * dot(InB, InB));
}

float AngleBetweenVectors(in float3 InA, in float3 InB)
{
	return acos(CosBetweenVectors(InA, InB));
}

float AngleBetweenVectorsFast(in float3 InA, in float3 InB)
{
	return AcosFast(CosBetweenVectors(InA, InB));
}

// Returns the sign bit of a float as exactly 1 or -1.
int SignFastInt(in float InValue)
{
	return 1 - int((asuint(InValue) & 0x80000000) >> 30);
}

int2 SignFastInt(in float2 InValue)
{
	return int2(SignFastInt(InValue.x), SignFastInt(InValue.y));
}
#endif	//SHADER_MATH_HLSL
