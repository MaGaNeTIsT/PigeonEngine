#ifndef _SHADER_VARIABLES_HLSL
#define _SHADER_VARIABLES_HLSL

#include "./ShaderDefCommon.hlsl"

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
	LightParams		_LightData[GLOBAL_LIGHT_COUNT];
}
cbuffer ConstantBufferPerDraw : register(b2)
{
	float4x4		_WorldMatrix;
	float4x4		_WorldInvMatrix;
	float4x4		_WorldInvTransposeMatrix;
	float4			_CustomParameter;
};

SamplerState		_PointClampSampler		: register(s0);
SamplerState		_PointWrapSampler		: register(s1);
SamplerState		_LinearClampSampler		: register(s2);
SamplerState		_LinearWrapSampler		: register(s3);

Texture2D<float>	_GlobalAOInput			: register(t0);

Texture2D			_EngineLightShadowMap0	: register(t1);
Texture2D			_EngineLightShadowMap1	: register(t2);
Texture2D			_EngineLightShadowMap2	: register(t3);
Texture2D			_EngineLightShadowMap3	: register(t4);

Texture2D			_EngineGBufferA			: register(t5);
Texture2D			_EngineGBufferB			: register(t6);
Texture2D			_EngineGBufferC			: register(t7);
Texture2D			_EngineGBufferD			: register(t8);

Texture2D			_EngineSceneNormal		: register(t5);
Texture2D			_EngineSceneColor		: register(t6);
Texture2D			_EngineSceneDepth		: register(t7);

Texture2D			_CustomTextureA			: register(t5);
Texture2D			_CustomTextureB			: register(t6);
Texture2D			_CustomTextureC			: register(t7);
Texture2D			_CustomTextureD			: register(t8);

#endif