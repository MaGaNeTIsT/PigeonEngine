#ifndef _FULL_SCREEN_TRIANGLE_PE_VS_HLSL
#define _FULL_SCREEN_TRIANGLE_PE_VS_HLSL

void main(in float2 InPosition : TEXCOORD0, out float4 OutPosition : SV_Position, out float2 OutTexcoord : TEXCOORD0)
{
	OutPosition = float4(InPosition.xy * 2.0 - 1.0, 0.0, 1.0);
	OutTexcoord = float2(InPosition.x, 1.0 - InPosition.y);
}

#endif