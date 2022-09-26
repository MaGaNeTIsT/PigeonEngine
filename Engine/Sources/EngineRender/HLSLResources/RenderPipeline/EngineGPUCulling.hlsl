#ifndef _ENGINE_GPU_CULLING_CS_HLSL
#define _ENGINE_GPU_CULLING_CS_HLSL

#include "../Common/ShaderCommon.hlsl"

#define GROUP_THREAD_SIZE_X		(64)
#define HIERARCHICAL_COUNT		(7)

struct AABBBoxInfo
{
	float3 Min;
	float3 Max;
	float2 Padding;
};

cbuffer ConstantBufferGPUCulling : register(b1)
{
	uint4 _Parameters;	// x is number of objects. y is used most detail layer of HZB. z is used HZB layers count. w is not used.
};

StructuredBuffer<AABBBoxInfo>	_CullingTestBuffer		: register(t0);
RWStructuredBuffer<uint>		_CullingResultBuffer	: register(u0);
Texture2D<float>	_HierarchicalZBuffer_0	: register(t1);
Texture2D<float>	_HierarchicalZBuffer_1	: register(t2);
Texture2D<float>	_HierarchicalZBuffer_2	: register(t3);
Texture2D<float>	_HierarchicalZBuffer_3	: register(t4);
Texture2D<float>	_HierarchicalZBuffer_4	: register(t5);
Texture2D<float>	_HierarchicalZBuffer_5	: register(t6);
Texture2D<float>	_HierarchicalZBuffer_6	: register(t7);

bool ZTestForAABB(float inputZ, uint2 min, uint2 max, float2 size, Texture2D<float> buffer)
{
	float testZ;
	[loop] for (uint y = min.y; y <= max.y; y++)
	{
		[loop] for (uint x = min.x; x <= max.x; x++)
		{
			testZ = buffer[uint2(x, y)];
			if (inputZ <= testZ)
			{
				return true;
			}
		}
	}
	return false;
}

[numthreads(GROUP_THREAD_SIZE_X, 1, 1)]
void main(uint dispatchThreadID :SV_DispatchThreadID, uint groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	if (dispatchThreadID.x >= _Parameters.x)
		return;

	/*
	//            5--------6(max)           Y(up vector)
	//           /|       /|                ^
	//          / |      / |  (dimension y) |
	//         4--------7  |                |    Z(forward vector)
	//         |  1-----|--2                |   /
	//         | /      | /                 |  /(dimension z)
	//         |/       |/                  | /
	//    (min)0--------3                   0---------------->X(right vector)
	//                                         (dimension x)
	*/

	float nearestZ;
	float2 minClamp, maxClamp;
	{
		float4 position[8];
		{
			AABBBoxInfo testInfo = _CullingTestBuffer[dispatchThreadID.x];
			position[0] = float4(testInfo.Min, 1.0);
			position[1] = float4(testInfo.Min.xy, testInfo.Max.z, 1.0);
			position[2] = float4(testInfo.Max.x, testInfo.Min.y, testInfo.Max.z, 1.0);
			position[3] = float4(testInfo.Max.x, testInfo.Min.yz, 1.0);
			position[4] = float4(testInfo.Min.x, testInfo.Max.y, testInfo.Min.z, 1.0);
			position[5] = float4(testInfo.Min.x, testInfo.Max.yz, 1.0);
			position[6] = float4(testInfo.Max, 1.0);
			position[7] = float4(testInfo.Max.xy, testInfo.Min.z, 1.0);
		}
		bool outScreen = true;
		[unroll(8)] for (uint indexPoint = 0u; indexPoint < 8u; indexPoint++)
		{
			position[indexPoint] = TransformWorldToClip(position[indexPoint].xyz);
			position[indexPoint] = float4(position[indexPoint].xyz / position[indexPoint].w, position[indexPoint].w);
			outScreen = outScreen && (any(position[indexPoint].xy < -1.0 || position[indexPoint].xy > 1.0) ? true : false);
			position[indexPoint].y = -position[indexPoint].y;
			position[indexPoint].xy = clamp((position[indexPoint].xy * 0.5) + 0.5, 0.0, 1.0);
		}
		if (outScreen)
		{
			_CullingResultBuffer[dispatchThreadID] = 0x2u;
			return;
		}
		nearestZ = position[0].z;
		minClamp = position[0].xy;
		maxClamp = position[0].xy;
		[unroll(7)] for (uint c = 1u; c < 8u; c++)
		{
			minClamp.x = min(position[c].x, minClamp.x);
			minClamp.y = min(position[c].y, minClamp.y);
			maxClamp.x = max(position[c].x, maxClamp.x);
			maxClamp.y = max(position[c].y, maxClamp.y);
			nearestZ = min(position[c].z, nearestZ);
		}
	}

	Texture2D<float> hzbBuffers[HIERARCHICAL_COUNT] = {
		_HierarchicalZBuffer_0,
		_HierarchicalZBuffer_1,
		_HierarchicalZBuffer_2,
		_HierarchicalZBuffer_3,
		_HierarchicalZBuffer_4,
		_HierarchicalZBuffer_5,
		_HierarchicalZBuffer_6 };
	int indexMax = int(_Parameters.z) - 1;
	uint2 minCoord[HIERARCHICAL_COUNT], maxCoord[HIERARCHICAL_COUNT], bufferMax[HIERARCHICAL_COUNT];
	{
		uint2 bufferSize[HIERARCHICAL_COUNT];
		[unroll(HIERARCHICAL_COUNT)] for (int bufferIndex = 0; bufferIndex < HIERARCHICAL_COUNT; bufferIndex++)
		{
			hzbBuffers[bufferIndex].GetDimensions(bufferSize[bufferIndex].x, bufferSize[bufferIndex].y);
			bufferSize[bufferIndex] = clamp(bufferSize[bufferIndex], 1u, 8192u);
		}
		int indexHZB;
		[unroll(HIERARCHICAL_COUNT)] for (int h = 0; h < HIERARCHICAL_COUNT; h++)
		{
			indexHZB = (h > indexMax) ? 0 : h;
			bufferMax[h] = max(1u, bufferSize[indexHZB].xy - 1u);
			minCoord[h] = uint2(bufferMax[h] * minClamp.xy);
			maxCoord[h] = uint2(bufferMax[h] * maxClamp.xy);
		}
	}

	float deviceZ;
	[unroll(HIERARCHICAL_COUNT)] for (int i = 0; i < HIERARCHICAL_COUNT; i++)
	{
		if (indexMax >= i)
		{
			if (!ZTestForAABB(nearestZ, minCoord[i], maxCoord[i], float2(bufferMax[i]), hzbBuffers[i]))
			{
				//0000 0000 0000 0000 0000 0000 0000 0011
				uint result = 0xfu & uint(i + 1);
				_CullingResultBuffer[dispatchThreadID] = result << 2;
				return;
			}
		}
	}
	_CullingResultBuffer[dispatchThreadID] = 0x1u;
}

#endif