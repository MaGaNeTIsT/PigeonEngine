#ifndef SHALLOW_WATER_EQUATION_MODIFIER_HLSL
#define SHALLOW_WATER_EQUATION_MODIFIER_HLSL

// ---------------------------------------------------------------------------
// Shallow water equation, the modifier frame.
//
// Provenance: FluxModifierTransform, which the legacy SWE_ModifierWhirlpool.usf
// kept as a function and the legacy SWE_Modifier.usf wrote out inline, term for
// term. It is the transform from the simulation's UV space into a modifier's own
// space: the offset from the modifier's centre, expressed in the simulation's
// aspect corrected frame and rotated into the modifier's forward direction. The
// two modifier passes are its only callers, which is why it has one home here
// instead of a copy in each of them. The modifier pass's call is this same
// arithmetic with its ModSize, ForwardVector and ModLoc readings in place of the
// InModifier* arguments below.
//
// What the port changed, and it is only the parameter path:
//
//   old the parameters were the same six arguments, but the two call sites fed
//   them differently: the whirlpool pass passed its ModLocation / ModRadius *
//   2 / float2(1, 0) / Location / Scale, the modifier pass its ModLoc / ModSize
//   / ForwardVector.xy / Location / Scale
//       new unchanged. Both call sites keep their own arguments and the function
//       keeps its signature, so the scale argument is still allowed to arrive as
//       the scalar ModRadius * 2 and broadcast, as it did before
//   old the whole non compute branch of the modifier passes
//       new gone. The new system has no pixel path for a compute pass, so the
//       transform is only ever called from a compute entry point
//   old the modifier passes' own #define aliases of UCGVectorParams
//       new nothing here. Everything this function needs arrives as an argument,
//       which is why it is the one piece of the family the port did not have to
//       rewrite, and why this file includes nothing
// ---------------------------------------------------------------------------

// No sampler, no shared step, no binding: same arithmetic in, same arithmetic
// out, so there is nothing to include.
float2 FluxModifierTransform(
	in float2	InTexcoord,
	in float2	InModifierLocation,
	in float2	InModifierScale,
	in float2	InModifierForward,
	in float2	InWorldLocation,
	in float2	InWorldScale)
{
	float2 SimulationAspect = float2( InWorldScale.y / InWorldScale.x, 1 );
	float2 Texcoord1 = (InTexcoord.xy - 0.5) * (InModifierScale.xy / (InWorldScale.xy * SimulationAspect.xy));
	Texcoord1 = (InModifierForward.xy * Texcoord1.x) + (float2( -InModifierForward.y, InModifierForward.x ) * Texcoord1.y);
	Texcoord1 = Texcoord1.xy * SimulationAspect.xy + ((InModifierLocation.xy - InWorldLocation.xy) / InWorldScale.xy);
	return Texcoord1;
}

#endif	//SHALLOW_WATER_EQUATION_MODIFIER_HLSL
