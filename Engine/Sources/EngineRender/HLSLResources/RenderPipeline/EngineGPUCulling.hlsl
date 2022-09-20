#ifndef _ENGINE_GPU_CULLING_CS_HLSL
#define _ENGINE_GPU_CULLING_CS_HLSL

#include "../Common/ShaderCommon.hlsl"

#define GROUP_THREAD_SIZE_X		(8)
#define GROUP_THREAD_SIZE_Y		(8)

struct AABBBoxInfo
{
	float3 Anchor;
	float3 Dimensions;
	float2 Padding;
};

cbuffer ConstantBufferGPUCulling : register(b1)
{

};

StructuredBuffer<AABBBoxInfo> _CullingTestBuffer : register(t0);

[numthreads(GROUP_THREAD_SIZE_X, GROUP_THREAD_SIZE_Y, 1)]
void main(uint2 dispatchThreadID :SV_DispatchThreadID, uint2 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex, uint2 groupThreadID : SV_GroupThreadID)
{

}

#endif