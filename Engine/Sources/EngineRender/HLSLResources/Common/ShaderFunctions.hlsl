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

float Power2(float v)
{
	return (v * v);
}

float Power5(float v)
{
	float v2 = v * v;
	return (v2 * v2 * v);
}

#endif