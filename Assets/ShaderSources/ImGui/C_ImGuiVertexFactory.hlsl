#ifndef C_IMGUI_VERTEX_FACTORY_HLSL
#define C_IMGUI_VERTEX_FACTORY_HLSL

// ---------------------------------------------------------------------------
// The ImGui vertex factory -- the UI's own.
//
// Why the UI needs a factory of its own rather than a VF_TYPE branch of the
// engine's: the vertex data is produced by ImGui and uploaded by the engine
// verbatim, so the stream is one interleaved ImDrawVert -- a two component pixel
// position, a two component atlas coordinate and a packed eight bit per channel
// colour, twenty bytes per vertex. The engine's generic factory describes meshes,
// whose attributes live one per stream in separate slots, and no declaration
// inside that layout describes this stream. This file states the declaration that
// does, and Programs/P_ImGui.json names it directly from both of its stages.
//
// The three attributes sit in one slot and in stream order, because the elements
// of an input layout are appended inside their slot: position at byte 0, texture
// coordinate at byte 8, colour at byte 16. The layout the renderer creates for the
// pipeline says the same thing in the same order, and that layout -- not this
// declaration -- carries the colour's real format: the format vocabulary a vertex
// factory JSON can express has no normalized eight bit type, so the renderer
// states R8G8B8A8_UNORM itself and this file keeps the shader side at float4,
// which is what an input with a normalized format maps into.
//
// The attributes are unconditional, and that is the rule this file shares with its
// JSON: an input declared without a condition gets no VF_HAS_* guard here, so the
// input stream the compiler lays out and the struct the shader compiles cannot
// disagree about which members exist. The stream the renderer binds always carries
// all three, so there is nothing for a guard to switch off.
//
// The interpolants are not declared here. VFVarying's members come from
// Public/VertexFactory/ImGuiVarying.hlsl, the same file the generic factory's
// ImGui branch includes: the pixel stage reads those members by name through
// BuildMaterialPixelInput, so there is exactly one definition of the ImGui
// interpolant set and the two factories can differ only in how they fill it. That
// fragment has to stay for as long as either factory exists.
//
// Referenced bindings, declared by components the program must list:
//   DrawParams.WorldMatrix / WorldInvTransposeMatrix  Components/C_DrawParams.json
//   ViewParams.ViewProjectionMatrix                   Components/C_EngineSystem.json
//
// The ImGui pass publishes both itself: the world matrix is the identity, because
// an overlay has no draw transform, and the view projection is the orthographic
// matrix that turns pixel coordinates into clip space. The pass owns its instance
// of the shader, so nothing about the UI depends on the scene's view.
//
// Placeholder names are written here without braces on purpose: this file is
// injected text and gets scanned by the placeholder pass again, so a mention in
// brace form would be replaced by a multi line injection.
// ---------------------------------------------------------------------------

#include "Public/VertexFactory/VertexFactoryTypes.hlsl"
#include "Public/VertexFactory/VertexFactoryCommon.hlsl"
#include "Public/Core/ShaderSpaceTransform.hlsl"

// Mirrors the Inputs declaration of ImGui/C_ImGuiVertexFactory.json. The semantic
// name, index and format of every entry have to match the generated
// FinalInputStream and the reflection's input layout; the slot each one lands in is
// the slot the renderer binds its one vertex stream to.
struct VFAttributes
{
	float2	Position	: POSITION0;
	float2	UV0			: TEXCOORD0;
	float4	Color		: COLOR0;
};

void VFUnpackVertex(in VFAttributes InAttributes, in VFContext InContext, out VFVertex OutVertex)
{
	// A UI coordinate has no depth of its own. The clip space z is the projection's
	// business, so the missing component is filled with zero rather than guessed at
	// here.
	OutVertex.ObjectPosition = float3(InAttributes.Position, 0.0);

	// A UI quad is not lit and not normal mapped, so it has no normal stream and the
	// frame is the identity frame rather than a conditional fallback.
	OutVertex.ObjectNormal = float3(0.0, 0.0, 1.0);
	OutVertex.ObjectTangent = float4(1.0, 0.0, 0.0, 1.0);

	OutVertex.UV0 = InAttributes.UV0;
	OutVertex.UV1 = float2(0.0, 0.0);
	OutVertex.Color = InAttributes.Color;

	OutVertex.Custom[0] = float4(0.0, 0.0, 0.0, 0.0);
	OutVertex.Custom[1] = float4(0.0, 0.0, 0.0, 0.0);
}

void VFTransformToWorld(inout VFVertex InOutVertex, in VFContext InContext)
{
	// A UI draw is not instanced, so instance space is object space. The world matrix
	// is the identity for a plain overlay and is what would let the same factory draw
	// a UI panel that lives in the scene.
	InOutVertex.InstancePosition = InOutVertex.ObjectPosition;
	InOutVertex.InstanceNormal = InOutVertex.ObjectNormal;

	InOutVertex.WorldPosition = TransformObjectToWorld(InOutVertex.InstancePosition, WorldMatrix);
	InOutVertex.WorldNormal = TransformObjectToWorldNormal(InOutVertex.InstanceNormal, WorldInvTransposeMatrix, true);

	float3 WorldTangent = TransformObjectToWorldDir(InOutVertex.ObjectTangent.xyz, WorldMatrix, true);
	BuildWorldTangentFrame(InOutVertex.WorldNormal, float4(WorldTangent, InOutVertex.ObjectTangent.w),
		InOutVertex.WorldTangent, InOutVertex.WorldBitangent);
}

void VFTransformVertex(in VFVertex InVertex, in VFContext InContext, in float3 InWorldPositionOffset, out VFVarying OutVarying)
{
	// The offset is a world space displacement, so it is added before the view
	// projection is applied. A UI material has no reason to displace, but it goes
	// through the same step as every other factory rather than around it.
	float3 FinalWorldPosition = InVertex.WorldPosition + InWorldPositionOffset;

	OutVarying.Position = TransformWorldToClip(float4(FinalWorldPosition, 1.0), ViewProjectionMatrix);

	OutVarying.WorldPosition = FinalWorldPosition;
	OutVarying.WorldNormal = InVertex.WorldNormal;
	OutVarying.WorldTangent = InVertex.WorldTangent;
	OutVarying.WorldBitangent = InVertex.WorldBitangent;
	OutVarying.UV0 = InVertex.UV0;
	OutVarying.UV1 = InVertex.UV1;
	OutVarying.Color = InVertex.Color;
}
#endif	//C_IMGUI_VERTEX_FACTORY_HLSL
