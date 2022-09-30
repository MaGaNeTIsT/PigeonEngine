#ifndef _ENGINE_DEFAULT_SKY_BOX_VS_HLSL
#define _ENGINE_DEFAULT_SKY_BOX_VS_HLSL

#include "../Common/ShaderCommon.hlsl"

void main(in float4 pos : POSITION0, in float2 uv0 : TEXCOORD0, out float4 posCS : SV_POSITION, out float2 uv : TEXCOORD0)
{
	posCS = float4(pos.xy * _CameraViewportMinSizeAndInvBufferSize.zw * 2.0 - 1.0, 1.0, 1.0);
	posCS.y = -posCS.y;
	uv = uv0;
}

#endif