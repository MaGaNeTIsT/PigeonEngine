#ifndef SHADER_SYSTEM_MACROS_HLSL
#define SHADER_SYSTEM_MACROS_HLSL

// ---------------------------------------------------------------------------
// System macros.
//
// This is the body behind the SYSTEM_MACROS injection point: every program,
// graphics or compute, gets these definitions before anything else.
//
// The name is spelled without braces on purpose. This file is normally reached
// through an #include, so the injector never sees its text, but keeping the
// brace form out means the file stays safe if it is ever spliced in directly.
//
// Everything here is guarded with #ifndef because the program description and
// the entry JSON hand their Defines to the compiler as -D options. A command
// line definition has to stay authoritative; a bare #define with the same name
// would instead be a macro redefinition.
//
// Engine macros are read only for materials and domains: they may test
// SHADER_* / PE_* / VF_*, never redefine them.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderBindless.hlsl"

// ---------------------------------------------------------------------------
// Platform
// ---------------------------------------------------------------------------

#ifndef PE_PLATFORM_D3D12
#define PE_PLATFORM_D3D12					1
#endif	//PE_PLATFORM_D3D12

#ifndef PE_PLATFORM_D3D11
#define PE_PLATFORM_D3D11					0
#endif	//PE_PLATFORM_D3D11

// ---------------------------------------------------------------------------
// Quality level
//
// Drives the optional shader side simplification branches. The concrete meaning
// of each level belongs to the pass that tests it; 0 means "unset".
// ---------------------------------------------------------------------------

#ifndef PE_SHADER_QUALITY_LEVEL
#define PE_SHADER_QUALITY_LEVEL				0
#endif	//PE_SHADER_QUALITY_LEVEL

// ---------------------------------------------------------------------------
// Stage identity
//
// Used as a boolean flag, e.g. #if SHADER_STAGE_PIXEL. Set by the program
// description, one of the three is 1 for a given compilation.
// ---------------------------------------------------------------------------

#ifndef SHADER_STAGE_VERTEX
#define SHADER_STAGE_VERTEX					0
#endif	//SHADER_STAGE_VERTEX

#ifndef SHADER_STAGE_PIXEL
#define SHADER_STAGE_PIXEL					0
#endif	//SHADER_STAGE_PIXEL

#ifndef SHADER_STAGE_COMPUTE
#define SHADER_STAGE_COMPUTE				0
#endif	//SHADER_STAGE_COMPUTE

// ---------------------------------------------------------------------------
// Compute thread group
//
// The compiler reflection does not carry numthreads, so the values have to be
// compile time constants agreed on by the compute shader and the dispatcher.
// ---------------------------------------------------------------------------

#ifndef SHADER_THREAD_GROUP_X
#define SHADER_THREAD_GROUP_X				8
#endif	//SHADER_THREAD_GROUP_X

#ifndef SHADER_THREAD_GROUP_Y
#define SHADER_THREAD_GROUP_Y				8
#endif	//SHADER_THREAD_GROUP_Y

#ifndef SHADER_THREAD_GROUP_Z
#define SHADER_THREAD_GROUP_Z				1
#endif	//SHADER_THREAD_GROUP_Z
#endif	//SHADER_SYSTEM_MACROS_HLSL
