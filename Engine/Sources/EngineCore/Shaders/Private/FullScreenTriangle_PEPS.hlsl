#ifndef _FULL_SCREEN_TRIANGLE_PE_PS_HLSL
#define _FULL_SCREEN_TRIANGLE_PE_PS_HLSL

Texture2D	_InTexture : register(t0);
sampler		_InSampler : register(s0);

void main(in float4 InPosition : SV_Position, in float2 InTexcoord : TEXCOORD0, out float4 OutTarget : SV_Target)
{
	OutTarget = float4(_InTexture.Sample(_InSampler, InTexcoord).xyz, 1.0);
}

#endif