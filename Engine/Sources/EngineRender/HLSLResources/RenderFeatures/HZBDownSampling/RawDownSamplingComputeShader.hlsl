#ifndef _RAW_DOWN_SAMPLING_CS_HLSL
#define _RAW_DOWN_SAMPLING_CS_HLSL

#include "../../Common/ShaderCommon.hlsl"
#include "./HZBDownSamplingFunctions.hlsl"

#define GROUP_THREAD_SIZE_X		(8)
#define GROUP_THREAD_SIZE_Y		(8)

Texture2D				_SrcBuffer : register(t0);
RWTexture2D<float>		_DstBuffer : register(u0);

float GetCompareInitValue()
{
	return 0.0;
}
float CompareValue(float v0, float v1)
{
	return max(v0, v1);
}
float GetSrcBufferValue(float2 uv)
{
	return _SrcBuffer.SampleLevel(_PointClampSampler, uv, 0).r;
}
void SetDstBufferValue(uint2 coord, float value)
{
	_DstBuffer[coord] = value;
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
	RawDownSample(dispatchThreadID, srcBufferSize, dstBufferSize);
}

#endif