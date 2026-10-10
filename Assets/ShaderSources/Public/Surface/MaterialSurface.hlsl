#ifndef MATERIAL_SURFACE_HLSL
#define MATERIAL_SURFACE_HLSL

// ---------------------------------------------------------------------------
// MaterialSurface: what a material writes, and the contract every domain keeps.
//
// The structure itself is NOT declared here. Each domain declares the complete
// MaterialSurface in its own .hlsl -- the common core fields plus whatever it
// extends them with -- and a material's entry point is written against whatever
// selected domain declared. That is deliberate: it is what makes "the fields an
// artist can write depend on the chosen domain" true rather than a convention.
//
// This file documents the core, so a domain author has one place to look.
//
//   common core fields (every domain has these)
//   ---------------------------------------------------------------
//   BaseColor          float4    rgb base colour, a opacity     (1, 1, 1, 1)
//   Metallic           float                                     (0)
//   Reflectance        float     dielectric specular reflectance (0.5)
//   Roughness          float     perceptual roughness            (1)
//   Normal             float3    tangent space normal            (0, 0, 1)
//   EmissiveColor      float3                                    (0, 0, 0)
//   AmbientOcclusion   float                                     (1)
//   Opacity            float                                     (1)
//   AlphaCutoff        float     used by the mask surface mode    (0.5)
//
//   per domain extension fields
//   ---------------------------------------------------------------
//   DefaultLit   none
//   Unlit        none (the lighting related fields are simply ignored)
//   ClearCoat    ClearCoat / ClearCoatRoughness / ClearCoatNormal / ...
//   Sheen        SheenColor / SheenRoughness
//   Subsurface   SubsurfaceColor / SubsurfaceRadius / SubsurfaceFalloff
//
// Rules a domain must follow:
//
// 1. It declares the whole structure. The core fields keep their names and
//    their meaning; a domain never renames or repurposes one.
// 2. It implements InitMaterialSurface, which must write EVERY field. The
//    defaults above are the contract, and writing them all is what removes the
//    need for "is this map connected" switches: a material that does not touch
//    Normal simply keeps the default.
// 3. It may add fields. It must not reach into the core and add something of
//    its own there, because the core is shared by every domain and by the
//    encode side.
// 4. It never changes another domain's semantics. Each domain is self contained.
//
// Because the structure is domain owned, the material entry point signature stays
// fixed while its field set does not:
//
//   void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
//
// which is exactly the point: what an artist can write is decided by the domain
// they picked, and the shape they see in their template is the shape that
// domain offers.
// ---------------------------------------------------------------------------
#endif	//MATERIAL_SURFACE_HLSL
