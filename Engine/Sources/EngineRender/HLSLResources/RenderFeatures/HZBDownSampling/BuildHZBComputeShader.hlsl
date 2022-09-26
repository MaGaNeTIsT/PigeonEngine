#ifndef _BUILD_HZB_CS_HLSL
#define _BUILD_HZB_CS_HLSL

#include "../../Common/ShaderCommon.hlsl"
#include "./HZBDownSamplingFunctions.hlsl"

#define GROUP_THREAD_SIZE_X		(8)
#define GROUP_THREAD_SIZE_Y		(8)

Texture2D<float> _SrcBuffer : register(t0);
RWTexture2D<float> _DstBuffer : register(u0);

float GetSrcHZBValue(uint2 coord)
{
	return _SrcBuffer.Load(int3(coord.xy, 0));
}
float CompareHZBValue(float v0, float v1)
{
	return max(v0, v1);
}
void SetDstHZBValue(uint2 coord, float v)
{
	_DstBuffer[coord] = v;
}

[numthreads(GROUP_THREAD_SIZE_X, GROUP_THREAD_SIZE_Y, 1)]
void main(uint2 dispatchThreadID :SV_DispatchThreadID)
{
	uint2 dstBufferSize;
	_DstBuffer.GetDimensions(dstBufferSize.x, dstBufferSize.y);
	if (any(dispatchThreadID >= dstBufferSize))
		return;
	HZBDownSamplingByPowerOfTwo(dispatchThreadID);
}

#endif