#ifndef SHADER_SPACE_TRANSFORM_HLSL
#define SHADER_SPACE_TRANSFORM_HLSL

// ---------------------------------------------------------------------------
// Space transformations.
//
// Pure functions: this library reads no constant buffer. Every matrix a
// function needs is passed in, and the caller decides which matrix it can
// actually see at that point. That keeps the include graph free of hidden
// dependencies: a program that never references C_DrawParams simply never has a
// world matrix to pass, and nothing here breaks.
//
// Rewritten from the legacy Public/ShaderSpaceTransform.hlsl. Three changes, all
// of them load bearing:
//
// 1. No constant buffer is referenced, here or anywhere in the library. The
//    compiler emits the buffers from the JSON declarations and their members
//    are plain globals, but the caller is the one who passes them down.
// 2. The legacy ENGINE_MATRIX_* and underscore prefixed aliases are gone.
// 3. Every matrix product is mul(Matrix, Vector). The project keeps -Zpc and
//    uploads matrices without transposing, so the shader side matrix is the
//    transpose of the CPU matrix and the matrix first form is the correct one.
//
// Naming note: the legacy TransformObjectToClip is now
// TransformObjectToWorldClip, because it takes both matrices and the name says
// so.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"

// ---------------------------------------------------------------------------
// Generic
// ---------------------------------------------------------------------------

float4 TransformPositionToSpecificSpace(in float4 InPosition, in float4x4 InMatrix)
{
	return mul(InMatrix, InPosition);
}

float3 TransformDirectionToSpecificSpace(in float3 InDirection, in float3x3 InMatrix, in bool InIsNormalize)
{
	float3 Result = mul(InMatrix, InDirection);
	if (InIsNormalize)
	{
		Result = SafeNormalize(Result);
	}
	return Result;
}

// ---------------------------------------------------------------------------
// Object space
// ---------------------------------------------------------------------------

float3 TransformObjectToWorld(in float3 InPosition, in float4x4 InWorldMatrix)
{
	return mul(InWorldMatrix, float4(InPosition, 1.0)).xyz;
}

float3 TransformWorldToObject(in float3 InPosition, in float4x4 InWorldInvMatrix)
{
	return mul(InWorldInvMatrix, float4(InPosition, 1.0)).xyz;
}

// Uses the inverse transpose so non uniform scale does not skew the normal.
float3 TransformObjectToWorldNormal(in float3 InNormal, in float4x4 InWorldInvTransposeMatrix, in bool InIsNormalize)
{
	float3 NormalWS = mul((float3x3)InWorldInvTransposeMatrix, InNormal);
	if (InIsNormalize)
	{
		NormalWS = SafeNormalize(NormalWS);
	}
	return NormalWS;
}

float3 TransformObjectToWorldDir(in float3 InDirection, in float4x4 InWorldMatrix, in bool InIsNormalize)
{
	float3 DirectionWS = mul((float3x3)InWorldMatrix, InDirection);
	if (InIsNormalize)
	{
		DirectionWS = SafeNormalize(DirectionWS);
	}
	return DirectionWS;
}

float3 TransformWorldToObjectDir(in float3 InDirection, in float4x4 InWorldInvMatrix, in bool InIsNormalize)
{
	float3 DirectionOS = mul((float3x3)InWorldInvMatrix, InDirection);
	if (InIsNormalize)
	{
		DirectionOS = SafeNormalize(DirectionOS);
	}
	return DirectionOS;
}

// ---------------------------------------------------------------------------
// View and clip space
// ---------------------------------------------------------------------------

float3 TransformWorldToView(in float3 InPosition, in float4x4 InViewMatrix)
{
	return mul(InViewMatrix, float4(InPosition, 1.0)).xyz;
}

float4 TransformWorldToClip(in float4 InPosition, in float4x4 InViewProjectionMatrix)
{
	return mul(InViewProjectionMatrix, InPosition);
}

float3 TransformViewToWorld(in float3 InPosition, in float4x4 InViewInvMatrix)
{
	return mul(InViewInvMatrix, float4(InPosition, 1.0)).xyz;
}

float4 TransformViewToClip(in float4 InPosition, in float4x4 InProjectionMatrix)
{
	return mul(InProjectionMatrix, InPosition);
}

float3 TransformClipToView(in float4 InPosition, in float4x4 InProjectionInvMatrix)
{
	float4 PositionVS = mul(InProjectionInvMatrix, InPosition);
	return (PositionVS.xyz / PositionVS.w);
}

float3 TransformClipToWorld(in float4 InPosition, in float4x4 InViewProjectionInvMatrix)
{
	float4 PositionWS = mul(InViewProjectionInvMatrix, InPosition);
	return (PositionWS.xyz / PositionWS.w);
}

float3 TransformWorldToViewDir(in float3 InDirection, in float4x4 InViewMatrix, in bool InIsNormalize)
{
	float3 DirectionVS = mul((float3x3)InViewMatrix, InDirection);
	if (InIsNormalize)
	{
		DirectionVS = SafeNormalize(DirectionVS);
	}
	return DirectionVS;
}

float3 TransformViewToWorldDir(in float3 InDirection, in float4x4 InViewInvMatrix, in bool InIsNormalize)
{
	float3 DirectionWS = mul((float3x3)InViewInvMatrix, InDirection);
	if (InIsNormalize)
	{
		DirectionWS = SafeNormalize(DirectionWS);
	}
	return DirectionWS;
}

// Object space all the way to clip space: two matrices, so two parameters. The
// only reason to prefer this over TransformWorldToClip is to skip the
// intermediate world space value the caller would otherwise have to carry.
float4 TransformObjectToWorldClip(in float3 InPosition, in float4x4 InWorldMatrix, in float4x4 InViewProjectionMatrix)
{
	float3 PositionWS = mul(InWorldMatrix, float4(InPosition, 1.0)).xyz;
	return mul(InViewProjectionMatrix, float4(PositionWS, 1.0));
}

// ---------------------------------------------------------------------------
// Tangent space
// ---------------------------------------------------------------------------

// Builds the tangent to world matrix. The columns are the tangent frame axes,
// which is the layout the matrix-first product below expects.
float3x3 CreateTangentMatrix(in float3 InNormal, in float3 InTangent, in bool InIsRecalculateTangent)
{
	float3 Binormal = SafeNormalize(cross(InNormal, InTangent));
	if (InIsRecalculateTangent)
	{
		InTangent = SafeNormalize(cross(Binormal, InNormal));
	}
	return float3x3(
		InTangent.x, Binormal.x, InNormal.x,
		InTangent.y, Binormal.y, InNormal.y,
		InTangent.z, Binormal.z, InNormal.z);
}

float3x3 CreateTangentMatrix(in float3 InNormal, in float3 InTangent, out float3 OutBinormal, in bool InIsRecalculateTangent)
{
	OutBinormal = SafeNormalize(cross(InNormal, InTangent));
	if (InIsRecalculateTangent)
	{
		InTangent = SafeNormalize(cross(OutBinormal, InNormal));
	}
	return float3x3(
		InTangent.x, OutBinormal.x, InNormal.x,
		InTangent.y, OutBinormal.y, InNormal.y,
		InTangent.z, OutBinormal.z, InNormal.z);
}

float3 TransformTangentToWorldDir(in float3 InDirection, in float3x3 InTangentMatrix, in bool InIsNormalize)
{
	float3 TargetDir = mul(InTangentMatrix, InDirection);
	if (InIsNormalize)
	{
		TargetDir = SafeNormalize(TargetDir);
	}
	return TargetDir;
}

// ---------------------------------------------------------------------------
// View helpers
// ---------------------------------------------------------------------------

float3 GetCameraWorldPosition(in float4 InViewPosition)
{
	return InViewPosition.xyz;
}
#endif	//SHADER_SPACE_TRANSFORM_HLSL
