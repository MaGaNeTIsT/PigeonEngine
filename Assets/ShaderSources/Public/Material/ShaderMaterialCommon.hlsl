#ifndef SHADER_MATERIAL_COMMON_HLSL
#define SHADER_MATERIAL_COMMON_HLSL

// ---------------------------------------------------------------------------
// The one header a material author includes.
//
// A material's .hlsl does not need to know where anything lives: this pulls in
// the system macros, the domain identities and the surface contract, and it
// gives the material the short spellings it is allowed to use.
//
// What a material may use:
//
//   SAMPLE_TEX2D / SAMPLE_TEX2D_LEVEL / SAMPLE_TEX2D_BIAS / SAMPLE_TEX2D_GRAD
//   SAMPLE_TEXCUBE / SAMPLE_TEXCUBE_LEVEL / SAMPLE_TEX3D / LOAD_TEX2D
//       Sampling always goes through these. They are aliases for the PE_ forms,
//       which are the real entry points; going through them is what makes the
//       bindless switch a change in one library file rather than in every
//       material.
//
//   MATERIAL_DOMAIN_*        the domain identities, for tests against
//                            MATERIAL_DOMAIN
//   MATERIAL_DOMAIN          the value this compilation was pinned to
//
// What a material must not do:
//
//   - declare a constant buffer, a texture or a sampler. Bindings are declared
//     in the material's JSON, and the compiler emits them.
//   - use a leading underscore on anything.
//   - test SHADER_USE_* style switches. They are gone: a program declares the
//     components it uses, and the compiler culls what a stage does not
//     reference.
//   - define an engine macro. MATERIAL_USE_* is the material's own namespace;
//     everything else belongs to the engine.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderSystemMacros.hlsl"
#include "Public/Material/MaterialDomainTypes.hlsl"
#include "Public/Surface/MaterialSurface.hlsl"
#include "Public/Surface/SurfaceData.hlsl"
#include "Public/Surface/SurfaceDerive.hlsl"
#include "Public/Surface/SurfaceCommon.hlsl"

// Short spellings, for material code only. Engine code uses the PE_ names, so
// that the places that must be touched when the binding path changes stay
// greppable.
#define SAMPLE_TEX2D(__Name, __Sampler, __UV)					PE_SAMPLE_TEX2D(__Name, __Sampler, __UV)
#define SAMPLE_TEX2D_LEVEL(__Name, __Sampler, __UV, __Lod)		PE_SAMPLE_TEX2D_LEVEL(__Name, __Sampler, __UV, __Lod)
#define SAMPLE_TEX2D_BIAS(__Name, __Sampler, __UV, __Bias)		PE_SAMPLE_TEX2D_BIAS(__Name, __Sampler, __UV, __Bias)
#define SAMPLE_TEX2D_GRAD(__Name, __Sampler, __UV, __Ddx, __Ddy)	PE_SAMPLE_TEX2D_GRAD(__Name, __Sampler, __UV, __Ddx, __Ddy)
#define LOAD_TEX2D(__Name, __Coord)								PE_LOAD_TEX2D(__Name, __Coord)
#define SAMPLE_TEX3D(__Name, __Sampler, __UVW)					PE_SAMPLE_TEX3D(__Name, __Sampler, __UVW)
#define SAMPLE_TEX3D_LEVEL(__Name, __Sampler, __UVW, __Lod)		PE_SAMPLE_TEX3D_LEVEL(__Name, __Sampler, __UVW, __Lod)
#define SAMPLE_TEXCUBE(__Name, __Sampler, __Direction)			PE_SAMPLE_TEXCUBE(__Name, __Sampler, __Direction)
#define SAMPLE_TEXCUBE_LEVEL(__Name, __Sampler, __Direction, __Lod)	PE_SAMPLE_TEXCUBE_LEVEL(__Name, __Sampler, __Direction, __Lod)
#define SAMPLE_TEX2DARRAY(__Name, __Sampler, __UV)				PE_SAMPLE_TEX2DARRAY(__Name, __Sampler, __UV)
#define SAMPLE_TEX2DARRAY_LEVEL(__Name, __Sampler, __UV, __Lod)	PE_SAMPLE_TEX2DARRAY_LEVEL(__Name, __Sampler, __UV, __Lod)

// The sampler names a material is allowed to reference. There are three, they
// come from the engine's sampler configuration, and a material may not declare
// its own: that is what makes a quality setting able to change the sampling
// behaviour of every material at once.
#define SAMPLER_MATERIAL_2D			SamplerMaterial2D
#define SAMPLER_MATERIAL_CUBE		SamplerMaterialCube
#define SAMPLER_MATERIAL_3D			SamplerMaterial3D
#endif	//SHADER_MATERIAL_COMMON_HLSL
