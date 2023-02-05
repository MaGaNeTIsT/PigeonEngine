#ifndef _ENGINE_TEST_VS_HLSL
#define _ENGINE_TEST_VS_HLSL

float3 SafeNormalize(const float3 input)
{
	float3 output = normalize(input);
	bool i = any(isinf(output));
	output = i ? float3(0.0, 0.0, 1.0) : output;
	return output;
}

struct LightParams
{
	float4	Color;
	float4	Params0;
	float4	Params1;
};

cbuffer ConstantBufferPerFrame : register(b0)
{
	float4x4		_ViewMatrix;
	float4x4		_ViewInvMatrix;
	float4x4		_ProjectionMatrix;
	float4x4		_ProjectionInvMatrix;
	float4x4		_ViewProjectionMatrix;
	float4x4		_ViewProjectionInvMatrix;
	float4x4		_LightViewProjectionMatrix;
	float4			_TimeParams;
	float4			_DepthMultiAdd;
	float4			_ScreenToViewSpaceParams;
	float4			_CameraViewportMinSizeAndInvBufferSize;
	float4			_CameraViewportSizeAndInvSize;
	float4			_CameraViewportRect;
	float4			_CameraWorldPosition;
};
cbuffer ConstantBufferLightData : register(b1)
{
	uint4			_LightCount;
	LightParams		_LightData[16];
}
cbuffer ConstantBufferPerDraw : register(b2)
{
	float4x4		_WorldMatrix;
	float4x4		_WorldInvMatrix;
	float4x4		_WorldInvTransposeMatrix;
};

cbuffer ConstantBufferSkeleton : register(b4)
{
	float4			_SkeletonBoneNum;
	float4x4		_SkeletonMatrix[576];
};

struct TestAttribute
{
	float4	position		: POSITION0;
	float4	normal			: NORMAL0;
	uint4	boneIndices		: BLENDINDICES;
	float4	boneWeights		: BLENDWEIGHT;
};
struct TestVarying
{
	float4	positionCS	: SV_POSITION;
	float4	normal		: NORMAL0;
};

float4 TransformPositionToSpecificSpace(const float4 position, const float4x4 mat)
{
	return mul(position, mat).xyzw;
}

float3 TransformDirectionToSpecificSpace(const float3 dir, const float3x3 mat, uniform bool bNormalize = true)
{
	float3 d = mul(dir, mat);
	if (bNormalize == true)
		d = SafeNormalize(d);
	return d;
}

float3 TransformObjectToWorld(const float3 position, const float4x4 mat)
{
	return mul(float4(position, 1), mat).xyz;
}

float4 TransformWorldToClip(const float3 position)
{
	return mul(float4(position, 1), _ViewProjectionMatrix);
}

float3 TransformObjectToWorldNormal(const float3 normal, const float4x4 mat, uniform bool bNormalize = true)
{
	float3 normalWS = mul(normal, (float3x3)mat);
	if (bNormalize == true)
		normalWS = SafeNormalize(normalWS);
	return normalWS;
}

TestVarying main(TestAttribute input)
{	
	uint4  blendIndices	= input.boneIndices.xyzw;
	float4 blendWeight	= input.boneWeights.xyzw;
	
	float4x4 localToWorldMatrix =
		_SkeletonMatrix[blendIndices.x] * blendWeight.x +
		_SkeletonMatrix[blendIndices.y] * blendWeight.y +
		_SkeletonMatrix[blendIndices.z] * blendWeight.z +
		_SkeletonMatrix[blendIndices.w] * blendWeight.w;
	
	localToWorldMatrix /= blendWeight.x + blendWeight.y + blendWeight.z + blendWeight.w;
	float3 posWS = TransformPositionToSpecificSpace(float4(input.position.xyz, 1.0), localToWorldMatrix).xyz;
	posWS.xyz = TransformObjectToWorld(posWS.xyz, _WorldMatrix);

	uint inverseOffset = uint(_SkeletonBoneNum.x);
	float4x4 inverseTransposeLocalToWorldMatrix =
		_SkeletonMatrix[inverseOffset + blendIndices.x] * blendWeight.x +
		_SkeletonMatrix[inverseOffset + blendIndices.y] * blendWeight.y +
		_SkeletonMatrix[inverseOffset + blendIndices.z] * blendWeight.z +
		_SkeletonMatrix[inverseOffset + blendIndices.w] * blendWeight.w;
	inverseTransposeLocalToWorldMatrix /= blendWeight.x + blendWeight.y + blendWeight.z + blendWeight.w;
	float3 normalWS = TransformDirectionToSpecificSpace(input.normal.xyz, (float3x3)inverseTransposeLocalToWorldMatrix);
	normalWS.xyz = TransformObjectToWorldNormal(normalWS.xyz, _WorldInvTransposeMatrix);

	TestVarying output;

	output.positionCS	= TransformWorldToClip(posWS.xyz);
	output.normal		= float4(normalWS.xyz, 0.0);
	
	return output;
}

#endif