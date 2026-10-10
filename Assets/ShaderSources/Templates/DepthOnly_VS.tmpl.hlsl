#ifndef DEPTH_ONLY_VS_TMPL_HLSL
#define DEPTH_ONLY_VS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Depth only pass, vertex stage.
//
// The same vertex work as the base pass, minus the material's pixel half. It
// exists as its own program rather than as a mode of the base pass because a
// shadow or prepass view has no GBuffer and no colour target, and a program that
// declared them would pay for bindings it never writes.
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
	Context.PrimitiveID = 0;

	{VF_VS_BODY}
}

#endif	//DEPTH_ONLY_VS_TMPL_HLSL
