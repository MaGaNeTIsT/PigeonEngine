#ifndef TRANSLUCENT_PS_TMPL_HLSL
#define TRANSLUCENT_PS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Translucent pass, pixel stage.
//
// The forward path. Where the base pass derives a surface and lets the deferred
// lighting pass shade it later, this one does everything in one go: material,
// derive, shade, and write the result straight into the scene colour with the
// surface's own coverage in alpha.
//
// It is the reason EvaluateSurface exists. A domain's shading half is not
// deferred specific, and running it here means a translucent surface and an
// opaque one are shaded by exactly the same code.
//
// No GBuffer targets: a translucent surface has nothing to put in a deferred
// buffer, since the thing behind it has not been resolved yet.
//
// The material's own indirect light is added here rather than being read back
// from a target, because there is no target holding it for this pixel.
// ---------------------------------------------------------------------------

#include "Public/Lighting/ShaderDirectLighting.hlsl"
#include "Public/Lighting/ShaderIndirectLighting.hlsl"

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}
{DRAW_PARAMS}
{VF_VERTEX_FACTORY}
{PROGRAMMABLE}
{LIGHTING}

void PSMain(in VFVarying InVarying,
	in uint InPrimitiveID : SV_PrimitiveID,
	in bool InIsFrontFace : SV_IsFrontFace,
	out float4 OutSceneColor : SV_Target0)
{
	MaterialPixelInput MatInput = BuildMaterialPixelInput(InVarying, ViewPosition.xyz, InPrimitiveID, InIsFrontFace);

	SurfaceData Surface;
	ShadingResult Result;
	EvaluateSurface(MatInput, Surface, Result);

	// The surface's own light plus whatever the domain shaded. There is no indirect term: an
	// indirect contribution arrives through the lighting path, not as a surface field.
	float3 Color = Result.Diffuse + Result.Specular + Surface.EmissiveColor;

	// Coverage in alpha. The blend state is the renderer's, driven by the surface
	// mode; the shader only has to report it.
	OutSceneColor = float4(Color, Surface.Opacity);
}

#endif	//TRANSLUCENT_PS_TMPL_HLSL
