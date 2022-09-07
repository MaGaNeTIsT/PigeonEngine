#ifndef _GTAO_SPATIAL_FILTER_HLSL
#define _GTAO_SPATIAL_FILTER_HLSL

#include "../Common/ShaderCommon.hlsl"

#define GTAO_NUMTAPS							(8)
#define GROUP_THREAD_SIZE_X						(8)
#define GROUP_THREAD_SIZE_Y						(8)
#define GTAO_MAX_PIXEL_SCREEN_RADIUS			(256.0)

const static int2 GROUP_THREAD_OFFSET			= int2(2, 2);
const static uint GROUP_THREAD_COLUMN_OFFSET	= GROUP_THREAD_SIZE_X + 4;
const static uint GROUP_THREAD_MAX				= (GROUP_THREAD_SIZE_X + 4) * (GROUP_THREAD_SIZE_Y + 4);
const static uint MAX_THREAD_GROUP_SIZE			= GROUP_THREAD_SIZE_X * GROUP_THREAD_SIZE_Y;

#define GROUP_SHARED_ARRAY_SIZE					(MAX_THREAD_GROUP_SIZE * 3u)


cbuffer	ConstantComputeBuffer : register(b1)
{
	float4 _ResultBufferParams;
	float4 _DepthBufferParams;
	float4 _FadeAttenParams;
	float4 _AdjustAngleThicknessParams;
	float4 _IntensityPowerParams;		// x is ambient occlusion adjust intensity. y is ambient occlusion adjust power. z w is not used.
};
Texture2D<float4> _GTAOInput : register(t0);
RWTexture2D<float> _ResultBuffer : register(u0);
groupshared float _DeviceZBuffer[GROUP_SHARED_ARRAY_SIZE];
groupshared float _AOBuffer[GROUP_SHARED_ARRAY_SIZE];



float4 EncodeFloatRGBA(float v)
{
	float4 enc = float4(1.0, 255.0, 65025.0, 16581375.0) * v;
	float4 encValue = frac(enc);
	encValue -= encValue.yzww * float4(0.0039215686275, 0.0039215686275, 0.0039215686275, 0.0);
	return encValue;
}
float DecodeFloatRGBA(float4 rgba)
{
	return dot(rgba, float4(1.0, 0.0039215686275, 1.53787e-5, 6.03086294e-8));
}
void SetDeviceZ(float deviceZ, uint index)
{
	_DeviceZBuffer[index] = deviceZ;
}
float GetCachedDeviceZFromSharedMemory(int2 coord)
{
	return _DeviceZBuffer[coord.x + (coord.y * GROUP_THREAD_COLUMN_OFFSET)];
}
float GetCachedViewZFromSharedMemory(int2 coord)
{
	return ConvertDeviceZToViewZ(GetCachedDeviceZFromSharedMemory(coord));
}
void SetAOBuffer(float ao, uint index)
{
	_AOBuffer[index] = ao;
}
float GetCachedAOBufferFromSharedMemory(int2 coord)
{
	return _AOBuffer[coord.x + (coord.y * GROUP_THREAD_COLUMN_OFFSET)];
}
float2 BufferPositionToUV(int2 coord)
{
	return ((coord + 0.5) * _ResultBufferParams.zw);
}
float2 GetAOAndDeviceZFromInput(float2 uv)
{
	float2 deviceZAndAO;
	float4 encodeDeviceZAndAO = _GTAOInput.SampleLevel(_LinearClampSampler, uv, 0).rgba;
	deviceZAndAO.y = encodeDeviceZAndAO.a;
	encodeDeviceZAndAO.a = 0.0;
	deviceZAndAO.x = DecodeFloatRGBA(encodeDeviceZAndAO);
	return deviceZAndAO;
}
void CacheAOAndDeviceZ(uint index, int2 groupOrigin)
{
	int2 coord = groupOrigin + int2(index % GROUP_THREAD_COLUMN_OFFSET, index / GROUP_THREAD_COLUMN_OFFSET);
	float2 uv = BufferPositionToUV(coord);
	float2 deviceZAndAO = GetAOAndDeviceZFromInput(uv);
	SetDeviceZ(deviceZAndAO.x, index);
	SetAOBuffer(deviceZAndAO.y, index);
}



float GTAOSpatialFilter(int2 groupThreadID, int2 dispatchThreadID)
{
	float2 zDiff;

	// Get the zDiffs array.
	int2 groupThreadOffsetPos = groupThreadID + GROUP_THREAD_OFFSET;
	float thisZ = GetCachedDeviceZFromSharedMemory(groupThreadOffsetPos);

	{
		int2 x2Offset = int2(2, 0);
		int2 x1Offset = int2(1, 0);

		float xN2Z = GetCachedDeviceZFromSharedMemory(groupThreadOffsetPos - x2Offset);
		float xN1Z = GetCachedDeviceZFromSharedMemory(groupThreadOffsetPos - x1Offset);
		float xP1Z = GetCachedDeviceZFromSharedMemory(groupThreadOffsetPos + x1Offset);
		float xP2Z = GetCachedDeviceZFromSharedMemory(groupThreadOffsetPos + x2Offset);

		// Get extrapolated point either side.
		float c1 = abs((xN1Z + (xN1Z - xN2Z)) - thisZ);
		float c2 = abs((xP1Z + (xP1Z - xP2Z)) - thisZ);

		zDiff.x = c1 < c2 ? (xN1Z - xN2Z) : (xP2Z - xP1Z);
	}

	{
		int2 y2Offset = int2(0, 2);
		int2 y1Offset = int2(0, 1);

		float yN2Z = GetCachedDeviceZFromSharedMemory(groupThreadOffsetPos - y2Offset);
		float yN1Z = GetCachedDeviceZFromSharedMemory(groupThreadOffsetPos - y1Offset);
		float yP1Z = GetCachedDeviceZFromSharedMemory(groupThreadOffsetPos + y1Offset);
		float yP2Z = GetCachedDeviceZFromSharedMemory(groupThreadOffsetPos + y2Offset);

		// Get extrapolated point either side.
		float c1 = abs((yN1Z + (yN1Z - yN2Z)) - thisZ);
		float c2 = abs((yP1Z + (yP1Z - yP2Z)) - thisZ);

		zDiff.y = c1 < c2 ? (yN1Z - yN2Z) : (yP2Z - yP1Z);
	}

	float sumAO = 0.0;
	float sumWeight = 0.0;

	int x, y;

	// Get the Z Value to compare against.
	float depthBase = thisZ - (zDiff.x * 2) - (zDiff.y * 2);

	for (y = -2; y <= 2; y++)
	{
		float planeZ = depthBase;

		for (x = -2; x <= 2; x++)
		{
			// Get value and see how much it compares to the centre with the gradients.
			//float xDiff = (float)abs(x);

			int2 bufferPos = groupThreadOffsetPos + int2(x, y);
			float2 bufferedZAndAO;
			bufferedZAndAO.x = GetCachedDeviceZFromSharedMemory(bufferPos);
			bufferedZAndAO.y = GetCachedAOBufferFromSharedMemory(bufferPos);

			float weight = 1.0;
			// 			if ((x == 0) && (y == 0)) //Need do profile to see whether disble branch is more efficent.
			// 			{
			// 				weight = 1.0;
			// 			}
			// 			else
			{
				// Get the bilateral weight. This is a function of the difference in height between the plane equation and the base depth
				// Compare the Z at this sample with the gradients.
				float bufferedZDiff = abs(planeZ - bufferedZAndAO.x);

				weight = 1.0 - saturate(bufferedZDiff * 1000.0);
			}

			sumAO += bufferedZAndAO.y * weight;
			sumWeight += weight;

			planeZ += zDiff.x;
		}
		depthBase += zDiff.y;
	}
	sumAO /= sumWeight;

	sumAO *= (CUSTOM_SHADER_PI_HALF);

	// user adjust AO.
	float intensityAO = _IntensityPowerParams.x;
	float powerAO = _IntensityPowerParams.y;
	sumAO = 1.0 - (1.0 - pow(abs(sumAO), powerAO)) * intensityAO;

	return sumAO;
}



[numthreads(GROUP_THREAD_SIZE_X, GROUP_THREAD_SIZE_Y, 1)]
void main(uint2 dispatchThreadID :SV_DispatchThreadID, uint2 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex, uint2 groupThreadID : SV_GroupThreadID)
{
	{
		int2 groupOrigin = int2(groupID.x * GROUP_THREAD_SIZE_X, groupID.y * GROUP_THREAD_SIZE_Y) - GROUP_THREAD_OFFSET;
		uint index = groupIndex * 3u;
		if (index < GROUP_THREAD_MAX)
		{
			CacheAOAndDeviceZ(index, groupOrigin);
			CacheAOAndDeviceZ(index + 1u, groupOrigin);
			CacheAOAndDeviceZ(index + 2u, groupOrigin);
		}
		GroupMemoryBarrierWithGroupSync();
	}
	if (any(dispatchThreadID >= uint2(_ResultBufferParams.xy)))
		return;

	int2 dispatchThreadPos = int2(dispatchThreadID);
	float ao = GTAOSpatialFilter(int2(groupThreadID), dispatchThreadPos);
	_ResultBuffer[dispatchThreadPos + _CameraViewportRect.xy] = ao;
}

#endif