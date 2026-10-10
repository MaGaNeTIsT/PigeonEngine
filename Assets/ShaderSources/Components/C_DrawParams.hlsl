#ifndef C_DRAW_PARAMS_HLSL
#define C_DRAW_PARAMS_HLSL

// ---------------------------------------------------------------------------
// Body of the per draw component.
//
// Injected at the DRAW_PARAMS placeholder by a program that needs per draw data.
// The constant buffer itself is declared in the JSON, so there is nothing to
// declare here; this file exists because every referenced component must carry a
// template, and because the placeholder is what records, in the template, that
// this program reads per draw values.
//
// Why per draw data is its own component rather than part of the system layer:
// it is only meaningful for a program that draws geometry. A fullscreen pass has
// no world matrix to speak of, so a program that does not need it does not list
// this component, and nothing is declared, compiled or bound for it.
//
// The world matrices are uploaded without transposing (see the matrix convention
// in the compiler manual), which is why every transform in the library is
// written as mul(Matrix, Vector).
// ---------------------------------------------------------------------------
#endif	//C_DRAW_PARAMS_HLSL
