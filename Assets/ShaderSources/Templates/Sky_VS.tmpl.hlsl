#ifndef SKY_VS_TMPL_HLSL
#define SKY_VS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Sky pass, vertex stage.
//
// The same fullscreen triangle as every other fullscreen pass. It declares no
// draw parameters, because the sky is drawn in view space and does not care
// where the camera is.
//
// The difference from the blit is the depth it lands on: this pass tests and
// writes depth, so a quad at the near plane would cover every pixel the base
// pass wrote. The sky is the far plane - the pixel stage reconstructs the view
// ray from the view projection, so the depth only has to be the one that loses
// against whatever geometry is already there.
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

	// The fullscreen factory has no world space to derive a depth from, so the far plane is
	// stated here instead of being transformed. RENDER_DEPTH_FAR is 1.0 and the depth buffer is
	// cleared to it, so a LESS_EQUAL test keeps the sky exactly where nothing was drawn.
	OutVarying.Position.z = 1.0;
}

#endif	//SKY_VS_TMPL_HLSL
