#ifndef M_ENGINE_DEBUG_WIREFRAME_HLSL
#define M_ENGINE_DEBUG_WIREFRAME_HLSL

// ---------------------------------------------------------------------------
// Engine debug wireframe material.
//
// One flat colour for a debug overlay, with no lighting. The Unlit domain
// carries it: the material writes a colour, the domain claims no light channel,
// and the pass writes the result straight into the target.
//
// VertexOps is false in its JSON, so it shares the neutral vertex shader with
// every other material that does not touch vertices. A wireframe's shape is the
// mesh's, and an overlay that displaced it would be measuring itself.
//
// The colour is a material parameter rather than a per primitive buffer. A
// per draw or per primitive colour is the renderer's business in this design --
// it writes the material parameters before the draw -- so colouring each
// primitive differently stays possible without the shader knowing how, which is
// what the legacy per instance colour buffer was doing from the wrong side.
// ---------------------------------------------------------------------------

#if SHADER_STAGE_PIXEL
void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);

	OutSurface.BaseColor = WireframeColor;
}

// Coverage only, for the depth only passes. A wireframe is opaque, so the honest
// answer is the surface's own coverage rather than a shortcut that would make a
// debug draw behave differently in a depth prepass than in the colour pass.
void CalcMaterialOpacity(in MaterialPixelInput InInput, out float OutOpacity, out float OutAlphaCutoff)
{
	MaterialSurface TempSurface;
	InitMaterialSurface(TempSurface);
	CalcMaterialSurface(InInput, TempSurface);
	OutOpacity = TempSurface.Opacity;
	OutAlphaCutoff = TempSurface.AlphaCutoff;
}
#endif	//SHADER_STAGE_PIXEL

#endif	//M_ENGINE_DEBUG_WIREFRAME_HLSL
