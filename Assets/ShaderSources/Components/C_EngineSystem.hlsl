#ifndef C_ENGINE_SYSTEM_HLSL
#define C_ENGINE_SYSTEM_HLSL

// ---------------------------------------------------------------------------
// Body of the system layer component.
//
// The text of this file is what Components/C_EngineSystem.json injects at the
// SYSTEM_MACROS placeholder, so it is spliced into every program template,
// graphics or compute, ahead of everything else.
//
// Never write a placeholder name in brace form inside this file, not even in a
// comment. The injector rewrites every match in the assembled text, and the
// replacement text spans several lines, so a mention in a comment would break
// out of the comment and the rest of the line would become real code.
//
// The constant buffers and samplers the system layer owns are NOT declared
// here. They are declared in the JSON and the compiler emits them as part of
// the generated header, which sits above the template body.
//
// Kept intentionally thin: the actual definitions live in the library so other
// code can include them directly without going through the injection point.
// ---------------------------------------------------------------------------

#include "Public/System/ShaderSystemMacros.hlsl"
#endif	//C_ENGINE_SYSTEM_HLSL
