#ifndef FULL_SCREEN_VS_TMPL_HLSL
#define FULL_SCREEN_VS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Fullscreen pass, vertex stage.
//
// Draws the fullscreen triangle and nothing else: the vertex factory supplies
// the clip space position, this template only wires the vertex id and instance
// id into the factory context.
//
// Placeholder order is a hard constraint, not a style choice:
//   SYSTEM_MACROS         system macros first, everything below may test them
//   VF_VARYING_STRUCT     before anything that names VFVarying
//   VF_VERTEX_FACTORY     before the entry point, it declares VFAttributes
//   PROGRAMMABLE          before the entry point as well: it is the component that
//                         defines ExecuteMaterialVertex, and VF_VS_BODY calls it.
//                         A program that lists the component but leaves this
//                         placeholder out compiles the call with nothing behind it.
//   VF_VS_BODY            inside the entry point, it is the body
//
// Names are written without braces in this comment because this file is the
// injected text: a brace form mention would itself be replaced, and the
// replacement spans several lines, which would break the comment open.
//
// The factory context is filled from SV_VertexID / SV_InstanceID rather than
// from an input stream member, which keeps the input layout to the attributes
// the factory actually declared.
// ---------------------------------------------------------------------------

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}
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
	Context.PrimitiveID = 0;

	{VF_VS_BODY}
}
#endif	//FULL_SCREEN_VS_TMPL_HLSL
