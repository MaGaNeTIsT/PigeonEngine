#ifndef DEBUG_WIREFRAME_VS_TMPL_HLSL
#define DEBUG_WIREFRAME_VS_TMPL_HLSL

// ---------------------------------------------------------------------------
// Debug wireframe pass, vertex stage.
//
// The mesh's own edges, drawn as an unlit overlay. Which edges those are is the
// rasterizer's answer, not this shader's: the debug draw selects the wireframe
// fill mode when it builds the pipeline state, so the geometry that arrives here
// is the ordinary triangle list and the vertex stage is the ordinary vertex
// stage. That is why this template has no branch for "am I drawing lines" -- by
// the time the vertex stage runs, the question no longer exists.
//
// It is a separate program rather than a mode of the base pass because it writes
// one target and no GBuffer, and a program that declared seven render targets
// would pay for bindings it never writes.
//
// Placeholder order is a hard constraint, not a style choice:
//
//   SYSTEM_MACROS         first, everything below may test them
//   VF_VARYING_STRUCT     before anything that names VFVarying
//   DRAW_PARAMS           the per draw constant buffer, for the view projection
//   VF_VERTEX_FACTORY     declares VFAttributes and the factory's three functions
//   PROGRAMMABLE          the engine's bridge and the material entry point; it
//                         names VFAttributes, so it has to come after the factory
//
// Names in this comment are written without braces so the injector does not
// rewrite them: this file is the injected text.
//
// The body is this program's own rather than the shared one, and for one reason:
// a debug draw puts every instance of one shape into a single draw call and hands
// the per instance transform in a buffer instead of in the draw block. The
// standard orchestration takes its object to world step from the draw transform,
// which is a single matrix for the whole call, so it cannot place instance seven.
// Everything else is the standard orchestration - unpack, material vertex hook,
// clip - and the material hook still runs, which is what keeps a material's world
// position offset working for a debug shape exactly as it does for a mesh.
// ---------------------------------------------------------------------------

{SYSTEM_MACROS}
{VF_VARYING_STRUCT}
{DRAW_PARAMS}
{VF_VERTEX_FACTORY}
{PROGRAMMABLE}

void VSMain(in VFAttributes InAttributes,
	in uint InVertexID : SV_VertexID,
	in uint InInstanceID : SV_InstanceID,
	out VFVarying OutVarying)
{
	VFContext Context;
	Context.VertexID = InVertexID;
	Context.InstanceID = InInstanceID;

	// The rasterizer produces a primitive id per triangle; the vertex stage has
	// no such semantic, and a vertex factory that needs one for a lookup derives
	// it from the vertex id instead.
	Context.PrimitiveID = 0;

	VFVertex Vertex;
	VFUnpackVertex(InAttributes, Context, Vertex);

	// The instance transform replaces the object to world step. DrawParams is still
	// the draw transform and still published (as the identity) - the material hook
	// and the view projection both read through it - but the placement of this
	// vertex comes from the buffer, indexed by the instance the rasterizer is on.
	const float3 InstanceWorldPosition = TransformObjectToWorld(
		Vertex.ObjectPosition, DebugPrimitiveTransforms[InInstanceID].InstanceTransform);
	Vertex.InstancePosition = InstanceWorldPosition;
	Vertex.WorldPosition = InstanceWorldPosition;
	Vertex.WorldNormal = Vertex.ObjectNormal;
	Vertex.WorldTangent = Vertex.ObjectTangent.xyz;
	Vertex.WorldBitangent = float3(0.0, 1.0, 0.0);
	// The free channel of VFVertex is the instance colour the material bridge reads, so a
	// material that wants this shape's colour finds it there.
	const float4 InstanceColor = DebugPrimitiveColors[InInstanceID].InstanceColor;
	Vertex.Custom[0] = InstanceColor;

	// Same reason as the programmable bridge: the varying goes into an inout parameter before
	// anything has written it, and FXC requires the argument of one to be fully initialized.
	OutVarying = (VFVarying)0;

	MaterialVertexInput MatInput = BuildMaterialVertexInput(Vertex, Context);
	float3 WorldPositionOffset = float3(0.0, 0.0, 0.0);
#if MATERIAL_HAS_FUNCTIONS
	CalcMaterialVertex(MatInput, WorldPositionOffset, OutVarying);
#endif	//MATERIAL_HAS_FUNCTIONS

	VFTransformVertex(Vertex, Context, WorldPositionOffset, OutVarying);

	// The colour of this instance, which is what the caller asked for when it queued
	// the shape. It is written after the factory's step so the factory's own vertex
	// colour, which a position only stream does not carry, cannot overwrite it.
	OutVarying.Color = InstanceColor;
}

#endif	//DEBUG_WIREFRAME_VS_TMPL_HLSL
