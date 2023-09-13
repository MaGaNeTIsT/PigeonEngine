#ifndef _SHADER_SPACE_TRANSFORM_HLSL
#define _SHADER_SPACE_TRANSFORM_HLSL

#include "./ShaderVariables.hlsl"
#include "./ShaderFunctions.hlsl"

#define ENGINE_CAMERA_POSITION		(_CameraWorldPosition)
#define ENGINE_MATRIX_W				(_WorldMatrix)
#define ENGINE_MATRIX_V				(_ViewMatrix)
#define ENGINE_MATRIX_P				(_ProjectionMatrix)
#define ENGINE_MATRIX_I_W			(_WorldInvMatrix)
#define ENGINE_MATRIX_I_V			(_ViewInvMatrix)
#define ENGINE_MATRIX_I_P			(_ProjectionInvMatrix)
#define ENGINE_MATRIX_I_T_W			(_WorldInvTransposeMatrix)
#define ENGINE_MATRIX_VP			(_ViewProjectionMatrix)
#define ENGINE_MATRIX_I_VP			(_ViewProjectionInvMatrix)

float3 GetCameraWorldPosition()
{
	return ENGINE_CAMERA_POSITION.xyz;
}
float4 TransformPositionToSpecificSpace(const float4 InPosition, const float4x4 InMatrix)
{
	return mul(InPosition, InMatrix).xyzw;
}
float3 TransformDirectionToSpecificSpace(const float3 InDirection, const float3x3 InMatrix, uniform bool bNormalize = true)
{
	float3 Result = mul(InDirection, InMatrix);
	if (bNormalize == true)
	{
		Result = SafeNormalize(Result);
	}
	return Result;
}
float3 TransformObjectToWorld(const float3 InPosition)
{
	return mul(float4(InPosition, 1.0), ENGINE_MATRIX_W).xyz;
}
float4 TransformObjectToClip(const float3 InPosition)
{
	float3 PositionWS = mul(float4(InPosition, 1.0), ENGINE_MATRIX_W).xyz;
	return mul(float4(PositionWS, 1.0), ENGINE_MATRIX_VP);
}
float3 TransformWorldToObject(const float3 position)
{
	return mul(float4(position, 1), ENGINE_MATRIX_I_W).xyz;
}
float3 TransformWorldToView(const float3 position)
{
	return mul(float4(position, 1), ENGINE_MATRIX_V).xyz;
}
float4 TransformWorldToClip(const float3 position)
{
	return mul(float4(position, 1), ENGINE_MATRIX_VP);
}
float3 TransformViewToWorld(const float3 position)
{
	return mul(float4(position, 1), ENGINE_MATRIX_I_V).xyz;
}
float4 TransformViewToClip(const float3 position)
{
	return mul(float4(position, 1), ENGINE_MATRIX_P);
}
float3 TransformClipToView(const float4 position)
{
	float4 positionVS = mul(position, ENGINE_MATRIX_I_P);
	return (positionVS.xyz / positionVS.w);
}
float3 TransformClipToWorld(const float4 position)
{
	float4 positionWS = mul(position, ENGINE_MATRIX_I_VP);
	return (positionWS.xyz / positionWS.w);
}
float3 TransformScreenToView(float2 uv, float viewZ)
{
	float2 projViewPos;
	projViewPos.x = uv.x * _ScreenToViewSpaceParams.x + _ScreenToViewSpaceParams.z;
	projViewPos.y = uv.y * _ScreenToViewSpaceParams.y + _ScreenToViewSpaceParams.w;
	return float3(projViewPos * viewZ, viewZ);
}
float3 TransformObjectToWorldNormal(const float3 normal, uniform bool bNormalize = true)
{
	float3 normalWS = mul(normal, (float3x3)ENGINE_MATRIX_I_T_W);
	if (bNormalize == true)
		normalWS = SafeNormalize(normalWS);
	return normalWS;
}
float3 TransformObjectToWorldDir(const float3 dir, uniform bool bNormalize = true)
{
	float3 dirWS = mul(dir, (float3x3)ENGINE_MATRIX_W);
	if (bNormalize == true)
		dirWS = SafeNormalize(dirWS);
	return dirWS;
}
float3 TransformWorldToObjectDir(const float3 dir, uniform bool bNormalize = true)
{
	float3 dirOS = mul(dir, (float3x3)ENGINE_MATRIX_I_W);
	if (bNormalize == true)
		dirOS = SafeNormalize(dirOS);
	return dirOS;
}
float3 TransformWorldToViewDir(const float3 dir, uniform bool bNormalize = true)
{
	float3 dirVS = mul(dir, (float3x3)ENGINE_MATRIX_V);
	if (bNormalize == true)
		dirVS = SafeNormalize(dirVS);
	return dirVS;
}
float3 TransformViewToWorldDir(const float3 dir, uniform bool bNormalize = true)
{
	float3 dirWS = mul(dir, (float3x3)ENGINE_MATRIX_I_V);
	if (bNormalize == true)
		dirWS = SafeNormalize(dirWS);
	return dirWS;
}
float3x3 CreateTangentMatrix(float3 normal, float3 tangent, uniform bool reCalculateTangent = false)
{
	float3 binormal = SafeNormalize(cross(normal, tangent));
	if (reCalculateTangent == true)
		tangent = SafeNormalize(cross(binormal, normal));
	return float3x3(tangent.x, tangent.y, tangent.z,
		binormal.x, binormal.y, binormal.z,
		normal.x, normal.y, normal.z);
}
float3x3 CreateTangentMatrix(in float3 normal, in float3 tangent, out float3 binormal, uniform bool reCalculateTangent = false)
{
	binormal = SafeNormalize(cross(normal, tangent));
	if (reCalculateTangent == true)
		tangent = SafeNormalize(cross(binormal, normal));
	return float3x3(tangent.x, tangent.y, tangent.z,
		binormal.x, binormal.y, binormal.z,
		normal.x, normal.y, normal.z);
}
float3 TransformTangentToSpaceDir(const float3 dir, const float3x3 tangentMatrix, uniform bool bNormalize = false)
{
	float3 targetDir = mul(dir, tangentMatrix);
	if (bNormalize == true)
		targetDir = SafeNormalize(targetDir);
	return targetDir;
}
float ConvertDeviceZToViewZ(float deviceZ)
{
	return rcp(deviceZ * _DepthMultiAdd.x + _DepthMultiAdd.y);
}

#endif