#ifndef ENVIRONMENT_BRDF_HLSL
#define ENVIRONMENT_BRDF_HLSL

// ---------------------------------------------------------------------------
// Environment BRDF (DFG) lookup table: the BRDF half of the split sum.
//
// Each texel integrates the split sum's BRDF term for one (N dot V, perceptual
// roughness) pair and stores (scale, bias), so the sampling side evaluates
//
//   F0 * scale + bias
//
// with one texture fetch. This is the same integral the analytic Karis fit
// approximated; the table removes the fit error and is the general form.
//
// The table is independent of the environment, so it is generated once and then
// only sampled.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"

#ifndef ENVIRONMENT_BRDF_THREAD_GROUP_SIZE
#define ENVIRONMENT_BRDF_THREAD_GROUP_SIZE	(8)
#endif	//ENVIRONMENT_BRDF_THREAD_GROUP_SIZE

float2 EnvironmentBRDFHammersley(in uint InIndex, in uint InCount)
{
	uint Bits = InIndex;
	Bits = (Bits << 16u) | (Bits >> 16u);
	Bits = ((Bits & 0x55555555u) << 1u) | ((Bits & 0xAAAAAAAAu) >> 1u);
	Bits = ((Bits & 0x33333333u) << 2u) | ((Bits & 0xCCCCCCCCu) >> 2u);
	Bits = ((Bits & 0x0F0F0F0Fu) << 4u) | ((Bits & 0xF0F0F0F0u) >> 4u);
	Bits = ((Bits & 0x00FF00FFu) << 8u) | ((Bits & 0xFF00FF00u) >> 8u);
	const float RadicalInverse = float(Bits) * 2.3283064365386963e-10f;
	return float2(float(InIndex) / float(InCount), RadicalInverse);
}

float3 EnvironmentBRDFImportanceSampleGGX(in float2 InXi, in float3 InNormal, in float InAlpha)
{
	const float AlphaSquared = InAlpha * InAlpha;
	const float Phi = 2.0 * PE_SHADER_PI * InXi.x;
	const float CosTheta = sqrt((1.0 - InXi.y) / (1.0 + ((AlphaSquared - 1.0) * InXi.y)));
	const float SinTheta = sqrt(saturate(1.0 - (CosTheta * CosTheta)));

	const float3 HalfVectorTangent = float3(cos(Phi) * SinTheta, sin(Phi) * SinTheta, CosTheta);
	const float3 Up = (abs(InNormal.z) < 0.999f) ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	const float3 Tangent = normalize(cross(Up, InNormal));
	const float3 Bitangent = cross(InNormal, Tangent);
	return normalize((Tangent * HalfVectorTangent.x) + (Bitangent * HalfVectorTangent.y) + (InNormal * HalfVectorTangent.z));
}

// Schlick-GGX with the IBL remapping k = alpha / 2, which is the geometry term the split sum
// fit was derived against.
float EnvironmentBRDFGeometrySchlickGGX(in float InNdotV, in float InAlpha)
{
	const float K = InAlpha * 0.5f;
	return InNdotV / ((InNdotV * (1.0 - K)) + K);
}

float EnvironmentBRDFGeometrySmith(in float InNdotV, in float InNdotL, in float InAlpha)
{
	return EnvironmentBRDFGeometrySchlickGGX(InNdotV, InAlpha)
		* EnvironmentBRDFGeometrySchlickGGX(InNdotL, InAlpha);
}

float2 EnvironmentBRDFIntegrate(in float InNdotV, in float InPerceptualRoughness)
{
	const float3 ViewDirection = float3(sqrt(saturate(1.0 - (InNdotV * InNdotV))), 0.0, InNdotV);
	const float Alpha = InPerceptualRoughness * InPerceptualRoughness;
	const uint NumSamples = max(SampleCount, 1u);

	float Scale = 0.0;
	float Bias = 0.0;
	for (uint SampleIndex = 0u; SampleIndex < NumSamples; SampleIndex++)
	{
		const float2 Xi = EnvironmentBRDFHammersley(SampleIndex, NumSamples);
		const float3 HalfVector = EnvironmentBRDFImportanceSampleGGX(Xi, float3(0.0, 0.0, 1.0), Alpha);
		const float3 LightDirection = normalize((2.0 * dot(ViewDirection, HalfVector) * HalfVector) - ViewDirection);

		const float NdotL = saturate(LightDirection.z);
		const float NdotH = saturate(HalfVector.z);
		const float VdotH = saturate(dot(ViewDirection, HalfVector));
		if (NdotL <= 0.0)
		{
			continue;
		}

		const float Geometry = EnvironmentBRDFGeometrySmith(InNdotV, NdotL, Alpha);
		const float Visibility = (Geometry * VdotH) / max(NdotH * InNdotV, 1e-4);
		const float FresnelReflectance = pow(1.0 - VdotH, 5.0);
		Scale += (1.0 - FresnelReflectance) * Visibility;
		Bias += FresnelReflectance * Visibility;
	}

	return float2(Scale, Bias) / float(NumSamples);
}

[numthreads(ENVIRONMENT_BRDF_THREAD_GROUP_SIZE, ENVIRONMENT_BRDF_THREAD_GROUP_SIZE, 1)]
void CSMain(in uint3 InDispatchThreadID : SV_DispatchThreadID)
{
	const uint TableSize = max(Resolution, 1u);
	if ((InDispatchThreadID.x >= TableSize) || (InDispatchThreadID.y >= TableSize))
	{
		return;
	}

	const float2 UV = (float2(InDispatchThreadID.xy) + 0.5) / float(TableSize);
	// The x axis is N dot V, the y axis perceptual roughness. Clamping NdotV away from zero
	// keeps the division in the visibility term finite at grazing angles.
	const float NdotV = max(UV.x, 1e-4);
	const float PerceptualRoughness = saturate(UV.y);
	EnvironmentBRDFLUT[InDispatchThreadID.xy] = EnvironmentBRDFIntegrate(NdotV, PerceptualRoughness);
}
#endif	//ENVIRONMENT_BRDF_HLSL
