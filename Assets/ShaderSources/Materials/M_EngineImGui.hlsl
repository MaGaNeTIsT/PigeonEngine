#ifndef M_ENGINE_IMGUI_HLSL
#define M_ENGINE_IMGUI_HLSL

// ---------------------------------------------------------------------------
// Engine ImGui material.
//
// The UI's own material: a texture sample tinted by the interpolated vertex
// colour, with no lighting. The Unlit domain carries it, and the Translucent
// surface mode is what puts the alpha into the blend state, which is what makes a
// translucent panel translucent.
//
// The vertex colour is the interesting half. ImGui packs a widget's tint and its
// coverage into the same four bytes, so multiplying the sample by it -- rather
// than choosing one of the two -- is what makes a faded panel fade and an
// unfaded one not, and it is also what ImGui's own renderer does. Anything else
// would draw a widget that disagrees with the widget ImGui thinks it drew.
//
// The projection matrix is not here. It used to be a material parameter, which
// meant a UI draw needed a constant buffer that no other pass had; it is a view
// matrix in this design, so the pass publishes it in ViewParams and this material
// is left with the two things it actually owns: its texture and its tint.
//
// VertexOps is false in its JSON, so it shares the neutral vertex shader with
// every other material that does not touch vertices. The vertex work a UI draw
// needs -- unpacking the ImDrawVert stream -- belongs to the vertex factory, not
// to the material.
//
// The bindless header is included rather than assumed. The macro this file
// samples through is defined there, and a material that used it without pulling
// it in would compile only because of the order its injection point happens to
// have today.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"

#if SHADER_STAGE_PIXEL
void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);

	// SamplerLinearClamp, not a material sampler: the UI samples an atlas and a
	// wrap address mode would bleed neighbouring glyphs into the edge of every
	// quad, so the texture names the engine sampler that matches the behaviour
	// ImGui's own renderer uses.
	float4 TextureColor = PE_SAMPLE_TEX2D(FontTexture, SamplerLinearClamp, InInput.UV0);

	OutSurface.BaseColor = TextureColor * InInput.VertexColor;
}

// Coverage only, for a pass that clips but does not shade. The UI is translucent,
// so nothing calls this today; it is here because a material that a depth pass
// picks up must answer the question, and answering it with the same product the
// colour path uses is the only answer that cannot disagree with the colour path.
void CalcMaterialOpacity(in MaterialPixelInput InInput, out float OutOpacity, out float OutAlphaCutoff)
{
	MaterialSurface TempSurface;
	InitMaterialSurface(TempSurface);
	CalcMaterialSurface(InInput, TempSurface);
	OutOpacity = TempSurface.Opacity;
	OutAlphaCutoff = TempSurface.AlphaCutoff;
}
#endif	//SHADER_STAGE_PIXEL

#endif	//M_ENGINE_IMGUI_HLSL
