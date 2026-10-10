#ifndef SHADER_CONSTANTS_HLSL
#define SHADER_CONSTANTS_HLSL

// ---------------------------------------------------------------------------
// Engine-wide shader math constants.
//
// Ported from the legacy Common/ShaderGlobalMarco.hlsl (the name is spelled
// "Macro" here). The legacy file switched its precision constants on
// ENABLE_CONSOLE_SHADING; the tighter console values are the defaults now and
// the switch is gone, because the same limits are safe on every target.
//
// Every constant is wrapped in #ifndef so a command line -D value stays
// authoritative instead of tripping a macro redefinition.
// ---------------------------------------------------------------------------

#ifndef PE_SHADER_PI
#define PE_SHADER_PI						(3.1415926535897932)
#endif	//PE_SHADER_PI

#ifndef PE_SHADER_PI_HALF
#define PE_SHADER_PI_HALF					(1.5707963267948966)
#endif	//PE_SHADER_PI_HALF

#ifndef PE_SHADER_TWO_PI
#define PE_SHADER_TWO_PI					(6.2831853071795865)
#endif	//PE_SHADER_TWO_PI

#ifndef PE_SHADER_PI_INV
#define PE_SHADER_PI_INV					(0.3183098861837907)
#endif	//PE_SHADER_PI_INV

#ifndef PE_SHADER_DEG_TO_RAD
#define PE_SHADER_DEG_TO_RAD				(0.0174532925199433)
#endif	//PE_SHADER_DEG_TO_RAD

#ifndef PE_SHADER_RAD_TO_DEG
#define PE_SHADER_RAD_TO_DEG				(57.29577951308232)
#endif	//PE_SHADER_RAD_TO_DEG

#ifndef PE_SHADER_FLT_EPS
#define PE_SHADER_FLT_EPS					(1e-5)
#endif	//PE_SHADER_FLT_EPS

// Largest finite value a 16 bit float can hold.
#ifndef PE_SHADER_MEDIUM_PRECISION_FLT_MAX
#define PE_SHADER_MEDIUM_PRECISION_FLT_MAX	(65504.0)
#endif	//PE_SHADER_MEDIUM_PRECISION_FLT_MAX

// Smallest normal 16 bit float (2^-14).
#ifndef PE_SHADER_MEDIUM_PRECISION_FLT_MIN
#define PE_SHADER_MEDIUM_PRECISION_FLT_MIN	(0.00006103515625)
#endif	//PE_SHADER_MEDIUM_PRECISION_FLT_MIN

// Smallest perceptual roughness whose fourth power still stays inside the fp16
// normal range, plus its squared counterpart. Below these values the specular
// lobe degenerates into denormals on half-precision targets.
#ifndef MIN_PERCEPTUAL_ROUGHNESS
#define MIN_PERCEPTUAL_ROUGHNESS			(0.045)
#endif	//MIN_PERCEPTUAL_ROUGHNESS

#ifndef MIN_ROUGHNESS
#define MIN_ROUGHNESS						(0.002025)
#endif	//MIN_ROUGHNESS

// Guards against a degenerate N dot V feeding the specular denominator.
#ifndef PE_SHADER_MIN_N_DOT_V
#define PE_SHADER_MIN_N_DOT_V				(1e-4)
#endif	//PE_SHADER_MIN_N_DOT_V
#endif	//SHADER_CONSTANTS_HLSL
