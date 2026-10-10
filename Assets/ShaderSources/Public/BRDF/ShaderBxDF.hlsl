#ifndef SHADER_BX_DF_HLSL
#define SHADER_BX_DF_HLSL

// ---------------------------------------------------------------------------
// Physically based BRDF terms.
//
// A port of the legacy Public/ShaderBxDF.hlsl, which is itself a port of
// Filament's model:
//   Physically Based Rendering in Filament
//   https://google.github.io/filament/Filament.html
//
// The mathematics is unchanged. Three things about the port:
//
// 1. Filament's own names (D_GGX, V_SmithGGXCorrelated, F_Schlick) are spelled
//    out in PascalCase, because the project style reserves the underscore and
//    the all caps form for constants and macros. Each rename keeps the Filament
//    name in the comment so the correspondence stays searchable.
// 2. The legacy ENABLE_CONSOLE_SHADING switch is gone. It selected between a
//    shorter expression and a numerically better one; the better one is kept,
//    because the Lagrange identity form avoids the fp16 cancellation that the
//    short form suffers in exactly the highlights where it is visible. There is
//    no target where the short form is still worth having.
// 3. The legacy per feature switches (SHADER_HAS_CLEAR_COAT_NORMAL,
//    SHADER_HAS_SHEEN_COLOR, ENABLE_LAMBERT_DIFFUSE / ENABLE_BURLEY_DIFFUSE)
//    are gone too. Anything a term needs is a parameter, so a term compiles
//    whether or not the calling domain has that feature, and the domain decides
//    which diffuse model it wants by calling it.
//
// Every visibility term is clamped to the largest value a half precision float
// can hold. A non clamped one goes to infinity at grazing angles and the
// infinity survives into the highlight.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"

// ---------------------------------------------------------------------------
// Microfacet distributions
// ---------------------------------------------------------------------------

// Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"
float DistributionGGX(in float InNdotH, in float InRoughness, in float3 InNormal, in float3 InHalfVector)
{
	// OneMinusNoHSquared suffers cancellation in half precision when NdotH is
	// close to one -- which is precisely the highlight. Lagrange's identity
	// gives the same quantity without the subtraction:
	//     ||a cross b||^2 = ||a||^2 ||b||^2 - (a dot b)^2
	// and since both are unit length, ||N cross H||^2 = 1 - NdotH^2.
	float3 NxH = cross(InNormal, InHalfVector);
	float OneMinusNoHSquared = dot(NxH, NxH);

	float A = InNdotH * InRoughness;
	float K = InRoughness / (OneMinusNoHSquared + A * A);
	float D = K * K * PE_SHADER_PI_INV;
	return ClampMediumPrecision(D);
}

// Burley 2012, "Physically-Based Shading at Disney"
// InAt and InAb are the roughness along the tangent and the bitangent.
float DistributionGGXAnisotropic(in float InNdotH, in float InTdotH, in float InBdotH, in float InAt, in float InAb)
{
	// The dots below work out to perceptualRoughness^8, which does not fit in
	// half precision unless roughness is clamped high, so the divide stays in
	// full precision.
	float A2 = InAt * InAb;
	float3 V = float3(InAb * InTdotH, InAt * InBdotH, A2 * InNdotH);
	float V2 = dot(V, V);
	float W2 = A2 / V2;
	return A2 * W2 * W2 * PE_SHADER_PI_INV;
}

// Ashikhmin 2007, "Distribution-based BRDFs" -- velvet.
float DistributionAshikhmin(in float InNdotH, in float InRoughness)
{
	float A2 = InRoughness * InRoughness;
	float Cos2h = InNdotH * InNdotH;
	float Sin2h = max(1.0 - Cos2h, 0.0078125);	// 2^(-14/2), keeps sin2h^2 above zero in fp16
	float Sin4h = Sin2h * Sin2h;
	float Cot2 = -Cos2h / (A2 * Sin2h);
	return 1.0 / (PE_SHADER_PI * (4.0 * A2 + 1.0) * Sin4h) * (4.0 * exp(Cot2) + Sin4h);
}

// Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
float DistributionCharlie(in float InNdotH, in float InRoughness)
{
	float InvAlpha = 1.0 / InRoughness;
	float Cos2h = InNdotH * InNdotH;
	float Sin2h = max(1.0 - Cos2h, 0.0078125);
	return (2.0 + InvAlpha) * pow(Sin2h, InvAlpha * 0.5) / (2.0 * PE_SHADER_PI);
}

// ---------------------------------------------------------------------------
// Visibility (masking and shadowing)
// ---------------------------------------------------------------------------

// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
float VisibilitySmithGGXCorrelated(in float InNdotV, in float InNdotL, in float InRoughness)
{
	float A2 = InRoughness * InRoughness;
	float GGXV = InNdotL * sqrt((InNdotV - A2 * InNdotV) * InNdotV + A2);
	float GGXL = InNdotV * sqrt((InNdotL - A2 * InNdotL) * InNdotL + A2);
	float V = 0.5 / (GGXV + GGXL);
	return ClampMediumPrecision(V);
}

// Hammon 2017, "PBR Diffuse Lighting for GGX+Smith Microsurfaces"
// Mathematically an approximation, and deliberately so: it drops two square
// roots, and the error is below what a highlight shows.
float VisibilitySmithGGXCorrelatedFast(in float InNdotV, in float InNdotL, in float InRoughness)
{
	float V = 0.5 / lerp(2.0 * InNdotL * InNdotV, InNdotL + InNdotV, InRoughness);
	return ClampMediumPrecision(V);
}

// Heitz 2014, anisotropic form.
float VisibilitySmithGGXCorrelatedAnisotropic(in float InNdotV, in float InNdotL,
	in float InTdotV, in float InBdotV, in float InTdotL, in float InBdotL,
	in float InAt, in float InAb)
{
	float LambdaV = InNdotL * length(float3(InAt * InTdotV, InAb * InBdotV, InNdotV));
	float LambdaL = InNdotV * length(float3(InAt * InTdotL, InAb * InBdotL, InNdotL));
	float V = 0.5 / (LambdaV + LambdaL);
	return ClampMediumPrecision(V);
}

// Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with
// Importance Sampling" -- used by the clear coat layer.
float VisibilityKelemen(in float InLdotH)
{
	return ClampMediumPrecision(0.25 / (InLdotH * InLdotH));
}

// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for
// The Order: 1886" -- used by the cloth sheen layer.
float VisibilityNeubelt(in float InNdotV, in float InNdotL)
{
	return ClampMediumPrecision(1.0 / (4.0 * (InNdotL + InNdotV - InNdotL * InNdotV)));
}

// ---------------------------------------------------------------------------
// Fresnel
// ---------------------------------------------------------------------------

// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
float3 FresnelSchlick(in float InLdotH, in float3 InF0, in float InF90)
{
	return InF0 + (InF90 - InF0) * Power5(1.0 - InLdotH);
}

// The scalar form, for layers whose F0 is a single value.
float FresnelSchlick(in float InLdotH, in float InF0, in float InF90)
{
	return InF0 + (InF90 - InF0) * Power5(1.0 - InLdotH);
}

// F90 = 1. The default for a dielectric base layer.
float3 FresnelSchlick(in float InLdotH, in float3 InF0)
{
	float F = Power5(1.0 - InLdotH);
	return F + InF0 * (1.0 - F);
}

// ---------------------------------------------------------------------------
// Diffuse
// ---------------------------------------------------------------------------

// Lambertian, energy conserving: includes the 1/pi. Diffuse reflectance is
// DiffuseColor * DiffuseLambert().
float DiffuseLambert()
{
	return PE_SHADER_PI_INV;
}

// Burley 2012, "Physically-Based Shading at Disney" -- retro reflective
// diffuse, the artist facing "Disney diffuse".
float DiffuseBurley(in float InNdotV, in float InNdotL, in float InLdotH, in float InRoughness)
{
	float F90 = 0.5 + 2.0 * InRoughness * InLdotH * InLdotH;
	float LightScatter = FresnelSchlick(InNdotL, 1.0, F90);
	float ViewScatter = FresnelSchlick(InNdotV, 1.0, F90);
	return LightScatter * ViewScatter * PE_SHADER_PI_INV;
}

// Energy conserving wrapped diffuse. Does not include the 1/pi.
float DiffuseWrap(in float InNdotL, in float InWrap)
{
	return saturate((InNdotL + InWrap) / Power2(1.0 + InWrap));
}

// ---------------------------------------------------------------------------
// Composed lobes
// ---------------------------------------------------------------------------

// The base layer: isotropic specular times the BRDF weight, without the light
// radiance and without N dot L, both of which the lighting stage multiplies in.
float3 SpecularBRDFIsotropic(in float InNdotL, in float InNdotV, in float InNdotH, in float InLdotH,
	in float3 InF0, in float InRoughness, in float3 InNormal, in float3 InHalfVector)
{
	float D = DistributionGGX(InNdotH, InRoughness, InNormal, InHalfVector);
	float V = VisibilitySmithGGXCorrelated(InNdotV, InNdotL, InRoughness);
	float3 F = FresnelSchlick(InLdotH, InF0);
	return (D * V) * F;
}

float3 SpecularBRDFAnisotropic(in float InNdotL, in float InNdotV, in float InNdotH, in float InLdotH,
	in float3 InF0, in float InRoughness, in float3 InNormal, in float3 InHalfVector,
	in float3 InViewDirection, in float3 InLightDirection,
	in float3 InAnisotropicT, in float3 InAnisotropicB, in float InAnisotropy)
{
	float TdotV = dot(InAnisotropicT, InViewDirection);
	float BdotV = dot(InAnisotropicB, InViewDirection);
	float TdotL = dot(InAnisotropicT, InLightDirection);
	float BdotL = dot(InAnisotropicB, InLightDirection);
	float TdotH = dot(InAnisotropicT, InHalfVector);
	float BdotH = dot(InAnisotropicB, InHalfVector);

	// One roughness parameter drives both axes; Kulla 2017, "Revisiting
	// Physically Based Shading at Imageworks".
	float At = max(InRoughness * (1.0 + InAnisotropy), MIN_ROUGHNESS);
	float Ab = max(InRoughness * (1.0 - InAnisotropy), MIN_ROUGHNESS);

	float D = DistributionGGXAnisotropic(InNdotH, TdotH, BdotH, At, Ab);
	float V = VisibilitySmithGGXCorrelatedAnisotropic(InNdotV, InNdotL, TdotV, BdotV, TdotL, BdotL, At, Ab);
	float3 F = FresnelSchlick(InLdotH, InF0);
	return (D * V) * F;
}

// The clear coat layer, IOR fixed at 1.5. OutFcc is the layer's Fresnel term,
// which the caller needs to blend the base layer underneath.
float3 ClearCoatBRDF(in float InNdotL, in float InNdotV, in float InNdotH, in float InLdotH,
	in float3 InNormal, in float3 InHalfVector,
	in float3 InClearCoatNormal, in float InClearCoatRoughness, in float InClearCoatStrength,
	out float OutFcc)
{
	// When the material has a normal map the geometric normal is used for the
	// coat, so the base layer's detail is not applied twice.
	float ClearCoatNdotH = saturate(dot(InClearCoatNormal, InHalfVector));

	float D = DistributionGGX(ClearCoatNdotH, InClearCoatRoughness, InNormal, InHalfVector);
	float V = VisibilityKelemen(InLdotH);
	float F = FresnelSchlick(InLdotH, 0.04, 1.0) * InClearCoatStrength;

	OutFcc = F;
	return (D * V) * F;
}

// Cloth sheen. InSheenColor is passed in rather than read from a parameter
// bundle, so this compiles for a domain that has no sheen fields.
float3 ClothSheenBRDF(in float InNdotL, in float InNdotV, in float InNdotH, in float3 InSheenColor, in float InRoughness)
{
	float D = DistributionCharlie(InNdotH, InRoughness);
	float V = VisibilityNeubelt(InNdotV, InNdotL);
	return (D * V) * InSheenColor;
}
#endif	//SHADER_BX_DF_HLSL
