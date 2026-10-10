#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderCommon.h>
#include <ShaderAsset/ShaderAsset.h>
#include <Shader/ShaderStageResource.h>
#include <Shader/ShaderBindingPlan.h>
#include <RHI/RHITypes.h>

namespace PigeonEngine
{

	class IRCommandList;
	class EShaderInstance;
	class RShaderParameterBuffers;

	/*
	* ShaderAsset - engine side binding bridge.
	*
	* A shader asset describes a shader as (program, stage entry, binding plan); the
	* render layer needs (IRRHIShader*, input layouts, root indices). This file is the
	* single place that translates one into the other, so no proxy or renderer has to
	* reach into the asset internals or redo register arithmetic.
	*
	* Retrofit note: this bridge is the seed of the unified primitive render path. When
	* the proxy layer grows its shared shader program slot, the slot owns these helpers
	* and this file disappears.
	*/

	// Stage entry / stage resource of one program. Null when the program has no such
	// stage, or the stage entry was never composed.
	PE_NODISCARD extern const EShaderStageEntry*	ResolveShaderStageEntry(const EShaderProgram& InProgram, const EShaderStageType InStage);
	PE_NODISCARD extern const RShaderStageResource*	ResolveShaderStageResource(const EShaderProgram& InProgram, const EShaderStageType InStage);

	// Fills the shader half of a graphics pipeline description: vertex shader, pixel
	// shader and the input layout the vertex shader was compiled with. Returns FALSE
	// when either stage is missing so the caller can skip creating the PSO.
	PE_NODISCARD extern BOOL32	FillGraphicsPipelineShaders(const EShaderProgram& InProgram, RRHIGraphicsPipelineDesc& OutDesc);
	// Compute counterpart of FillGraphicsPipelineShaders.
	PE_NODISCARD extern BOOL32	FillComputePipelineShader(const EShaderProgram& InProgram, RRHIComputePipelineDesc& OutDesc);

	/*
	* Stage masks a caller hands to EShaderJobBuilder to say which stages it needs. The
	* compile layer takes one stage per request, so the builder asks for each requested
	* stage and folds the resulting manifests into one PAsset per shader name.
	*/
	PE_CONSTEXPR_STATIC UINT32 ENGINE_SHADER_STAGE_MASK_GRAPHICS	= (1u << 0u) | (1u << 1u);
	PE_CONSTEXPR_STATIC UINT32 ENGINE_SHADER_STAGE_MASK_COMPUTE		= (1u << 2u);

	// Variant the engine internal shaders ship with when the caller has no preference.
	PE_CONSTEXPR_STATIC const CHAR* ENGINE_SHADER_DEFAULT_VARIANT_NAME = "Base";

	// A shader is not looked up by name any more: a caller states the parameters it knows
	// (material, program, vertex axis, stages) through EShaderJobBuilder::EnsureAssembly,
	// which packages the hand written json + hlsl and applies the variant.

	// Creates the runtime instance of one variant of one shader asset. The instance owns
	// its own parameter set; the caller owns the instance.
	PE_NODISCARD extern EShaderInstance*	CreateShaderInstance(const EShaderAsset* InAsset, const EString& InVariantName);

	// Render thread: the GPU constant buffers of an instance, or null while the creation
	// command has not been drained yet. The instance only carries the reference; the object
	// itself lives in the render resource registry.
	PE_NODISCARD extern RShaderParameterBuffers*	ResolveShaderParameterBuffers(const EShaderInstance& InInstance);

	// Binds every constant buffer the stage actually uses at the root index the binding plan
	// reports, except the blocks the render tier publishes itself (IsRenderTierShaderBlock):
	// those roots belong to their own block objects, and a holder of an all zero mirror of one
	// of them must not push it over the real values. This is the material path - a draw's
	// material answers for its own blocks, and the renderer binds the engine blocks.
	// The buffers are the render side's own objects and are uploaded by the caller before the
	// bind ([InOutBuffers].UploadFromParameterSet); a block this stage has no buffer for is
	// skipped, so a previously bound buffer stays where it is. Callers pass the program the
	// instance resolved, not the asset, because the plan lives on the stage entry.
	extern void	BindShaderParameterBuffers(IRCommandList* InCommandList, RShaderParameterBuffers& InOutBuffers
		, const EShaderProgram& InProgram, const EShaderStageType InStage, const BOOL32 InIsCompute);

	// Binds exactly one block, and nothing else, at the root index the binding plan reports.
	// The render tier's own block objects call this with the name they publish, which is what
	// makes "a draw binds the blocks in its own scope" hold structurally: an object can write
	// its own root and no other. A stage whose plan does not carry the block, or which has no
	// buffer for it, is skipped.
	extern void	BindShaderParameterBlock(IRCommandList* InCommandList, RShaderParameterBuffers& InOutBuffers
		, const EShaderProgram& InProgram, const EShaderStageType InStage, const BOOL32 InIsCompute
		, const EName& InBlockName);

	// Binds the resource values of a parameter set through the binding plan: the slot inside
	// each table comes from the plan, never from the reflection directly, so a resource and the
	// pipeline state it is bound for cannot disagree. A resource the set holds no value for is
	// skipped, so a previously bound view stays where it is.
	extern void	BindShaderParameterResources(IRCommandList* InCommandList, const EShaderParameterSet& InParameters
		, const EShaderBindingPlan& InBindingPlan, const ERHIShaderStage InStage);

	// TRUE when the stage entry carries a usable reflection. Programs loaded from a
	// stage asset that failed to parse reflection report FALSE.
	PE_NODISCARD extern BOOL32	IsShaderStageEntryUsable(const EShaderProgram& InProgram, const EShaderStageType InStage);

	// Display name of a material for GPU debug markers. Never empty, so a marker can be
	// built unconditionally.
	PE_NODISCARD extern EString	GetShaderInstanceDisplayName(const EShaderInstance* InInstance);

	/*
	* Records that something which should have been drawn was not.
	*
	* A draw that cannot run is otherwise silent: the object stops appearing and nothing in the
	* frame says why, and an absence is the hardest symptom there is to chase. Every draw path
	* therefore reports its skips here, at the point that knows the reason - the resolution that
	* came back empty, the stage whose device object is missing, the call that refused to record.
	*
	* The report is deduplicated by owner and reason, so a thing that stays broken costs one line
	* rather than one per frame: the log stays readable while the state lasts, and a line that is
	* followed by nothing is the definition of a defect.
	*
	* InOwner is what a reader can match against the scene ("primitive 1234 pass 'Base'", "the sky
	* dome", "the ImGui overlay"). InReason says what was missing or which call refused. Both are
	* text because only the caller knows them.
	*
	* It lives in this header because every draw path already includes it: this is where the
	* binding and the stage assets are resolved, so it is the one place a drawer is already
	* looking at when it has to decide it cannot draw.
	*/
	extern void	ReportSkippedDraw(const EString& InOwner, const EString& InReason);

} // namespace PigeonEngine
