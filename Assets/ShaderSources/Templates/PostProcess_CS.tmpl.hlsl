#ifndef POST_PROCESS_CS_TMPL_HLSL
#define POST_PROCESS_CS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Post process compute skeleton.
//
// Thin on purpose. A post process algorithm is a whole entry point, so unlike a
// graphics pass there is nothing structural for the template to own: it brings in
// the system macros and hands over to whichever algorithm the program description
// named.
//
// The algorithm arrives at COMPUTE_BODY from a component under Compute/, which is
// also where its bindings are declared. A pass whose body is missing leaves the
// placeholder as text, so a program that uses this template always lists one.
// ---------------------------------------------------------------------------

{SYSTEM_MACROS}
{COMPUTE_BODY}

#endif	//POST_PROCESS_CS_TMPL_HLSL
