#ifndef _DYNAMIC_WIND_CS_HLSL
#define _DYNAMIC_WIND_CS_HLSL

#include "../../Common/ShaderCommon.hlsl"

#define GROUP_THREAD_SIZE_X		(4)
#define GROUP_THREAD_SIZE_Y		(4)
#define GROUP_THREAD_SIZE_Z		(4)

cbuffer	ConstantComputeBuffer : register(b3)
{
	uint4 _DynamicWindParams;
};
Texture2D<float4> _Input : register(t5);
RWTexture2D<float4> _ResultBuffer : register(u0);

[numthreads(GROUP_THREAD_SIZE_X, GROUP_THREAD_SIZE_Y, GROUP_THREAD_SIZE_Z)]
void main(uint3 dispatchThreadID :SV_DispatchThreadID)
{
	if (any(dispatchThreadID >= _DynamicWindParams.xyz))
		return;

}

#endif