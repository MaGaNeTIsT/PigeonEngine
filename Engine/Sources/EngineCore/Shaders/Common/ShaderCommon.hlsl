#ifndef _SHADER_COMMON_HLSL
#define _SHADER_COMMON_HLSL

#define ENABLE_CONSOLE_SHADING			1

#define MEDIUM_PRECISION_FLT_MAX		(65504.0)
#define MEDIUM_PRECISION_FLT_MIN		(0.00006103515625)

#if (ENABLE_CONSOLE_SHADING)
#define FLT_EPS							(1e-5)
#define SaturateMediumPrecision(x)		(x)
#else
#define FLT_EPS							(MEDIUM_PRECISION_FLT_MIN)
#define SaturateMediumPrecision(x)		(min(x, MEDIUM_PRECISION_FLT_MAX))
#endif

// Min roughness such that (MIN_PERCEPTUAL_ROUGHNESS^4) > 0 in fp16 (i.e. 2^(-14/4), rounded up)
#if (ENABLE_CONSOLE_SHADING)
#define MIN_PERCEPTUAL_ROUGHNESS		(0.045)
#define MIN_ROUGHNESS					(0.002025)
#else
#define MIN_PERCEPTUAL_ROUGHNESS		(0.089)
#define MIN_ROUGHNESS					(0.007921)
#endif

#define MIN_N_DOT_V						(1e-4)

#define CUSTOM_SHADER_PI				(3.1415926535897932)
#define CUSTOM_SHADER_PI_HALF			(1.5707963267948966)
#define CUSTOM_SHADER_PI_DERIVATIVE		(0.3183098861837907)

#include "./ShaderDefCommon.hlsl"
#include "../Public/ShaderStructCommon.hlsl"
#include "../Public/ShaderSelectFunctions.hlsl"
#include "../Public/ShaderFunctions.hlsl"
#include "../Public/ShaderNoises.hlsl"
#include "../Public/ShaderVariables.hlsl"
#include "../Public/ShaderLightings.hlsl"
#include "../Public/ShaderFastMath.hlsl"
#include "../Public/ShaderSpaceTransform.hlsl"
#include "../Public/ShaderRenderCommon.hlsl"
#if SHADER_USE_DEFERRED_SHADING
#include "../Public/ShaderDeferredCommon.hlsl"
#endif
#include "../Public/ShaderBxDF.hlsl"
#include "../Public/ShaderShadingFunctions.hlsl"

#endif	//_SHADER_COMMON_HLSL