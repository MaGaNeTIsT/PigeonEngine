#ifndef SHADER_SPHERICAL_HARMONICS_HLSL
#define SHADER_SPHERICAL_HARMONICS_HLSL

// ---------------------------------------------------------------------------
// Order 3 spherical harmonics: the basis, and the two operations that have to
// agree about it.
//
// The projection (a bake, run once when the environment changes) and the
// reconstruction (the lighting pass, run per pixel) live in one file on purpose.
// SH coefficients are only meaningful against the basis they were projected
// against, and in most code bases the two directions are written in different
// files - which is how an axis swap or an index reorder survives review and shows
// up later as ambient light arriving from the wrong side. Sharing
// EvaluateSHBasisComponent between both directions makes the pair structurally
// consistent rather than consistent by inspection.
//
// That also makes this file independent of the engine's world axis convention:
// the "z" in the band 1 term is whatever axis the caller passes as its up axis,
// which is correct because the evaluator passes the same one the projector did -
// it is the same function.
//
// ---------------------------------------------------------------------------
// Why order 3 and not higher
//
// Irradiance is an integral over a hemisphere, so it is a low frequency quantity
// by construction: the clamped cosine kernel sits almost entirely inside bands 0
// and 1, and band 2 carries most of what is left. Order 4 would cost seven more
// coefficients per colour plus seven more basis evaluations per pixel, to resolve
// detail that the convolution has already integrated away. High frequency
// environment detail is the specular prefilter chain's job, not this file's.
//
// ---------------------------------------------------------------------------
// Ringing
//
// A truncated SH series reconstructs negative values where the environment has
// strong contrast, which reads as a dark ring around the horizon. GetSHWindow
// damps the higher bands so the reconstruction stays positive, and that is why
// this file does not need a higher order to be safe. The window is applied to the
// coefficients at bake time, so it costs nine multiplies once rather than one
// multiply per pixel.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"

#define SH_COEFFICIENT_COUNT	(9)
#define SH_L_MAX				(2)

// The band a coefficient belongs to: band 0 has one term, band 1 has three, band 2
// has five. The storage order is the canonical one, and it is the order the
// coefficients are projected, windowed, stored and evaluated in:
//
//   [0] Y(0, 0)    [1] Y(1,-1)   [2] Y(1, 0)   [3] Y(1, 1)
//   [4] Y(2,-2)    [5] Y(2,-1)   [6] Y(2, 0)   [7] Y(2, 1)   [8] Y(2, 2)
uint GetSHBand(in uint InCoefficientIndex)
{
	return ((InCoefficientIndex == 0u) ? 0u : ((InCoefficientIndex < 4u) ? 1u : 2u));
}

// The real SH basis value of one coefficient, for a unit direction, in the storage
// order above.
//
// This function is the single definition of the basis, and both the projection and
// the reconstruction call it. A second copy of these polynomials is exactly how a
// basis stops matching its own coefficients.
float EvaluateSHBasisComponent(in uint InCoefficientIndex, in float3 InDirection)
{
	const float x = InDirection.x;
	const float y = InDirection.y;
	const float z = InDirection.z;
	switch (InCoefficientIndex)
	{
		case 0u: return 0.282095;
		case 1u: return 0.488603 * y;
		case 2u: return 0.488603 * z;
		case 3u: return 0.488603 * x;
		case 4u: return 1.092548 * x * y;
		case 5u: return 1.092548 * y * z;
		case 6u: return 0.315392 * ((3.0 * z * z) - 1.0);
		case 7u: return 1.092548 * x * z;
		case 8u: return 0.546274 * ((x * x) - (y * y));
		default: return 0.0;
	}
}

// The Hanning window of one band: (1 + cos(pi * l / (lMax + 1))) / 2.
//
// Band 0 gets 1, so the environment's total irradiance survives the window even as
// its shape is smoothed - which is what keeps this a shape change and not a
// brightness change. If the ambient ever looks flatter than the environment
// deserves, this function is the single knob to relax.
float GetSHWindow(in uint InBand)
{
	return ((1.0 + cos(PE_SHADER_PI * (float(InBand) / float(SH_L_MAX + 1)))) * 0.5);
}

// The Lambertian transfer coefficient of one band: the SH coefficients of the
// clamped cosine kernel, where the pi of the hemisphere integral lives. This is
// what turns radiance coefficients into irradiance coefficients.
float GetSHLambertianTransfer(in uint InBand)
{
	return ((InBand == 0u) ? PE_SHADER_PI : ((InBand == 1u) ? ((2.0 * PE_SHADER_PI) / 3.0) : (PE_SHADER_PI * 0.25)));
}

// Irradiance [lux] for a surface normal, from the environment's radiance
// coefficients.
//
// The transfer and the basis are both applied here, so what comes back is the
// hemisphere integral E(n) and not a band limited radiance: a caller multiplies it
// by the diffuse BRDF weight (albedo / pi) and has outgoing luminance.
float3 EvaluateIrradianceSH(in float4 InCoefficients[SH_COEFFICIENT_COUNT], in float3 InNormal)
{
	float3 Result = float3(0.0, 0.0, 0.0);
	for (uint CoefficientIndex = 0u; CoefficientIndex < SH_COEFFICIENT_COUNT; CoefficientIndex++)
	{
		const uint Band = GetSHBand(CoefficientIndex);
		Result += InCoefficients[CoefficientIndex].rgb
			* (GetSHLambertianTransfer(Band) * EvaluateSHBasisComponent(CoefficientIndex, InNormal));
	}
	return Result;
}
#endif	//SHADER_SPHERICAL_HARMONICS_HLSL
