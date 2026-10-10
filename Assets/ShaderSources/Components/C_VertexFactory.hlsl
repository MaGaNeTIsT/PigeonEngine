#ifndef C_VERTEX_FACTORY_HLSL
#define C_VERTEX_FACTORY_HLSL

// ---------------------------------------------------------------------------
// Body of the vertex factory component.
//
// The text of this file is what Components/C_VertexFactory.json injects at the
// VF_VERTEX_FACTORY placeholder, which only exists in a vertex stage template.
// It selects the concrete factory from the VF_TYPE variant define and pulls in
// the vertex factory contract the selected factory builds on.
//
// The other two injection points the component owns, VF_VARYING_STRUCT and
// VF_VS_BODY, are supplied from the component's own Replaces list: one is a
// struct declaration the pixel stage also needs, the other is a statement
// sequence that has to land inside the vertex stage entry point. Neither can be
// expressed as a file injected at this placeholder.
//
// Placeholder names are written here without braces on purpose. This file is
// injected text and gets scanned by the placeholder pass again, so a brace form
// mention in a comment would be replaced by a multi line injection and the
// comment would break open.
//
// Adding a vertex factory means adding a file under Public/VertexFactory/ and a row in
// the metadata table of VertexFactoryTypes.hlsl, plus a home for its stream
// declaration: a factory whose attributes are mesh streams, one per slot, adds a branch
// here, and a factory whose stream layout is its own gets a component of its own -
// FullScreen/ and ImGui/ have theirs - so no compilation ever sees two meanings for one
// attribute. Nothing else changes.
//
// VF_TYPE marks which factory a compilation is for, and nothing finer than that. A
// single branch can therefore serve more than one kind of asset, which is exactly how
// the engine mesh works: EngineMeshVF.hlsl has no VF_TYPE test of its own, because a
// skinned mesh and a static mesh are the same factory reading a different set of
// streams. Anything that distinguishes them arrives as a VF_HAS_* flag, so a new
// attribute kind is a JSON condition and a branch in the factory file, never a new
// VF_TYPE.
//
// ---------------------------------------------------------------------------
// The escape gate registry
//
// VF_VS_BODY is the escape gate: a factory whose vertex body the standard
// orchestration cannot express supplies a body of its own at that injection
// point instead of calling ExecuteMaterialVertex, and the design asks for the
// exception to be registered. This comment is the registry, and it is empty.
//
// The factory it was written for, ProceduralGrass, turns out not to need it. The
// gate was specified against a sketch in which a factory's world space values
// came out of the engine's own transform step; the implemented contract has that
// step as a function the factory owns, VFTransformToWorld, with VFTransformVertex
// left holding the projection. A factory whose geometry is already in world space
// therefore has a place to say so -- ProceduralGrassVF.hlsl copies local into
// world and documents why, and FullScreenVF.hlsl is the precedent for the mirror
// image -- and grass is expressible in three functions after all, which is worth
// more than a body of its own because it keeps the material's vertex hook and the
// material's world position offset working for a blade.
//
// The gate itself is unchanged and needs no decision to stay available: a factory
// that genuinely cannot be written in three functions adds its branch here, in
// the same shape as the VF_TYPE branches below, and adds its name to this
// paragraph.
// ---------------------------------------------------------------------------

#include "Public/VertexFactory/VertexFactoryTypes.hlsl"
#include "Public/VertexFactory/VertexFactoryCommon.hlsl"

#if VF_TYPE == VF_TYPE_ENGINE_MESH
#include "Public/VertexFactory/EngineMeshVF.hlsl"
#endif	//VF_TYPE == VF_TYPE_ENGINE_MESH

#if VF_TYPE == VF_TYPE_PROCEDURAL_GRASS
#include "Public/VertexFactory/ProceduralGrassVF.hlsl"
#endif	//VF_TYPE == VF_TYPE_PROCEDURAL_GRASS

// VF_TYPE_FULLSCREEN and VF_TYPE_IMGUI are not here on purpose: both factories declare
// a stream layout of their own, so each has a component of its own (FullScreen/ and
// ImGui/) and neither is ever compiled through this file. Their VF_TYPE values are
// still theirs - they are what the axis carries - the branch simply has no home here.
#endif	//C_VERTEX_FACTORY_HLSL
