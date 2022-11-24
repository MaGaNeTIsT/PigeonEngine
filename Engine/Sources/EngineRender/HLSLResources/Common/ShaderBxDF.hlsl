#ifndef _SHADER_BXDF_HLSL
#define _SHADER_BXDF_HLSL

#include "./ShaderDefCommon.hlsl"
#include "./ShaderFunctions.hlsl"

BRDFTerm InitBRDFTerm()
{
    BRDFTerm brdf;
    brdf.Fd = 0.0;
    brdf.Fr = 0.0;
    return brdf;
}

AnisotropicRoughness FetchAnisotropicRoughness(float roughness, float anisotropy)
{
    AnisotropicRoughness r;
    r.at = max(roughness * (1.0 + anisotropy), 0.001);
    r.ab = max(roughness * (1.0 - anisotropy), 0.001);
    return r;
}

/*
* Physically Based Rendering in Filament https://google.github.io/filament/Filament.html#about/authors
*/

#ifdef ENABLE_CONSOLE_SHADING
float D_GGX(float NdotH, float roughness)
#else
float D_GGX(float NdotH, float roughness, const float3 normal, const float3 halfVector)
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
    float oneMinusNoHSquared = 1.0 - NdotH * NdotH;
#else
	float3 NxH = cross(normal, halfVector);
    float oneMinusNoHSquared = dot(NxH, NxH);
#endif

    float a = NdotH * roughness;
    float k = roughness / (oneMinusNoHSquared + a * a);
    float d = k * k * CUSTOM_SHADER_PI_DERIVATIVE;
    return (SaturateMediumPrecision(d));
}

float D_GGX_Anisotropic(float NdotH, float TdotH, float BdotH, AnisotropicRoughness anisotropicRoughness)
{
	// Burley 2012, "Physically-Based Shading at Disney"

    // The values at and ab are perceptualRoughness^2, a2 is therefore perceptualRoughness^4
    // The dot product below computes perceptualRoughness^8. We cannot fit in fp16 without clamping
    // the roughness to too high values so we perform the dot product and the division in fp32
    float a2 = anisotropicRoughness.at * anisotropicRoughness.ab;
    float3 v = float3(anisotropicRoughness.ab * TdotH, anisotropicRoughness.at * BdotH, a2 * NdotH);	// High precision
    float v2 = dot(v, v);	// High precision
    float w2 = a2 / v2;
    return (a2 * w2 * w2 * CUSTOM_SHADER_PI_DERIVATIVE);
}

float D_Ashikhmin(float NdotH, float roughness)
{
	// Ashikhmin 2007, "Distribution-based BRDFs" Ashikhmin's velvet NDF
	float a2 = roughness * roughness;
	float cos2h = NdotH * NdotH;
	float sin2h = max(1.0 - cos2h, 0.0078125);	// 2^(-14/2), so sin2h^2 > 0 in fp16
	float sin4h = sin2h * sin2h;
	float cot2 = -cos2h / (a2 * sin2h);
	return 1.0 / (CUSTOM_SHADER_PI * (4.0 * a2 + 1.0) * sin4h) * (4.0 * exp(cot2) + sin4h);
}

float D_Charlie(float NdotH, float roughness)
{
	// Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
	float invAlpha  = 1.0 / roughness;
    float cos2h = NdotH * NdotH;
    float sin2h = max(1.0 - cos2h, 0.0078125);	// 2^(-14/2), so sin2h^2 > 0 in fp16
    return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * CUSTOM_SHADER_PI);
}

float V_SmithGGXCorrelated(float NdotV, float NdotL, float roughness)
{
	// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    float a2 = roughness * roughness;
	// TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
    float GGXV = NdotL * sqrt((NdotV - a2 * NdotV) * NdotV + a2);
    float GGXL = NdotV * sqrt((NdotL - a2 * NdotL) * NdotL + a2);
	float v = 0.5 / (GGXV + GGXL);
	// a2=0 => v = 1 / 4*NoL*NoV   => min=1/4, max=+inf
    // a2=1 => v = 1 / 2*(NoL+NoV) => min=1/4, max=+inf
    // clamp to the maximum value representable in mediump    
	return (SaturateMediumPrecision(v));
}

float V_SmithGGXCorrelated_Fast(float NdotV, float NdotL, float roughness)
{
    // Hammon 2017, "PBR Diffuse Lighting for GGX+Smith Microsurfaces"
	
	//This approximation is mathematically wrong but saves two square root operations and is good enough for real-time mobile applications.
	//V(v, l, a) = 0.5 / ((n . l)((n . v)(1 - a) + a) + (n . v)((n . l)(1 - a) + a))
	//Proposes the same approximation based on the same observation that the square root can be removed.
	//It does so by rewriting the expressions as lerps [Earl Hammon. 217. PBR Diffuse Lighting for GGX+Smith Microsurfaces. GDC 2017.]
	//V(v, l, a) = 0.5 / lerp(2(n . l)(n . v), (n . l) + (n . v), a)
    float v = 0.5 / lerp(2.0 * NdotL * NdotV, NdotL + NdotV, roughness);
    return (SaturateMediumPrecision(v));
}

float V_SmithGGXCorrelated_Anisotropic(float NdotV, float NdotL, float TdotV, float BdotV, float TdotL, float BdotL, AnisotropicRoughness anisotropicRoughness)
{
	// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    // TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
    float lambdaV = NdotL * length(float3(anisotropicRoughness.at * TdotV, anisotropicRoughness.ab * BdotV, NdotV));
    float lambdaL = NdotV * length(float3(anisotropicRoughness.at * TdotL, anisotropicRoughness.ab * BdotL, NdotL));
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

float3 F_Schlick(float u, float3 f0, float f90)
{
	// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
	
	//An inexpensive approximation of the Fresnel term for the Cook-Torrance specular BRDF:
	//F_Schlick(v, h, f0, f90) = f0 + (f90 - f0)(1 - (v . h))^5
    return (f0 + (f90 - f0) * Power5(1.0 - u));
}

float F_Schlick(float u, float f0, float f90)
{
    return (f0 + (f90 - f0) * Power5(1.0 - u));
}

float3 F_Schlick(float u, float3 f0)
{
	// Using f90 set to 1.0
    float f = Power5(1.0 - u);
    return (f + f0 * (1.0 - f));
}

float Diffuse_Lambert()
{
	// Simple lambertian diffuse term. Diffuse reflectance = diffuseColor * Diffuse_Lambert();
    return CUSTOM_SHADER_PI_DERIVATIVE;
}

float Diffuse_Burley(float NdotV, float NdotL, float LdotH, float roughness)
{
	// Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * roughness * LdotH * LdotH;
    float lightScatter = F_Schlick(NdotL, 1.0, f90);
    float viewScatter = F_Schlick(NdotV, 1.0, f90);
    return (lightScatter * viewScatter * CUSTOM_SHADER_PI_DERIVATIVE);
}

float Diffuse_Wrap(float NdotL, float w)
{
	// Energy conserving wrap diffuse term, does *not* include the divide by pi
    return (saturate((NdotL + w) / Power2(1.0 + w)));
}

BRDFTerm StandardBRDF(const NormalViewLightDotParams content, float roughness, float3 diffuseColor, float3 f0)
{
    float  D = D_GGX(content.NdotH, roughness);
    float  V = V_SmithGGXCorrelated(content.NdotV, content.NdotL, roughness);
    float3 F = F_Schlick(content.LdotH, f0);

    BRDFTerm brdf = InitBRDFTerm();

    // Diffuse BRDF
    brdf.Fd = diffuseColor * Diffuse_Lambert();
    // Specular BRDF
    brdf.Fr = (D * V) * F;
	
	return brdf;
}

BRDFTerm StandardBRDF_Anisotropic(const NormalViewLightDotParams_Anisotropic content, float roughness, float3 diffuseColor, float3 f0, float anisotropy)
{
    AnisotropicRoughness r = FetchAnisotropicRoughness(roughness, anisotropy);

    float  D = D_GGX_Anisotropic(content.NdotH, content.TdotH, content.BdotH, r);
    float  V = V_SmithGGXCorrelated_Anisotropic(content.NdotV, content.NdotL, content.TdotV, content.BdotV, content.TdotL, content.BdotL, r);
    float3 F = F_Schlick(content.LdotH, f0);

    BRDFTerm brdf = InitBRDFTerm();

    // Diffuse BRDF
    brdf.Fd = diffuseColor * Diffuse_Lambert();
    // Specular BRDF
    brdf.Fr = (D * V) * F;
	
	return brdf;
}

float3 ClearCoatBRDF(const NormalViewLightDotParams content, float roughness, float3 diffuseColor, float3 f0, float clearCoatStrength, float clearCoatPerceptualRoughness)
{
    // Compute Fd and Fr from standard model
    BRDFTerm brdf = StandardBRDF(content, roughness, diffuseColor, f0);

    // Remapping and linearization of clear coat roughness
    clearCoatPerceptualRoughness = clamp(clearCoatPerceptualRoughness, 0.089, 1.0);
    float clearCoatRoughness = clearCoatPerceptualRoughness * clearCoatPerceptualRoughness;
    
    // Clear coat BRDF
    float Dc    = D_GGX(content.NdotH, clearCoatRoughness);
    float Vc    = V_Kelemen(content.LdotH);
    float Fc    = F_Schlick(content.LdotH, 0.04, 1.0) * clearCoatStrength;
    float Frc   = (Dc * Vc) * Fc;

    // Account for energy loss in the base layer
    return ((brdf.Fd + brdf.Fr * (1.0 - Fc)) * (1.0 - Fc) + Frc);
}

BRDFTerm ClothBRDF(const NormalViewLightDotParams content, float roughness, float3 sheenColor)
{
    //#define CLOTH_HAS_SUBSURFACE_COLOR

	float  D = D_Charlie(content.NdotH, roughness);
    float  V = V_Neubelt(content.NdotV, content.NdotL);
	float3 F = sheenColor;

    BRDFTerm brdf = InitBRDFTerm();

    // Specular BRDF
    brdf.Fr = (D * V) * F;
	
	// Diffuse BRDF
    brdf.Fd = Diffuse_Lambert();
#ifdef CLOTH_HAS_SUBSURFACE_COLOR
    // Energy conservative wrap diffuse to simulate subsurface scattering
    brdf.Fd *= Diffuse_Wrap(content.NdotL, 0.5);
#endif
    return brdf;
}

#endif