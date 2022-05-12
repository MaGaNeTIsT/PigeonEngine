#ifndef _SHADER_FIELD_PS_HLSL
#define _SHADER_FIELD_PS_HLSL

#include "../Common/ShaderFunctions.hlsl"

cbuffer ConstatntBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;

    float4 CameraPosition;
	float4 Parameter;
}

FieldFrontpOutput main(FieldpInput pin)
{
	FieldFrontpOutput pout;

	int octave = 10;

	float dx = fbm2(pin.TexCoord + float2(0.0001, 0.0), octave) - fbm2(pin.TexCoord + float2(-0.0001, 0.0), octave);
	float dz = fbm2(pin.TexCoord + float2(0.0, 0.0001), octave) - fbm2(pin.TexCoord + float2(0.0, -0.0001), octave);

	float3 normal = float3(dx * 3000.0, 1.0, -dz * 3000.0);
	normal = normalize(normal);

	float3 lightDir = Parameter.yzw;
	lightDir = normalize(lightDir);
	//float light = saturate(0.5 - dot(normal, lightDir) * 0.5);

	float light = -dot(lightDir, normal);

	
	float3 diffuse = float3(0.627451, 0.321568, 0.176470);
	diffuse *= light;

	float dist = distance(pin.WorldPosition, CameraPosition);

	float3 eye = pin.WorldPosition.xyz - CameraPosition.xyz;
	eye = normalize(eye);

	float m;
	m = saturate(-dot(lightDir, eye));
	m = pow(m, 50);

	diffuse += m * dist * 0.003;

	float3 vy = float3(0.0, 1.0, 0.0);
	float atm = saturate(1.0 - dot(-lightDir, vy));
	float3 rcolor = 1.0 - float3(0.5, 0.8, 1.0) * atm;

	float ld = 0.5 - dot(lightDir, eye) * 0.5;
	diffuse += rcolor * dist * ld * float3(0.5, 0.8, 1.0) * 0.004;

	pout.Diffuse.rgb = diffuse;
	pout.Diffuse.a = 1.0;

	return pout;
}

#endif