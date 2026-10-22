#ifndef ENGINE_MESH_VF_HLSL
#define ENGINE_MESH_VF_HLSL

// ---------------------------------------------------------------------------
// Engine mesh vertex factory (VF_TYPE_ENGINE_MESH).
//
// One factory for every engine mesh, static or skinned. A static mesh carries the
// attributes and nothing else; a skinned mesh carries the same attributes plus a
// skin stream, and the skin stream is what makes this factory blend bone matrices.
// There is no second factory identity and no VF_TYPE test in this file: every
// difference between the two meshes arrives as a VF_HAS_* flag, which is the same
// mechanism the optional attributes already use.
//
// The mesh factory: the mesh asset's own vertex layout, transformed by the draw's
// world matrix. Position is required, the rest are optional and fall back to a
// constant when the mesh does not carry them, which is what keeps VFVertex fully
// written no matter which stream profile the asset produced.
//
// Required attributes : Position
// Optional attributes : TexCoord, Normal, Tangent, TexCoord1, Color, and
//                       SkinIndex + SkinWeight, which only ever appear together
// VS only resources   : BoneMatrices, BoneInverseTransposeMatrices. They are
//                       declared by Components/C_VertexFactory.json under the same
//                       VF_HAS_SKIN_WEIGHT condition this file reads them under;
//                       no binding is declared here.
//
// There is no bitangent stream. The tangent carries its handedness in w and the
// bitangent is derived from the orthonormal world frame, which is one stream
// less to ship and the only form PS input actually needs.
//
// Referenced bindings, declared by components the program must list:
//   DrawParams.WorldMatrix / WorldInvTransposeMatrix  Components/C_DrawParams.json
//   ViewParams.ViewProjectionMatrix                   Components/C_EngineSystem.json
// The factory cannot declare them, because bindings live in the JSON, so the
// program that selects this factory lists C_DrawParams.json. C_EngineSystem.json
// is Index 0 of every program and needs no thought.
//
// VFUnpackVertex fills every member of VFVertex, VFTransformToWorld fills the
// instance and world spaces, and VFTransformVertex writes the interpolants, so
// nothing downstream ever asks whether an attribute was present.
//
// The skin stream is four influences wide: one element of each stream is one
// vertex's worth of influences, and the mesh stores one 16 bit bone index and one
// float per influence. The count is enforced against the mesh's own EffectBoneNum
// in EShaderJobBuilder::ResolveVertexAxis, which is also the one place the two
// VF_HAS_SKIN_* flags are decided, so this file never has to guess at either.
// ---------------------------------------------------------------------------

#include "Public/VertexFactory/VertexFactoryTypes.hlsl"
#include "Public/VertexFactory/VertexFactoryCommon.hlsl"
#include "Public/Core/ShaderSpaceTransform.hlsl"

// Mirrors the Inputs declaration of Components/C_VertexFactory.json verbatim.
// The semantic name, index and component count have to match the generated
// FinalInputStream and the reflection InputLayout, or the input layout the
// renderer builds will not match the compiled input signature.
//
// Position is declared unconditionally even though its JSON entry carries a
// VF_TYPE condition: that condition selects between this three component mesh
// position, the two component UI position of the ImGui factory and the handle of
// the grass factory, so for a compilation that selects this factory the entry is
// always present.
//
// The optional members are the VF_HAS_* side of that mirror: the JSON entry
// carries the same condition, so this declaration and the input layout gain and
// lose the same members together.
struct VFAttributes
{
	float3	Position	: POSITION0;
#if VF_HAS_NORMAL
	float3	Normal		: NORMAL0;
#endif	//VF_HAS_NORMAL
#if VF_HAS_TANGENT
	float4	Tangent		: TANGENT0;
#endif	//VF_HAS_TANGENT
#if VF_HAS_UV0
	float2	UV0			: TEXCOORD0;
#endif	// VF_HAS_UV0
#if VF_HAS_UV1
	float2	UV1			: TEXCOORD1;
#endif	//VF_HAS_UV1
#if VF_HAS_COLOR
	float4	Color		: COLOR0;
#endif	//VF_HAS_COLOR
#if VF_HAS_SKIN_INDEX
	uint4	SkinIndex	: BLENDINDICES0;
#endif	//VF_HAS_SKIN_INDEX
#if VF_HAS_SKIN_WEIGHT
	float4	SkinWeight	: BLENDWEIGHT0;
#endif	//VF_HAS_SKIN_WEIGHT
};

#if VF_HAS_SKIN_WEIGHT
// Blends the bone matrices of the four influences this vertex carries and deforms the
// local position, normal and tangent with the result. The arithmetic is the legacy
// ShaderSkinnings.hlsl one: a weighted sum divided by the sum of the weights, with the
// matrix product written the way the rest of this library writes it, mul(Matrix, Vector).
//
// The two forms are equivalent, which is worth stating because the legacy shader wrote
// mul(Vector, Matrix). Matrices are uploaded without transposing, so the float4x4 read
// back out of the structured buffer is the CPU matrix's transpose, and transpose(M) * v
// is exactly what a row vector times a matrix computed before.
//
// The normal goes through the inverse transpose pair so a non uniform bone scale does
// not skew it; the tangent is a surface direction and takes the blend matrix itself. The
// handedness in w is left alone, and VFTransformToWorld orthonormalises the frame.
void SkinVertex(in uint4 InBoneIndices, in float4 InBoneWeights
	, inout float3 InOutPosition, inout float3 InOutNormal, inout float4 InOutTangent)
{
	float4x4 BlendMatrix = (float4x4)0;
	float4x4 BlendInverseTranspose = (float4x4)0;
	float TotalWeight = 0.0;
	[unroll]
	for (uint Influence = 0u; Influence < 4u; Influence++)
	{
		const uint BoneIndex = InBoneIndices[Influence];
		const float Weight = InBoneWeights[Influence];
		BlendMatrix += BoneMatrices[BoneIndex] * Weight;
		BlendInverseTranspose += BoneInverseTransposeMatrices[BoneIndex] * Weight;
		TotalWeight += Weight;
	}
	BlendMatrix /= TotalWeight;
	BlendInverseTranspose /= TotalWeight;

	InOutPosition = TransformPositionToSpecificSpace(float4(InOutPosition, 1.0), BlendMatrix).xyz;
	InOutNormal = TransformDirectionToSpecificSpace(InOutNormal, (float3x3)BlendInverseTranspose, true);
	InOutTangent.xyz = TransformDirectionToSpecificSpace(InOutTangent.xyz, (float3x3)BlendMatrix, true);
}
#endif	//VF_HAS_SKIN_WEIGHT

void VFUnpackVertex(in VFAttributes InAttributes, in VFContext InContext, out VFVertex OutVertex)
{
	OutVertex.ObjectPosition = InAttributes.Position;

#if VF_HAS_NORMAL
	OutVertex.ObjectNormal = InAttributes.Normal;
#else
	OutVertex.ObjectNormal = float3(0.0, 0.0, 1.0);
#endif	//VF_HAS_NORMAL

#if VF_HAS_TANGENT
	OutVertex.ObjectTangent = InAttributes.Tangent;
#else
	// A mesh with no tangent stream still gets a usable frame: any perpendicular
	// of the normal is a legal tangent basis, and the handedness is right handed.
	OutVertex.ObjectTangent = BuildTangentFallback(OutVertex.ObjectNormal);
#endif	//VF_HAS_TANGENT

#if VF_HAS_UV0
	OutVertex.UV0 = InAttributes.UV0;
#else
	OutVertex.UV0 = float2(0.0, 0.0);
#endif	// VF_HAS_UV0

#if VF_HAS_UV1
	OutVertex.UV1 = InAttributes.UV1;
#else
	OutVertex.UV1 = float2(0.0, 0.0);
#endif	//VF_HAS_UV1

#if VF_HAS_COLOR
	OutVertex.Color = InAttributes.Color;
#else
	// White is the fallback that makes a material multiplying by the vertex
	// colour behave correctly for a mesh that has none.
	OutVertex.Color = float4(1.0, 1.0, 1.0, 1.0);
#endif	//VF_HAS_COLOR

#if VF_HAS_SKIN_WEIGHT
	// Skin the unpacked values before anything reads them. The step belongs here rather
	// than in VFTransformToWorld because this is the only place that holds the raw skin
	// attributes: VFVertex's layout is fixed and its free channels belong to the material
	// bridge, which reads Custom[0] as the instance colour. What VFTransformToWorld sees
	// is therefore already the skinned local value, which is what its local members
	// document.
	SkinVertex(InAttributes.SkinIndex, InAttributes.SkinWeight
		, OutVertex.ObjectPosition, OutVertex.ObjectNormal, OutVertex.ObjectTangent);
#endif	//VF_HAS_SKIN_WEIGHT

	// The instance and world spaces are VFTransformToWorld's to fill, and it runs next. They
	// are written here anyway, because the contract at the top of this file is not a style
	// note: FXC wants an out parameter to be completely initialized by the callee (error
	// X3508) and cannot see that the transform which follows is what fills them. The instance
	// values are the documented ones for a factory without an instance stream - the local
	// values, which the transform copies - and the world ones are the untransformed identity
	// frame, so a factory bug that skipped the transform shows as a vertex at the origin
	// rather than as one with a plausible position.
	OutVertex.InstancePosition = OutVertex.ObjectPosition;
	OutVertex.InstanceNormal = OutVertex.ObjectNormal;

	OutVertex.WorldPosition = float3(0.0, 0.0, 0.0);
	OutVertex.WorldNormal = float3(0.0, 0.0, 1.0);
	OutVertex.WorldTangent = float3(1.0, 0.0, 0.0);
	OutVertex.WorldBitangent = float3(0.0, 1.0, 0.0);

	// Free channels: this factory has nothing to put in them, and leaving them
	// undefined would make a material that reads one read garbage.
	OutVertex.Custom[0] = float4(0.0, 0.0, 0.0, 0.0);
	OutVertex.Custom[1] = float4(0.0, 0.0, 0.0, 0.0);
}

void VFTransformToWorld(inout VFVertex InOutVertex, in VFContext InContext)
{
	// An engine mesh has no instance stream, so instance space is object space and
	// one transform takes it to world space. The instance members are filled
	// rather than removed, which is what makes "is this draw instanced" a non
	// question for a material.
	InOutVertex.InstancePosition = InOutVertex.ObjectPosition;
	InOutVertex.InstanceNormal = InOutVertex.ObjectNormal;

	InOutVertex.WorldPosition = TransformObjectToWorld(InOutVertex.InstancePosition, WorldMatrix);

	// The normal goes through the inverse transpose so a non uniform scale does
	// not skew it. The tangent is a surface direction, so the world matrix is the
	// right one for it, and the frame is orthonormalized afterwards.
	InOutVertex.WorldNormal = TransformObjectToWorldNormal(InOutVertex.InstanceNormal, WorldInvTransposeMatrix, true);

	float3 WorldTangent = TransformObjectToWorldDir(InOutVertex.ObjectTangent.xyz, WorldMatrix, true);
	BuildWorldTangentFrame(InOutVertex.WorldNormal, float4(WorldTangent, InOutVertex.ObjectTangent.w),
		InOutVertex.WorldTangent, InOutVertex.WorldBitangent);
}

void VFTransformVertex(in VFVertex InVertex, in VFContext InContext, in float3 InWorldPositionOffset, inout VFVarying OutVarying)
{
	// The offset is a world space displacement, so it is added before the view
	// projection is applied. That ordering is the whole point of the material
	// returning an offset instead of a position: the engine owns world to clip.
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
#endif	//ENGINE_MESH_VF_HLSL
