#ifndef _SHADER_WATER_FIELD_PS_HLSL
#define _SHADER_WATER_FIELD_PS_HLSL

#include "../Common/ShaderFunctions.hlsl"

cbuffer ConstatntBuffer : register(b0)
{

	matrix World;
	matrix View;
	matrix Projection;

	float4 CameraPosition;
	float4 Parameter;
	matrix WorldInvTranspose;
	float4 Parameter2;

	matrix LightViewMatrix;
	matrix LightProjectionMatrix;

}

Texture2D		g_PositionMap	: register(t0);
Texture2D		g_ShadowMap		: register(t4);
Texture2D		g_Texture		: register(t6);
Texture2D		g_Depth			: register(t7);
SamplerState	g_SamplerState	: register(s0);

void main(  in  float4 inPosition		: SV_POSITION,
            in  float4 inWorldPosition  : POSITION0,
			in  float4 inNormal			: NORMAL0,
			in  float4 inTangent        : TANGENT0,
			in  float4 inBinormal       : BINORMAL0,
			in  float4 inDiffuse		: COLOR0,
			in  float2 inTexCoord		: TEXCOORD0,

			out float4 outDiffuse		: SV_Target)
{
	//GetDepthMap&PositionMap

	float2 xy = float2((inPosition.x - 0.5) / Parameter2.x, (inPosition.y - 0.5) / Parameter2.y);
	float depth = inPosition.z;
	float4 worldPos = g_PositionMap.Sample(g_SamplerState, xy);
	float distFromSurface = distance(worldPos.xyz, inWorldPosition.xyz);
	float alpha = clamp(pow(distFromSurface / 5.0, 2.0), 0.01, 0.99);

	//NormalMap

	int octave = 5;
	float Epsilon = 0.001;
	float Amplitude = 0.03;
	float offset = Parameter.x * 0.50;
	float2 uv = inTexCoord * 0.5;
	float F0 = Amplitude * fbm3(inWorldPosition.xyz, octave, offset);
	float Fx = Amplitude * fbm3(inWorldPosition.xyz + float3(Epsilon, 0.0, 0.0), octave, offset);
	float Fy = Amplitude * fbm3(inWorldPosition.xyz + float3(0.0, Epsilon, 0.0), octave, offset);
	float Fz = Amplitude * fbm3(inWorldPosition.xyz + float3(0.0, 0.0, Epsilon), octave, offset);
	float3 dF = float3((Fx - F0) / Epsilon, (Fy - F0) / Epsilon, (Fz - F0) / Epsilon);

	float3 normal = normalize(inNormal.xyz);
	normal = normalize(normal - dF);

	//Eye

	float3 eye = inWorldPosition.xyz - CameraPosition.xyz;
	eye = normalize(eye);

	//Light

	float3 lightdir = normalize(Parameter.yzw);

	//WaterSurfaceColor

	float dE = pow(saturate(dot(-eye, normal)), 3.0);
	float3 color = g_Texture.Sample(g_SamplerState, float2(clamp(1.0 - dE, 0.5, 0.99), 0.5)).rgb;

	//Lighting
	float3 A, D, S;
	int shadow = ComputeShadow(LightViewMatrix, LightProjectionMatrix, inWorldPosition.xyz, g_ShadowMap, g_SamplerState);
	if (shadow)
	{
		D = 0.0; S = 0.0;
		A = 0.2 * float3(0.15, 0.15, 0.15);
	}
	else
	{
		ComputeBlinnPhongLight(0.2, color, float3(0.4, 0.4, 0.4), 6.0, normal, lightdir, float3(0.8, 0.8, 0.8), float3(0.15, 0.15, 0.15), eye, A, D, S);
	}

	outDiffuse.rgb = A + D + S;

	float fresnel = pow(1.0 - saturate(dot(-eye, normal)), 5.0);

	outDiffuse.rgb = float3(1.0, 1.0, 1.0) * fresnel + outDiffuse.rgb * (1.0 - fresnel);
	outDiffuse.a = clamp(lerp(0.0, 1.0, fresnel) + alpha, 0.0, 1.0);
}

#endif