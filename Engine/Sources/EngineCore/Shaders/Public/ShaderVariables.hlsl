#ifndef _SHADER_VARIABLES_HLSL
#define _SHADER_VARIABLES_HLSL

#include "./ShaderDefCommon.hlsl"

#define PER_OBJECT_TRANSFORM					float4x4 _WorldMatrix; float4x4 _WorldInvMatrix; float4x4 _WorldInvTransposeMatrix;
#define RENDER_AO_INPUT(__AOInputName,__Slot)	Texture2D<float> __AOInputName : register(__Slot);

cbuffer ConstantBufferPerFrame : register(b0)
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

SamplerState		_PointClampSampler		: register(s0);
SamplerState		_PointWrapSampler		: register(s1);
SamplerState		_LinearClampSampler		: register(s2);
SamplerState		_LinearWrapSampler		: register(s3);

#endif