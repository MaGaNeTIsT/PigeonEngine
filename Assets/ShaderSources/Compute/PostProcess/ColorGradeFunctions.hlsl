#ifndef COLOR_GRADE_FUNCTIONS_HLSL
#define COLOR_GRADE_FUNCTIONS_HLSL

// ---------------------------------------------------------------------------
// Colour grading helpers.
//
// The algorithm is the legacy one, kept as is. What changed is the styling and
// nothing else: PascalCase, an In prefix on every parameter, braces on their own
// line. Grading maths is easy to get subtly wrong and there is no reason to touch
// it while porting.
//
// Kept in their own file so a second grading pass, or a grading stage inside a
// larger post process, can use them without pulling in the entry point.
// ---------------------------------------------------------------------------

float3 ApplyBrightness(in float3 InColor, in float InBrightness)
{
	return InColor * InBrightness;
}

float3 ApplyContrast(in float3 InColor, in float InContrast)
{
	return (InColor - 0.5) * InContrast + 0.5;
}

// Rec. 601 luma weights, which is what the legacy used. Note that
// Public/Core/ShaderPacking.hlsl has a Rec. 709 Luminance; the two disagree on
// purpose and grading kept the legacy weights so existing materials do not shift.
float3 ApplySaturation(in float3 InColor, in float InSaturation)
{
	float LuminanceValue = dot(InColor, float3(0.299, 0.587, 0.114));
	return lerp(LuminanceValue.xxx, InColor, InSaturation);
}

float3 ApplyGamma(in float3 InColor, in float InGamma)
{
	return pow(max(InColor, 0.0), 1.0 / max(InGamma, 1e-4));
}

#endif	//COLOR_GRADE_FUNCTIONS_HLSL
