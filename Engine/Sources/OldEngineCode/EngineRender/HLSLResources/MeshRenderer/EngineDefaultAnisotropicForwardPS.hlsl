//#ifndef _ENGINE_DEFAULT_ANISOTROPIC_FORWARD_PS_HLSL
//#define _ENGINE_DEFAULT_ANISOTROPIC_FORWARD_PS_HLSL
//
//#ifndef MATERIAL_HAS_NORMALMAP
//#define MATERIAL_HAS_NORMALMAP
//#endif
//#ifndef ENABLE_BURLEY_DIFFUSE
//#define ENABLE_BURLEY_DIFFUSE
//#endif
//#ifndef MATERIAL_HAS_ANISOTROPY
//#define MATERIAL_HAS_ANISOTROPY
//#endif
//#ifdef MATERIAL_HAS_CLEAR_COAT
//#undef MATERIAL_HAS_CLEAR_COAT
//#endif
//#ifdef MATERIAL_HAS_CLEAR_COAT_NORMAL
//#undef MATERIAL_HAS_CLEAR_COAT_NORMAL
//#endif
//#ifdef MATERIAL_HAS_SHEEN_COLOR
//#undef MATERIAL_HAS_SHEEN_COLOR
//#endif
//#ifdef MATERIAL_HAS_SUBSURFACE_COLOR
//#undef MATERIAL_HAS_SUBSURFACE_COLOR
//#endif
//
//#include "../Common/ShaderCommon.hlsl"
//
//Texture2D	_RoughnessTexture				: register(t9);
//Texture2D	_MetallicnessTexture			: register(t10);
//Texture2D	_ReflectanceTexture				: register(t11);
//Texture2D	_AnisotropyTexture				: register(t12);
//Texture2D	_AnisotropyDirectionTexture		: register(t13);
//
//cbuffer ConstantBufferMaterialData : register(b3)
//{
//	float4	_BaseColorRoughness;
//	float4	_EmissiveAmbientOcclusion;
//	float4	_MetallicnessReflectanceIsGlossy;
//	float4	_AnisotropyStrengthDirection;
//}
//
//float GetShadowGaussian(int x, int y)
//{
//	const static float _ShadowGaussian[25] = {
//		0.0159325, 0.0292619, 0.0358351, 0.0292619, 0.0159325,
//		0.0292619, 0.053743,  0.0658155, 0.053743,  0.0292619,
//		0.0358351, 0.0658155, 0.0806,    0.0658155, 0.0358351,
//		0.0292619, 0.053743,  0.0658155, 0.053743,  0.0292619,
//		0.0159325, 0.0292619, 0.0358351, 0.0292619, 0.0159325 };
//	return (_ShadowGaussian[(y + 2) * 5 + (x + 2)]);
//}
//
//float4 main(VaryingForward input) : SV_Target
//{
//	float    globalAO				= _GlobalAOInput.Sample(_LinearClampSampler, input.positionSS.xy).r;
//	float3   geometricNormalWS		= SafeNormalize(input.normal.xyz);
//	float3   geometricTangentWS		= SafeNormalize(input.tangent.xyz);
//	float3   geometricBinormalWS	= float3(0.0, 1.0, 0.0);
//	float3x3 tangentToWorld			= CreateTangentMatrix(geometricNormalWS, geometricTangentWS, geometricBinormalWS, true);
//
//	float3   materialNormalWS		= TransformTangentToSpaceDir(SafeNormalize(_CustomTextureA.Sample(_LinearWrapSampler, input.uv0).rgb * 2.0 - 1.0), tangentToWorld);
//	float3   materialAlbedo			= _CustomTextureB.Sample(_LinearWrapSampler, input.uv0).rgb * _BaseColorRoughness.rgb;
//	float    materialRoughness		= saturate(abs(_RoughnessTexture.Sample(_LinearWrapSampler, input.uv0).r - _MetallicnessReflectanceIsGlossy.z)) * _BaseColorRoughness.w;
//	float3   materialEmissive		= _CustomTextureC.Sample(_LinearWrapSampler, input.uv0).rgb * _EmissiveAmbientOcclusion.rgb;
//	float    materialAO				= _CustomTextureD.Sample(_LinearWrapSampler, input.uv0).r * _EmissiveAmbientOcclusion.w;
//	float    materialMetallicness	= _MetallicnessTexture.Sample(_LinearWrapSampler, input.uv0).r * _MetallicnessReflectanceIsGlossy.x;
//	float    materialReflectance	= ComputeFresnelF0Dielectric(_ReflectanceTexture.Sample(_LinearWrapSampler, input.uv0).r * _MetallicnessReflectanceIsGlossy.y);
//#ifdef MATERIAL_HAS_ANISOTROPY
//	float    materialAnisotropy		= _AnisotropyTexture.Sample(_LinearWrapSampler, input.uv0).r * _AnisotropyStrengthDirection.x;
//	float    materialAnisotropyDir	= _AnisotropyDirectionTexture.Sample(_LinearWrapSampler, input.uv0).r * _AnisotropyStrengthDirection.y;
//	float3   materialAnisotropicT	= SafeNormalize(lerp(geometricBinormalWS, geometricTangentWS, materialAnisotropyDir));
//	float3   materialAnisotropicB	= cross(geometricNormalWS, materialAnisotropicT);
//#endif
//
//	float3 positionWS	= input.positionWS.xyz;
//	float3 viewDirWS	= SafeNormalize(GetCameraWorldPosition() - positionWS);
//
//	GBufferTerm GBuffer;
//	GBuffer.NormalWS			= materialNormalWS;
//	GBuffer.BaseColor			= materialAlbedo;
//	GBuffer.Emissive			= materialEmissive;
//	GBuffer.Roughness			= materialRoughness;
//	GBuffer.Metallicness		= materialMetallicness;
//	GBuffer.AmbientOcclusion	= materialAO * globalAO;
//
//	PixelParams pixelParams;
//	pixelParams.GeometricNormalWS				= geometricNormalWS;
//	pixelParams.ShadingNormalWS					= GBuffer.NormalWS;
//	pixelParams.EmissiveColor					= GBuffer.Emissive;
//	pixelParams.AmbientOcclusion				= GBuffer.AmbientOcclusion;
//	pixelParams.PerceptualRoughnessUnclamped	= GBuffer.Roughness;
//	pixelParams.PerceptualRoughness				= clamp(pixelParams.PerceptualRoughnessUnclamped, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
//	pixelParams.Roughness						= ComputePerceptualRoughnessToRoughness(pixelParams.PerceptualRoughness);
//	pixelParams.DiffuseColor					= ComputeDiffuseColor(GBuffer.BaseColor, GBuffer.Metallicness);
//	pixelParams.F0								= ComputeFresnelF0(materialReflectance, GBuffer.BaseColor, GBuffer.Metallicness);
//#ifdef MATERIAL_HAS_ANISOTROPY
//	pixelParams.Anisotropy						= materialAnisotropy;
//	pixelParams.AnisotropyT						= materialAnisotropicT;
//	pixelParams.AnisotropyB						= materialAnisotropicB;
//#endif
//
//	//float4 color = float4((0.1 * pixelParams.DiffuseColor * pixelParams.AmbientOcclusion) + pixelParams.EmissiveColor, 1.0);
//	float4 color = float4(pixelParams.EmissiveColor, 1.0);
//	for (uint i = 0u; i < (uint)_LightCount.x; i++)
//	{
//		ShadingLightParams light;
//		light.ColorIntensity	= _LightData[i].Color;
//		light.Direction			= -_LightData[i].Params0.xyz;
//		light.Attenuation		= 1.0;
//
//		float3 halfVecWS = SafeNormalize(light.Direction + viewDirWS);
//
//		float shadow = 0.0;
//		{
//			float4 shadowUV = mul(float4(positionWS, 1.0), _LightViewProjectionMatrix);
//			shadowUV = shadowUV / shadowUV.w;
//			shadowUV.xy = shadowUV.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
//			uint2 mapSize;
//			_EngineLightShadowMap0.GetDimensions(mapSize.x, mapSize.y);
//			float2 mapOffset = 1.0 / float2(mapSize);
//			[unroll]
//			for (int y = -2; y <= 2; y++)
//			{
//				[unroll]
//				for (int x = -2; x <= 2; x++)
//				{
//					float2 tempUV = float2(x, y) * mapOffset + shadowUV.xy;
//					float lightZ = _EngineLightShadowMap0.Sample(_LinearClampSampler, tempUV).r;
//					bool outScreen = any(tempUV.xy < 0.0) || any(tempUV.xy > 1.0);
//					float tempShadow = shadowUV.z < (lightZ + 1e-2) ? 1.0 : 0.0;
//					shadow += (outScreen ? 1.0 : tempShadow) * GetShadowGaussian(x, y);
//				}
//			}
//		}
//
//		NormalViewLightDotParams content = InitNormalViewLightDotParams(pixelParams.ShadingNormalWS, light.Direction, viewDirWS, halfVecWS);
//
//		color.rgb += SurfaceShading_Standard(pixelParams, content, light, viewDirWS, halfVecWS,
//#ifdef MATERIAL_HAS_ANISOTROPY
//			pixelParams.AnisotropyT,
//			pixelParams.AnisotropyB,
//			pixelParams.Anisotropy,
//#endif
//			shadow);
//	}
//	return color;
//}
//
//#endif