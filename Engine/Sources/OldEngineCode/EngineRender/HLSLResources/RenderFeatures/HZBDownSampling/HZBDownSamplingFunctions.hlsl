#ifndef _HZB_DOWN_SAMPLING_FUNCTIONS_HLSL
#define _HZB_DOWN_SAMPLING_FUNCTIONS_HLSL

#include "../../Common/ShaderCommon.hlsl"

float GetCompareInitValue();
float CompareValue(float v0, float v1);
float GetSrcBufferValue(float2 uv);
void SetDstBufferValue(uint2 coord, float value);
void RawDownSample(uint2 dispatchThreadID, uint2 srcBufferSize, uint2 dstBufferSize)
{
	float2 uv = (dispatchThreadID + 0.5) / dstBufferSize;
	float2 offset = 1.0 / srcBufferSize;
	float compareZ = GetCompareInitValue();
	[unroll] for (int y = -1; y <= 1; y++)
	{
		[unroll] for (int x = -1; x <= 1; x++)
		{
			compareZ = CompareValue(GetSrcBufferValue(uv + (float2(x, y) * offset)), compareZ);
		}
	}
	SetDstBufferValue(dispatchThreadID, compareZ);
}
float GetSrcHZBValue(uint2 coord);
float CompareHZBValue(float v0, float v1);
void SetDstHZBValue(uint2 coord, float v);
void HZBDownSamplingByPowerOfTwo(uint2 dispatchThreadID)
{
	uint2 coord = dispatchThreadID * 2u;
	float value = GetSrcHZBValue(coord);
	value = CompareHZBValue(GetSrcHZBValue(coord + uint2(1, 0)), value);
	value = CompareHZBValue(GetSrcHZBValue(coord + uint2(0, 1)), value);
	value = CompareHZBValue(GetSrcHZBValue(coord + uint2(1, 1)), value);
	SetDstHZBValue(dispatchThreadID, value);
}

#endif