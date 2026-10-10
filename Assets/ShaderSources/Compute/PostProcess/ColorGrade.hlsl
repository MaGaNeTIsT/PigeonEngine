#ifndef COLOR_GRADE_HLSL
#define COLOR_GRADE_HLSL

// ---------------------------------------------------------------------------
// Colour grade compute pass.
//
// The algorithm is the legacy CS_ColorGrade one. This file is a component
// template: the whole entry point is injected at the COMPUTE_BODY placeholder of
// Templates/PostProcess_CS.tmpl.hlsl, which is why the [numthreads] attribute
// lives here rather than in the template.
//
// What the port changed, and it is only the parameter path:
//
//   old InputTexture.Load(int3(c, 0))     new PE_LOAD_TEX2D(InputTexture, ...)
//       so the bindless switch stays a one file change, as everywhere else
//   old THREAD_GROUP_SIZE_X               new SHADER_THREAD_GROUP_X
//       the legacy names were the program's own; the new ones come from the
//       system macros so a dispatch and a shader cannot disagree (the compiler
//       reflection does not carry numthreads at all)
//   old globals from a cbuffer declared in the old JSON
//       new the same fields in ColorGradeParams, declared in the component's own
//       JSON, which is where every binding belongs now
//   a saturating path added to the source
//       the legacy read from a texture that may be a render target with values
//       above one; grading a negative value through pow() gives NaN, so the
//       source is clamped once on the way in
// ---------------------------------------------------------------------------

#include "Compute/PostProcess/ColorGradeFunctions.hlsl"

[numthreads(SHADER_THREAD_GROUP_X, SHADER_THREAD_GROUP_Y, 1)]
void CSMain(uint3 DispatchThreadID : SV_DispatchThreadID)
{
	uint2 TexCoord = DispatchThreadID.xy;

	float4 InputColor = PE_LOAD_TEX2D(InputTexture, int3(TexCoord, 0));
	InputColor = saturate(InputColor);

	float3 Color = InputColor.rgb;
	Color = ApplyBrightness(Color, Brightness);
	Color = ApplyContrast(Color, Contrast);
	Color = ApplySaturation(Color, Saturation);
	Color = Color * ColorTint.rgb;
	Color = ApplyGamma(Color, Gamma);

	OutputTexture[TexCoord] = float4(Color, InputColor.a);
}

#endif	//COLOR_GRADE_HLSL
