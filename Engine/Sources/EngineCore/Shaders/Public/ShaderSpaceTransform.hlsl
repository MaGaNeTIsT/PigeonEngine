#ifndef _SHADER_SPACE_TRANSFORM_HLSL
#define _SHADER_SPACE_TRANSFORM_HLSL

#include "./ShaderVariables.hlsl"
#include "./ShaderFunctions.hlsl"

#define ENGINE_CAMERA_POSITION		(_CameraWorldPosition)
#define ENGINE_MATRIX_V				(_ViewMatrix)
#define ENGINE_MATRIX_P				(_ProjectionMatrix)
#define ENGINE_MATRIX_I_V			(_ViewInvMatrix)
#define ENGINE_MATRIX_I_P			(_ProjectionInvMatrix)
#define ENGINE_MATRIX_VP			(_ViewProjectionMatrix)
#define ENGINE_MATRIX_I_VP			(_ViewProjectionInvMatrix)

#ifdef SHADER_USE_TRANSFORM
#define ENGINE_MATRIX_W				(_WorldMatrix)
#define ENGINE_MATRIX_I_W			(_WorldInvMatrix)
#define ENGINE_MATRIX_I_T_W			(_WorldInvTransposeMatrix)
#endif

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
#ifdef SHADER_USE_TRANSFORM
	return mul(float4(InPosition, 1.0), ENGINE_MATRIX_W).xyz;
#else
	return (InPosition.xyz);
#endif
}
float4 TransformObjectToClip(const float3 InPosition)
{
#ifdef SHADER_USE_TRANSFORM
	float3 PositionWS = mul(float4(InPosition, 1.0), ENGINE_MATRIX_W).xyz;
#else
	float3 PositionWS = InPosition.xyz;
#endif
	return mul(float4(PositionWS, 1.0), ENGINE_MATRIX_VP);
}
float3 TransformWorldToObject(const float3 InPosition)
{
#ifdef SHADER_USE_TRANSFORM
	return mul(float4(InPosition, 1.0), ENGINE_MATRIX_I_W).xyz;
#else
	return (InPosition.xyz);
#endif
}
float3 TransformWorldToView(const float3 InPosition)
{
	return mul(float4(InPosition, 1.0), ENGINE_MATRIX_V).xyz;
}
float4 TransformWorldToClip(const float3 InPosition)
{
	return mul(float4(InPosition, 1.0), ENGINE_MATRIX_VP);
}
float3 TransformViewToWorld(const float3 InPosition)
{
	return mul(float4(InPosition, 1.0), ENGINE_MATRIX_I_V).xyz;
}
float4 TransformViewToClip(const float3 InPosition)
{
	return mul(float4(InPosition, 1.0), ENGINE_MATRIX_P);
}
float3 TransformClipToView(const float4 InPosition)
{
	float4 PositionVS = mul(InPosition, ENGINE_MATRIX_I_P);
	return (PositionVS.xyz / PositionVS.w);
}
float3 TransformClipToWorld(const float4 InPosition)
{
	float4 PositionWS = mul(InPosition, ENGINE_MATRIX_I_VP);
	return (PositionWS.xyz / PositionWS.w);
}
float3 TransformScreenToView(float2 InUV, float InViewZ)
{
	float2 ProjViewPos;
	ProjViewPos.x = InUV.x * _ScreenToViewSpaceParams.x + _ScreenToViewSpaceParams.z;
	ProjViewPos.y = InUV.y * _ScreenToViewSpaceParams.y + _ScreenToViewSpaceParams.w;
	return float3(ProjViewPos * InViewZ, InViewZ);
}
float3 TransformObjectToWorldNormal(const float3 InNormal, uniform bool bNormalize = true)
{
#ifdef SHADER_USE_TRANSFORM
	float3 NormalWS = mul(InNormal.xyz, (float3x3)ENGINE_MATRIX_I_T_W);
#else
	float3 NormalWS = InNormal.xyz;
#endif
	if (bNormalize == true)
	{
		NormalWS = SafeNormalize(NormalWS);
	}
	return NormalWS;
}
float3 TransformObjectToWorldDir(const float3 InDir, uniform bool bNormalize = true)
{
#ifdef SHADER_USE_TRANSFORM
	float3 DirWS = mul(InDir.xyz, (float3x3)ENGINE_MATRIX_W);
#else
	float3 DirWS = InDir.xyz;
#endif
	if (bNormalize == true)
	{
		DirWS = SafeNormalize(DirWS);
	}
	return DirWS;
}
float3 TransformWorldToObjectDir(const float3 InDir, uniform bool bNormalize = true)
{
#ifdef SHADER_USE_TRANSFORM
	float3 DirOS = mul(InDir.xyz, (float3x3)ENGINE_MATRIX_I_W);
#else
	float3 DirOS = InDir.xyz;
#endif
	if (bNormalize == true)
	{
		DirOS = SafeNormalize(DirOS);
	}
	return DirOS;
}
float3 TransformWorldToViewDir(const float3 InDir, uniform bool bNormalize = true)
{
	float3 DirVS = mul(InDir, (float3x3)ENGINE_MATRIX_V);
	if (bNormalize == true)
	{
		DirVS = SafeNormalize(DirVS);
	}
	return DirVS;
}
float3 TransformViewToWorldDir(const float3 InDir, uniform bool bNormalize = true)
{
	float3 DirWS = mul(InDir, (float3x3)ENGINE_MATRIX_I_V);
	if (bNormalize == true)
	{
		DirWS = SafeNormalize(DirWS);
	}
	return DirWS;
}
float3x3 CreateTangentMatrix(float3 InNormal, float3 InTangent, uniform bool bRecalculateTangent = false)
{
	float3 Binormal = SafeNormalize(cross(InNormal, InTangent));
	if (bRecalculateTangent == true)
	{
		InTangent = SafeNormalize(cross(Binormal, InNormal));
	}
	return float3x3(InTangent.x, InTangent.y, InTangent.z,
		Binormal.x, Binormal.y, Binormal.z,
		InNormal.x, InNormal.y, InNormal.z);
}
float3x3 CreateTangentMatrix(in float3 InNormal, in float3 InTangent, out float3 OutBinormal, uniform bool bRecalculateTangent = false)
{
	OutBinormal = SafeNormalize(cross(InNormal, InTangent));
	if (bRecalculateTangent == true)
	{
		InTangent = SafeNormalize(cross(OutBinormal, InNormal));
	}
	return float3x3(InTangent.x, InTangent.y, InTangent.z,
		OutBinormal.x, OutBinormal.y, OutBinormal.z,
		InNormal.x, InNormal.y, InNormal.z);
}
float3 TransformTangentToSpaceDir(const float3 InDir, const float3x3 InTangentMatrix, uniform bool bNormalize = false)
{
	float3 TargetDir = mul(InDir, InTangentMatrix);
	if (bNormalize == true)
	{
		TargetDir = SafeNormalize(TargetDir);
	}
	return TargetDir;
}
float ConvertDeviceZToViewZ(float InDeviceZ)
{
	return rcp(InDeviceZ * _DepthMultiAdd.x + _DepthMultiAdd.y);
}

#endif	//_SHADER_SPACE_TRANSFORM_HLSL