#ifndef ENVIRONMENT_SH_HLSL
#define ENVIRONMENT_SH_HLSL

// ---------------------------------------------------------------------------
// Projecting the environment cube into order 3 spherical harmonics.
//
// This is the bake half: it runs once when the sky's cubemap changes, and what it
// produces is nine radiance coefficients in the basis
// Public/Lighting/ShaderSphericalHarmonics.hlsl defines. The coefficients are the
// environment's *shape*, not its brightness - the brightness is applied where they
// are consumed - so a change to the sky's luminance or tint does NOT need this to
// run again.
//
// ---------------------------------------------------------------------------
// One threadgroup, nine active threads, no reduction
//
// Thread i owns coefficient i, all three channels, and scans every sample itself.
// That shape is chosen for the properties rather than for the parallelism:
//
//   - no cross thread reduction, so no group shared memory, no barriers and no
//     atomics, which is what makes the result bit for bit reproducible;
//   - each thread writes one whole float4, so there is no read-modify-write race
//     on the destination;
//   - nine threads is a poor use of a warp, and it does not matter. The bake runs
//     once, and the work is nine times a few thousand texture fetches.
//
// A sample parallel version would use the whole group and then need a reduction to
// sum it - more machinery, and a summation order that is only deterministic if the
// reduction is written to be. Neither is worth it for a one shot bake.
//
// ---------------------------------------------------------------------------
// Sample directions: equal area, so the weight is a constant
//
// The samples come from a Fibonacci spiral, which distributes N points over the
// sphere with each point standing for the same solid angle, 4 pi / N. The
// alternative - walking the cube's texels - needs a per texel solid angle, because
// a cube's texel area varies across a face, and getting that wrong is a silent
// cosine weighting error. Sampling directions removes the question.
//
// N is the knob for the projection's accuracy, and it is very forgiving: the
// quantity being projected is an integral over the hemisphere, so a few thousand
// directions resolve it far past what nine coefficients can carry.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"
#include "Public/Lighting/ShaderSphericalHarmonics.hlsl"

// The thread group is one warp, and only the first nine lanes do anything: one per
// coefficient.
#ifndef ENVIRONMENT_SH_THREAD_GROUP_SIZE
#define ENVIRONMENT_SH_THREAD_GROUP_SIZE	(32)
#endif	//ENVIRONMENT_SH_THREAD_GROUP_SIZE

#ifndef ENVIRONMENT_SH_SAMPLE_COUNT
#define ENVIRONMENT_SH_SAMPLE_COUNT			(4096)
#endif	//ENVIRONMENT_SH_SAMPLE_COUNT

// One point of the Fibonacci spiral: a near uniform distribution where every point
// carries the same solid angle. The golden angle keeps successive points from
// lining up into rings.
float3 EnvironmentSHGetSampleDirection(in uint InSampleIndex, in uint InSampleCount)
{
	const float Index = float(InSampleIndex) + 0.5;
	const float Count = float(InSampleCount);
	// The polar coordinate walks evenly so that the cap area between two samples is
	// constant, which is what makes the distribution equal area.
	const float Height = 1.0 - (2.0 * Index / Count);
	const float RingRadius = sqrt(saturate(1.0 - (Height * Height)));
	const float Azimuth = Index * 2.39996322972865332;	// the golden angle, pi * (3 - sqrt(5))
	return float3(cos(Azimuth) * RingRadius, Height, sin(Azimuth) * RingRadius);
}

[numthreads(ENVIRONMENT_SH_THREAD_GROUP_SIZE, 1, 1)]
void CSMain(in uint3 InDispatchThreadID : SV_DispatchThreadID)
{
	const uint CoefficientIndex = InDispatchThreadID.x;
	if (CoefficientIndex >= SH_COEFFICIENT_COUNT)
	{
		return;
	}

	const uint SampleCount = ENVIRONMENT_SH_SAMPLE_COUNT;
	const uint Band = GetSHBand(CoefficientIndex);

	float3 Sum = float3(0.0, 0.0, 0.0);
	for (uint SampleIndex = 0u; SampleIndex < SampleCount; SampleIndex++)
	{
		const float3 Direction = EnvironmentSHGetSampleDirection(SampleIndex, SampleCount);
		const float3 Radiance = PE_SAMPLE_TEXCUBE_LEVEL(EnvironmentCube, SamplerMaterialCube, Direction, 0.0).rgb;
		Sum += Radiance * EvaluateSHBasisComponent(CoefficientIndex, Direction);
	}

	// Every sample stands for the same solid angle, and the window is applied here so
	// that the reconstruction pays nothing for it. The result is dimensionless
	// radiance - see the note at the top about why the brightness is not in here.
	const float SolidAngle = (4.0 * PE_SHADER_PI) / float(SampleCount);
	EnvironmentSHOutput[CoefficientIndex] = float4(Sum * (SolidAngle * GetSHWindow(Band)), 0.0);
}
#endif	//ENVIRONMENT_SH_HLSL
