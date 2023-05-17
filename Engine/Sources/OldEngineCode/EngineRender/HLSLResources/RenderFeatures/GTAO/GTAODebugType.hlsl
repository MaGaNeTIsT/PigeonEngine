#ifndef _GTAO_DEBUG_TYPE_HLSL
#define _GTAO_DEBUG_TYPE_HLSL

#include "../../Common/ShaderCommon.hlsl"

#define GROUP_THREAD_SIZE_X		(8)
#define GROUP_THREAD_SIZE_Y		(8)

cbuffer	ConstantComputeBuffer : register(b3)
{
	float4 _ResultBufferParams;
	float4 _DepthBufferParams;
	float4 _FadeAttenParams;
	float4 _AdjustAngleThicknessParams;
	float4 _IntensityPowerParams;
};
Texture2D<float4> _GTAOIntegralInput : register(t5);
Texture2D<float> _GTAOFilterInput : register(t6);
RWTexture2D<float4> _ResultBuffer : register(u0);

[numthreads(GROUP_THREAD_SIZE_X, GROUP_THREAD_SIZE_Y, 1)]
void main(uint2 dispatchThreadID :SV_DispatchThreadID)
{
	if (any(dispatchThreadID >= uint2(_ResultBufferParams.xy)))
		return;
	float2 uv = (dispatchThreadID + 0.5) * _ResultBufferParams.zw;
	float4 integralInput = _GTAOIntegralInput.SampleLevel(_LinearClampSampler, uv, 0).rgba;
	float filterInput = _GTAOFilterInput.SampleLevel(_LinearClampSampler, uv, 0).r;
	uint debugType = (uint)_IntensityPowerParams.z;
	float4 result = 0.0;
	result = debugType == 1 ? float4(integralInput.xyz, 1.0) : result;
	result = debugType == 2 ? float4(integralInput.www, 1.0) : result;
	result = debugType == 3 ? float4(filterInput.xxx, 1.0) : result;
	_ResultBuffer[dispatchThreadID] = result;
}

#endif