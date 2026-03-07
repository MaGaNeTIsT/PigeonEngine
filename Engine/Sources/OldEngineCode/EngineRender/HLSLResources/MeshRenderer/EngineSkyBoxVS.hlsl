//#ifndef _ENGINE_SKY_BOX_VS_HLSL
//#define _ENGINE_SKY_BOX_VS_HLSL
//
//#include "../Common/ShaderCommon.hlsl"
//
//VaryingSkyBox main(AttributeSkyBox input)
//{
//	float4 positionCS = float4(input.position.xy * _CameraViewportMinSizeAndInvBufferSize.zw * 2.0 - 1.0, 1.0, 1.0);
//	positionCS.y = -positionCS.y;
//	VaryingSkyBox output;
//	output.positionCS = positionCS;
//	output.uv0 = input.uv0;
//	return output;
//}
//
//#endif