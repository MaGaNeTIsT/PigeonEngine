//#ifndef _ENGINE_DEFAULT_TERRAIN_PS_HLSL
//#define _ENGINE_DEFAULT_TERRAIN_PS_HLSL
//
//#include "../Common/ShaderCommon.hlsl"
//
//float4 main(VaryingTerrain input) : SV_Target
//{
//	float3 normalModelWS	= SafeNormalize(input.normal.xyz);
//	float  ao				= _GlobalAOInput.Sample(_LinearClampSampler, (input.positionCS.xy + _CameraViewportMinSizeAndInvBufferSize.xy) * _CameraViewportMinSizeAndInvBufferSize.zw).r;
//	float3 normalWS			= TransformTangentToSpaceDir(SafeNormalize(_CustomTextureA.Sample(_LinearWrapSampler, input.uv0).rgb * 2.0 - 1.0), CreateTangentMatrix(normalModelWS, SafeNormalize(input.tangent.xyz)));
//	float3 albedo			= _CustomTextureB.Sample(_LinearWrapSampler, input.uv0).rgb;
//	float4 property			= _CustomTextureC.Sample(_LinearWrapSampler, input.uv0).rgba;
//
//	float3 indirectDiffuse = 0.4 * albedo * ao;
//
//	float4 color = float4(indirectDiffuse, 1.0);
//	for (uint i = 0u; i < (uint)_LightCount.x; i++)
//	{
//		float NdotL = saturate(dot(normalWS, -_LightData[i].Params0.xyz));
//		color.rgb += albedo * _LightData[i].Color.rgb * _LightData[i].Color.w * NdotL;
//	}
//	return color;
//}
//
//#endif