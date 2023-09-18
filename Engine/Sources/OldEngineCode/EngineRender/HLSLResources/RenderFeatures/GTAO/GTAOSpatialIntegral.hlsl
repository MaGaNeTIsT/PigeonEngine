//#ifndef _GTAO_SPATIAL_INTEGRAL_CS_HLSL
//#define _GTAO_SPATIAL_INTEGRAL_CS_HLSL
//
//#include "../../Common/ShaderCommon.hlsl"
//
//#define GTAO_NUMTAPS							(8)
//#define GROUP_THREAD_SIZE_X						(8)
//#define GROUP_THREAD_SIZE_Y						(8)
//#define GTAO_MAX_PIXEL_SCREEN_RADIUS			(256.0)
//
//const static int2 GROUP_THREAD_OFFSET			= int2(1, 1);
//const static uint GROUP_THREAD_COLUMN_OFFSET	= GROUP_THREAD_SIZE_X + 2;
//const static uint GROUP_THREAD_MAX				= (GROUP_THREAD_SIZE_X + 2) * (GROUP_THREAD_SIZE_Y + 2);
//const static uint MAX_THREAD_GROUP_SIZE			= GROUP_THREAD_SIZE_X * GROUP_THREAD_SIZE_Y;
//
//#define GROUP_SHARED_ARRAY_SIZE					(MAX_THREAD_GROUP_SIZE * 2u)
//
//
//cbuffer	ConstantComputeBuffer : register(b3)
//{
//	float4 _ResultBufferParams;
//	float4 _DepthBufferParams;
//	float4 _FadeAttenParams;			// x y is fade radius mul add coefficient. z is fade distance. w is falloff's attenuation factory.
//	float4 _AdjustAngleThicknessParams;	// x is world space radius adjust. y is sin rotatation delta angle. z is cos rotatation delta angle. w is thickness blend factory.
//	float4 _IntensityPowerParams;
//};
//Texture2D<float> _CameraDepth : register(t5);
//RWTexture2D<float4> _ResultBuffer : register(u0);
//groupshared float _DeviceZBuffer[GROUP_SHARED_ARRAY_SIZE];
//
//
//
//float4 EncodeFloatRGBA(float v)
//{
//	float4 enc = float4(1.0, 255.0, 65025.0, 16581375.0) * v;
//	float4 encValue = frac(enc);
//	encValue -= encValue.yzww * float4(0.0039215686275, 0.0039215686275, 0.0039215686275, 0.0);
//	return encValue;
//}
//float DecodeFloatRGBA(float4 rgba)
//{
//	return dot(rgba, float4(1.0, 0.0039215686275, 1.53787e-5, 6.03086294e-8));
//}
//void SetDeviceZ(float deviceZ, uint index)
//{
//	_DeviceZBuffer[index] = deviceZ;
//}
//float GetCachedDeviceZFromSharedMemory(int2 coord)
//{
//	return _DeviceZBuffer[coord.x + (coord.y * GROUP_THREAD_COLUMN_OFFSET)];
//}
//float GetCachedViewZFromSharedMemory(int2 coord)
//{
//	return ConvertDeviceZToViewZ(GetCachedDeviceZFromSharedMemory(coord));
//}
//float2 BufferPositionToUV(int2 coord)
//{
//	return ((coord + 0.5) * _ResultBufferParams.zw);
//}
//float GetDeviceZFromInput(float2 uv)
//{
//	return _CameraDepth.SampleLevel(_LinearClampSampler, uv, 0).r;
//}
//float GetViewZFromInput(float2 uv)
//{
//	return ConvertDeviceZToViewZ(GetDeviceZFromInput(uv));
//}
//void CacheDeviceZ(uint index, int2 groupOrigin)
//{
//	int2 coord = groupOrigin + int2(index % GROUP_THREAD_COLUMN_OFFSET, index / GROUP_THREAD_COLUMN_OFFSET);
//	float2 uv = BufferPositionToUV(coord);
//	uv += _DepthBufferParams.zw * 0.125;	
//	SetDeviceZ(GetDeviceZFromInput(uv), index);
//}
//
//
//
//float3 GetNormalVS(float2 uv, int2 groupThreadID, float3 posMidVS)
//{
//	int2 offsetX = int2(1, 0);
//	int2 offsetY = int2(0, 1);
//	int2 coord = groupThreadID + GROUP_THREAD_OFFSET;
//
//	float deviceZMid = GetCachedDeviceZFromSharedMemory(coord);
//	float deviceZLeft = GetCachedDeviceZFromSharedMemory(coord - offsetX);
//	float deviceZTop = GetCachedDeviceZFromSharedMemory(coord - offsetY);
//	float deviceZRight = GetCachedDeviceZFromSharedMemory(coord + offsetX);
//	float deviceZBottom = GetCachedDeviceZFromSharedMemory(coord + offsetY);
//
//	float deviceZDdx = TakeSmallerAbsDelta(deviceZLeft, deviceZMid, deviceZRight);
//	float deviceZDdy = TakeSmallerAbsDelta(deviceZTop, deviceZMid, deviceZBottom);
//
//	float zRight = ConvertDeviceZToViewZ(deviceZMid + deviceZDdx);
//	float zDown = ConvertDeviceZToViewZ(deviceZMid + deviceZDdy);
//
//	float3 right = TransformScreenToView(uv + float2(_ResultBufferParams.z, 0.0), zRight) - posMidVS;
//	float3 down = TransformScreenToView(uv + float2(0.0, _ResultBufferParams.w), zDown) - posMidVS;
//
//	return SafeNormalize(cross(right, down));
//}
//float2 SearchForLargestAngleDual(float2 baseUV, float2 screenDir, float searchRadius, float initialOffset, float3 posVS, float3 viewDirVS, float attenFactor)
//{
//	float  vecLenSq, vecLen, angle, fallOff;
//	float3 vec;
//	float2 viewZs = 0.0;
//
//	float2 bestAngle = float2(-1.0, -1.0);
//	float  thickness = _AdjustAngleThicknessParams.w;
//
//	for (uint i = 0u; i < GTAO_NUMTAPS; i++)
//	{
//		float fi = (float)i;
//
//		float2 offsetUV = screenDir * max(searchRadius * (fi + initialOffset), (fi + 1.0));
//		offsetUV.y *= -1;
//		float4 uv2 = baseUV.xyxy + float4(offsetUV.xy, -offsetUV.xy);
//
//		// Positive Direction
//		viewZs.x = GetViewZFromInput(uv2.xy);
//		viewZs.y = GetViewZFromInput(uv2.zw);
//
//		vec = TransformScreenToView(uv2.xy, viewZs.x) - posVS;
//		vecLenSq = dot(vec, vec);
//		vecLen = rsqrt(vecLenSq + 0.0001);
//		angle = dot(vec, viewDirVS) * vecLen;
//
//		fallOff = saturate(vecLenSq * attenFactor);
//		angle = lerp(angle, bestAngle.x, fallOff);
//		bestAngle.x = (angle > bestAngle.x) ? angle : lerp(angle, bestAngle.x, thickness);
//
//		// Negative Direction
//		vec = TransformScreenToView(uv2.zw, viewZs.y) - posVS;
//		vecLenSq = dot(vec, vec);
//		vecLen = rsqrt(vecLenSq + 0.0001);
//		angle = dot(vec, viewDirVS) * vecLen;
//
//		fallOff = saturate(vecLenSq * attenFactor);
//		angle = lerp(angle, bestAngle.y, fallOff);
//		bestAngle.y = (angle > bestAngle.y) ? angle : lerp(angle, bestAngle.y, thickness);
//	}
//
//	bestAngle.x = acosFast(clamp(bestAngle.x, -1.0, 1.0));
//	bestAngle.y = acosFast(clamp(bestAngle.y, -1.0, 1.0));
//
//	return bestAngle;
//}
//float ComputeInnerIntegral(float2 angles, float3 screenDir, float3 viewDirVS, float3 normalVS)
//{
//	// Given the angles found in the search plane we need to project the View Space GBuffer Normal onto the plane defined by the search axis and the View Direction and perform the inner integrate.
//	float3 planeNormal = SafeNormalize(cross(screenDir, viewDirVS));
//	float3 perp = cross(viewDirVS, planeNormal);
//	float3 projNormal = normalVS - planeNormal * dot(normalVS, planeNormal);
//
//	float lenProjNormal = length(projNormal) + 0.000001;
//	float recipMag = 1.0 / (lenProjNormal);
//
//	float cosAng = dot(projNormal, perp) * recipMag;
//
//	float gamma = acosFast(cosAng) - (CUSTOM_SHADER_PI_HALF);
//	float cosGamma = dot(projNormal, viewDirVS) * recipMag;
//	float sinGamma = cosAng * -2.0;
//
//	// clamp to normal hemisphere.
//	angles.x = gamma + max(-angles.x - gamma, -(CUSTOM_SHADER_PI_HALF));
//	angles.y = gamma + min(angles.y - gamma, (CUSTOM_SHADER_PI_HALF));
//
//	float ao = ((lenProjNormal) * 0.25 *
//		((angles.x * sinGamma + cosGamma - cos((2.0 * angles.x) - gamma)) +
//			(angles.y * sinGamma + cosGamma - cos((2.0 * angles.y) - gamma))));
//
//	return ao;
//}
//float CalculateGTAO(float2 uv, int2 dispatchThreadID, int2 groupThreadID)
//{
//	float viewZ = GetCachedViewZFromSharedMemory(groupThreadID + GROUP_THREAD_OFFSET);
//
//	if (viewZ > _FadeAttenParams.z)
//	{
//		return 1.0;
//	}
//	else
//	{
//		uv += _DepthBufferParams.zw * 0.125;
//
//		float3 posVS = TransformScreenToView(uv, viewZ);
//		float3 normalVS = GetNormalVS(uv, groupThreadID, posVS);
//		float3 viewDirVS = -SafeNormalize(posVS.xyz);
//
//		float worldRadiusAdjust = _AdjustAngleThicknessParams.x;
//
//		float pixelRadius = max(min(worldRadiusAdjust / posVS.z, GTAO_MAX_PIXEL_SCREEN_RADIUS), (float)GTAO_NUMTAPS);
//		float stepRadius = pixelRadius / ((float)GTAO_NUMTAPS + 1);
//		float attenFactor = _FadeAttenParams.w;
//
//		float3 randomAndOffset = GetRandomVectorFromCoord(dispatchThreadID);
//		float2 randomVec = randomAndOffset.xy;
//		float  offset = randomAndOffset.z;
//
//		float sum = 0.0;
//
//		const uint numAngles = 2u;	// Need to match with cpp's delta angle.
//
//		float sinDeltaAngle = _AdjustAngleThicknessParams.y;
//		float cosDeltaAngle = _AdjustAngleThicknessParams.z;
//
//		float3 screenDir = float3(randomVec.x, randomVec.y, 0.0);
//
//		for (uint angle = 0u; angle < numAngles; angle++)
//		{
//			float2 angles = SearchForLargestAngleDual(uv, screenDir.xy * _ResultBufferParams.zw, stepRadius, offset, posVS, viewDirVS, attenFactor);
//
//			sum += ComputeInnerIntegral(angles, screenDir, viewDirVS, normalVS);
//
//			// Rotate for the next angle.
//			float2 tempScreenDir = screenDir.xy;
//			screenDir.x = (tempScreenDir.x * cosDeltaAngle) + (tempScreenDir.y * -sinDeltaAngle);
//			screenDir.y = (tempScreenDir.x * sinDeltaAngle) + (tempScreenDir.y * cosDeltaAngle);
//			offset = frac(offset + 0.617);
//		}
//
//		float ao = sum;
//
//		ao = ao * 0.5;
//		ao *= 2.0 * 0.3183098861;
//
//		// Fade out based on user defined distance.
//		ao = lerp(ao, 1.0, saturate(viewZ * _FadeAttenParams.x + _FadeAttenParams.y));
//
//		return ao;
//	}
//}
//
//
//
//[numthreads(GROUP_THREAD_SIZE_X, GROUP_THREAD_SIZE_Y, 1)]
//void main(uint2 dispatchThreadID :SV_DispatchThreadID, uint2 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex, uint2 groupThreadID : SV_GroupThreadID)
//{
//	{
//		int2 groupOrigin = int2(groupID.x * GROUP_THREAD_SIZE_X, groupID.y * GROUP_THREAD_SIZE_Y) - GROUP_THREAD_OFFSET;
//		uint index = groupIndex * 2u;
//		if (index < GROUP_THREAD_MAX)
//		{
//			CacheDeviceZ(index, groupOrigin);
//			CacheDeviceZ(index + 1u, groupOrigin);
//		}
//		GroupMemoryBarrierWithGroupSync();
//	}
//	if (any(dispatchThreadID >= uint2(_ResultBufferParams.xy)))
//		return;
//
//	int2 groupThreadPos = int2(groupThreadID);
//	int2 dispatchThreadPos = int2(dispatchThreadID);
//	float2 uv = BufferPositionToUV(dispatchThreadID);
//	float ao = CalculateGTAO(uv, dispatchThreadPos, groupThreadPos);
//	float deviceZ = GetCachedDeviceZFromSharedMemory(groupThreadPos + GROUP_THREAD_OFFSET);
//	float4 encodeZ = EncodeFloatRGBA(deviceZ);
//	encodeZ.a = ao;
//	_ResultBuffer[dispatchThreadPos + _CameraViewportRect.xy] = encodeZ;
//}
//
//#endif