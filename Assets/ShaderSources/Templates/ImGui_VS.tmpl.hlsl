#ifndef IMGUI_VS_TMPL_HLSL
#define IMGUI_VS_TMPL_HLSL

// ---------------------------------------------------------------------------
// ImGui pass, vertex stage.
//
// A triangle list of UI vertices, transformed by the pass's own view projection.
// The projection is deliberately not a material parameter: the engine owns the
// world to clip step for every factory, so the orthographic matrix that turns
// pixel coordinates into clip space is published as this pass's
// ViewProjectionMatrix, and the draw's world matrix stays identity. That is what
// puts the UI matrix in the same place as every other pass's matrix instead of in
// a second constant buffer that only one pass reads.
//
// The vertex factory supplies the rest: it unpacks the UI vertex stream, which is
// an ImDrawVert and stays one all the way to the input layout.
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

#endif	//IMGUI_VS_TMPL_HLSL
