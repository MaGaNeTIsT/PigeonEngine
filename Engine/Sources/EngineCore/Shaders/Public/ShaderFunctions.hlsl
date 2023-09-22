#ifndef _SHADER_FUNCTIONS_HLSL
#define _SHADER_FUNCTIONS_HLSL

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

#endif	//_SHADER_FUNCTIONS_HLSL