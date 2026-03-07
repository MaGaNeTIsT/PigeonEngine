//#ifndef _ENGINE_DEFAULT_CLOTH_VS_HLSL
//#define _ENGINE_DEFAULT_CLOTH_VS_HLSL
//
//#include "../Common/ShaderCommon.hlsl"
//
//VaryingForward main(AttributeForward input)
//{
//	VaryingForward output;
//
//	output.positionWS		= float4(TransformObjectToWorld(input.position.xyz), 1.0);
//	output.positionCS		= TransformWorldToClip(output.positionWS.xyz);
//	output.positionSS		= output.positionCS.xyzw / output.positionCS.w;
//	output.positionSS.xy	= output.positionSS.xy * float2(0.5, -0.5) + 0.5;
//	output.normal			= TransformObjectToWorldNormal(input.normal.xyz).xyzz;
//	output.tangent			= TransformObjectToWorldDir(input.tangent.xyz).xyzz;
//	output.uv0				= input.uv0;
//
//	return output;
//}
//
//#endif