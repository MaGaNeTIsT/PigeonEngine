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
#define SAMPLE_TEX2D(InName, InSampler, InUV)					PE_SAMPLE_TEX2D(InName, InSampler, InUV)
#define SAMPLE_TEX2D_LEVEL(InName, InSampler, InUV, InLod)		PE_SAMPLE_TEX2D_LEVEL(InName, InSampler, InUV, InLod)
#define SAMPLE_TEX2D_BIAS(InName, InSampler, InUV, InBias)		PE_SAMPLE_TEX2D_BIAS(InName, InSampler, InUV, InBias)
#define SAMPLE_TEX2D_GRAD(InName, InSampler, InUV, InDdx, InDdy)	PE_SAMPLE_TEX2D_GRAD(InName, InSampler, InUV, InDdx, InDdy)
#define LOAD_TEX2D(InName, InCoord)								PE_LOAD_TEX2D(InName, InCoord)
#define SAMPLE_TEX3D(InName, InSampler, InUVW)					PE_SAMPLE_TEX3D(InName, InSampler, InUVW)
#define SAMPLE_TEX3D_LEVEL(InName, InSampler, InUVW, InLod)		PE_SAMPLE_TEX3D_LEVEL(InName, InSampler, InUVW, InLod)
#define SAMPLE_TEXCUBE(InName, InSampler, InDirection)			PE_SAMPLE_TEXCUBE(InName, InSampler, InDirection)
#define SAMPLE_TEXCUBE_LEVEL(InName, InSampler, InDirection, InLod)	PE_SAMPLE_TEXCUBE_LEVEL(InName, InSampler, InDirection, InLod)
#define SAMPLE_TEX2DARRAY(InName, InSampler, InUV)				PE_SAMPLE_TEX2DARRAY(InName, InSampler, InUV)
#define SAMPLE_TEX2DARRAY_LEVEL(InName, InSampler, InUV, InLod)	PE_SAMPLE_TEX2DARRAY_LEVEL(InName, InSampler, InUV, InLod)

// The sampler names a material is allowed to reference. There are three, they
// come from the engine's sampler configuration, and a material may not declare
// its own: that is what makes a quality setting able to change the sampling
// behaviour of every material at once.
#define SAMPLER_MATERIAL_2D			SamplerMaterial2D
#define SAMPLER_MATERIAL_CUBE		SamplerMaterialCube
#define SAMPLER_MATERIAL_3D			SamplerMaterial3D
#endif	//SHADER_MATERIAL_COMMON_HLSL
