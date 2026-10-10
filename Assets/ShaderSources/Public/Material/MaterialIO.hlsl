#ifndef MATERIAL_IO_HLSL
#define MATERIAL_IO_HLSL

// ---------------------------------------------------------------------------
// The material facing interfaces.
//
// Three structures and the two entry points a material implements. They are
// declared here rather than inside the programmable template because of
// injection order: the programmable template is injected last, while a domain is
// injected before it, and a domain has to be able to name MaterialPixelInput in
// its DeriveSurface signature. Declaring them in a header both sides include
// sidesteps the ordering entirely.
//
// ---------------------------------------------------------------------------
// Why the vertex stage hands the material a packet instead of the raw varying
//
// A material reads the packed vertex data the engine computed and writes exactly
// two things: a world space position offset, and whatever extra interpolants it
// wants. It does not write the clip space position, and it does not write the
// engine's own interpolants. That split is what lets the engine own the
// WorldPosition -> clip space step, where the offset has to be added before the
// view projection is applied.
//
//   in  MaterialVertexInput    engine computed values, every space, read only
//   out float3 WPO             world space offset, added to the world position
//   inout VFVarying            the material's own interpolants only; the engine
//                              overwrites the system members after the call
//
// ---------------------------------------------------------------------------
// Why the pixel stage hands the material a packet too
//
// Same shape on the other side. The material gets the interpolated values plus
// the cheap derived values the engine already had to compute (world position,
// view direction, screen position), so a material that needs one of them does
// not have to redo it, and it writes the domain's MaterialSurface and nothing
// else. The domain turns that into SurfaceData, and the engine turns that into
// GBuffer channels.
// ---------------------------------------------------------------------------

#include "Public/Core/ShaderConstants.hlsl"

// ---------------------------------------------------------------------------
// Vertex stage input
//
// Every space at once, because the engine has already computed all of them by
// the time a material runs: unpacking and skinning happen first. A material that
// wants the object space position and the world space normal does not need two
// code paths, it just reads two fields.
// ---------------------------------------------------------------------------

struct MaterialVertexInput
{
	// local (as authored)
	float3	LocalPosition;
	float3	LocalNormal;
	float4	LocalTangent;			// xyz = tangent, w = handedness
	float2	LocalUV0;
	float2	LocalUV1;
	float4	LocalColor;

	// world
	float3	WorldPosition;
	float3	WorldNormal;
	float3	WorldTangent;
	float3	WorldBitangent;

	// identity, for effects that vary per draw or per primitive
	uint	VertexID;
	uint	InstanceID;
	uint	PrimitiveID;

	// per instance data reaching a material that wants it, zero when the vertex
	// factory has no instance stream
	float4	InstanceColor;
};

// ---------------------------------------------------------------------------
// Pixel stage input
// ---------------------------------------------------------------------------

struct MaterialPixelInput
{
	// world space, interpolated
	float3	WorldPosition;
	float3	WorldNormal;			// interpolated geometric normal, unit length
	float3	WorldTangent;
	float3	WorldBitangent;

	// texture coordinates, zero when the mesh has none
	float2	UV0;
	float2	UV1;

	// vertex colour, white when the mesh has none, which is the value that makes
	// a material that multiplies by it behave correctly for both cases
	float4	VertexColor;

	// view space conveniences
	float3	ViewDirection;			// unit vector from the surface towards the eye

	// screen space. ScreenPosition is SV_Position: xy in pixels, zw the
	// reciprocal of w, which is what a screen space effect or a depth fetch
	// needs. ScreenUV is the same thing normalized to [0, 1].
	float4	ScreenPosition;
	float2	ScreenUV;

	// identity
	uint	PrimitiveID;
	bool	IsFrontFace;
};

// ---------------------------------------------------------------------------
// Entry points a material implements
//
// A material writes these in its own .hlsl. The engine calls them through the
// injection points, so the caller is compiled before the callee and the
// declaration order is what makes that legal.
//
// Both are optional in the sense that a material which does not need the vertex
// stage simply does not write that function and the material asset declares
// VertexOps = false, which routes it to the shared neutral vertex material
// instead. A material that has no vertex function but is compiled in a program
// that declares one gets the neutral implementation, see
// Surfaces/ProgrammableShaderTemplate.hlsl.
// ---------------------------------------------------------------------------

// in  Input      the packed values described above
// out WPO        world space offset; float3(0, 0, 0) for a material that does
//                not displace anything
// inout Varying  the material's own interpolants. The engine writes its own
//                members into this same structure afterwards, so a material
//                must not write the system members: writes to them are simply
//                overwritten, which is a silent no op rather than an error.
//
// void CalcMaterialVertex(in MaterialVertexInput InInput, out float3 OutWorldPositionOffset, inout VFVarying InOutVarying);

// in  Input      interpolated values and derived conveniences
// out Surface    the selected domain's MaterialSurface
//
// void CalcMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface);
//
// Coverage only, for a pass that clips but does not shade. It answers both
// halves of the question, because a threshold without a value to compare it
// against is not enough to decide anything:
//
// in  Input      same as above
// out Opacity    the surface's coverage, 0 to 1
// out AlphaCutoff  the threshold the coverage is tested against
//
// void CalcMaterialOpacity(in MaterialPixelInput InInput, out float OutOpacity, out float OutAlphaCutoff);
#endif	//MATERIAL_IO_HLSL
