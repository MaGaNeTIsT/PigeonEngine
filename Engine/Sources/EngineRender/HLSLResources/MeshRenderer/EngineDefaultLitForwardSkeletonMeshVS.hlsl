#ifndef _ENGINE_DEFAULT_LIT_FORWARD_SKELETON_MESH_VS_HLSL
#define _ENGINE_DEFAULT_LIT_FORWARD_SKELETON_MESH_VS_HLSL

#include "../Common/ShaderCommon.hlsl"

VaryingForward main(AttributeForwardSkeletonMesh input)
{	
	uint4  blendIndices	= input.blendIndices.xyzw;
	float4 blendWeight	= input.blendWeight.xyzw;
	
	float4x4 localToWorldMatrix =
		_SkeletonMatrix[blendIndices.x] * blendWeight.x +
		_SkeletonMatrix[blendIndices.y] * blendWeight.y +
		_SkeletonMatrix[blendIndices.z] * blendWeight.z +
		_SkeletonMatrix[blendIndices.w] * blendWeight.w;
	
	localToWorldMatrix /= blendWeight.x + blendWeight.y + blendWeight.z + blendWeight.w;
	float3 posWS = TransformPositionToSpecificSpace(float4(input.position.xyz, 1.0), localToWorldMatrix).xyz;
	posWS.xyz = TransformObjectToWorld(posWS.xyz);
	float3 tangentWS = TransformDirectionToSpecificSpace(input.tangent.xyz, (float3x3)localToWorldMatrix);
	tangentWS.xyz = TransformObjectToWorldDir(tangentWS.xyz);

	float4x4 inverseTransposeLocalToWorldMatrix =
		_SkeletonInverseTransposeMatrix[blendIndices.x] * blendWeight.x +
		_SkeletonInverseTransposeMatrix[blendIndices.y] * blendWeight.y +
		_SkeletonInverseTransposeMatrix[blendIndices.z] * blendWeight.z +
		_SkeletonInverseTransposeMatrix[blendIndices.w] * blendWeight.w;
	inverseTransposeLocalToWorldMatrix /= blendWeight.x + blendWeight.y + blendWeight.z + blendWeight.w;
	float3 normalWS = TransformDirectionToSpecificSpace(input.normal.xyz, (float3x3)inverseTransposeLocalToWorldMatrix);
	normalWS.xyz = TransformObjectToWorldNormal(normalWS.xyz);

	float3 binormalWS = SafeNormalize(cross(normalWS, tangentWS));
	tangentWS = cross(binormalWS, normalWS);

	VaryingForward output;
	
	output.uv0				= input.uv0;
	output.normal			= float4(normalWS, 0.0);
	output.tangent			= float4(tangentWS, 0.0);
	output.positionWS		= float4(posWS, 1.0);
	output.positionCS		= TransformWorldToClip(output.positionWS.xyz);
	output.positionSS		= output.positionCS.xyzw / output.positionCS.w;
	output.positionSS.xy	= output.positionSS.xy * float2(0.5, -0.5) + 0.5;
	
	return output;
}

#endif