#ifndef _SHADER_LIGHTINGS_HLSL
#define _SHADER_LIGHTINGS_HLSL

#ifndef SHADER_USE_DIRECTIONAL_LIGHT_INPUT
#define SHADER_USE_DIRECTIONAL_LIGHT_INPUT			0
#endif	//SHADER_USE_DIRECTIONAL_LIGHT_INPUT

#if (SHADER_USE_DIRECTIONAL_LIGHT_INPUT)
#ifndef SHADER_DIRECTIONAL_LIGHT_INPUT_SLOT
#define SHADER_DIRECTIONAL_LIGHT_INPUT_SLOT			t0
#endif	//SHADER_DIRECTIONAL_LIGHT_INPUT_SLOT
#endif

#ifndef SHADER_USE_POINT_LIGHT_INPUT
#define SHADER_USE_POINT_LIGHT_INPUT				0
#endif	//SHADER_USE_POINT_LIGHT_INPUT

#ifndef SHADER_USE_SPOT_LIGHT_INPUT
#define SHADER_USE_SPOT_LIGHT_INPUT					0
#endif	//SHADER_USE_SPOT_LIGHT_INPUT

#if ((SHADER_USE_DIRECTIONAL_LIGHT_INPUT) || (SHADER_USE_POINT_LIGHT_INPUT) || (SHADER_USE_SPOT_LIGHT_INPUT))
#define SHADER_USE_LIGHT_INPUT						1
#else
#define SHADER_USE_LIGHT_INPUT						0
#endif

struct DirectionalLightInputDatas
{
	float4x4	_ViewMatrix;
	float4x4	_ViewInvMatrix;
	float4x4	_ProjectionMatrix;
	float4x4	_ProjectionInvMatrix;
	float4x4	_ViewProjectionMatrix;
	float4x4	_ViewProjectionInvMatrix;
	float4		_ColorIntensity;
	uint2		_ShadowMapSize;
	float3		_WorldPosition;
};

#if (SHADER_USE_LIGHT_INPUT)

#if SHADER_USE_DIRECTIONAL_LIGHT_INPUT
StructuredBuffer<DirectionalLightInputDatas> _DirectionalLightInput : register(SHADER_DIRECTIONAL_LIGHT_INPUT_SLOT);
#endif

#endif	//SHADER_USE_LIGHT_INPUT

#endif	//_SHADER_LIGHTINGS_HLSL