#ifndef ENVIRONMENT_PREFILTER_HLSL
#define ENVIRONMENT_PREFILTER_HLSL

// ---------------------------------------------------------------------------
// Environment prefilter: the radiance half of the split sum.
//
// One dispatch per mip. For every output texel, the direction the texel stands
// for is the sample normal (V = N, the split sum's assumption), and the kernel
// integrates the raw environment cube against the GGX distribution of the mip's
// perceptual roughness. The result is written to one face of one mip through a
// per-mip UAV view, so a mip chain is built level by level.
//
// The chain stores the environment's SHAPE: every sample is divided by the
// cube's mean linear luminance. The sky light's tint and mean luminance are
// applied where the chain is sampled, which is why changing either one costs a
// constant buffer and no dispatch.
//
// The source is sampled at a mip chosen from the sample's solid angle, the usual
// prefilter trick: importance samples that land on a tiny solid angle would
// otherwise turn a bright texel into a firefly, and the chain is baked once, so
// the bias that buys is cheaper than a higher sample count.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"
#include "Public/System/ShaderBindless.hlsl"

#ifndef ENVIRONMENT_PREFILTER_THREAD_GROUP_SIZE
#define ENVIRONMENT_PREFILTER_THREAD_GROUP_SIZE	(8)
#endif	//ENVIRONMENT_PREFILTER_THREAD_GROUP_SIZE

float2 EnvironmentPrefilterHammersley(in uint InIndex, in uint InCount)
{
	// Radical inverse in base two (van der Corput), paired with the uniform index.
	uint Bits = InIndex;
	Bits = (Bits << 16u) | (Bits >> 16u);
	Bits = ((Bits & 0x55555555u) << 1u) | ((Bits & 0xAAAAAAAAu) >> 1u);
	Bits = ((Bits & 0x33333333u) << 2u) | ((Bits & 0xCCCCCCCCu) >> 2u);
	Bits = ((Bits & 0x0F0F0F0Fu) << 4u) | ((Bits & 0xF0F0F0F0u) >> 4u);
	Bits = ((Bits & 0x00FF00FFu) << 8u) | ((Bits & 0xFF00FF00u) >> 8u);
	const float RadicalInverse = float(Bits) * 2.3283064365386963e-10f;
	return float2(float(InIndex) / float(InCount), RadicalInverse);
}

float3 EnvironmentPrefilterImportanceSampleGGX(in float2 InXi, in float3 InNormal, in float InAlpha)
{
	const float AlphaSquared = InAlpha * InAlpha;
	const float Phi = 2.0 * PE_SHADER_PI * InXi.x;
	const float CosTheta = sqrt((1.0 - InXi.y) / (1.0 + ((AlphaSquared - 1.0) * InXi.y)));
	const float SinTheta = sqrt(saturate(1.0 - (CosTheta * CosTheta)));

	const float3 HalfVectorTangent = float3(cos(Phi) * SinTheta, sin(Phi) * SinTheta, CosTheta);

	// Tangent frame around the normal. The prefilter only needs any consistent frame; the
	// distribution is rotationally symmetric around the normal.
	const float3 Up = (abs(InNormal.z) < 0.999f) ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	const float3 Tangent = normalize(cross(Up, InNormal));
	const float3 Bitangent = cross(InNormal, Tangent);
	return normalize((Tangent * HalfVectorTangent.x) + (Bitangent * HalfVectorTangent.y) + (InNormal * HalfVectorTangent.z));
}

float EnvironmentPrefilterDistributionGGX(in float InNdotH, in float InAlpha)
{
	const float AlphaSquared = InAlpha * InAlpha;
	const float Denominator = ((InNdotH * InNdotH) * (AlphaSquared - 1.0)) + 1.0;
	return AlphaSquared / max(PE_SHADER_PI * Denominator * Denominator, 1e-7);
}

// The direction one cube face texel stands for. The face order is the D3D cube order, which
// is the order the six imported faces and the prefiltered cube's array slices share.
float3 EnvironmentPrefilterFaceDirection(in uint InFace, in float2 InUV)
{
	switch (InFace)
	{
	case 0u: return normalize(float3(1.0, -InUV.y, -InUV.x));
	case 1u: return normalize(float3(-1.0, -InUV.y, InUV.x));
	case 2u: return normalize(float3(InUV.x, 1.0, InUV.y));
	case 3u: return normalize(float3(InUV.x, -1.0, -InUV.y));
	case 4u: return normalize(float3(InUV.x, -InUV.y, 1.0));
	default: return normalize(float3(-InUV.x, -InUV.y, -1.0));
	}
}

[numthreads(ENVIRONMENT_PREFILTER_THREAD_GROUP_SIZE, ENVIRONMENT_PREFILTER_THREAD_GROUP_SIZE, 1)]
void CSMain(in uint3 InDispatchThreadID : SV_DispatchThreadID)
{
	const uint MipSize = max(MipResolution, 1u);
	if ((InDispatchThreadID.x >= MipSize) || (InDispatchThreadID.y >= MipSize) || (InDispatchThreadID.z >= 6u))
	{
		return;
	}

	const float2 UV = (((float2(InDispatchThreadID.xy) + 0.5) * 2.0) / float(MipSize)) - 1.0;
	const float3 Normal = EnvironmentPrefilterFaceDirection(InDispatchThreadID.z, UV);

	// Perceptual roughness is linear in the mip index, which is the spacing the sampling side
	// assumes when it maps roughness to a level.
	const float PerceptualRoughness = ((MipCount > 1u)
		? (float(MipIndex) / float(MipCount - 1u)) : 0.0);
	const float Alpha = PerceptualRoughness * PerceptualRoughness;

	// Split sum prefilter: V = N, so the integral becomes a weighted average of the
	// environment around the normal. The weight is N dot L, which is also the Lambert weight
	// the reconstruction assumes.
	const float3 ViewDirection = Normal;
	const uint NumSamples = max(SampleCount, 1u);
	const float SourceResolution = max(SourceCubeResolution, 1u);
	const float SampleSolidAngle = 4.0 * PE_SHADER_PI / (6.0 * SourceResolution * SourceResolution);

	float3 RadianceSum = float3(0.0, 0.0, 0.0);
	float WeightSum = 0.0;
	for (uint SampleIndex = 0u; SampleIndex < NumSamples; SampleIndex++)
	{
		const float2 Xi = EnvironmentPrefilterHammersley(SampleIndex, NumSamples);
		const float3 HalfVector = EnvironmentPrefilterImportanceSampleGGX(Xi, Normal, Alpha);
		const float3 LightDirection = normalize((2.0 * dot(ViewDirection, HalfVector) * HalfVector) - ViewDirection);
		const float NdotL = saturate(dot(Normal, LightDirection));
		if (NdotL <= 0.0)
		{
			continue;
		}

		// Pick the source mip whose texel solid angle matches the sample's. For a source with
		// one mip this clamps to level zero; for a future HDR cube it is what keeps the high
		// roughness mips from aliasing.
		const float NdotH = saturate(dot(Normal, HalfVector));
		const float Distribution = EnvironmentPrefilterDistributionGGX(NdotH, Alpha);
		const float Pdf = (Distribution * NdotH / (4.0 * max(dot(ViewDirection, HalfVector), 1e-4))) + 1e-4;
		const float SamplePdfSolidAngle = 1.0 / ((float(NumSamples) * Pdf) + 1e-4);
		const float SourceMip = max(0.5 * log2(SamplePdfSolidAngle / SampleSolidAngle), 0.0);

		const float3 Radiance = PE_SAMPLE_TEXCUBE_LEVEL(EnvironmentCube, SamplerMaterialCube, LightDirection, SourceMip).rgb;
		RadianceSum += Radiance * NdotL;
		WeightSum += NdotL;
	}

	// Normalize the environment to unit mean: the tint and the mean luminance are applied at
	// sample time, not baked here.
	float3 Result = RadianceSum / max(WeightSum, 1e-4);
	Result /= max(CubeMeanLuminance, 0.0001);
	PrefilteredEnvironmentCube[InDispatchThreadID] = float4(Result, 1.0);
}
#endif	//ENVIRONMENT_PREFILTER_HLSL
