#ifndef _ENGINE_DEFAULT_CLOTH_PS_HLSL
#define _ENGINE_DEFAULT_CLOTH_PS_HLSL

#define CLOTH_HAS_SUBSURFACE_COLOR

#include "../Common/ShaderCommon.hlsl"

Texture2D	_RoughnessTexture			: register(t2);
Texture2D	_EmissiveTexture			: register(t8);
Texture2D	_SheenColorTexture			: register(t9);
Texture2D	_MetallicnessTexture		: register(t10);
Texture2D	_AmbientOcclusionTexture	: register(t11);

cbuffer ConstantBufferMaterialData : register(b3)
{
	float4	_BaseColorRoughness;
	float4	_EmissiveAmbientOcclusion;
	float4	_SheenColorMetallicness;
	float4	_SubsurfaceColor;
}

float4 main(VaryingCloth input) : SV_Target
{
	float3 normalModelWS		= SafeNormalize(input.normal.xyz);
	float3 normalWS				= TransformTangentToSpaceDir(SafeNormalize(_NormalTexture.Sample(_LinearWrapSampler, input.uv0).rgb * 2 - 1), CreateTangentMatrix(normalModelWS, SafeNormalize(input.tangent.xyz)));
	float  globalAO				= _GlobalAOInput.Sample(_LinearClampSampler, input.positionSS.xy).r;
	float3 materialAlbedo		= _AlbedoTexture.Sample(_LinearWrapSampler, input.uv0).rgb;
	float3 materialEmissive		= _EmissiveTexture.Sample(_LinearWrapSampler, input.uv0).rgb;
	float3 materialSheenColor	= _SheenColorTexture.Sample(_LinearWrapSampler, input.uv0).rgb;
	//float3 materialProperty		= _PropertyTexture.Sample(_LinearWrapSampler, input.uv0).rgb;
	float  materialRoughness	= _RoughnessTexture.Sample(_LinearWrapSampler, input.uv0).r;
	float  materialMetallicness	= _MetallicnessTexture.Sample(_LinearWrapSampler, input.uv0).r;
	float  materialAO			= _AmbientOcclusionTexture.Sample(_LinearWrapSampler, input.uv0).r;

	float3 positionWS		= input.positionWS.xyz;

	GBufferTerm GBuffer;
	GBuffer.NormalWS			= normalWS;
	GBuffer.BaseColor			= materialAlbedo * _BaseColorRoughness.rgb;
	GBuffer.Emissive			= _EmissiveAmbientOcclusion.rgb * materialEmissive;
	//GBuffer.Roughness			= materialProperty.b * _BaseColorRoughness.w;
	//GBuffer.Metallicness		= materialProperty.r * _SheenColorMetallicness.w;
	//GBuffer.AmbientOcclusion	= materialProperty.g * _EmissiveAmbientOcclusion.w * globalAO;
	GBuffer.Roughness			= materialRoughness * _BaseColorRoughness.w;
	GBuffer.Metallicness		= materialMetallicness * _SheenColorMetallicness.w;
	GBuffer.AmbientOcclusion	= materialAO * _EmissiveAmbientOcclusion.w * globalAO;

	float3 viewDir = SafeNormalize(GetCameraWorldPosition() - positionWS);

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
	pixelParams.SheenColor						= _SheenColorMetallicness.rgb * materialSheenColor * pixelParams.SpecularColor;
#ifdef CLOTH_HAS_SUBSURFACE_COLOR
	pixelParams.SubsurfaceColor					= _SubsurfaceColor.rgb;
#endif

	float4 color = float4((0.2 * GBuffer.BaseColor * GBuffer.AmbientOcclusion) + GBuffer.Emissive, 1.0);
	for (uint i = 0u; i < (uint)_LightCount.x; i++)
	{
		ShadingLightParams light;
		light.ColorIntensity	= _LightData[i].Color;
		light.Direction			= -_LightData[i].Params0.xyz;
		light.Attenuation		= 1.0;

		NormalViewLightDotParams content = InitNormalViewLightDotParams(GBuffer.NormalWS, light.Direction, viewDir);

		color.rgb += SurfaceShading_Cloth(pixelParams, content, light, GBuffer.AmbientOcclusion);
	}
	return color;
}

#endif