#ifndef COPY_PS_TMPL_HLSL
#define COPY_PS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Copy pass, pixel stage.
//
// This is not a blit any more: it is the scene to display boundary. The scene
// colour target holds linear scene referred luminance in cd/m2 and is never
// clamped, so this pass is where a display value is decided - exposure, tone
// map, transfer function, in that order, all of it stated in
// Public/PostProcess/ShaderToneMapping.hlsl.
//
// The encoding is explicit because the back buffer is R8G8B8A8_UNORM and not
// _SRGB: a UNORM target stores what it is given, so the linear to sRGB step has
// to happen in the shader. Pointing the back buffer at an _SRGB format instead
// would move the step into the fixed function unit and double encode the moment
// anyone also called LinearToSrgb here.
//
//   CopySourceTexture   declared by the program description, lands on t0
//   ExposureParams      ViewParams, published by RViewProxy for this view
//   SamplerLinearClamp  owned by the system layer, lands on s1
//
// The sample goes through PE_SAMPLE_TEX2D, which is the only sanctioned way to
// read a texture: it is what makes the bindless switch a one file change later.
// ---------------------------------------------------------------------------

#include "Public/PostProcess/ShaderToneMapping.hlsl"

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}

void PSMain(in VFVarying InVarying,
	out float4 OutTarget : SV_Target0)
{
	// Scene referred in, display referred out. Everything between the lighting pass and
	// here stayed linear and unclamped, which is what leaves this curve the range to roll
	// off instead of a set of already clipped pixels.
	const float3 SceneLuminance = PE_SAMPLE_TEX2D(CopySourceTexture, SamplerLinearClamp, InVarying.UV0).rgb;

	OutTarget = float4(ApplySceneToDisplay(SceneLuminance, ExposureParams), 1.0);
}
#endif	//COPY_PS_TMPL_HLSL
