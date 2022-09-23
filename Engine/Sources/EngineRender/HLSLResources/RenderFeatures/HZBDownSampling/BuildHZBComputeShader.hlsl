#ifndef _BUILD_HZB_CS_HLSL
#define _BUILD_HZB_CS_HLSL

#include "../../Common/ShaderCommon.hlsl"
#include "./HZBDownSamplingFunctions.hlsl"

#define GROUP_THREAD_SIZE_X		(8)
#define GROUP_THREAD_SIZE_Y		(8)



Texture2D<float> _SrcBuffer : register(t0);
RWTexture2D<float> _DstBuffer : register(u0);



float GetSrcBufferValue(uint2 srcCoord, uint2 srcCoordMax)
{
	srcCoord = min(srcCoordMax, srcCoord);
	return _SrcBuffer[srcCoord];
}
void SetDstBufferValue(uint2 dstCoord, float value)
{
	_DstBuffer[dstCoord] = value;
}
float CompareSrcValue(float v0, float v1, float v2, float v3)
{
	return max(max(max(v0, v1), v2), v3);
}
uint2 ConvertCoordDstToSrc(uint2 coord, uint2 srcCoordMax, uint2 dstCoordMax)
{
	float2 tempCoord = float2(coord) / float2(dstCoordMax);
	tempCoord = tempCoord * float2(srcCoordMax);
	return uint2(tempCoord);
}



[numthreads(GROUP_THREAD_SIZE_X, GROUP_THREAD_SIZE_Y, 1)]
void main(uint2 dispatchThreadID :SV_DispatchThreadID)
{
	uint2 dstBufferSize;
	_DstBuffer.GetDimensions(dstBufferSize.x, dstBufferSize.y);
	if (any(dispatchThreadID >= dstBufferSize))
		return;
	uint2 srcBufferSize;
	_SrcBuffer.GetDimensions(srcBufferSize.x, srcBufferSize.y);
	HZBDownSamplingByPowerOfTwo(dispatchThreadID, (srcBufferSize - 1u), (dstBufferSize - 1u));
}

#endif