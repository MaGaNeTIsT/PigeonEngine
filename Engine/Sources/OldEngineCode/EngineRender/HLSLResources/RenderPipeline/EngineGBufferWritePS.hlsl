//#ifndef _ENGINE_GBUFFER_WRITE_PS_HLSL
//#define _ENGINE_GBUFFER_WRITE_PS_HLSL
//
//#include "../Common/ShaderCommon.hlsl"
//
//DeferredOutput main(Varying input)
//{
//	float3 normalModelWS	= SafeNormalize(input.normal.xyz);
//	float  ao				= _GlobalAOInput.Sample(_LinearClampSampler, (input.positionCS.xy + _CameraViewportMinSizeAndInvBufferSize.xy) * _CameraViewportMinSizeAndInvBufferSize.zw).r;
//	float3 normalWS			= TransformTangentToSpaceDir(SafeNormalize(_CustomTextureA.Sample(_LinearWrapSampler, input.uv0).rgb * 2 - 1), CreateTangentMatrix(normalModelWS, SafeNormalize(input.tangent.xyz)));
//	float3 albedo			= _CustomTextureB.Sample(_LinearWrapSampler, input.uv0).rgb;
//	float4 property			= _CustomTextureC.Sample(_LinearWrapSampler, input.uv0).rgba;
//
//	ao = property.g * ao;
//
//	DeferredOutput output;
//	output.color	= float4(0.4 * albedo * ao, 1);
//	output.normal	= float4(normalWS, 1);
//	output.albedo	= float4(albedo, 1);
//	output.property = property;
//	return output;
//}
//
//#endif