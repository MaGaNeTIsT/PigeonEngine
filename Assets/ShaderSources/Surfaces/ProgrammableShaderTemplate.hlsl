#ifndef PROGRAMMABLE_SHADER_TEMPLATE_HLSL
#define PROGRAMMABLE_SHADER_TEMPLATE_HLSL

// ---------------------------------------------------------------------------
// The programmable layer: one entry point for both stages, and the surface
// assembly.
//
// The design has exactly one of these. It is injected at PROGRAMMABLE, it is
// referenced by a vertex stage and by a pixel stage alike, and the vertex half
// and the pixel half are separated by the stage defines rather than by two
// different components. The reason is slot economy and, more importantly, that
// the vertex half and the pixel half are two halves of one contract: whatever
// the vertex stage writes into the interpolants is what the pixel stage reads.
//
// Injected text, so it is scanned by the placeholder pass:
//   MATERIAL_FUNCTIONS   the material, or the default material implementation
//   DOMAIN_FUNCTIONS     the selected domain
// Both must be injected before this file, because the vertex half calls into the
// material and the pixel half builds on the domain's MaterialSurface.
//
// Ordering against the vertex factory: this file is injected before
// VF_VERTEX_FACTORY, so it is the one that pulls in the vertex factory contract
// and the factory's VFTransformVertex can use what it declares.
// ---------------------------------------------------------------------------

#include "Public/Material/ShaderMaterialCommon.hlsl"
#include "Public/Material/MaterialIO.hlsl"
#include "Public/BRDF/ShaderBxDF.hlsl"
#include "Public/VertexFactory/VertexFactoryCommon.hlsl"
#include "Public/VertexFactory/VertexFactoryTypes.hlsl"

// Order matters here and it is not cosmetic. The domain declares the
// MaterialSurface the material's pixel entry point is written against, so the
// domain has to be injected first. The design states the same rule.
//
// The domain is pixel stage only: its shading half talks to the light buffers,
// which a vertex stage does not declare. The guard also means an unresolved
// DOMAIN_FUNCTIONS placeholder is dropped by the preprocessor instead of
// reaching the compiler as text.
#if SHADER_STAGE_PIXEL
{DOMAIN_FUNCTIONS}
#endif	//SHADER_STAGE_PIXEL

// The material is injected for both stages. Its file guards its own entry points
// by stage, so the vertex stage gets CalcMaterialVertex and the pixel stage gets
// CalcMaterialSurface without either stage seeing a function it cannot compile.
//
// The guard is not decoration: a fullscreen pass has no material at all, and
// without it the unresolved placeholder would reach the compiler as literal
// text. The assembler sets MATERIAL_HAS_FUNCTIONS when the component list
// names a material.
#if MATERIAL_HAS_FUNCTIONS
{MATERIAL_FUNCTIONS}
#endif	//MATERIAL_HAS_FUNCTIONS

// ===========================================================================
// Vertex stage
//
// Three steps, and the material only takes part in the middle one:
//
//   1. the factory unpacks the input stream and composes the spaces
//   2. the material reads any space and returns a world space offset plus its
//      own interpolants
//   3. the factory turns world position plus offset into clip space and writes
//      the system interpolants
//
// Step 3 running after step 2 is what makes the system interpolants
// unmodifiable: a material that writes one has it overwritten, silently, which
// is the documented behaviour rather than an accident.
// ===========================================================================

#if SHADER_STAGE_VERTEX

MaterialVertexInput BuildMaterialVertexInput(in VFVertex InVertex, in VFContext InContext)
{
	MaterialVertexInput Result;
	Result.LocalPosition = InVertex.ObjectPosition;
	Result.LocalNormal = InVertex.ObjectNormal;
	Result.LocalTangent = InVertex.ObjectTangent;
	Result.LocalUV0 = InVertex.UV0;
	Result.LocalUV1 = InVertex.UV1;
	Result.LocalColor = InVertex.Color;

	Result.WorldPosition = InVertex.WorldPosition;
	Result.WorldNormal = InVertex.WorldNormal;
	Result.WorldTangent = InVertex.WorldTangent;
	Result.WorldBitangent = InVertex.WorldBitangent;

	Result.VertexID = InContext.VertexID;
	Result.InstanceID = InContext.InstanceID;
	Result.PrimitiveID = InContext.PrimitiveID;

	Result.InstanceColor = InVertex.Custom[0];
	return Result;
}

// The engine's bridge. A program template calls this from its vertex entry point
// through the VF_VS_BODY injection point; an escape gate factory supplies a body
// of its own instead and this one goes unused.
//
// OutVarying is filled by the material first and by the factory afterwards, so
// the struct carries both the material's own members and the system members, and
// the two never collide because the factory writes last.
void ExecuteMaterialVertex(in VFAttributes InAttributes, in VFContext InContext, out VFVarying OutVarying)
{
	// The varying is caller owned state: the material writes its own interpolants into it and
	// the factory completes the system members afterwards, so it is initialized whole before
	// either of them runs. That is also what the bridge's inout parameters require - FXC
	// refuses a call whose inout argument is not completely initialized - and it keeps the
	// members neither of them writes defined rather than whatever the stack held.
	OutVarying = (VFVarying)0;

	VFVertex Vertex;
	VFUnpackVertex(InAttributes, InContext, Vertex);
	VFTransformToWorld(Vertex, InContext);

	MaterialVertexInput MatInput = BuildMaterialVertexInput(Vertex, InContext);

	// The material writes its own interpolants into OutVarying; the system members are still
	// zero at this point and are filled in below.
	//
	// With no material there is nothing to call and nothing to displace: the
	// offset stays zero and the factory writes the interpolants alone. That is the
	// provider-is-the-implementation rule showing through, and it is why a
	// fullscreen pass needs no material at all.
	float3 WorldPositionOffset = float3(0.0, 0.0, 0.0);
#if MATERIAL_HAS_FUNCTIONS
	CalcMaterialVertex(MatInput, WorldPositionOffset, OutVarying);
#endif	//MATERIAL_HAS_FUNCTIONS

	VFTransformVertex(Vertex, InContext, WorldPositionOffset, OutVarying);
}

#endif	//SHADER_STAGE_VERTEX

// ===========================================================================
// Pixel stage
// ===========================================================================

#if SHADER_STAGE_PIXEL

// The read only view of the interpolants a material gets. Everything here is
// either an interpolated value or something the engine already had to compute,
// so a material that needs it does not recompute it.
//
// PrimitiveID and IsFrontFace arrive as separate system inputs to the pixel
// entry point rather than as interpolants: both are system semantics that the
// rasterizer produces directly, so carrying them through the interpolator list
// would cost bandwidth for values the pixel stage already has.
MaterialPixelInput BuildMaterialPixelInput(in VFVarying InVarying, in float3 InViewPosition,
	in uint InPrimitiveID, in bool InIsFrontFace)
{
	MaterialPixelInput Result;
	Result.WorldPosition = InVarying.WorldPosition;
	Result.WorldNormal = InVarying.WorldNormal;
	Result.WorldTangent = InVarying.WorldTangent;
	Result.WorldBitangent = InVarying.WorldBitangent;
	Result.UV0 = InVarying.UV0;
	Result.UV1 = InVarying.UV1;
	Result.VertexColor = InVarying.Color;

	Result.ViewDirection = ComputeViewDirection(InVarying.WorldPosition, float4(InViewPosition, 1.0));

	Result.ScreenPosition = InVarying.Position;
	Result.ScreenUV = InVarying.Position.xy * InvScreenSize;

	Result.PrimitiveID = InPrimitiveID;
	Result.IsFrontFace = InIsFrontFace;
	return Result;
}

// The domain's fallbacks, kept because a domain is engine code and needs a guard
// against a missing implementation. A domain that sets its DOMAIN_HAS_* switch to
// 1 provides its own and these step aside.
#if DOMAIN_HAS_SURFACE_DERIVE == 0
void DeriveSurface(in MaterialSurface InMaterial, in MaterialPixelInput InInput, out SurfaceData OutSurface)
{
	OutSurface.BaseColor = InMaterial.BaseColor.rgb;
	OutSurface.DiffuseColor = ComputeDiffuseColor(InMaterial.BaseColor.rgb, InMaterial.Metallic);
	OutSurface.F0 = ComputeFresnelF0(InMaterial.Reflectance, InMaterial.Metallic, InMaterial.BaseColor.rgb);
	OutSurface.EmissiveColor = InMaterial.EmissiveColor;
	OutSurface.Normal = InInput.WorldNormal;
	OutSurface.PerceptualRoughness = ClampPerceptualRoughness(InMaterial.Roughness);
	OutSurface.Roughness = ComputePerceptualRoughnessToRoughness(OutSurface.PerceptualRoughness);
	OutSurface.Metallic = InMaterial.Metallic;
	OutSurface.Reflectance = InMaterial.Reflectance;
	OutSurface.AmbientOcclusion = InMaterial.AmbientOcclusion;
	OutSurface.Opacity = ComputeSurfaceOpacity(InMaterial.BaseColor.a, InMaterial.Opacity);
	OutSurface.AlphaCutoff = InMaterial.AlphaCutoff;
	OutSurface.LightChannelMask = LIGHT_CHANNEL_ALL;
}
#endif	//DOMAIN_HAS_SURFACE_DERIVE

#if DOMAIN_HAS_SURFACE_SHADING == 0
void ShadeSurface(in SurfaceData InSurface, in MaterialPixelInput InInput, out ShadingResult OutResult)
{
	OutResult.Diffuse = InSurface.DiffuseColor * DiffuseLambert();
	OutResult.Specular = float3(0.0, 0.0, 0.0);
}
#endif	//DOMAIN_HAS_SURFACE_SHADING

// The pixel half of the material contract: init, then let the material have it.
// The material writes into a structure that is already fully defaulted, which is
// what removes every "is this input connected" test from material code.
//
// With no material the initialised defaults are the answer. A pass that has no
// material slot -- the sky, the deferred lighting pass -- therefore gets a well
// defined all default surface rather than an uninitialised one.
void EvaluateMaterialSurface(in MaterialPixelInput InInput, out MaterialSurface OutSurface)
{
	InitMaterialSurface(OutSurface);
#if MATERIAL_HAS_FUNCTIONS
	CalcMaterialSurface(InInput, OutSurface);
#endif	//MATERIAL_HAS_FUNCTIONS
}

// The standard path: material, then derive, then shade. A program that wants to
// split these -- a deferred base pass does, because it derives but does not
// shade -- calls the pieces directly instead.
void EvaluateSurface(in MaterialPixelInput InInput, out SurfaceData OutSurface, out ShadingResult OutResult)
{
	MaterialSurface Material;
	EvaluateMaterialSurface(InInput, Material);
	DeriveSurface(Material, InInput, OutSurface);
	ShadeSurface(OutSurface, InInput, OutResult);
}

#endif	//SHADER_STAGE_PIXEL
#endif	//PROGRAMMABLE_SHADER_TEMPLATE_HLSL
