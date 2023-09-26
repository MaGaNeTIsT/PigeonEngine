#ifndef _SCENE_LIGHTING_PE_PS_HLSL
#define _SCENE_LIGHTING_PE_PS_HLSL

#define SHADER_USE_DEFERRED_SHADING				1

#define SHADER_USE_SAMPLER_INPUT				1
#define SHADER_USE_VIEW_INPUT					1
#define SHADER_VIEW_INPUT_SLOT					b0

#define SHADER_USE_GBUFFER_INPUT				1
#define SHADER_GBUFFER_A_INPUT_SLOT				t0
#define SHADER_GBUFFER_B_INPUT_SLOT				t1
#define SHADER_GBUFFER_C_INPUT_SLOT				t2

#define SHADER_LIGHT_COMMON_INPUT_SLOT			b1

#define SHADER_USE_DIRECTIONAL_LIGHT_INPUT		1
#define SHADER_DIRECTIONAL_LIGHT_INPUT_SLOT		t3

#define SHADER_USE_POINT_LIGHT_INPUT			1
#define SHADER_USE_SPOT_LIGHT_INPUT				1

#include "../Common/ShaderCommon.hlsl"

void main(in float4 InPosition : SV_Position, in float2 InTexcoord : TEXCOORD0, out float4 OutTarget : SV_Target)
{
	uint2 TexCoord = (uint2)InPosition.xy;

	float3 WorldPosition;
	{
		float4 SVPosition = float4(InTexcoord.x, 1.0 - InTexcoord.y, 1.0, 1.0);
		SVPosition.xy = (SVPosition.xy * 2.0) - 1.0;
		WorldPosition.xyz = TransformClipToWorld(SVPosition.xyzw).xyz;
	}

	float3 ViewDirection;
	{
		ViewDirection.xyz = _CameraWorldPosition.xyz - WorldPosition.xyz;
		ViewDirection.xyz = SafeNormalize(ViewDirection.xyz).xyz;
	}

	float4 GBufferA, GBufferB, GBufferC;
	LoadGBuffer(TexCoord, GBufferA, GBufferB, GBufferC);

	GBufferTerm GBuffer;
	DecodeGBuffer(GBufferA, GBufferB, GBufferC, GBuffer);

	uint DLightNum = _LightNum[SHADER_DIRECTIONAL_LIGHT_INDEX];
	uint PLightNum = _LightNum[SHADER_POINT_LIGHT_INDEX];
	uint SLightNum = _LightNum[SHADER_SPOT_LIGHT_INDEX];

	float3 Color = float3(0.0, 0.0, 0.0);

	[branch]
	if (DLightNum > 0u)
	{
		[unroll(SHADER_DIRECTIONAL_LIGHT_NUM_MAX)]
		for (uint LightIndex = 0u; LightIndex < DLightNum; LightIndex++)
		{
			DirectionalLightInputDatas	DLight		= _DirectionalLightInput[LightIndex];

			ShadingLightParams			LightParams	= InitShadingLightParams(DLight._WorldDirection.xyz,
				DLight._ColorIntensity.xyz, DLight._ColorIntensity.w);

			float3						HalfVector	= SafeNormalize(LightParams.Direction.xyz + ViewDirection.xyz).xyz;

			Color += ShadingDefaultLight(GBuffer, LightParams, ViewDirection.xyz, HalfVector.xyz);
		}
	}

	[branch]
	if (PLightNum > 0u)
	{
		[unroll(SHADER_POINT_LIGHT_NUM_MAX)]
		for (uint LightIndex = 0u; LightIndex < PLightNum; LightIndex++)
		{

		}
	}

	[branch]
	if (SLightNum > 0u)
	{
		[unroll(SHADER_SPOT_LIGHT_NUM_MAX)]
		for (uint LightIndex = 0u; LightIndex < SLightNum; LightIndex++)
		{

		}
	}

	OutTarget = float4(Color.rgb, 1.0);
}

#endif