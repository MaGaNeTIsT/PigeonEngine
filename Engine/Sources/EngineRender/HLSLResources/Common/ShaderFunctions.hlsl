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

#endif