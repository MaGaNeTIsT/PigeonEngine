#ifndef IMGUI_PS_TMPL_HLSL
#define IMGUI_PS_TMPL_HLSL

// ---------------------------------------------------------------------------
// ImGui pass, pixel stage.
//
// The UI is drawn with the ordinary surface assembly and then blended, which is
// the whole trick: the material has already multiplied the atlas sample by the
// vertex colour, so the colour that arrives here is the widget's final colour and
// its alpha is the coverage that the blend state consumes.
//
// Coverage travels in alpha and the renderer turns blending on, because the
// surface mode is Translucent. The shader's half of that contract is to report
// the coverage honestly and write nothing else about it -- which blend state to
// use is the renderer's decision, driven by the same material asset.
//
// No mask branch: a translucent surface is never alpha clipped. The cutoff test
// belongs to the passes that write depth, and this one does not write depth at
// all, so a branch for it would be a branch that can never be taken.
//
// Placeholder order is a hard constraint, not a style choice; the reasoning is in
// the base pass pixel template, and this one adds nothing to it.
// ---------------------------------------------------------------------------

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}
{DRAW_PARAMS}
{VF_VERTEX_FACTORY}
{PROGRAMMABLE}
{LIGHTING}

void PSMain(in VFVarying InVarying,
	in uint InPrimitiveID : SV_PrimitiveID,
	in bool InIsFrontFace : SV_IsFrontFace,
	out float4 OutTarget : SV_Target0)
{
	MaterialPixelInput MatInput = BuildMaterialPixelInput(InVarying, ViewPosition.xyz, InPrimitiveID, InIsFrontFace);

	SurfaceData Surface;
	ShadingResult Result;
	EvaluateSurface(MatInput, Surface, Result);

	// The surface's own light. There is no indirect term to add: indirect light is a lighting
	// contribution, so it arrives the way the lights do rather than as a surface field.
	//
	// No exposure correction either, unlike the debug overlay: this pass writes to the back
	// buffer after the display boundary has been applied, so its colour is already display
	// referred and the editor's palette means what it says.
	float3 Color = Result.Diffuse + Result.Specular + Surface.EmissiveColor;

	// Coverage in alpha; the blend state is the renderer's, driven by the surface
	// mode. The UI's alpha is what makes a translucent panel translucent, so it is
	// the one channel this pass must not round off.
	OutTarget = float4(Color, Surface.Opacity);
}

#endif	//IMGUI_PS_TMPL_HLSL
