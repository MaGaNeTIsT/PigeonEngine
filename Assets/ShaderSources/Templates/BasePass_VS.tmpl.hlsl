#ifndef BASE_PASS_VS_TMPL_HLSL
#define BASE_PASS_VS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Base pass, vertex stage.
//
// The deferred geometry pass. It writes the GBuffer and the indirect part of the
// scene colour, and it does not shade: the direct lighting happens later, in the
// deferred lighting pass, which is why this template never calls ShadeSurface.
//
// Placeholder order is a hard constraint, not a style choice:
//
//   SYSTEM_MACROS         first, everything below may test them
//   VF_VARYING_STRUCT     before anything that names VFVarying
//   DRAW_PARAMS           the per draw constant buffer, for the world matrices
//   VF_VERTEX_FACTORY     declares VFAttributes and the factory's three functions
//   PROGRAMMABLE          the engine's bridge and the material entry point; it
//                         names VFAttributes, so it has to come after the factory
//
// The domain and the material are NOT injected here. They are consumed by the
// programmable layer, which is the surface assembly, and injecting them in two
// places would splice the same code into the compilation unit twice.
//
// Names in this comment are written without braces so the injector does not
// rewrite them: this file is the injected text.
// ---------------------------------------------------------------------------

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}
{DRAW_PARAMS}
{VF_VERTEX_FACTORY}
{PROGRAMMABLE}

void VSMain(in VFAttributes InAttributes,
	in uint InVertexID : SV_VertexID,
	in uint InInstanceID : SV_InstanceID,
	out VFVarying OutVarying)
{
	VFContext Context;
	Context.VertexID = InVertexID;
	Context.InstanceID = InInstanceID;

	// The rasterizer produces a primitive id per triangle; the vertex stage has
	// no such semantic, and a vertex factory that needs one for a lookup derives
	// it from the vertex id instead.
	Context.PrimitiveID = 0;

	{VF_VS_BODY}
}
#endif	//BASE_PASS_VS_TMPL_HLSL
