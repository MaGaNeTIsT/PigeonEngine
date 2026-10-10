#ifndef D_UNLIT_HLSL
#define D_UNLIT_HLSL


// ---------------------------------------------------------------------------
// Unlit domain: colour in, colour out, no lighting at all.
//
// The domain exists to prove the framework is a framework: it shares the whole
// pipeline with DefaultLit -- the same MaterialSurface contract, the same
// DeriveSurface / ShadeSurface pair, the same encoder entry point -- and differs
// only in what it puts in the fields.
//
// The interesting part is how "no lighting" is expressed. It is not a special
// case in the lighting code and not a flag anyone has to test: the domain clears
// the light channel mask, and the lighting loop skips any light whose bit the
// surface does not carry. In a deferred renderer that is also what makes the
// deferred lighting pass skip the pixel without shading it with a guessed
// material.
//
// The colour reaches the screen as emission. That is not a workaround: a surface
// taking part in no lighting term is exactly a surface that emits its own colour,
// and emission is the one exit a surface's own light has into the scene colour
// target.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// MaterialSurface: the same core fields.
//
// Unlit declares them because the artist still writes BaseColor and coverage.
// The lighting shaped fields are present but ignored: keeping the field set
// identical means switching a material from DefaultLit to Unlit does not
// invalidate what it already wrote, and a domain that later wants to light some
// of them can do so without breaking the material.
// ---------------------------------------------------------------------------

struct MaterialSurface
{
	float4	BaseColor;				// rgb base colour, a opacity
	float	Metallic;
	float	Reflectance;
	float	Roughness;
	float3	Normal;
	float3	EmissiveColor;
	float	AmbientOcclusion;
	float	Opacity;
	float	AlphaCutoff;
};

void InitMaterialSurface(out MaterialSurface OutSurface)
{
	OutSurface.BaseColor = float4(1.0, 1.0, 1.0, 1.0);
	OutSurface.Metallic = 0.0;
	OutSurface.Reflectance = 0.5;
	OutSurface.Roughness = 1.0;
	OutSurface.Normal = float3(0.0, 0.0, 1.0);
	OutSurface.EmissiveColor = float3(0.0, 0.0, 0.0);
	OutSurface.AmbientOcclusion = 1.0;
	OutSurface.Opacity = 1.0;
	OutSurface.AlphaCutoff = 0.5;
}

// ---------------------------------------------------------------------------
// Derive
// ---------------------------------------------------------------------------

void DeriveSurface(in MaterialSurface InMaterial, in MaterialPixelInput InInput, out SurfaceData OutSurface)
{
	OutSurface.BaseColor = InMaterial.BaseColor.rgb;
	OutSurface.DiffuseColor = InMaterial.BaseColor.rgb;
	OutSurface.F0 = float3(0.0, 0.0, 0.0);

	// Nothing here reads roughness, but a coherent bundle is worth more than a
	// partial one: a later domain change does not have to revisit this function,
	// and the GBuffer decoder reads these fields without asking which domain
	// wrote them.
	OutSurface.PerceptualRoughness = 1.0;
	OutSurface.Roughness = 1.0;
	OutSurface.Metallic = 0.0;
	OutSurface.Reflectance = 0.0;

	// The geometric normal, not the material's tangent space one: an unlit
	// surface has no lighting for a normal to affect, and storing the flat
	// geometric one keeps the decode side meaningful.
	OutSurface.Normal = InInput.WorldNormal;
	OutSurface.EmissiveColor = InMaterial.EmissiveColor;
	OutSurface.AmbientOcclusion = 1.0;
	OutSurface.Opacity = ComputeSurfaceOpacity(InMaterial.BaseColor.a, InMaterial.Opacity);
	OutSurface.AlphaCutoff = InMaterial.AlphaCutoff;

	// The whole point of this domain. With no channel claimed, every light is
	// skipped before the BRDF is entered.
	OutSurface.LightChannelMask = 0u;

	// The colour leaves as emission, which is what the base pass writes into the scene colour
	// target. An authored emissive value rides along additively, which is how a material can
	// brighten itself without touching BaseColor.
	OutSurface.EmissiveColor = InMaterial.BaseColor.rgb + InMaterial.EmissiveColor;
}

// ---------------------------------------------------------------------------
// Shade
//
// Nothing to do: the channel mask is zero, so every light already declined.
// Returning zero explicitly rather than looping keeps that obvious, and stops a
// future change to the mask from silently lighting unlit surfaces.
// ---------------------------------------------------------------------------

#if SHADER_STAGE_PIXEL
void ShadeSurface(in SurfaceData InSurface, in MaterialPixelInput InInput, out ShadingResult OutResult)
{
	OutResult.Diffuse = float3(0.0, 0.0, 0.0);
	OutResult.Specular = float3(0.0, 0.0, 0.0);
}
#endif	//SHADER_STAGE_PIXEL
#endif	//D_UNLIT_HLSL
