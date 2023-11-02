#ifndef _DEBUG_WIREFRAME_PRIMITIVE_PE_PS_HLSL
#define _DEBUG_WIREFRAME_PRIMITIVE_PE_PS_HLSL

void main(in float4 InPosition : SV_Position, in float4 InColor : COLOR0, out float4 OutTarget : SV_Target)
{
    OutTarget = float4(InColor.rgb, 1.0);
}

#endif