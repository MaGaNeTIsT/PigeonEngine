#ifndef _SHADER_BXDF_HLSL
#define _SHADER_BXDF_HLSL

#include "./ShaderDefCommon.hlsl"
#include "./ShaderFunctions.hlsl"

/*
* Physically Based Rendering in Filament https://google.github.io/filament/Filament.html#about/authors
*/

/*
* GGX distribution described in Bruce Walter et al. 2007. Microfacet Models for Refraction through Rough Surfaces.
* Proceedings of the Eurographics Symposium on Rendering. Equivalent to the Trowbridge-Reitz distribution.
*/
float D_GGX(float NdotH, float roughness)
{
#if 0
    float a = NdotH * roughness;
    float k = roughness / (1.0 - NdotH * NdotH + a * a);
    return (k * k * CUSTOM_SHADER_PI_DERIVATIVE);
#endif

#if 1
    float a2 = roughness * roughness;
    float f = (NdotH * a2 - NdotH) * NdotH + 1.0;
    return (a2 / (CUSTOM_SHADER_PI * f * f));
#endif
}

/*
* GGX solution involves Lagrange's identity for half precision.
* |a x b|^2 = |a|^2 |b|^2 - (a . b)^2
*/
float D_GGX_MediumPrecision(float NdotH, float roughness, const float3 n, const float3 h)
{
    float3 NcrxH = cross(n, h);
    float a = NdotH * roughness;
    float k = roughness / (dot(NcrxH, NcrxH) + a * a);
    float d = k * k * CUSTOM_SHADER_PI_DERIVATIVE;
    return (SaturateMediumPrecision(d));
}

/*
* V(v, l, a) = G(v, l, a) / (4(n . v)(n . l)) = V1(l, a)V1(v, a)
* V1(v, a) = 1 / ((n . v) + sqrt(a^2 + (1 - a^2)(n . v)^2))
* V(v, l, a) = 0.5 / ((n . l)sqrt((n . v)^2 (1 - a^2) + a^2) + (n . v)sqrt((n . l)^2 (1 - a^2) + a^2))
*/
float V_SmithGGXCorrelated(float NdotV, float NdotL, float roughness)
{
    float a2 = roughness * roughness;
    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
    return (0.5 / (GGXV + GGXL));
}

/*
* This approximation is mathematically wrong but saves two square root operations and is good enough for real-time mobile applications.
* V(v, l, a) = 0.5 / ((n . l)((n . v)(1 - a) + a) + (n . v)((n . l)(1 - a) + a))
* Proposes the same approximation based on the same observation that the square root can be removed.
* It does so by rewriting the expressions as lerps [Earl Hammon. 217. PBR Diffuse Lighting for GGX+Smith Microsurfaces. GDC 2017.]
* V(v, l, a) = 0.5 / lerp(2(n . l)(n . v), (n . l) + (n . v), a)
*/
float V_SmithGGXCorrelatedFast(float NdotV, float NdotL, float roughness)
{
    float a = roughness;
    float GGXV = NdotL * (NdotV * (1.0 - a) + a);
    float GGXL = NdotV * (NdotL * (1.0 - a) + a);
    return (0.5 / (GGXV + GGXL));
}

/*
* An inexpensive approximation of the Fresnel term for the Cook-Torrance specular BRDF:
* F_Schlick(v, h, f0, f90) = f0 + (f90 - f0)(1 - (v . h))^5
*/
float3 F_Schlick(float u, float3 f0, float f90)
{
    return (f0 + (float3(f90) - f0) * pow(1.0 - u, 5.0));
}

// Using f90 set to 1.0.
float3 F_Schlick(float u, float3 f0)
{
#if 0
    float f = pow(1.0 - u, 5.0);
    return (f + f0 * (1.0 - f));
#endif

#if 1
    return (f0 + (float3(1.0) - f0) * pow(1.0 - u, 5.0));
#endif
}

// Simple lambertian diffuse term. Diffuse reflectance = diffuseColor * Diffuse_Lambert();
float Diffuse_Lambert()
{
    return CUSTOM_SHADER_PI_DERIVATIVE;
}

/*
* For completeness, the Disney diffuse BRDF expressed in [Burley12] is the following:
*/
float F_Schlick_Complete(float u, float f0, float f90)
{
    return (f0 + (f90 - f0) * pow(1.0 - u, 5.0));
}
float Diffuse_Burley(float NdotV, float NdotL, float LdotH, float roughness)
{
    float f90 = 0.5 + 2.0 * roughness * LdotH * LdotH;
    float lightScatter = F_Schlick_Complete(NdotL, 1.0, f90);
    float viewScatter = F_Schlick_Complete(NdotV, 1.0, f90);
    return (lightScatter * viewScatter * CUSTOM_SHADER_PI_DERIVATIVE);
}

void BRDF(const GeometryViewLightDotTerm content, float roughness, float3 diffuseColor, float3 f0)
{
    float  D = D_GGX(content.NdotH, roughness);
    float3 F = F_Schlick(content.LdotH, f0);
    float  V = V_SmithGGXCorrelated(content.NdotV, content.NdotL, roughness);

    // Specular BRDF
    float3 Fr = (D * V) * F;
    // Diffuse BRDF
    float3 Fd = diffuseColor * Diffuse_Lambert();
}

#endif