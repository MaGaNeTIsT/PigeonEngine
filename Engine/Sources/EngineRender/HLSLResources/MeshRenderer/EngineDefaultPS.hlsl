#ifndef _ENGINE_DEFAULT_PS_HLSL
#define _ENGINE_DEFAULT_PS_HLSL

#include "../Common/ShaderCommon.hlsl"

#define CLOTH_HAS_SUBSURFACE_COLOR

cbuffer ConstantBufferMaterialData : register(b3)
{
	float4	_BaseColorRoughness;
	float4	_EmissiveAmbientOcclusion;
	float4	_SheenColorMetallicness;
	float4	_SubsurfaceColor;
}

float4 main(Varying input) : SV_Target
{
	float3 normalModelWS	= SafeNormalize(input.normal.xyz);
	float3 normalWS			= TransformTangentToSpaceDir(SafeNormalize(_NormalTexture.Sample(_LinearWrapSampler, input.uv0).rgb * 2 - 1), CreateTangentMatrix(normalModelWS, SafeNormalize(input.tangent.xyz)));
	float3 albedo			= _AlbedoTexture.Sample(_LinearWrapSampler, input.uv0).rgb;
	float4 property			= _PropertyTexture.Sample(_LinearWrapSampler, input.uv0).rgb;
	float3 positionWS		= input.positionWS.xyz;

	GBufferTerm GBuffer;
	GBuffer.NormalWS			= normalWS;
	GBuffer.BaseColor			= albedo * _BaseColorRoughness.rgb;
	GBuffer.Roughness			= property.b * _BaseColorRoughness.w;
	GBuffer.Emissive			= _EmissiveAmbientOcclusion.rgb;
	GBuffer.AmbientOcclusion	= property.g * _EmissiveAmbientOcclusion.w;
	GBuffer.Metallicness		= property.r * _SheenColorMetallicness.w;

	float3 viewDir = SafeNormalize(ENGINE_CAMERA_POSITION - positionWS);

	PixelParams pixelParams;
	pixelParams.PerceptualRoughnessUnclamped	= GBuffer.Roughness;
#ifdef ENABLE_CONSOLE_SHADING
	pixelParams.PerceptualRoughness				= clamp(GBuffer.Roughness, 0.045, 1.0);
#else
	pixelParams.PerceptualRoughness				= clamp(GBuffer.Roughness, 0.089, 1.0);
#endif
	pixelParams.Roughness						= pixelParams.PerceptualRoughness * pixelParams.PerceptualRoughness;
	pixelParams.DiffuseColor					= RemappingDiffuseColor(GBuffer.BaseColor, GBuffer.Metallicness);
	pixelParams.SpecularColor					= RemappingSpecularColor(GBuffer.BaseColor, GBuffer.Metallicness);
	pixelParams.F0								= RemappingFresnelf0(0.5, GBuffer.BaseColor, GBuffer.Metallicness);
	pixelParams.SheenColor						= _SheenColorMetallicness.rgb;
#ifdef CLOTH_HAS_SUBSURFACE_COLOR
	pixelParams.SubsurfaceColor					= _SubsurfaceColor.rgb;
#endif

	float4 color = 0.0;
	for (uint i = 0u; i < (uint)_LightCount.x; i++)
	{
		ShadingLightParams light;
		light.ColorIntensity	= _LightData[i].Color;
		light.Direction			= -_LightData[i].Params0.xyz;
		light.Attenuation		= 1.0;

		NormalViewLightDotParams content = InitNormalViewLightDotParams(GBuffer.NormalWS, light.Direction, viewDir);

		color.rgb += SurfaceShading_Cloth(pixelParams, content, light, GBuffer.AmbientOcclusion);
	}
	color.a = 1.0;
	return color;
}

#endif