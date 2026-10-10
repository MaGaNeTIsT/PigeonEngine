#ifndef VERTEX_FACTORY_COMMON_HLSL
#define VERTEX_FACTORY_COMMON_HLSL

// ---------------------------------------------------------------------------
// The vertex factory contract.
//
// Every VF works on the same intermediate structure, so the vertex step a
// material may take part in, the transform to clip space and the escape gate can
// all be written once against a stable shape:
//
//   VFAttributes  <- the raw input stream, per VF, mirrors the JSON Inputs
//   VFVertex      <- unpacked and space transformed, this file
//   VFVarying     <- interpolated to the pixel stage, per VF
//
// VFAttributes and VFVarying are supplied by each VF through the injection
// points owned by Components/C_VertexFactory.json. They are deliberately not
// declared here, because only one VF is compiled at a time.
//
// ---------------------------------------------------------------------------
// A vertex factory implements three functions
//
//   void VFUnpackVertex(in VFAttributes InAttributes, in VFContext InContext, out VFVertex OutVertex);
//       Reads the input stream, applies the documented fallback for every
//       attribute the mesh does not carry, applies the deformation the factory
//       owns (a factory that reads a skin stream blends its bone matrices here,
//       which is the only step that holds those attributes), and leaves the local
//       space values and the texture coordinates filled. World space is not this
//       function's job.
//
//   void VFTransformToWorld(inout VFVertex InOutVertex, in VFContext InContext);
//       Composes local -> instance -> world and fills the world space members. The
//       local values it is handed are final, skinning included. A non instanced
//       factory fills the instance members with the local values and composes one
//       transform.
//
//   void VFTransformVertex(in VFVertex InVertex, in VFContext InContext, in float3 InWorldPositionOffset,
//       out VFVarying OutVarying);
//       Writes the system interpolants, including the clip space position, from
//       the world position plus the material's offset. Called after the material
//       has had its say, which is why the offset is a parameter.
//
// The layout of VFVertex is fixed. A factory that has no data for a member fills
// in the documented fallback instead of removing the member, which is what keeps
// a material's vertex step independent of the active vertex factory. The same rule
// is why a factory's own per vertex state does not live here: the skinned engine
// mesh keeps its skin attributes inside its factory file and consumes them in
// VFUnpackVertex, rather than widening a structure every factory and the material
// bridge share.
//
// ---------------------------------------------------------------------------
// Who owns which space
//
// The engine composes local -> instance -> world before the material runs, so a
// material can read any of the three without knowing how the mesh got there.
// MaterialVertexInput in Public/Material/MaterialIO.hlsl is the read only view of
// this structure that a material actually sees.
// ---------------------------------------------------------------------------

struct VFContext
{
	uint	VertexID;
	uint	InstanceID;
	uint	PrimitiveID;
};

struct VFVertex
{
	// local space, as authored. A skinned factory overwrites these with the
	// skinned result, so they are "local to whatever the factory produced".
	float3	ObjectPosition;
	float3	ObjectNormal;
	float4	ObjectTangent;		// xyz = tangent, w = handedness
	float2	UV0;
	float2	UV1;
	float4	Color;

	// instance space. Equal to the local values for a factory without an instance
	// stream, which is what makes "is this draw instanced" a non question for a
	// material that only reads them.
	float3	InstancePosition;
	float3	InstanceNormal;

	// world space
	float3	WorldPosition;
	float3	WorldNormal;
	float3	WorldTangent;		// unit length, handedness already applied
	float3	WorldBitangent;

	// free channels, per factory
	float4	Custom[2];
};

// Fallback tangent for a mesh that has no tangent stream: any perpendicular of
// the normal is a valid basis, the handedness is assumed to be right handed.
float4 BuildTangentFallback(in float3 InNormal)
{
	float3 Normal = InNormal;
	float3 Tangent = (abs(Normal.z) < 0.999) ? cross(float3(0.0, 0.0, 1.0), Normal) : cross(float3(1.0, 0.0, 0.0), Normal);
	Tangent = normalize(Tangent);
	return float4(Tangent, 1.0);
}

// Orthonormal world space frame from a world normal and tangent, handedness
// applied. A factory that has no tangent stream calls this with the fallback.
void BuildWorldTangentFrame(in float3 InWorldNormal, in float4 InWorldTangent, out float3 OutTangent, out float3 OutBitangent)
{
	float3 Normal = normalize(InWorldNormal);
	float3 Tangent = normalize(InWorldTangent.xyz - Normal * dot(Normal, InWorldTangent.xyz));
	OutTangent = Tangent;
	OutBitangent = cross(Normal, Tangent) * InWorldTangent.w;
}
#endif	//VERTEX_FACTORY_COMMON_HLSL
