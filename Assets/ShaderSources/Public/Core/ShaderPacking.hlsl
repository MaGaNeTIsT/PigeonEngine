#ifndef SHADER_PACKING_HLSL
#define SHADER_PACKING_HLSL

// ---------------------------------------------------------------------------
// Bit, half and direction packing helpers shared by the GBuffer encoders, the
// decoders and the deferred lighting passes.
//
// Anything that knows the *meaning* of a GBuffer channel lives in
// Public/GBuffer/ShaderGBufferIO.hlsl; this file only knows how to move bits.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"

// ---------------------------------------------------------------------------
// Scalar conversions
// ---------------------------------------------------------------------------

// Rec. 709 relative luminance.
float Luminance(in float3 InColor)
{
	return dot(InColor, float3(0.212671, 0.715160, 0.072169));
}

float SrgbToLinear(in float InValue)
{
	float Result;
	if (InValue <= 0.04045)
	{
		Result = InValue / 12.92;
	}
	else
	{
		Result = pow((InValue + 0.055) / 1.055, 2.4);
	}
	return Result;
}

float LinearToSrgb(in float InValue)
{
	float Result;
	if (InValue <= 0.0031308)
	{
		Result = InValue * 12.92;
	}
	else
	{
		Result = pow(InValue, 1.0 / 2.4) * 1.055 - 0.055;
	}
	return Result;
}

float3 SrgbToLinear(in float3 InColor)
{
	return float3(SrgbToLinear(InColor.r), SrgbToLinear(InColor.g), SrgbToLinear(InColor.b));
}

float3 LinearToSrgb(in float3 InColor)
{
	return float3(LinearToSrgb(InColor.r), LinearToSrgb(InColor.g), LinearToSrgb(InColor.b));
}

// ---------------------------------------------------------------------------
// Bit field helpers (used by the integer GBuffer target)
// ---------------------------------------------------------------------------

// Reads InBitCount bits from InValue starting at InBitOffset, right aligned.
uint ExtractBits(in uint InValue, in uint InBitOffset, in uint InBitCount)
{
	uint Mask = (InBitCount >= 32u) ? 0xFFFFFFFFu : ((1u << InBitCount) - 1u);
	return (InValue >> InBitOffset) & Mask;
}

// Writes the low InBitCount bits of InBits into InValue at InBitOffset.
uint InsertBits(in uint InValue, in uint InBits, in uint InBitOffset, in uint InBitCount)
{
	uint Mask = (InBitCount >= 32u) ? 0xFFFFFFFFu : ((1u << InBitCount) - 1u);
	return (InValue & ~(Mask << InBitOffset)) | ((InBits & Mask) << InBitOffset);
}

// Packs eight values into one uint, InBitCount bits each, lowest value first.
uint PackUint8x4(in uint4 InValues, in uint InBitCount)
{
	uint Result = 0u;
	Result |= ExtractBits(InValues.x, 0u, InBitCount) << (InBitCount * 0u);
	Result |= ExtractBits(InValues.y, 0u, InBitCount) << (InBitCount * 1u);
	Result |= ExtractBits(InValues.z, 0u, InBitCount) << (InBitCount * 2u);
	Result |= ExtractBits(InValues.w, 0u, InBitCount) << (InBitCount * 3u);
	return Result;
}

uint4 UnpackUint8x4(in uint InValue, in uint InBitCount)
{
	return uint4(
		ExtractBits(InValue, InBitCount * 0u, InBitCount),
		ExtractBits(InValue, InBitCount * 1u, InBitCount),
		ExtractBits(InValue, InBitCount * 2u, InBitCount),
		ExtractBits(InValue, InBitCount * 3u, InBitCount));
}

// ---------------------------------------------------------------------------
// Octahedron direction encoding
//
// [Cigolle 2014, "A Survey of Efficient Representations for Independent Unit
// Vectors"] -- mean / max error for a 16:16 layout is 0.08380 / 0.23467 degrees.
//
// The Unit variants work in [-1, 1]; the Encode/Decode pair is the [0, 1]
// normalized form that fits an integer or unorm render target channel.
// ---------------------------------------------------------------------------

float2 UnitVectorToOctahedron(in float3 InVector)
{
	float3 Vector = InVector;
	Vector.xy /= dot(1.0, abs(Vector));
	if (Vector.z <= 0.0)
	{
		Vector.xy = (1.0 - abs(Vector.yx)) * SelectInternal(Vector.xy >= 0.0, float2(1.0, 1.0), float2(-1.0, -1.0));
	}
	return Vector.xy;
}

float3 OctahedronToUnitVector(in float2 InOct)
{
	float3 Vector = float3(InOct, 1.0 - dot(1.0, abs(InOct)));
	float Offset = max(-Vector.z, 0.0);
	Vector.xy += SelectInternal(Vector.xy >= 0.0, float2(-Offset, -Offset), float2(Offset, Offset));
	return normalize(Vector);
}

// [0, 1] form for render target storage.
float2 EncodeOctahedron(in float3 InNormal)
{
	return UnitVectorToOctahedron(InNormal) * 0.5 + 0.5;
}

float3 DecodeOctahedron(in float2 InEncoded)
{
	return OctahedronToUnitVector(InEncoded * 2.0 - 1.0);
}

// Half octahedron: only covers the hemisphere around +Z, so it is not usable
// for a full view space normal, but it is cheaper for tangent space data.
float2 UnitVectorToHemiOctahedron(in float3 InVector)
{
	float3 Vector = InVector;
	Vector.xy /= dot(1.0, abs(Vector));
	return float2(Vector.x + Vector.y, Vector.x - Vector.y);
}

float3 HemiOctahedronToUnitVector(in float2 InOct)
{
	float2 Oct = float2(InOct.x + InOct.y, InOct.x - InOct.y);
	float3 Vector = float3(Oct, 2.0 - dot(1.0, abs(Oct)));
	return normalize(Vector);
}

// Remaps a texel coordinate that fell outside the octahedral map so hardware
// bilinear filtering wraps around the border instead of bleeding across faces.
uint2 OctahedralMapWrapBorder(in uint2 InTexelCoord, in uint InResolution, in uint InBorderSize)
{
	uint2 TexelCoord = InTexelCoord;
	if (TexelCoord.x < InBorderSize)
	{
		TexelCoord.x = InBorderSize - 1u + InBorderSize - TexelCoord.x;
		TexelCoord.y = InResolution - 1u - TexelCoord.y;
	}
	if (TexelCoord.x >= InResolution - InBorderSize)
	{
		TexelCoord.x = (InResolution - InBorderSize) - (TexelCoord.x - (InResolution - InBorderSize - 1u));
		TexelCoord.y = InResolution - 1u - TexelCoord.y;
	}
	if (TexelCoord.y < InBorderSize)
	{
		TexelCoord.y = InBorderSize - 1u + InBorderSize - TexelCoord.y;
		TexelCoord.x = InResolution - 1u - TexelCoord.x;
	}
	if (TexelCoord.y >= InResolution - InBorderSize)
	{
		TexelCoord.y = (InResolution - InBorderSize) - (TexelCoord.y - (InResolution - InBorderSize - 1u));
		TexelCoord.x = InResolution - 1u - TexelCoord.x;
	}

	return (TexelCoord - InBorderSize);
}

// Spherical excess (solid angle) of the spherical triangle A, B, C.
// https://en.wikipedia.org/wiki/Spherical_trigonometry#Area_and_spherical_excess
float ComputeSphericalExcess(in float3 InA, in float3 InB, in float3 InC)
{
	float CosAB = dot(InA, InB);
	float SinAB = 1.0 - CosAB * CosAB;
	float CosBC = dot(InB, InC);
	float SinBC = 1.0 - CosBC * CosBC;
	float CosCA = dot(InC, InA);
	float CosC = CosCA - CosAB * CosBC;
	float SinC = sqrt(SinAB * SinBC - CosC * CosC);
	float Inv = (1.0 - CosAB) * (1.0 - CosBC);
	return 2.0 * atan2(SinC, sqrt((SinAB * SinBC * (1.0 + CosBC) * (1.0 + CosAB)) / Inv) + CosC);
}

// InTexelCoord is expected to be centered on the octahedral texel, in the range
// [0.5, 0.5 + Resolution - 1].
float OctahedralSolidAngle(in float2 InTexelCoord, in float InInvResolution)
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
#endif	//SHADER_PACKING_HLSL
