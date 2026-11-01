#ifndef M_DEMO_UNLIT_HLSL
#define M_DEMO_UNLIT_HLSL

// ---------------------------------------------------------------------------
// Demo unlit material: an emission map times a per channel excitation.
//
// The material a project copies for its first emitting material. It is the Unlit
// domain's shape - colour in, colour out, no lighting - plus the two things the
// engine's own materials do not show: a texture that asks for the descriptor heap
// path, and an emission the material states in physical units.
//
// The convention behind the numbers, because they look large on purpose: an
// emission is a luminance the material states, and the engine takes it as it is.
// Nothing downstream rescales it to the exposure - doing that would break the
// physical agreement every light in the frame is held to - so a surface that has
// to be visible under a daylight exposure has to emit a daylight sized number.
// Sunny-16 puts display white near 25 000, a clear sky sits around 8 000, and the
// default excitation below is 15 000: bright, and still inside the range the tone
// curve resolves rather than clips.
//
// The split of labour is the point of the material: the map supplies the shape of
// the emission (a linear 0..1 value per texel) and the constant buffer supplies its
// level per channel, so an artist tints the glow by raising one channel above the
// others without touching the image.
//
// Both texture paths come out of this one file. The sample goes through
// PE_SAMPLE_TEX2D, which the library writes twice - once for a register bound
// texture, once for a heap index - and the compiler picks the one this target can
// express: a 6.6 build reads EmissiveMap through ResourceDescriptorHeap with the
// index the engine wrote into MaterialParams, an earlier build samples the
// register bound texture its declaration produced, and this file does not change
// between them.
//
// VertexOps is false in its JSON, so it shares the neutral vertex shader with
// every other material that does not touch vertices.
//
// The bindless header is included rather than assumed: the macro this file
// samples through is defined there, and a material that used it without pulling
// it in would compile only because of the order its injection point happens to
// have today.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"

#if SHADER_STAGE_PIXEL
void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);

	// The map is the emission's shape, the constant buffer is its level. What reaches the engine is
	// the product, in the unit the frame's exposure is set against.
	const float3 EmissiveShape = PE_SAMPLE_TEX2D(EmissiveMap, SamplerMaterial2D, InInput.UV0).rgb;

	// The unlit domain adds the base colour and the emission into the scene colour target, so a
	// material that emits through the emission field leaves the base colour at zero: leaving the
	// default white there would add an unlit white term on top of the glow.
	OutSurface.BaseColor = float4(0.0, 0.0, 0.0, 1.0);
	OutSurface.EmissiveColor = EmissiveShape * EmissiveExcitation;
}

// Coverage only, for a pass that clips but does not shade. It answers with the
// same product the colour path uses, so a depth pass and the base pass cannot
// disagree about where a surface is.
void CalcMaterialOpacity(in MaterialPixelInput InInput, out float OutOpacity, out float OutAlphaCutoff)
{
	MaterialSurface TempSurface;
	InitMaterialSurface(TempSurface);
	CalcMaterialSurface(InInput, TempSurface);
	OutOpacity = TempSurface.Opacity;
	OutAlphaCutoff = TempSurface.AlphaCutoff;
}
#endif	//SHADER_STAGE_PIXEL

#endif	//M_DEMO_UNLIT_HLSL
