#ifndef DEPTH_ONLY_PS_TMPL_HLSL
#define DEPTH_ONLY_PS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Depth only pass, pixel stage.
//
// Two jobs, and only when the surface mode asks for one of them:
//
//   Mask         evaluate the material's coverage and discard below the cutoff.
//                This is what makes an alpha clipped surface cast an alpha
//                clipped shadow.
//   everything else
//                the entry point still runs, but it writes nothing, so the depth
//                the vertex stage produced is all this pass contributes.
//
// There is no output parameter. A pixel entry point with no render target is
// legal and it is the honest description of the pass: the depth buffer is
// written by the rasterizer from SV_Position, not by the shader.
//
// It calls the material's coverage entry point rather than the full material
// entry point, which is why that entry point exists: evaluating every texture a
// material has, in order to read one channel, would make a shadow pass as
// expensive as a base pass.
// ---------------------------------------------------------------------------

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}
{DRAW_PARAMS}
{VF_VERTEX_FACTORY}
{PROGRAMMABLE}

void PSMain(in VFVarying InVarying,
	in uint InPrimitiveID : SV_PrimitiveID,
	in bool InIsFrontFace : SV_IsFrontFace)
{
	MaterialPixelInput MatInput = BuildMaterialPixelInput(InVarying, ViewPosition.xyz, InPrimitiveID, InIsFrontFace);

	float Opacity;
	float AlphaCutoff;
	CalcMaterialOpacity(MatInput, Opacity, AlphaCutoff);

#if MATERIAL_SURFACE_MODE == MATERIAL_SURFACE_MODE_MASK
	if (IsAlphaDiscarded(Opacity, AlphaCutoff))
	{
		discard;
	}
#endif	//MATERIAL_SURFACE_MODE == MATERIAL_SURFACE_MODE_MASK
}

#endif	//DEPTH_ONLY_PS_TMPL_HLSL
