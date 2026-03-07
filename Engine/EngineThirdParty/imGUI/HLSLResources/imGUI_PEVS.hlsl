#ifndef _IMGUI_VS_HLSL
#define _IMGUI_VS_HLSL

struct Attribute
{
	float2	Position	: POSITION;
	float2	Texcoord	: TEXCOORD0;
	float4	Color		: COLOR0;
};

struct Varying
{
	float4	Position	: SV_POSITION;
	float2	Texcoord	: TEXCOORD0;
	float4	Color		: COLOR0;
};

cbuffer ConstantBuffer : register(b0)
{
	float4x4 _ProjectionMatrix;
};

void main(in Attribute Input, out Varying Output)
{
	Output.Position	= mul(_ProjectionMatrix, float4(Input.Position.xy, 0.0, 1.0));
	Output.Texcoord	= Input.Texcoord;
	Output.Color	= Input.Color;
}

#endif