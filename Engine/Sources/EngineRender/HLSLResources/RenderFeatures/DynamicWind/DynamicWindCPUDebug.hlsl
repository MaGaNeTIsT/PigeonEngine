#ifndef _DYNAMIC_WIND_CPU_DEBUG_CS_HLSL
#define _DYNAMIC_WIND_CPU_DEBUG_CS_HLSL

#include "./DynamicWindCommon.hlsl"

cbuffer	ConstantComputeBuffer : register(b3)
{
	uint4 _DynamicCPUWindParams;
};
Texture2D _InputCPUWindTexture0 : register(t5);
Texture2D _InputCPUWindTexture1 : register(t6);
RWTexture2D<float4> _ResultTexture : register(u0);

[numthreads(GROUP_THREAD_SIZE_X, GROUP_THREAD_SIZE_Y, 1)]
void main(uint3 dispatchThreadID :SV_DispatchThreadID)
{
	uint2 texSize;
	_ResultTexture.GetDimensions(texSize.x, texSize.y);

	if (any(dispatchThreadID.xy >= texSize.xy))
	{
		return;
	}

	float2 uv = (dispatchThreadID.xy + 0.5) / texSize.xy;
	float4 result = float4(0.0.xxx, 1.0);

	if (_DynamicCPUWindParams.x == 0u)
	{
		result.rgb = _InputCPUWindTexture0.SampleLevel(_LinearClampSampler, uv, 0.0).rrr;
	}
	else if (_DynamicCPUWindParams.x == 1u)
	{
		result.r = _InputCPUWindTexture0.SampleLevel(_LinearClampSampler, uv, 0.0).r;
		result.g = _InputCPUWindTexture1.SampleLevel(_LinearClampSampler, uv, 0.0).r;
		float len = length(result.rg);
		float2 dir = result.rg / len;
		dir = (dir * 0.5 + 0.5);
		result.rg = len < 1e-5 ? 0.0.xx : dir * len;
	}

	_ResultTexture[dispatchThreadID.xy] = result;
}

#endif