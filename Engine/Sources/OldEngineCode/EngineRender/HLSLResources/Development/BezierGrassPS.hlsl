//#ifndef _ENGINE_BEZIER_GRASS_PS_HLSL
//#define _ENGINE_BEZIER_GRASS_PS_HLSL
//
//#include "./BezierGrassCommon.hlsl"
//
//cbuffer BezierGrassPropertyConstantBuffer : register(b2)
//{
//	float4	_Params0;
//	float4	_RootColor;
//	float4	_TipColor;
//}
//
//float4 main(Varying Input, bool IsFrontFace : SV_IsFrontFace) : SV_Target
//{
//	float BladeGray = Input.NormalWS.w;
//	float3 GrassNormalWS = SafeNormalize(IsFrontFace ? Input.NormalWS.xyz : -Input.NormalWS.xyz);
//	float3 GrassColor = lerp(_RootColor.rgb, _TipColor.rgb, BladeGray);
//
//	float4 Color = float4(0.0.xxx, 1.0);
//#if 0
//	for (uint i = 0u; i < (uint)_LightCount.x; i++)
//	{
//		ShadingLightParams L;
//		L.ColorIntensity	= _LightData[i].Color;
//		L.Direction			= -_LightData[i].Params0.xyz;
//		L.Attenuation		= 1.0;
//
//		Color.rgb += GrassColor.rgb * L.ColorIntensity.rgb * saturate(dot(L.Direction.xyz, GrassNormalWS.xyz)) * L.Attenuation;
//		//Color.rgb += saturate(dot(L.Direction.xyz, GrassNormalWS.xyz));
//	}
//#else
//	Color.rgb = GrassColor;
//#endif
//
//	return Color;
//}
//
//#endif