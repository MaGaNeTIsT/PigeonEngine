#ifndef _SHADER_COMMON_HLSL
#define _SHADER_COMMON_HLSL

#include "./ShaderGlobalMarco.hlsl"
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
#if (SHADER_USE_DEFERRED_SHADING)
#include "../Public/ShaderDeferredCommon.hlsl"
#endif
#include "../Public/ShaderBxDF.hlsl"
#include "../Public/ShaderShadingFunctions.hlsl"
#if (SHADER_USE_SKELETON_INPUT)
#include "../Public/ShaderSkinnings.hlsl"
#endif

#endif	//_SHADER_COMMON_HLSL