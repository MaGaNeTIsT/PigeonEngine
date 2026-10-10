#ifndef C_LIGHTING_HLSL
#define C_LIGHTING_HLSL

// ---------------------------------------------------------------------------
// Body of the lighting component.
//
// Injected at the LIGHTING placeholder by a program whose shading needs the
// lights. The bindings themselves are declared in the JSON:
//
//   EnvironmentCube    a cube SRV, sampled through SamplerMaterialCube, which
//                      comes from the generated material sampler group
//   DirectionalLights  a structured buffer, element layout in the JSON
//   LocalLights        a structured buffer, element layout in the JSON
//
// ---------------------------------------------------------------------------
// The element layouts live in the JSON rather than in Public/Lighting, and that
// is on purpose: the binding contract belongs next to the binding. Two rules
// keep the two sides in step:
//
//   1. EVERY MEMBER IS ONE 16 BYTE SLOT - a float4 or a uint4, never a float3
//      followed by a float. A compiler will pack the tighter form into the same
//      register today, but that is a detail of one packing rule; a slot per
//      member is a layout that both sides can compute from the field count
//      alone, which is what makes the reflection check on the C++ side possible.
//
//   2. THE SPLIT IS DESCRIBED ONCE, in Decode* in
//      Public/Lighting/ShaderLightData.hlsl. The generated names carry a _Struct
//      suffix (DirectionalLights_Struct), that file aliases them, and only the
//      decoders know which component of which slot means what. Shading code
//      reads named fields and never touches a slot.
//
// Renaming or reordering a field here means updating ShaderLightData.hlsl.
//
// ---------------------------------------------------------------------------
// Units. The counts are in GlobalParams, and the values are photometric:
//
//   DirectionalLights  .x DirectionAndIlluminanceLux  .xyz direction, .w [lux]
//   LocalLights        .x WorldPositionAndAttenuationRadiusMeters, .w [m]
//                      .y ColorAndLuminousIntensityCandela, .w [candela]
//
// See ShaderLightData.hlsl for why there is no luminous efficacy constant in the
// pipeline.
//
// A light list is a global fact and not a view one, so both buffers are uploaded
// once per frame and every view shares them. The counts are published in
// GlobalParams for the same reason. What a view does own is the shadow
// projection of a cascade, which is why the light structs carry a shadow map
// index and no matrix.
//
// There are no shadow map bindings yet: the shadow terms are stubs that return
// "fully lit" until the shadow algorithms land, and declaring buffers nothing
// samples would only cost slots and documents a layout that is not settled.
// ---------------------------------------------------------------------------
#endif	//C_LIGHTING_HLSL
