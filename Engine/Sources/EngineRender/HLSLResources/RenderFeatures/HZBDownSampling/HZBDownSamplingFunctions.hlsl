#ifndef _HZB_DOWN_SAMPLING_FUNCTIONS_HLSL
#define _HZB_DOWN_SAMPLING_FUNCTIONS_HLSL

#include "../../Common/ShaderCommon.hlsl"

float	GetSrcBufferValue(uint2 srcCoord, uint2 srcSizeMax);
void	SetDstBufferValue(uint2 dstCoord, float value);
float	CompareSrcValue(float v0, float v1, float v2, float v3);


uint2 ConvertCoordDstToSrc(uint2 coord)
{
	return (coord * 2u);
}
void HZBDownSamplingByPowerOfTwo(uint2 dispatchThreadID, uint2 srcSizeMax)
{
	uint2 cacheCoord = ConvertCoordDstToSrc(dispatchThreadID);
	float value = GetSrcBufferValue(cacheCoord, srcSizeMax);
	float value1 = GetSrcBufferValue(cacheCoord + uint2(1, 0), srcSizeMax);
	float value2 = GetSrcBufferValue(cacheCoord + uint2(0, 1), srcSizeMax);
	float value3 = GetSrcBufferValue(cacheCoord + uint2(1, 1), srcSizeMax);
	value = CompareSrcValue(value, value1, value2, value3);
	SetDstBufferValue(dispatchThreadID, value);
}

#endif