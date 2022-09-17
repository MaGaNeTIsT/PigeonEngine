#ifndef _ENGINE_DEFAULT_PS_HLSL
#define _ENGINE_DEFAULT_PS_HLSL

#include "../Common/ShaderCommon.hlsl"

float4 main(Varying input) : SV_Target
{
	float3 normalModelWS	= SafeNormalize(input.normal.xyz);
	float3 normalWS			= TransformTangentToSpaceDir(SafeNormalize(_NormalTexture.Sample(_LinearWrapSampler, input.uv0).rgb * 2 - 1), CreateTangentMatrix(normalModelWS, SafeNormalize(input.tangent.xyz)));
	float3 albedo			= _AlbedoTexture.Sample(_LinearWrapSampler, input.uv0).rgb;
	float4 property			= _PropertyTexture.Sample(_LinearWrapSampler, input.uv0).rgba;

	float4 color = 0;
	for (uint i = 0u; i < (uint)_DirectionalLightCount; i++)
	{
		float NdotL = saturate(dot(normalWS, -_DirectionalLightData[i].forward.xyz));
		color.rgb += albedo * NdotL;
	}
	color.a = 1;
	return color;
}

#endif