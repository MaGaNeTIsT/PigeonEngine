#ifndef DEBUG_WIREFRAME_PS_TMPL_HLSL
#define DEBUG_WIREFRAME_PS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Debug wireframe pass, pixel stage.
//
// One target, one flat colour. The pass runs the ordinary surface assembly -- the
// material produces a surface, the domain derives it and shades it -- and then
// writes the result, which for the Unlit domain is the material's own colour
// arriving through the indirect channel. Nothing here inspects the geometry: a
// debug overlay is supposed to look the same on every surface it crosses, so the
// pixel stage has no normal, no coverage and no lighting to consider.
//
// It is the forward shape rather than the deferred one, which is why it calls
// EvaluateSurface and writes a single target instead of encoding a GBuffer. A
// debug pass has no later pass to shade it, and a GBuffer written by a debug
// overlay would be wrong for the geometry underneath it.
//
// The surface mode is Opaque, so alpha is not coverage and the target's alpha is
// written as 1: the material's opacity is deliberately not consulted, because a
// wireframe the scene can see through reads as a bug rather than as a feature.
//
// No mask branch, for the same reason: this pass is Opaque by definition, and a
// branch that can never be taken is a lie about what the pass does. A masked
// debug overlay would be a different program.
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

	// The same sum the translucent pass makes, so a domain that puts its colour somewhere
	// other than emission -- a future lit domain -- is carried correctly without this pass
	// knowing which it is. For the Unlit domain this is the material's own colour, which is
	// what a wireframe is.
	//
	// The instance colour multiplies it, and the two together are the overlay's colour: the
	// material states the pass's overall tint (WireframeColor, exposed in the editor) and every
	// queued shape carries the colour its caller asked for. Both default to white, so an
	// untouched pair draws white.
	float3 Color = Result.Diffuse + Result.Specular + Surface.EmissiveColor;

	// An overlay's colour is authored for a display, not for a scene: the editor picks it, and
	// it means the same thing whether the camera is exposed for noon or for moonlight. The
	// scene colour target holds luminance, so writing the colour there unscaled would put it at
	// about 1 cd/m2, which any daylight exposure turns black. Dividing by the exposure gain is
	// what undoes the camera: 1 / gain is the luminance that maps to display white, so the
	// overlay lands at the brightness it was authored at and stays legible at any exposure.
	//
	// Going through the exposure and back is a deliberate two way trip rather than a bypass:
	// the overlay shares the target, and a value that skipped exposure would be the only thing
	// in the frame that did. Drawing the overlay after the display boundary instead - in the
	// editor UI layer rather than in the scene - is the cleaner end state and a later change.
	const float DisplayWhiteLuminance = 1.0 / max(ExposureParams.w, 1e-8);
	OutTarget = float4(Color * InVarying.Color.rgb * DisplayWhiteLuminance, 1.0);
}

#endif	//DEBUG_WIREFRAME_PS_TMPL_HLSL
