#ifndef _HZB_DOWN_SAMPLING_FUNCTIONS_HLSL
#define _HZB_DOWN_SAMPLING_FUNCTIONS_HLSL

#include "../../Common/ShaderCommon.hlsl"

float	GetSrcBufferValue(uint2 srcCoord, uint2 srcCoordMax);
void	SetDstBufferValue(uint2 dstCoord, float value);
float	CompareSrcValue(float v0, float v1, float v2, float v3);
uint2	ConvertCoordDstToSrc(uint2 coord, uint2 srcCoordMax, uint2 dstCoordMax);

void HZBDownSamplingByPowerOfTwo(uint2 dispatchThreadID, uint2 srcCoordMax, uint2 dstCoordMax)
{
	uint2 cacheCoord = ConvertCoordDstToSrc(dispatchThreadID, srcCoordMax, dstCoordMax);
	float value = GetSrcBufferValue(cacheCoord, srcCoordMax);
	float value1 = GetSrcBufferValue(cacheCoord + uint2(1, 0), srcCoordMax);
	float value2 = GetSrcBufferValue(cacheCoord + uint2(0, 1), srcCoordMax);
	float value3 = GetSrcBufferValue(cacheCoord + uint2(1, 1), srcCoordMax);
	value = CompareSrcValue(value, value1, value2, value3);
	SetDstBufferValue(dispatchThreadID, value);
}

#endif