#ifndef C_SHADOWS_HLSL
#define C_SHADOWS_HLSL

// ---------------------------------------------------------------------------
// Body of the shadow component.
//
// The bindings are declared in the JSON and the compiler emits them as part of the
// generated header, which sits above the template body. This file exists because a
// component's Template is where its HLSL lives and the injection point needs a file
// to expand.
//
//   ShadowMapAtlas   the depth atlas every shadow view renders into. Its depth is the
//                    light space depth of the nearest caster, written by the shadow pass
//                    and read here with a typed load: there is no comparison sampler in
//                    the engine, so the depth test is written out in
//                    Public/Lighting/ShaderShadows.hlsl.
//
//   ShadowSlots      one element per shadow view plus element zero, the "this light casts
//                    no shadow" sentinel. The layout below mirrors EShadowSlotGPU in
//                    RenderProxy/ShadowViewProxy.h member for member, and the engine checks
//                    the two against each other at runtime the same way it checks a light
//                    list.
//
//                     xy  = tile UV scale      zw = tile UV offset in the atlas
//                     x   = cascade far split (world, from the view origin)
//                     y   = 1 / tile size (texel size in tile UV)
//                     z   = cascade or point face index
//                     w   = 1 when the slot is valid, 0 when it is the sentinel
//                     x   = constant depth bias, y = slope scaled bias,
//                     z   = normal offset in world units, w = cascade blend fraction
//
// The atlas and the slot table are view owned: a cascade belongs to the camera view it was
// derived from, so a program that declares this component is compiling its receiver side for
// one view and one frame. A compilation that only derives a surface - the base pass - does not
// declare it, and the code that would read these bindings is guarded by SHADER_SHADOW_RECEIVER
// rather than being compiled against names it does not have.
// ---------------------------------------------------------------------------
#endif	//C_SHADOWS_HLSL
