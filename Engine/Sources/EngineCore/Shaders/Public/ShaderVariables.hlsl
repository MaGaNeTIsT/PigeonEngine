#ifndef _SHADER_VARIABLES_HLSL
#define _SHADER_VARIABLES_HLSL

#ifndef SHADER_USE_SAMPLER_INPUT
#define SHADER_USE_SAMPLER_INPUT					0
#endif	//SHADER_USE_SAMPLER_INPUT

#ifndef SHADER_USE_VIEW_INPUT
#define SHADER_USE_VIEW_INPUT						0
#endif	//SHADER_USE_VIEW_INPUT

#ifndef SHADER_VIEW_INPUT_SLOT
#define SHADER_VIEW_INPUT_SLOT						b0
#endif	//SHADER_VIEW_INPUT_SLOT

#ifndef SHADER_USE_PERDRAW_INPUT
#define SHADER_USE_PERDRAW_INPUT					0
#endif	//SHADER_USE_PERDRAW_INPUT

#ifndef SHADER_PERDRAW_INPUT_SLOT
#define SHADER_PERDRAW_INPUT_SLOT					b1
#endif	//SHADER_PERDRAW_INPUT_SLOT

#ifndef SHADER_USE_TRANSFORM_INPUT
#define SHADER_USE_TRANSFORM_INPUT					0
#endif	//SHADER_USE_TRANSFORM_INPUT

#ifndef SHADER_USE_AO_INPUT
#define SHADER_USE_AO_INPUT							0
#endif	//SHADER_USE_AO_INPUT

#ifndef SHADER_AO_INPUT_SLOT
#define SHADER_AO_INPUT_SLOT						t0
#endif	//SHADER_AO_INPUT_SLOT

#if (SHADER_USE_SAMPLER_INPUT)
SamplerState		_PointClampSampler		: register(s0);
SamplerState		_PointWrapSampler		: register(s1);
SamplerState		_LinearClampSampler		: register(s2);
SamplerState		_LinearWrapSampler		: register(s3);
#endif

#if (SHADER_USE_VIEW_INPUT)
cbuffer ConstantBufferPerFrame : register(SHADER_VIEW_INPUT_SLOT)
{
	float4x4		_ViewMatrix;
	float4x4		_ViewInvMatrix;
	float4x4		_ProjectionMatrix;
	float4x4		_ProjectionInvMatrix;
	float4x4		_ViewProjectionMatrix;
	float4x4		_ViewProjectionInvMatrix;
	float4			_TimeParams;
	float4			_DepthMultiAdd;
	float4			_ScreenToViewSpaceParams;
	float4			_CameraViewportMinSizeAndInvBufferSize;
	float4			_CameraViewportSizeAndInvSize;
	float4			_CameraViewportRect;
	float4			_CameraWorldPosition;
};
#endif

#if (SHADER_USE_PERDRAW_INPUT)
cbuffer ConstantBufferPerDraw : register(SHADER_PERDRAW_INPUT_SLOT)
{
#if (SHADER_USE_TRANSFORM_INPUT)
	float4x4		_WorldMatrix;
	float4x4		_WorldInvMatrix;
	float4x4		_WorldInvTransposeMatrix;
#endif
#if (SHADER_USE_SKELETON_INPUT)
	uint4			_SkeletonParams;
#endif
};
#endif

#if (SHADER_USE_AO_INPUT)
Texture2D<float>	_ShaderAOInput : register(SHADER_AO_INPUT_SLOT);
#endif

#include "./ShaderLightings.hlsl"

#endif	//_SHADER_VARIABLES_HLSL