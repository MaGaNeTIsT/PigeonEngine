#ifndef _SHADER_FIELD_VS_HLSL
#define _SHADER_FIELD_VS_HLSL

#include "../Common/ShaderFunctions.hlsl"

cbuffer ConstatntBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
}

FieldpInput main(FieldvInput vin)
{
	FieldpInput vout;

	matrix wvp;
	wvp = mul(World, View);
	wvp = mul(wvp, Projection);

	vin.TexCoord *= 0.1;
	vin.Position.y = fbm2(vin.TexCoord, 5) * 17.5;

	vout.Position = mul(vin.Position, wvp);
	vout.WorldPosition = mul(vin.Position, World);

	vout.Diffuse = vin.Diffuse;

	vout.TexCoord = vin.TexCoord;

	return vout;
}

#endif