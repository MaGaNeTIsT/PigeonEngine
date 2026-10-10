#ifndef SHADER_TONE_MAPPING_HLSL
#define SHADER_TONE_MAPPING_HLSL

// ---------------------------------------------------------------------------
// The scene to display boundary.
//
// Everything above this file is scene referred and linear: the lighting pass
// writes luminance in cd/m2, the sky adds its own, and nothing clamps. This file
// is the one place where that stops being true, and it runs the three steps a
// display needs, in this order:
//
//   1. exposure     scene luminance    -> exposed signal
//   2. tone map     unbounded signal   -> [0, 1] with the highlights rolled off
//   3. encode       linear [0, 1]      -> the display's transfer function
//
// Exposure comes first because tone mapping is defined on the exposed signal,
// not on the scene luminance. The encode is last and is a separate step from the
// tone map: a tone map is a creative curve, the transfer function is what the
// target format expects, and the two only look like one step while the target
// happens to be sRGB.
//
// The order matters for the same reason the passes before it do not clamp:
// clipping before the tone map throws away exactly the range the curve exists to
// roll off.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"
#include "Public/Core/ShaderMath.hlsl"
#include "Public/Core/ShaderPacking.hlsl"

// The exposure gain RViewProxy already solved: 1 / (99.3 * 2^EV100 / ISO), shifted by the
// exposure compensation. ExposureParams belongs to the view, so it carries the values of
// the camera whose frame is being displayed. The gain is in .w and the values it came from
// are in the other three, for a debug display to read.
float ComputeExposureGain(in float4 InExposureParams)
{
	return max(InExposureParams.w, 0.0);
}

// ACES filmic tone map, the Stephen Hill fit of the RRT and ODT pair.
//
// A fit rather than the reference transform: the reference is a chain of matrices and
// rational polynomials over the full ACES colour space, and this has the same shape in a
// handful of multiplies. It is not a colour management step - the input is already in the
// working space and the output is display referred - so it runs per channel.
//
// The curve is what keeps a physical renderer presentable: scene luminance is unbounded,
// so saturating it directly would clip every highlight to the same white. This rolls the
// shoulder off instead, which is where highlight detail survives exposure.
float3 TonemapACES(in float3 InColor)
{
	const float A = 0.0245786;
	const float B = 0.000090537;
	const float C = 0.983729;
	const float D = 0.4329510;
	const float E = 0.238081;

	// Into the ACEScg primaries, then back out to the working ones.
	static const float3x3 ACESInput = float3x3(
		0.59719, 0.35458, 0.04823,
		0.07600, 0.90834, 0.01566,
		0.02840, 0.13383, 0.83777);
	static const float3x3 ACESOutput = float3x3(
		 1.60475, -0.53108, -0.07367,
		-0.10208,  1.10813, -0.00605,
		-0.00327, -0.07276,  1.07602);

	float3 Color = mul(ACESInput, max(InColor, 0.0));

	// The RRT and ODT fit, evaluated on the ACEScg signal.
	float3 Numerator = Color * (Color + A) - B;
	float3 Denominator = Color * (C * Color + D) + E;
	Color = Numerator / max(Denominator, 1e-6);

	return saturate(mul(ACESOutput, Color));
}

// The whole boundary in one call: scene luminance in, encoded display signal out. Going
// through one function is what keeps "what a display value means" decided in one place
// rather than at every call site.
float3 ApplySceneToDisplay(in float3 InSceneLuminance, in float4 InExposureParams)
{
	const float3 Exposed = InSceneLuminance * ComputeExposureGain(InExposureParams);
	return LinearToSrgb(TonemapACES(Exposed));
}
#endif	//SHADER_TONE_MAPPING_HLSL
