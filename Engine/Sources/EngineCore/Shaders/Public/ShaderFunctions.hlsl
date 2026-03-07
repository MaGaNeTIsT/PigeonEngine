#ifndef _SHADER_FUNCTIONS_HLSL
#define _SHADER_FUNCTIONS_HLSL

#include "./ShaderSelectFunctions.hlsl"

float3 SafeNormalize(const float3 Input)
{
	float3 Output = normalize(Input);
	bool i = any(isinf(Output));
	Output = i ? float3(0.0, 0.0, 1.0) : Output;
	return Output;
}

float TakeSmallerAbsDelta(float Left, float Mid, float Right)
{
	float a = Mid - Left;
	float b = Right - Mid;

	return (abs(a) < abs(b)) ? a : b;
}

float Power2(float v)
{
	return (v * v);
}

float Power5(float v)
{
	float v2 = v * v;
	return (v2 * v2 * v);
}

float Max3(float3 Input)
{
	return max(max(Input.x, Input.y), Input.z);
}

float Min3(float3 Input)
{
	return min(min(Input.x, Input.y), Input.z);
}

// Octahedron Normal Vectors
// [Cigolle 2014, "A Survey of Efficient Representations for Independent Unit Vectors"]
//						Mean	Max
// oct		8:8			0.33709 0.94424
// snorm	8:8:8		0.17015 0.38588
// oct		10:10		0.08380 0.23467
// snorm	10:10:10	0.04228 0.09598
// oct		12:12		0.02091 0.05874

float2 UnitVectorToOctahedron(float3 InVec)
{
	InVec.xy /= dot(1, abs(InVec));
	if (InVec.z <= 0.0)
	{
		InVec.xy = (1.0 - abs(InVec.yx)) * SelectInternal(InVec.xy >= 0.0, float2(1, 1), float2(-1, -1));
	}
	return InVec.xy;
}

float3 OctahedronToUnitVector(float2 InOct)
{
	float3 Vec = float3(InOct, 1.0 - dot(1.0, abs(InOct)));
	float t = max(-Vec.z, 0.0);
	Vec.xy += SelectInternal(Vec.xy >= 0.0, float2(-t, -t), float2(t, t));
	return normalize(Vec);
}

float2 UnitVectorToHemiOctahedron(float3 InVec)
{
	InVec.xy /= dot(1.0, abs(InVec));
	return float2(InVec.x + InVec.y, InVec.x - InVec.y);
}

float3 HemiOctahedronToUnitVector(float2 InOct)
{
	InOct = float2(InOct.x + InOct.y, InOct.x - InOct.y);
	float3 Vec = float3(InOct, 2.0 - dot(1.0, abs(InOct)));
	return normalize(Vec);
}

// Wrap around octahedral map for correct hardware bilinear filtering
uint2 OctahedralMapWrapBorder(uint2 InTexelCoord, uint InResolution, uint InBorderSize)
{
	if (InTexelCoord.x < InBorderSize)
	{
		InTexelCoord.x = InBorderSize - 1u + InBorderSize - InTexelCoord.x;
		InTexelCoord.y = InResolution - 1u - InTexelCoord.y;
	}
	if (InTexelCoord.x >= InResolution - InBorderSize)
	{
		InTexelCoord.x = (InResolution - InBorderSize) - (InTexelCoord.x - (InResolution - InBorderSize - 1u));
		InTexelCoord.y = InResolution - 1u - InTexelCoord.y;
	}
	if (InTexelCoord.y < InBorderSize)
	{
		InTexelCoord.y = InBorderSize - 1u + InBorderSize - InTexelCoord.y;
		InTexelCoord.x = InResolution - 1u - InTexelCoord.x;
	}
	if (InTexelCoord.y >= InResolution - InBorderSize)
	{
		InTexelCoord.y = (InResolution - InBorderSize) - (InTexelCoord.y - (InResolution - InBorderSize - 1u));
		InTexelCoord.x = InResolution - 1u - InTexelCoord.x;
	}

	return (InTexelCoord - InBorderSize);
}

// Computes the spherical excess (solid angle) of a spherical triangle with vertices A, B, C as unit length vectors
// https://en.wikipedia.org/wiki/Spherical_trigonometry#Area_and_spherical_excess
float ComputeSphericalExcess(float3 A, float3 B, float3 C)
{
	float CosAB = dot(A, B);
	float SinAB = 1.0 - CosAB * CosAB;
	float CosBC = dot(B, C);
	float SinBC = 1.0 - CosBC * CosBC;
	float CosCA = dot(C, A);
	float CosC = CosCA - CosAB * CosBC;
	float SinC = sqrt(SinAB * SinBC - CosC * CosC);
	float Inv = (1.0 - CosAB) * (1.0 - CosBC);
	return 2.0 * atan2(SinC, sqrt((SinAB * SinBC * (1.0 + CosBC) * (1.0 + CosAB)) / Inv) + CosC);
}

// TexelCoord should be centered on the octahedral texel, in the range [.5f, .5f + Resolution - 1]
float OctahedralSolidAngle(float2 InTexelCoord, float InInvResolution)
{
	float3 Direction10 = OctahedronToUnitVector((InTexelCoord + float2(0.5, -0.5) * InInvResolution) * 2.0 - 1.0);
	float3 Direction01 = OctahedronToUnitVector((InTexelCoord + float2(-0.5, 0.5) * InInvResolution) * 2.0 - 1.0);

	float SolidAngle0 = ComputeSphericalExcess(
		OctahedronToUnitVector((InTexelCoord + float2(-0.5, -0.5) * InInvResolution) * 2.0 - 1.0),
		Direction10,
		Direction01);

	float SolidAngle1 = ComputeSphericalExcess(
		OctahedronToUnitVector((InTexelCoord + float2(0.5, 0.5) * InInvResolution) * 2.0 - 1.0),
		Direction01,
		Direction10);

	return (SolidAngle0 + SolidAngle1);
}

#endif	//_SHADER_FUNCTIONS_HLSL