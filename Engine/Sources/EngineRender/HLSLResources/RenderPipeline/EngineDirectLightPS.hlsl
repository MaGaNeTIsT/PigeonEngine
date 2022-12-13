#ifndef _ENGINE_DIRECT_LIGHT_PS_HLSL
#define _ENGINE_DIRECT_LIGHT_PS_HLSL

#include "../Common/ShaderCommon.hlsl"

float4 main(VaryingScreenPolygon2D input) : SV_Target
{
	float3 normal = SafeNormalize(_EngineGBufferA.Sample(_LinearClampSampler,input.uv0).rgb);
	float3 albedo = _EngineGBufferB.Sample(_LinearClampSampler, input.uv0).rgb;
	float4 property = _EngineGBufferC.Sample(_LinearClampSampler, input.uv0).rgba;
	
	float4 color = 0.0;
	for (uint i = 0u; i < (uint)_LightCount.x; i++)
	{
		float NdotL = saturate(dot(normal, -_LightData[i].Params0.xyz));
		color.rgb += albedo * _LightData[i].Color.rgb * _LightData[i].Color.w * NdotL;
	}
	color.a = 1.0;
	return color;
}

#endif