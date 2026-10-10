#ifndef COMPUTE_PASS_CS_TMPL_HLSL
#define COMPUTE_PASS_CS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Compute pass skeleton.
//
// Thin on purpose, for the same reason PostProcess_CS.tmpl.hlsl is: a compute
// algorithm is a whole entry point, so there is nothing structural for the
// template to own. It brings in the system macros and hands over to whichever
// algorithm the program description named.
//
// The [numthreads] attribute stays in the algorithm file, not here. The thread
// group size belongs to the shader that declares it: SHADER_THREAD_GROUP_X and
// SHADER_THREAD_GROUP_Y arrive as defines from the program description, and the
// dispatcher has to agree with them because the compiler reflection carries no
// numthreads at all.
//
// The algorithm arrives at COMPUTE_BODY from a component under Compute/, which is
// also where its bindings are declared. A pass whose body is missing leaves the
// placeholder as text, so a program that uses this template always lists one.
// ---------------------------------------------------------------------------

{SYSTEM_MACROS}
{COMPUTE_BODY}

#endif	//COMPUTE_PASS_CS_TMPL_HLSL
