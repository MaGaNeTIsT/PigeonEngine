#ifndef C_INDIRECT_LIGHTING_HLSL
#define C_INDIRECT_LIGHTING_HLSL

// ---------------------------------------------------------------------------
// Body of the indirect lighting component.
//
// The bindings are declared in the JSON and the compiler emits them as part of
// the generated header, which sits above the template body. This file exists
// because a component's Template is where its HLSL lives and the injection point
// needs a file to expand.
//
//   IndirectLightingParams
//     SkyLightColorLuminance    float4   rgb = the sky's tint, w = its mean luminance [cd/m2]
//     SkyCubeMeanLuminance      float    the cube's own mean linear luminance, divided out
//     PrefilterMipCount         float    levels in the prefiltered chain, for the roughness map
//
// The block's name is not a scope in HLSL - a cbuffer declares its members and nothing
// else - so the field above is read as SkyLightColorLuminance and never as
// IndirectLightingParams.SkyLightColorLuminance.
//
//   PrefilteredEnvironmentCube  the split sum's radiance half, normalized to unit mean by
//                               the prefilter; a cube on the read side
//   EnvironmentBRDFLUT          the DFG table: scale in .x, bias in .y
//
// Registers are not named here: the compiler folds every component's bindings into one
// contiguous set and assigns the final slots, so the JSON 'Slot' fields are ordering hints
// and the engine reads the real ones back from the reflection.
//
//   EnvironmentSH
//     a structured buffer of nine float4, one radiance coefficient each, in the
//     order Public/Lighting/ShaderSphericalHarmonics.hlsl defines
//
// ---------------------------------------------------------------------------
// Why the environment is bound in two pieces instead of one
//
// The coefficients are the environment's shape and the parameter block is its
// brightness, and separating them is what makes the brightness free to edit: the
// bake is a compute dispatch over the whole cubemap, while the luminance and the
// tint are two values the lighting pass multiplies in. Dragging the sky's
// brightness slider therefore costs nothing, and only swapping the cubemap asset
// re-runs the bake.
//
// That split is a contract, not an optimisation: the bake is allowed to produce
// dimensionless values precisely because the scale is applied here. A future
// source of environment light that carries absolute radiance - an HDR capture -
// would record its mean luminance as this block's default rather than baking the
// scale in, which is what keeps one knob meaning one thing for both kinds of
// source.
//
// ---------------------------------------------------------------------------
// Why the lighting and base passes declare this component and the sky does not
//
// Both of those passes compile a domain's shading half, and the shading half is
// what reads these bindings. A pass that never shades culls them, and the sky is
// such a pass: it samples the environment cube directly for its own background,
// so it needs the cube and nothing here.
// ---------------------------------------------------------------------------
#endif	//C_INDIRECT_LIGHTING_HLSL
