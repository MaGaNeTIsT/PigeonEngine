#ifndef _SHADER_FIELD_DEFERRED_PS_HLSL
#define _SHADER_FIELD_DEFERRED_PS_HLSL

#include "../Common/ShaderFunctions.hlsl"

cbuffer ConstatntBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;

	float4 CameraPosition;
	float4 Parameter;
}

FieldDeferredpOutput main(FieldpInput pin)
{
	FieldDeferredpOutput pout;

	int octave = 10;

	float dx = fbm2(pin.TexCoord + float2(0.0001, 0.0), octave) - fbm2(pin.TexCoord + float2(-0.0001, 0.0), octave);
	float dz = fbm2(pin.TexCoord + float2(0.0, 0.0001), octave) - fbm2(pin.TexCoord + float2(0.0, -0.0001), octave);

	float3 normal = float3(dx * 3000.0, 1.0, -dz * 3000.0);
	normal = normalize(normal);

	float amibent = 0.4;
	float3 diffuse = float3(0.627451, 0.321568, 0.176470);
	float4 specular = float4(0.2, 0.2, 0.2, 5.0);

	pout.WorldPosition = float4(pin.WorldPosition.xyz, pin.Position.z); //1.0 - (((pin.Position.z / pin.Position.w) + 1.0) * 0.5)
	pout.WorldNormal = float4(normal, 1.0);
	pout.Diffuse = float4(diffuse, amibent);
	pout.Specular = specular;

	return pout;
}

#endif