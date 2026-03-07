#ifndef _FULL_SCREEN_TRIANGLE_PE_PS_HLSL
#define _FULL_SCREEN_TRIANGLE_PE_PS_HLSL

SamplerState	_PointClampSampler	: register(s0);
SamplerState	_PointWrapSampler	: register(s1);
SamplerState	_LinearClampSampler	: register(s2);
SamplerState	_LinearWrapSampler	: register(s3);
Texture2D		_InTexture			: register(t0);

void main(in float4 InPosition : SV_Position, in float2 InTexcoord : TEXCOORD0, out float4 OutTarget : SV_Target)
{
    OutTarget = float4(_InTexture.Sample(_LinearClampSampler, InTexcoord).xyz, 1.0);
}

#endif