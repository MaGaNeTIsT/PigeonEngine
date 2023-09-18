#ifndef _SHADER_BXDF_HLSL
#define _SHADER_BXDF_HLSL

#include "./ShaderStructCommon.hlsl"
#include "./ShaderFunctions.hlsl"

/*
* Physically Based Rendering in Filament https://google.github.io/filament/Filament.html#about/authors
*/

#ifdef ENABLE_CONSOLE_SHADING
float D_GGX(float NdotH, float Roughness)
#else
float D_GGX(float NdotH, float Roughness, const float3 Normal, const float3 HalfVector)
#endif
{
	// Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"

    // In mediump, there are two problems computing 1.0 - NoH^2
    // 1) 1.0 - NoH^2 suffers floating point cancellation when NoH^2 is close to 1 (highlights)
    // 2) NoH doesn't have enough precision around 1.0
    // Both problem can be fixed by computing 1-NoH^2 in highp and providing NoH in highp as well

    // However, we can do better using Lagrange's identity:
    //      ||a x b||^2 = ||a||^2 ||b||^2 - (a . b)^2
    // since N and H are unit vectors: ||N x H||^2 = 1.0 - NoH^2
    // This computes 1.0 - NoH^2 directly (which is close to zero in the highlights and has
    // enough precision).
    // Overall this yields better performance, keeping all computations in mediump
#ifdef ENABLE_CONSOLE_SHADING
    float OneMinusNoHSquared = 1.0 - NdotH * NdotH;
#else
	float3 NxH = cross(Normal, HalfVector);
    float OneMinusNoHSquared = dot(NxH, NxH);
#endif

    float a = NdotH * Roughness;
    float k = Roughness / (OneMinusNoHSquared + a * a);
    float d = k * k * CUSTOM_SHADER_PI_DERIVATIVE;
    return (SaturateMediumPrecision(d));
}

float D_GGX_Anisotropic(float NdotH, float TdotH, float BdotH, float at, float ab)
{
	// Burley 2012, "Physically-Based Shading at Disney"

    // The values at and ab are perceptualRoughness^2, a2 is therefore perceptualRoughness^4
    // The dot product below computes perceptualRoughness^8. We cannot fit in fp16 without clamping
    // the roughness to too high values so we perform the dot product and the division in fp32
    float a2 = at * ab;
    float3 v = float3(ab * TdotH, at * BdotH, a2 * NdotH);  // High precision
    float v2 = dot(v, v);	// High precision
    float w2 = a2 / v2;
    return (a2 * w2 * w2 * CUSTOM_SHADER_PI_DERIVATIVE);
}

float D_Ashikhmin(float NdotH, float Roughness)
{
	// Ashikhmin 2007, "Distribution-based BRDFs" Ashikhmin's velvet NDF
	float a2 = Roughness * Roughness;
	float cos2h = NdotH * NdotH;
	float sin2h = max(1.0 - cos2h, 0.0078125);	// 2^(-14/2), so sin2h^2 > 0 in fp16
	float sin4h = sin2h * sin2h;
	float cot2 = -cos2h / (a2 * sin2h);
	return 1.0 / (CUSTOM_SHADER_PI * (4.0 * a2 + 1.0) * sin4h) * (4.0 * exp(cot2) + sin4h);
}

float D_Charlie(float NdotH, float Roughness)
{
	// Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
	float invAlpha  = 1.0 / Roughness;
    float cos2h = NdotH * NdotH;
    float sin2h = max(1.0 - cos2h, 0.0078125);	// 2^(-14/2), so sin2h^2 > 0 in fp16
    return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * CUSTOM_SHADER_PI);
}

float V_SmithGGXCorrelated(float NdotV, float NdotL, float Roughness)
{
	// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    float a2 = Roughness * Roughness;
	// TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
    float GGXV = NdotL * sqrt((NdotV - a2 * NdotV) * NdotV + a2);
    float GGXL = NdotV * sqrt((NdotL - a2 * NdotL) * NdotL + a2);
	float v = 0.5 / (GGXV + GGXL);
	// a2=0 => v = 1 / 4*NoL*NoV   => min=1/4, max=+inf
    // a2=1 => v = 1 / 2*(NoL+NoV) => min=1/4, max=+inf
    // clamp to the maximum value representable in mediump    
	return (SaturateMediumPrecision(v));
}

float V_SmithGGXCorrelated_Fast(float NdotV, float NdotL, float Roughness)
{
    // Hammon 2017, "PBR Diffuse Lighting for GGX+Smith Microsurfaces"
	
	//This approximation is mathematically wrong but saves two square root operations and is good enough for real-time mobile applications.
	//V(v, l, a) = 0.5 / ((n . l)((n . v)(1 - a) + a) + (n . v)((n . l)(1 - a) + a))
	//Proposes the same approximation based on the same observation that the square root can be removed.
	//It does so by rewriting the expressions as lerps [Earl Hammon. 217. PBR Diffuse Lighting for GGX+Smith Microsurfaces. GDC 2017.]
	//V(v, l, a) = 0.5 / lerp(2(n . l)(n . v), (n . l) + (n . v), a)
    float v = 0.5 / lerp(2.0 * NdotL * NdotV, NdotL + NdotV, Roughness);
    return (SaturateMediumPrecision(v));
}

float V_SmithGGXCorrelated_Anisotropic(float NdotV, float NdotL, float TdotV, float BdotV, float TdotL, float BdotL, float at, float ab)
{
	// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    // TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
    float lambdaV = NdotL * length(float3(at * TdotV, ab * BdotV, NdotV));
    float lambdaL = NdotV * length(float3(at * TdotL, ab * BdotL, NdotL));
    float v = 0.5 / (lambdaV + lambdaL);
    return (SaturateMediumPrecision(v));
}

float V_Kelemen(float LdotH)
{
    // Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
    return (SaturateMediumPrecision(0.25 / (LdotH * LdotH)));
}

float V_Neubelt(float NdotV, float NdotL)
{
    // Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
    return (SaturateMediumPrecision(1.0 / (4.0 * (NdotL + NdotV - NdotL * NdotV))));
}

float3 F_Schlick(float u, float3 InF0, float InF90)
{
	// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
	
	//An inexpensive approximation of the Fresnel term for the Cook-Torrance specular BRDF:
	//F_Schlick(v, h, f0, f90) = f0 + (f90 - f0)(1 - (v . h))^5
    return (InF0 + (InF90 - InF0) * Power5(1.0 - u));
}

float F_Schlick(float u, float InF0, float InF90)
{
    return (InF0 + (InF90 - InF0) * Power5(1.0 - u));
}

float3 F_Schlick(float u, float3 InF0)
{
	// Using f90 set to 1.0
    float f = Power5(1.0 - u);
    return (f + InF0 * (1.0 - f));
}

float Diffuse_Lambert()
{
	// Simple lambertian diffuse term. Diffuse reflectance = diffuseColor * Diffuse_Lambert();
    return (CUSTOM_SHADER_PI_DERIVATIVE);
}

float Diffuse_Burley(float NdotV, float NdotL, float LdotH, float Roughness)
{
	// Burley 2012, "Physically-Based Shading at Disney"
    float F90 = 0.5 + 2.0 * Roughness * LdotH * LdotH;
    float lightScatter = F_Schlick(NdotL, 1.0, F90);
    float viewScatter = F_Schlick(NdotV, 1.0, F90);
    return (lightScatter * viewScatter * CUSTOM_SHADER_PI_DERIVATIVE);
}

float Diffuse_Wrap(float NdotL, float w)
{
	// Energy conserving wrap diffuse term, does *not* include the divide by pi
    return (saturate((NdotL + w) / Power2(1.0 + w)));
}

float DiffuseBRDF(const NormalViewLightDotParams InSVLContent, const PixelParams InPixelParams)
{
#ifdef ENABLE_LAMBERT_DIFFUSE
    return (Diffuse_Lambert());
#elif defined(ENABLE_BURLEY_DIFFUSE)
    return (Diffuse_Burley(InSVLContent.NdotVClamped, InSVLContent.NdotLClamped, InSVLContent.LdotHClamped, InPixelParams.Roughness));
#endif
}

float3 IsotropicBRDF(const NormalViewLightDotParams InSVLContent, const PixelParams InPixelParams, const float3 InNormal, const float3 InHalfVector)
{
#ifdef ENABLE_CONSOLE_SHADING
    float F90 = 50.0 * 0.33;
    F90 = saturate(dot(InPixelParams.F0, F90.xxx));
    float  D = D_GGX(InSVLContent.NdotHClamped, InPixelParams.Roughness);
    float  V = V_SmithGGXCorrelated(InSVLContent.NdotVClamped, InSVLContent.NdotLClamped, InPixelParams.Roughness);
    float3 F = F_Schlick(InSVLContent.LdotHClamped, InPixelParams.F0, F90);
#else
    float  D = D_GGX(InSVLContent.NdotHClamped, InPixelParams.Roughness, InNormal, InHalfVector);
    float  V = V_SmithGGXCorrelated_Fast(InSVLContent.NdotVClamped, InSVLContent.NdotLClamped, InPixelParams.Roughness);
    float3 F = F_Schlick(InSVLContent.LdotHClamped, InPixelParams.F0);  // F90 = 1.0
#endif

    return ((D * V) * F);
}

float3 AnisotropicBRDF(const NormalViewLightDotParams InSVLContent, const PixelParams InPixelParams, const float3 InViewDirection, const float3 InLightDirection, const float3 InHalfVector, float3 InAnisotropicT, float3 InAnisotropicB, float InAnisotropy)
{
    float TdotV = dot(InAnisotropicT, InViewDirection);
    float BdotV = dot(InAnisotropicB, InViewDirection);
    float TdotL = dot(InAnisotropicT, InLightDirection);
    float BdotL = dot(InAnisotropicB, InLightDirection);
    float TdotH = dot(InAnisotropicT, InHalfVector);
    float BdotH = dot(InAnisotropicB, InHalfVector);

    // Anisotropic parameters: at and ab are the roughness along the tangent and bitangent
    // to simplify materials, we derive them from a single roughness parameter
    // Kulla 2017, "Revisiting Physically Based Shading at Imageworks"
    float at = max(InPixelParams.Roughness * (1.0 + InAnisotropy), MIN_ROUGHNESS);
    float ab = max(InPixelParams.Roughness * (1.0 - InAnisotropy), MIN_ROUGHNESS);

    // Specular anisotropic BRDF
    float D = D_GGX_Anisotropic(InSVLContent.NdotHClamped, TdotH, BdotH, at, ab);
    float V = V_SmithGGXCorrelated_Anisotropic(InSVLContent.NdotVClamped, InSVLContent.NdotLClamped, TdotV, BdotV, TdotL, BdotL, at, ab);
#ifdef ENABLE_CONSOLE_SHADING
    float F90 = 50.0 * 0.33;
    F90 = saturate(dot(InPixelParams.F0, F90.xxx));
    float3 F = F_Schlick(InSVLContent.LdotHClamped, InPixelParams.F0, F90);
#else
    float3 F = F_Schlick(InSVLContent.LdotHClamped, InPixelParams.F0);  // F90 = 1.0
#endif

    return ((D * V) * F);
}

float ClearCoatBRDF(const NormalViewLightDotParams InSVLContent, const PixelParams InPixelParams, const float3 InNormal, const float3 InHalfVector, float3 InClearCoatNormal, float InClearCoatRoughness, float InClearCoatStrength, out float Fcc)
{
#if defined(MATERIAL_HAS_NORMALMAP) || defined(MATERIAL_HAS_CLEAR_COAT_NORMAL)
    // If the material has a normal map, we want to use the geometric normal
    // Instead to avoid applying the normal map details to the clear coat layer
    float ClearCoatNdotH = saturate(dot(InClearCoatNormal, InHalfVector));
#else
    float ClearCoatNdotH = InSVLContent.NdotHClamped;
#endif

    // Clear coat specular lobe
#ifdef ENABLE_CONSOLE_SHADING
    float D = D_GGX(ClearCoatNdotH, InClearCoatRoughness);
#else
    float D = D_GGX(ClearCoatNdotH, InClearCoatRoughness, InNormal, InHalfVector);
#endif
    float V = V_Kelemen(InSVLContent.LdotHClamped);
    float F = F_Schlick(InSVLContent.LdotHClamped, 0.04, 1.0) * InClearCoatStrength;  // Fix IOR to 1.5

    Fcc = F;
    return ((D * V) * F);
}

float3 ClothSheenBRDF(const NormalViewLightDotParams InSVLContent, const PixelParams InPixelParams)
{
    float  D = D_Charlie(InSVLContent.NdotHClamped, InPixelParams.Roughness);
    float  V = V_Neubelt(InSVLContent.NdotVClamped, InSVLContent.NdotLClamped);

#ifdef MATERIAL_HAS_SHEEN_COLOR
    return ((D * V) * InPixelParams.SheenColor);
#else
    return (D * V);
#endif
}

#endif