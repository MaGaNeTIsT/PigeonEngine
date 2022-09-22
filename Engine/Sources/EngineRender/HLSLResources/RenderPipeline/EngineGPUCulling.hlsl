#ifndef _ENGINE_GPU_CULLING_CS_HLSL
#define _ENGINE_GPU_CULLING_CS_HLSL

#include "../Common/ShaderCommon.hlsl"

#define GROUP_THREAD_SIZE_X		(64)

struct AABBBoxInfo
{
	float3 Min;
	float3 Max;
	float2 Padding;
};

cbuffer ConstantBufferGPUCulling : register(b1)
{
	uint4 _Parameters; // x is number of objects. yzw is not used.
};

StructuredBuffer<AABBBoxInfo>	_CullingTestBuffer		: register(t0);
RWStructuredBuffer<uint>		_CullingResultBuffer	: register(u0);

[numthreads(GROUP_THREAD_SIZE_X, 1, 1)]
void main(uint dispatchThreadID :SV_DispatchThreadID, uint groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	if (dispatchThreadID.x > _Parameters.x)
		return;
	_CullingResultBuffer[dispatchThreadID] = 1;
}

#endif