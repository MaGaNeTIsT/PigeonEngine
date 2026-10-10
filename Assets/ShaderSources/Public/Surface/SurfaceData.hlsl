#ifndef SURFACE_DATA_HLSL
#define SURFACE_DATA_HLSL

// ---------------------------------------------------------------------------
// SurfaceData: the BRDF parameter bundle that the encode side, the decode side
// and the shading functions all agree on.
//
// It is engine owned. A domain fills the fields its shading model needs inside
// DeriveSurface, the GBuffer encoders read them, the decoders reconstruct them,
// and ShadeSurface consumes them. Keeping one structure means the deferred path
// and the forward path feed the shading code with identically shaped input,
// which is what makes a single ShadeSurface implementation usable in both.
//
// Field notes:
//   DiffuseColor / F0 / EmissiveColor are HDR, not display values. They go into
//   a float or a 16 bit float render target without being clamped first.
//   Roughness is the true value, PerceptualRoughness is its square root. Both
//   are kept because the perceptual one is what artist facing parameters and
//   GBuffer storage use, while the BRDF wants the true one. The relation is
//   roughness = perceptualRoughness * perceptualRoughness, which is also what
//   the design means by "removing the perceptual linearity".
//   Standard two layer BRDFs such as clear coat take their shadowing and
//   fresnel terms from their own layer, which is why the clear coat domain can
//   carry extra fields without changing anything here.
//
// Domain extension: the design allows a domain to extend the BRDF parameter
// bundle (clear coat, sheen, subsurface). There is no extension point in this
// version because neither of the two first domains needs one, and it has to be
// designed together with the first domain that does. See the implementation log
// for the open item.
// ---------------------------------------------------------------------------

struct SurfaceData
{
	// ---- material response, HDR ----
	//
	// BaseColor is carried alongside the derived values for two reasons.
	//
	// The GBuffer stores BaseColor, because that is the cheapest complete
	// description of a metallic surface: BaseColor plus Metallic is four
	// channels, while storing the diffuse colour and the specular colour
	// separately would take six. The decode side therefore has to rebuild
	// DiffuseColor and F0, and it cannot do that from DiffuseColor alone: a metal
	// has no diffuse term, so it would decode black.
	//
	// So the derive step writes BaseColor together with the values derived from
	// it, and the decode side calls the same derive helpers the material path
	// uses, which is what makes a deferred pixel indistinguishable downstream.
	float3	BaseColor;
	float3	DiffuseColor;
	float3	F0;						// normal incidence specular reflectance
	float3	EmissiveColor;

	// ---- geometry ----
	float3	Normal;					// world space, normalized

	// ---- roughness, both forms ----
	float	PerceptualRoughness;
	float	Roughness;

	// ---- kept for the GBuffer round trip ----
	float	Metallic;
	float	Reflectance;

	// ---- coverage ----
	float	AmbientOcclusion;
	float	Opacity;
	float	AlphaCutoff;

	// ---- which lighting terms this pixel takes part in (goes to GBufferD) ----
	uint	LightChannelMask;
};

// ---------------------------------------------------------------------------
// ShadingResult: what ShadeSurface hands back.
//
// Lighting, both kinds, in luminance [cd/m2]. A surface's own emission is not
// here: it is carried in SurfaceData (EmissiveColor) and the base pass writes it
// into the scene colour target, so the deferred lighting pass adds its terms on
// top of emission that is already in place.
//
// The indirect term is summed here along with the lights, and that is the point
// rather than a convenience: indirect light is a lighting contribution like any
// other, so it arrives through the lighting path and no material ever declares an
// ambient parameter. The environment's diffuse half is the first thing to feed it
// (Public/Lighting/ShaderIndirectDiffuse.hlsl); a GI feature would add to the same
// sum, and nothing above this struct has to change when one does.
// ---------------------------------------------------------------------------

struct ShadingResult
{
	float3	Diffuse;
	float3	Specular;
};
#endif	//SURFACE_DATA_HLSL
