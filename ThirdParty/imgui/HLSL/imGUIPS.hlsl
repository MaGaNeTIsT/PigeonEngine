#ifndef _IMGUI_PS_HLSL
#define _IMGUI_PS_HLSL

struct Varying
{
	float4 pos : SV_POSITION;
	float4 clr : COLOR0;
	float2 uv  : TEXCOORD0;
};

sampler		_Sampler : register(s0);
Texture2D	_Texture : register(t0);

float4 main(Varying input) : SV_Target
{
	float4 output = input.clr * _Texture.Sample(_Sampler, input.uv);
	return output;
}

#endif