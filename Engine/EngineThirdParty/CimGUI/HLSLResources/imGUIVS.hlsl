#ifndef _IMGUI_VS_HLSL
#define _IMGUI_VS_HLSL

struct Attribute
{
	float2 pos : POSITION;
	float4 clr : COLOR0;
	float2 uv  : TEXCOORD0;
};

struct Varying
{
	float4 pos : SV_POSITION;
	float4 clr : COLOR0;
	float2 uv  : TEXCOORD0;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 _ProjectionMatrix;
};

Varying main(Attribute input)
{
	Varying output;
	output.pos = mul(_ProjectionMatrix, float4(input.pos.xy, 0.0, 1.0));
	output.clr = input.clr;
	output.uv = input.uv;
	return output;
}

#endif