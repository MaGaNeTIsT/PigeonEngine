#ifndef _IMGUI_PS_HLSL
#define _IMGUI_PS_HLSL

struct Varying
{
	float4	Position	: SV_POSITION;
	float2	Texcoord	: TEXCOORD0;
	float4	Color		: COLOR0;
};

Texture2D	_InTexture	: register(t0);
sampler		_InSampler	: register(s0);

void main(in Varying Input, out float4 Output : SV_Target)
{
	Output = Input.Color * _InTexture.Sample(_InSampler, Input.Texcoord);
}

#endif