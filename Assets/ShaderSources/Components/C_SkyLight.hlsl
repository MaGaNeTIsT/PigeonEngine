#ifndef C_SKY_LIGHT_HLSL
#define C_SKY_LIGHT_HLSL

// ---------------------------------------------------------------------------
// Sky light: the parameter block the sky pass reads, and nothing else.
//
// The constant buffer is NOT declared here. It is declared in C_SkyLight.json
// and the compiler emits it as part of the generated header, which sits above
// the template body. This file exists because a component's Template is where
// its HLSL lives and the injection point needs a file to expand.
//
//   SkyLightParams       b2
//     LightAdjustColor       float4   rgb = tint, a unused
//     LightAdjustIntensity   float    sky luminance [cd/m2]
//
// ---------------------------------------------------------------------------
// Why the sky needs a scale at all
//
// The environment cube is sampled as radiance, so its texels are the shape of
// the sky and not its brightness. An LDR cube holds values around 1, which as a
// luminance in cd/m2 is darker than a moonlit night - a real camera exposed for
// daylight turns it black. LightAdjustIntensity is what puts the cube on the
// same physical scale as the lights, and it is authored in the same unit the
// lights use: luminance in cd/m2. A clear sky is around 8000, an overcast one
// around 2000.
//
// This is the one light that is not in a light list. It is a single instance
// drawn as a fullscreen pass, so it needs no array and no index, and the sky
// pass is the only pass that declares this block.
// ---------------------------------------------------------------------------

#endif	//C_SKY_LIGHT_HLSL
