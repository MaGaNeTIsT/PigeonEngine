#ifndef FULL_SCREEN_VF_HLSL
#define FULL_SCREEN_VF_HLSL

// ---------------------------------------------------------------------------
// Fullscreen triangle vertex factory (VF_TYPE_FULLSCREEN).
//
// Three vertices, one static vertex buffer, one attribute: UV0. The clip space
// position is reconstructed from that coordinate instead of being read from a
// position stream, so the factory needs no POSITION attribute at all.
//
// Required attributes : UV0
// Optional attributes : none
// VS only resources   : none
//
// This is the one factory with no world space. Its output is already clip space,
// so it fills the world members with the local values (a material that reads
// them gets something defined rather than zero, and a fullscreen pass has no
// meaningful world position to report) and it ignores the world position offset
// entirely, because an offset in a space that does not exist is not a thing.
//
// The clip space mapping assumes the vertex buffer stores UV with the origin in
// the top left corner, which is the engine texture convention. The y flip below
// is the only place that convention is encoded.
// ---------------------------------------------------------------------------

#include "Public/VertexFactory/VertexFactoryCommon.hlsl"

// Mirrors the Inputs declaration of Components/C_VertexFactory.json verbatim.
// The semantic name, index and component count have to match the generated
// FinalInputStream and the reflection InputLayout, or the input layout the
// renderer builds will not match the compiled input signature.
struct VFAttributes
{
	float2	UV0			: TEXCOORD0;
};

void VFUnpackVertex(in VFAttributes InAttributes, in VFContext InContext, out VFVertex OutVertex)
{
	// UV covers the full viewport, so mapping it to the [-1, 1] clip range gives
	// the vertex its position without an extra stream.
	float2 ClipXY = InAttributes.UV0 * 2.0 - 1.0;
	ClipXY.y = -ClipXY.y;

	OutVertex.ObjectPosition = float3(ClipXY, 0.0);

	// The factory declares no optional attributes, so every remaining member gets
	// its constant fallback rather than a VF_HAS_* test.
	OutVertex.ObjectNormal = float3(0.0, 0.0, 1.0);
	OutVertex.ObjectTangent = float4(1.0, 0.0, 0.0, 1.0);
	OutVertex.UV0 = InAttributes.UV0;
	OutVertex.UV1 = float2(0.0, 0.0);
	OutVertex.Color = float4(1.0, 1.0, 1.0, 1.0);

	OutVertex.InstancePosition = OutVertex.ObjectPosition;
	OutVertex.InstanceNormal = OutVertex.ObjectNormal;

	OutVertex.WorldPosition = OutVertex.ObjectPosition;
	OutVertex.WorldNormal = OutVertex.ObjectNormal;
	OutVertex.WorldTangent = OutVertex.ObjectTangent.xyz;
	OutVertex.WorldBitangent = float3(0.0, 1.0, 0.0);

	OutVertex.Custom[0] = float4(0.0, 0.0, 0.0, 0.0);
	OutVertex.Custom[1] = float4(0.0, 0.0, 0.0, 0.0);
}

void VFTransformToWorld(inout VFVertex InOutVertex, in VFContext InContext)
{
	// Nothing to do. The unpack step already put the clip space coordinate into
	// the world members, which is the documented behaviour for this factory.
}

void VFTransformVertex(in VFVertex InVertex, in VFContext InContext, in float3 InWorldPositionOffset, inout VFVarying OutVarying)
{
	// InWorldPositionOffset is deliberately unused: see the header.
	OutVarying.Position = float4(InVertex.ObjectPosition.xy, 0.0, 1.0);
	OutVarying.UV0 = InVertex.UV0;
}
#endif	//FULL_SCREEN_VF_HLSL
