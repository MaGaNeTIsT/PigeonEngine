#ifndef _ENGINE_DIRECT_LIGHT_PS_HLSL
#define _ENGINE_DIRECT_LIGHT_PS_HLSL

#include "../Common/ShaderCommon.hlsl"

float4 main(VaryingScreenPolygon2D input) : SV_Target
{
	float3 normal = SafeNormalize(_EngineGBufferNormal.Sample(_LinearClampSampler,input.uv0).rgb);
	float3 albedo = _EngineGBufferAlbedo.Sample(_LinearClampSampler, input.uv0).rgb;
	float4 property = _EngineGBufferProperty.Sample(_LinearClampSampler, input.uv0).rgba;
	
	float4 color = 0;
	for (uint i = 0u; i < (uint)_DirectionalLightCount; i++)
	{
		float NdotL = saturate(dot(normal, -_DirectionalLightData[i].forward.xyz));
		color.rgb += albedo * _DirectionalLightData[i].color.rgb * NdotL;
	}
	color.a = 1;
	return color;
}

#endif