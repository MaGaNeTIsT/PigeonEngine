#ifndef C_FULL_SCREEN_VERTEX_FACTORY_HLSL
#define C_FULL_SCREEN_VERTEX_FACTORY_HLSL

// ---------------------------------------------------------------------------
// The fullscreen vertex factory -- the full screen passes' own.
//
// Why this factory has a component of its own rather than a VF_TYPE branch of the
// engine's generic one: the stream it reads is not a mesh stream. It carries one
// two component coordinate per vertex and nothing else, and the clip space
// position is reconstructed from that coordinate instead of being read from a
// position attribute. The generic component describes meshes, whose attributes
// live one per stream in the slots that layout reserves, and no declaration inside
// that layout describes a single stream that has to sit at slot 0 for the
// renderer's full screen triangle to feed it.
//
// Writing the slot down here is what makes the binding generic: the pipeline's
// input layout comes from this declaration through the reflection, and the draw
// that binds the triangle asks the reflection where to bind it. Moving this
// stream to another slot is one json change, and no render code carries a slot
// number for it.
//
// The factory itself lives in Public/VertexFactory/FullScreenVF.hlsl, where the
// clip space mapping and the UV convention are documented; this file only states
// which one it is, exactly like the ImGui component states its own.
//
// Referenced bindings, declared by components the program must list:
//   ViewParams.ViewProjectionMatrix   Components/C_EngineSystem.json (a dome or a
//                                     blit may declare it; the factory needs none)
//
// Placeholder names are written here without braces on purpose: this file is
// injected text and gets scanned by the placeholder pass again, so a mention in
// brace form would be replaced by a multi line injection.
// ---------------------------------------------------------------------------

#include "Public/VertexFactory/VertexFactoryTypes.hlsl"
#include "Public/VertexFactory/VertexFactoryCommon.hlsl"
#include "Public/VertexFactory/FullScreenVF.hlsl"
#endif	//C_FULL_SCREEN_VERTEX_FACTORY_HLSL
