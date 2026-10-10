#ifndef TRANSLUCENT_VS_TMPL_HLSL
#define TRANSLUCENT_VS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Translucent pass, vertex stage.
//
// Identical in shape to the base pass vertex stage. It is a separate program
// rather than a mode of it because the pass writes one target instead of seven,
// and a program that declares the GBuffer would pay for bindings it never writes.
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

#endif	//TRANSLUCENT_VS_TMPL_HLSL
