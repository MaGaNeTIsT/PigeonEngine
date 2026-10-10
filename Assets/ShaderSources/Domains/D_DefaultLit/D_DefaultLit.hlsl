#ifndef D_DEFAULT_LIT_HLSL
#define D_DEFAULT_LIT_HLSL


// ---------------------------------------------------------------------------
// DefaultLit domain: the standard metallic / roughness shading model.
//
// A domain owns three things and only three things:
//
//   1. the complete MaterialSurface, which is what decides what an artist can
//      write in this domain
//   2. DeriveSurface, which turns that into the engine's BRDF parameter bundle
//   3. ShadeSurface, which applies the lights to that bundle
//
// It does not write render targets. EncodeGBuffer and DecodeGBuffer_DefaultLit
// are engine owned (Public/GBuffer/), so this file never mentions a channel and
// adding a channel never touches an artist facing file.
//
// Both entry points are guarded by a define the domain's own JSON sets, which is
// what lets the programmable template carry a fallback that steps aside.
// ---------------------------------------------------------------------------

// The shading half is pixel stage only: it talks to the light buffers, which a
// vertex stage does not declare. MaterialSurface and InitMaterialSurface above
// stay available to both stages.
//
// The indirect half is a separate file from the direct one because the two have
// different bindings: the direct terms are bound by the lighting component, which
// every shading pass declares, while the environment belongs to the indirect
// lighting component. A pass that compiles this shading half declares both.
#if SHADER_STAGE_PIXEL
#include "Public/Lighting/ShaderDirectLighting.hlsl"
#include "Public/Lighting/ShaderIndirectDiffuse.hlsl"
#endif	//SHADER_STAGE_PIXEL

// ---------------------------------------------------------------------------
// MaterialSurface: the common core, no extension fields in this domain.
//
// The field names and their meaning are the contract documented in
// Public/Surface/MaterialSurface.hlsl. They are repeated here rather than
// inherited, because the structure an artist writes against has to be a real
// type in the compilation unit, and because a domain that does extend it should
// see the extension in the same place as the core.
// ---------------------------------------------------------------------------

struct MaterialSurface
{
	float4	BaseColor;				// rgb base colour, a opacity
	float	Metallic;
	float	Reflectance;
	float	Roughness;				// perceptual
	float3	Normal;					// tangent space
	float3	EmissiveColor;
	float	AmbientOcclusion;
	float	Opacity;
	float	AlphaCutoff;
};

// Must write every field. A material that connects no normal map simply leaves
// Normal at its default, which is what removes the need for a "has normal map"
// switch anywhere.
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
// Derive: material values to BRDF parameters
// ---------------------------------------------------------------------------

void DeriveSurface(in MaterialSurface InMaterial, in MaterialPixelInput InInput, out SurfaceData OutSurface)
{
	OutSurface.BaseColor = InMaterial.BaseColor.rgb;
	OutSurface.DiffuseColor = ComputeDiffuseColor(InMaterial.BaseColor.rgb, InMaterial.Metallic);
	OutSurface.F0 = ComputeFresnelF0(InMaterial.Reflectance, InMaterial.Metallic, InMaterial.BaseColor.rgb);

	// The artist writes perceptual roughness; the BRDF wants the true value.
	// Squaring here is the "remove the perceptual linearity" step, and it is done
	// once, in the one place both the GBuffer and the BRDF look at.
	OutSurface.PerceptualRoughness = ClampPerceptualRoughness(InMaterial.Roughness);
	OutSurface.Roughness = ComputePerceptualRoughnessToRoughness(OutSurface.PerceptualRoughness);

	// Tangent space to world. The frame is the one the engine interpolated and
	// put into MaterialPixelInput, handedness already applied, so it is used as
	// given rather than recomputed.
	OutSurface.Normal = TransformTangentToWorld(InMaterial.Normal,
		CreateTangentToWorldMatrix(InInput.WorldNormal, InInput.WorldTangent, InInput.WorldBitangent),
		true);

	OutSurface.EmissiveColor = InMaterial.EmissiveColor;
	OutSurface.AmbientOcclusion = InMaterial.AmbientOcclusion;
	OutSurface.Metallic = InMaterial.Metallic;
	OutSurface.Reflectance = InMaterial.Reflectance;
	OutSurface.Opacity = ComputeSurfaceOpacity(InMaterial.BaseColor.a, InMaterial.Opacity);
	OutSurface.AlphaCutoff = InMaterial.AlphaCutoff;

	// Every lighting term. A domain that wants to opt out of one clears its bit
	// here, and the lighting loop then skips that term with a single bit test.
	OutSurface.LightChannelMask = LIGHT_CHANNEL_ALL;

	// What is deliberately absent here is an ambient or indirect term. Indirect light is a
	// lighting contribution like any other, so a GI feature will feed it through the route the
	// lights take rather than through a material parameter, and this domain never grows an
	// Ambient input. Emission is the only thing a surface contributes on its own, and it is
	// carried in EmissiveColor.
}

// ---------------------------------------------------------------------------
// Shade: BRDF parameters to radiance
//
// The domain walks the light lists itself rather than being handed one light at
// a time, because a deferred lighting pass has to sum all of them anyway and a
// forward pass wants the same total. LightData is an internal packet of the
// lighting layer; it is not part of this signature.
// ---------------------------------------------------------------------------

#if SHADER_STAGE_PIXEL
void ShadeSurface(in SurfaceData InSurface, in MaterialPixelInput InInput, out ShadingResult OutResult)
{
	OutResult.Diffuse = float3(0.0, 0.0, 0.0);
	OutResult.Specular = float3(0.0, 0.0, 0.0);

	// Anisotropy is a clear coat and brushed metal feature; the default lit
	// domain is isotropic, so the anisotropic path is compiled out by a constant
	// false rather than by a switch.
	const bool IsAnisotropic = false;
	const float3 AnisotropicT = float3(1.0, 0.0, 0.0);
	const float3 AnisotropicB = float3(0.0, 0.0, 1.0);
	const float Anisotropy = 0.0;

	ShadeDirectionalLights(InSurface, InInput.WorldPosition, InInput.ViewDirection, InSurface.F0,
		IsAnisotropic, AnisotropicT, AnisotropicB, Anisotropy, OutResult);
	ShadeLocalLights(InSurface, InInput.WorldPosition, InInput.ViewDirection, InSurface.F0,
		IsAnisotropic, AnisotropicT, AnisotropicB, Anisotropy, OutResult);

	// The environment, after the lights. It is a lighting term and not a surface property, so
	// it is summed here with the rest rather than written into the GBuffer by the base pass -
	// which is also what keeps a material from having to declare an ambient input.
	ShadeIndirectDiffuse(InSurface, OutResult);
}
#endif	//SHADER_STAGE_PIXEL
#endif	//D_DEFAULT_LIT_HLSL
