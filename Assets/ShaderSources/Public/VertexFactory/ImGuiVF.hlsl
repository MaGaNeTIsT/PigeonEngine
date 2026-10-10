#ifndef IMGUI_VF_HLSL
#define IMGUI_VF_HLSL

// ---------------------------------------------------------------------------
// ImGui vertex factory (VF_TYPE_IMGUI).
//
// The UI factory. Its input stream is an ImDrawVert, verbatim: a two component
// position, a two component texture coordinate and an eight bit per channel
// colour, interleaved into one 20 byte stride. Nothing here widens, pads or
// reorders it, which is the point of the factory -- the vertex data is produced
// by ImGui and the engine uploads it as it is, so a UI draw costs one memcpy and
// no per vertex pass.
//
// Required attributes : Position (2D), TexCoord, Color
// Optional attributes : none
// VS only resources   : none
//
// Why this is not the engine mesh factory: the frozen input stream table gives
// attribute 0 the three component object space type, and a UI position is two
// components of screen space pixels, so it takes an attribute 0 declaration of
// its own. That declaration is selected by VF_TYPE, so no compilation ever sees
// both meanings for the attribute. Everything downstream of the position is then
// the ordinary path: the draw's world matrix (identity for a UI pass) places it,
// and the view projection the pass publishes puts it in clip space.
//
// The projection is deliberately not a material parameter. The engine owns the
// world to clip step for every factory, so the orthographic matrix that turns
// pixel coordinates into clip space is the ImGui pass's view projection, in
// ViewParams, exactly where every other pass keeps it.
//
// Referenced bindings, declared by components the program must list:
//   DrawParams.WorldMatrix / WorldInvTransposeMatrix  Components/C_DrawParams.json
//   ViewParams.ViewProjectionMatrix                   Components/C_EngineSystem.json
//
// The UV0 and Color members carry the same VF_HAS_* conditions as their JSON
// entries, so the shader declaration and the reflected input layout gain and
// lose members together instead of disagreeing at pipeline creation time.
// ---------------------------------------------------------------------------

#include "Public/VertexFactory/VertexFactoryTypes.hlsl"
#include "Public/VertexFactory/VertexFactoryCommon.hlsl"
#include "Public/Core/ShaderSpaceTransform.hlsl"

// Mirrors the Inputs declaration of Components/C_VertexFactory.json. The
// semantic name, index and component count have to match the generated
// FinalInputStream and the reflection InputLayout.
struct VFAttributes
{
	float2	Position	: POSITION0;
#if VF_HAS_UV0
	float2	UV0			: TEXCOORD0;
#endif	//VF_HAS_UV0
#if VF_HAS_COLOR
	float4	Color		: COLOR0;
#endif	//VF_HAS_COLOR
};

void VFUnpackVertex(in VFAttributes InAttributes, in VFContext InContext, out VFVertex OutVertex)
{
	// A UI coordinate has no depth of its own. The clip space z is the
	// projection's business, so the missing component is filled with zero rather
	// than guessed at here.
	OutVertex.ObjectPosition = float3(InAttributes.Position, 0.0);

	// A UI quad is not lit and not normal mapped, so there is no normal stream and
	// the frame is the identity frame rather than a VF_HAS_* fallback.
	OutVertex.ObjectNormal = float3(0.0, 0.0, 1.0);
	OutVertex.ObjectTangent = float4(1.0, 0.0, 0.0, 1.0);

#if VF_HAS_UV0
	OutVertex.UV0 = InAttributes.UV0;
#else
	OutVertex.UV0 = float2(0.0, 0.0);
#endif	//VF_HAS_UV0
	OutVertex.UV1 = float2(0.0, 0.0);

#if VF_HAS_COLOR
	OutVertex.Color = InAttributes.Color;
#else
	// White is the tint that leaves a texture sample unchanged, which is the
	// correct behaviour for a draw whose colour stream went missing.
	OutVertex.Color = float4(1.0, 1.0, 1.0, 1.0);
#endif	//VF_HAS_COLOR

	OutVertex.Custom[0] = float4(0.0, 0.0, 0.0, 0.0);
	OutVertex.Custom[1] = float4(0.0, 0.0, 0.0, 0.0);
}

void VFTransformToWorld(inout VFVertex InOutVertex, in VFContext InContext)
{
	// A UI draw is not instanced, so instance space is object space. The world
	// matrix is identity for a plain overlay and is what lets the same factory
	// draw a UI panel that lives in the scene.
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
#endif	//IMGUI_VF_HLSL
