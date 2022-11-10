#ifndef _SHADER_FUNCTIONS_HLSL
#define _SHADER_FUNCTIONS_HLSL

float3 SafeNormalize(const float3 input)
{
	float3 output = normalize(input);
	bool i = any(isinf(output));
	output = i ? float3(0.0, 0.0, 1.0) : output;
	return output;
}

float TakeSmallerAbsDelta(float left, float mid, float right)
{
	float a = mid - left;
	float b = right - mid;

	return (abs(a) < abs(b)) ? a : b;
}

GeometryViewLightDotTerm InitGeometryViewLightDotTerm(const float3 normal, const float3 lightDir, const float3 viewDir)
{
	float3 halfVec = SafeNormalize(lightDir + viewDir);
	GeometryViewLightDotTerm output;
	output.NdotV = abs(dot(normal, viewDir)) + 1e-5;
	output.NdotL = saturate(dot(normal, lightDir));
	output.NdotH = saturate(dot(normal, halfVec));
	output.LdotH = saturate(dot(lightDir, halfVec));
}

float3 RemappingBaseColor(float3 albedo, float metallic)
{
	return (albedo * (1.0 - metallic));
}

float RemappingRoughness(float perceptualRoughness)
{
#if 0
	return clamp(perceptualRoughness * perceptualRoughness, 0.089, 1.0);
#endif

#if 1
	return clamp(perceptualRoughness * perceptualRoughness, 0.045, 1.0);
#endif
}

float RemappingFresnelf0DielectricsFast(float reflectance)
{
	return (0.16 * reflectance * reflectance);
}

float3 RemappingFresnelf0Conductors(float3 albedo, float metallic)
{
	return (albedo * metallic);
}

float3 RemappingFresnelf0(float reflectance, float3 albedo, float metallic)
{
	return (0.16 * reflectance * reflectance * (1.0 - metallic) + albedo * metallic);
}

#endif