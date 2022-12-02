#ifndef _ENGINE_DEFAULT_CLOTH_PS_HLSL
#define _ENGINE_DEFAULT_CLOTH_PS_HLSL

#ifndef MATERIAL_HAS_NORMALMAP
#define MATERIAL_HAS_NORMALMAP
#endif
#ifndef ENABLE_BURLEY_DIFFUSE
#define ENABLE_BURLEY_DIFFUSE
#endif
#ifdef MATERIAL_HAS_ANISOTROPY
#undef MATERIAL_HAS_ANISOTROPY
#endif
#ifdef MATERIAL_HAS_CLEAR_COAT
#undef MATERIAL_HAS_CLEAR_COAT
#endif
#ifndef MATERIAL_HAS_SHEEN_COLOR
#define MATERIAL_HAS_SHEEN_COLOR
#endif
#ifndef MATERIAL_HAS_SUBSURFACE_COLOR
#define MATERIAL_HAS_SUBSURFACE_COLOR
#endif

#include "../Common/ShaderCommon.hlsl"

Texture2D	_RoughnessTexture			: register(t9);
Texture2D	_AmbientOcclusionTexture	: register(t10);

cbuffer ConstantBufferMaterialData : register(b3)
{
	float4	_BaseColorRoughness;
	float4	_EmissiveAmbientOcclusion;
	float4	_SheenColor;
	float4	_SubsurfaceColor;
}

float4 main(VaryingForward input) : SV_Target
{
	float    globalAO				= _GlobalAOInput.Sample(_LinearClampSampler, input.positionSS.xy).r;
	float3   geometricNormalWS		= SafeNormalize(input.normal.xyz);
	float3x3 tangentToWorld			= CreateTangentMatrix(geometricNormalWS, SafeNormalize(input.tangent.xyz), true);

	float3   materialNormalWS		= TransformTangentToSpaceDir(SafeNormalize(_CustomTextureA.Sample(_PointWrapSampler, input.uv0).rgb * 2.0 - 1.0), tangentToWorld);
	float3   materialAlbedo			= _CustomTextureB.Sample(_LinearWrapSampler, input.uv0).rgb * _BaseColorRoughness.rgb;
	float    materialRoughness		= _RoughnessTexture.Sample(_LinearWrapSampler, input.uv0).r * _BaseColorRoughness.w;
	float3   materialEmissive		= _CustomTextureC.Sample(_LinearWrapSampler, input.uv0).rgb * _EmissiveAmbientOcclusion.rgb;
	float    materialAO				= _AmbientOcclusionTexture.Sample(_LinearWrapSampler, input.uv0).r * _EmissiveAmbientOcclusion.w;
#ifdef MATERIAL_HAS_SHEEN_COLOR
	float3   materialSheenColor		= _CustomTextureD.Sample(_LinearWrapSampler, input.uv0).rgb * _SheenColor.rgb;
#endif
#ifdef MATERIAL_HAS_SUBSURFACE_COLOR
	float3   materialSubsurfaceClr	= _SubsurfaceColor.rgb;
#endif

	float3 positionWS	= input.positionWS.xyz;
	float3 viewDirWS	= SafeNormalize(GetCameraWorldPosition() - positionWS);

	GBufferTerm GBuffer;
	GBuffer.NormalWS			= materialNormalWS;
	GBuffer.BaseColor			= materialAlbedo;
	GBuffer.Emissive			= materialEmissive;
	GBuffer.Roughness			= materialRoughness;
	GBuffer.AmbientOcclusion	= materialAO * globalAO;

	PixelParams pixelParams;
	pixelParams.GeometricNormalWS				= geometricNormalWS;
	pixelParams.ShadingNormalWS					= GBuffer.NormalWS;
	pixelParams.EmissiveColor					= GBuffer.Emissive;
	pixelParams.AmbientOcclusion				= GBuffer.AmbientOcclusion;
	pixelParams.PerceptualRoughnessUnclamped	= GBuffer.Roughness;
	pixelParams.PerceptualRoughness				= clamp(pixelParams.PerceptualRoughnessUnclamped, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
	pixelParams.Roughness						= ComputePerceptualRoughnessToRoughness(pixelParams.PerceptualRoughness);
	pixelParams.DiffuseColor					= GBuffer.BaseColor;
#ifdef MATERIAL_HAS_SHEEN_COLOR
	pixelParams.SheenColor						= materialSheenColor;
#endif
#ifdef MATERIAL_HAS_SUBSURFACE_COLOR
	pixelParams.SubsurfaceColor					= materialSubsurfaceClr;
#endif

	float4 color = float4((0.01 * pixelParams.DiffuseColor * pixelParams.AmbientOcclusion) + pixelParams.EmissiveColor, 1.0);
	for (uint i = 0u; i < (uint)_LightCount.x; i++)
	{
		ShadingLightParams light;
		light.ColorIntensity	= _LightData[i].Color;
		light.Direction			= -_LightData[i].Params0.xyz;
		light.Attenuation		= 1.0;

		float3 halfVecWS = SafeNormalize(light.Direction + viewDirWS);

		float shadow = 0.0;
		{
			float4 shadowUV = mul(float4(positionWS, 1.0), _LightViewProjectionMatrix);
			shadowUV = shadowUV / shadowUV.w;
			shadowUV.xy = shadowUV.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
			uint2 mapSize;
			_EngineLightShadowMap0.GetDimensions(mapSize.x, mapSize.y);
			float2 mapOffset = 1.0 / float2(mapSize);
			[unroll]
			for (int y = -1; y <= 1; y++)
			{
				[unroll]
				for (int x = -1; x <= 1; x++)
				{
					float2 tempUV = float2(x, y) * mapOffset + shadowUV.xy;
					float lightZ = _EngineLightShadowMap0.Sample(_LinearClampSampler, tempUV).r;
					bool outScreen = any(tempUV.xy < 0.0) || any(tempUV.xy > 1.0);
					float tempShadow = shadowUV.z < (lightZ + 1e-2) ? 1.0 : 0.0;
					shadow += outScreen ? 1.0 : tempShadow;
				}
			}
			shadow /= 9.0;
		}

		NormalViewLightDotParams content = InitNormalViewLightDotParams(pixelParams.ShadingNormalWS, light.Direction, viewDirWS, halfVecWS);

		color.rgb += SurfaceShading_Cloth(pixelParams, content, light, shadow);
	}
	return color;
}

#endif