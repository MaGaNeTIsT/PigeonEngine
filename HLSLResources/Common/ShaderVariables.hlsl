#ifndef _SHADER_VARIABLES_HLSL
#define _SHADER_VARIABLES_HLSL

#include "./ShaderDefCommon.hlsl"

cbuffer ConstantBufferPerFrame : register(b0)
{
	float4x4				_ViewMatrix;
	float4x4				_ViewInvMatrix;
	float4x4				_ProjectionMatrix;
	float4x4				_ProjectionInvMatrix;
	float4x4				_ViewProjectionMatrix;
	float4x4				_ViewProjectionInvMatrix;
	float4					_TimeParams;
	float4					_ScreenParams;
	float3					_CameraWorldPosition;
	float					_DirectionalLightCount;
	DirectionalLightData	_DirectionalLightData[4];
};
cbuffer ConstantBufferPerDraw : register(b1)
{
	float4x4	_WorldMatrix;
	float4x4	_WorldInvMatrix;
	float4x4	_WorldInvTransposeMatrix;
	float4		_CustomParameter;
};
SamplerState	_PointClampSampler			: register(s0);
SamplerState	_LinearClampSampler			: register(s1);
SamplerState	_PointWrapSampler			: register(s2);
SamplerState	_LinearWrapSampler			: register(s3);

Texture2D		_EngineGBufferWorldNormal	: register(t0);
Texture2D		_EngineGBufferAlbedo		: register(t1);
Texture2D		_EngineGBufferProperty		: register(t2);
Texture2D		_EngineGBufferID			: register(t3);
Texture2D		_EngineCameraColor			: register(t4);
Texture2D		_EngineCameraDepth			: register(t5);
Texture2D		_EngineLightShadowMap		: register(t6);

Texture2D		_AlbedoTexture				: register(t8);
Texture2D		_NormalTexture				: register(t9);
Texture2D		_PropertyTexture			: register(t10);

#endif