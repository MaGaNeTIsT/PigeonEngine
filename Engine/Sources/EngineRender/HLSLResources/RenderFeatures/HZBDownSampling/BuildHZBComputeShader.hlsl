#ifndef _BUILD_HZB_CS_HLSL
#define _BUILD_HZB_CS_HLSL

#include "../../Common/ShaderCommon.hlsl"
#include "./HZBDownSamplingFunctions.hlsl"

#define GROUP_THREAD_SIZE_X		(8)
#define GROUP_THREAD_SIZE_Y		(8)



Texture2D<float> _SrcBuffer : register(t0);
RWTexture2D<float> _DstBuffer : register(u0);



float GetSrcBufferValue(uint2 srcCoord, uint2 srcSizeMax)
{
	srcCoord = min(srcSizeMax, srcCoord);
	return _SrcBuffer[srcCoord];
}
void SetDstBufferValue(uint2 dstCoord, float value)
{
	_DstBuffer[dstCoord] = value;
}
float CompareSrcValue(float v0, float v1, float v2, float v3)
{
	return min(min(min(v0, v1), v2), v3);
}



[numthreads(GROUP_THREAD_SIZE_X, GROUP_THREAD_SIZE_Y, 1)]
void main(uint2 dispatchThreadID :SV_DispatchThreadID)
{
	uint2 bufferSize;
	_DstBuffer.GetDimensions(bufferSize.x, bufferSize.y);
	if (any(dispatchThreadID >= bufferSize))
		return;
	_SrcBuffer.GetDimensions(bufferSize.x, bufferSize.y);
	HZBDownSamplingByPowerOfTwo(dispatchThreadID, (bufferSize - 1u));
}

#endif