#ifndef _SHADER_VARIABLES_HLSL
#define _SHADER_VARIABLES_HLSL

#include "./ShaderDefCommon.hlsl"

cbuffer ConstantBufferPerFrame : register(b0)
{
	float4x4	_ViewMatrix;
	float4x4	_ViewInvMatrix;
	float4x4	_ProjectionMatrix;
	float4x4	_ProjectionInvMatrix;
	float4x4	_ViewProjectionMatrix;
	float4x4	_ViewProjectionInvMatrix;
	float4		_TimeParams;
	float4		_DepthMultiAdd;
	float4		_ScreenToViewSpaceParams;
	float4		_CameraViewportMinSizeAndInvBufferSize;
	float4		_CameraViewportSizeAndInvSize;
	float4		_CameraViewportRect;
	float4		_CameraWorldPosition;
};
cbuffer ConstantBufferPerDraw : register(b1)
{
	float4x4	_WorldMatrix;
	float4x4	_WorldInvMatrix;
	float4x4	_WorldInvTransposeMatrix;
	float4		_CustomParameter;
};
cbuffer ConstantBufferLightData : register(b2)
{
	uint4			_LightCount;
	LightParams		_LightData[GLOBAL_LIGHT_COUNT];
}

SamplerState		_PointClampSampler		: register(s0);
SamplerState		_PointWrapSampler		: register(s1);
SamplerState		_LinearClampSampler		: register(s2);
SamplerState		_LinearWrapSampler		: register(s3);

Texture2D			_EngineGBufferColor		: register(t0);
Texture2D			_EngineGBufferNormal	: register(t1);
Texture2D			_EngineGBufferAlbedo	: register(t2);
Texture2D			_EngineGBufferProperty	: register(t3);
Texture2D			_EngineLightShadowMap0	: register(t4);
Texture2D			_EngineLightShadowMap1	: register(t5);
Texture2D			_EngineLightShadowMap2	: register(t6);
Texture2D			_EngineLightShadowMap3	: register(t7);

Texture2D			_EngineCameraColor		: register(t0);
Texture2D			_EngineCameraDepth		: register(t1);

Texture2D			_AlbedoTexture			: register(t0);
Texture2D			_NormalTexture			: register(t1);
Texture2D			_PropertyTexture		: register(t2);
Texture2D<float>	_GlobalAOInput			: register(t3);

#endif