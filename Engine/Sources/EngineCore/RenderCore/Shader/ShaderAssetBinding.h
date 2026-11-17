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
	class IRRHIDevice;
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
	// when a stage the caller asked for is missing so the caller can skip creating the PSO.
	//
	// InStageMask is RGraphicsPipelineStateDesc::STAGE_MASK_*. A pass that draws with no
	// pixel work (a depth only shadow pass) clears the pixel bit, and the pipeline is built
	// with a null pixel shader: the rasterizer then skips that stage entirely instead of
	// running an empty one. The program may still carry a pixel stage; it is simply not used.
	PE_NODISCARD extern BOOL32	FillGraphicsPipelineShaders(const EShaderProgram& InProgram, UINT32 InStageMask, RRHIGraphicsPipelineDesc& OutDesc);
	// Compute counterpart of FillGraphicsPipelineShaders.
	PE_NODISCARD extern BOOL32	FillComputePipelineShader(const EShaderProgram& InProgram, RRHIComputePipelineDesc& OutDesc);

	/*
	* Stage masks a caller hands to EShaderJobBuilder to say which stages it needs. The
	* compile layer takes one stage per request, so the builder asks for each requested
	* stage and folds the resulting manifests into one PAsset per shader name.
	*/
	// One bit per EShaderStageType. The compile layer names the stages it needs, and the
	// publishing entry points below name the stages a publisher feeds, so a mask and a stage
	// entry line up without a translation table.
	PE_CONSTEXPR_STATIC UINT32 ENGINE_SHADER_STAGE_MASK_VERTEX			= (1u << 0u);
	PE_CONSTEXPR_STATIC UINT32 ENGINE_SHADER_STAGE_MASK_PIXEL			= (1u << 1u);
	PE_CONSTEXPR_STATIC UINT32 ENGINE_SHADER_STAGE_MASK_COMPUTE			= (1u << 2u);
	PE_CONSTEXPR_STATIC UINT32 ENGINE_SHADER_STAGE_MASK_GRAPHICS		= ENGINE_SHADER_STAGE_MASK_VERTEX | ENGINE_SHADER_STAGE_MASK_PIXEL;

	/*
	* Claim scope names of the modules that publish resource bindings. The scope is the module's
	* identity in the claim registry, so it is spelled once here and shared by the registration
	* and the publish call rather than written as a literal at both ends. Modules that own only
	* constant buffers (the global and view blocks) have no entry: the claim surface is resources.
	*/
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_SKY						= PE_TEXT("Sky");
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_ENVIRONMENT_SH				= PE_TEXT("EnvironmentSH");
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_ENVIRONMENT_PREFILTER		= PE_TEXT("EnvironmentPrefilter");
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_ENVIRONMENT_BRDF			= PE_TEXT("EnvironmentBRDF");
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_LIGHTING					= PE_TEXT("Lighting");
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_SHADOW_MASK				= PE_TEXT("ShadowMask");
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_COPY						= PE_TEXT("Copy");
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_SKELETAL					= PE_TEXT("Skeletal");
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_DEBUG_WIREFRAME			= PE_TEXT("DebugWireframe");
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_BEZIER_GRASS				= PE_TEXT("BezierGrass");
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_IMGUI						= PE_TEXT("ImGui");
	// The water simulation passes publish their own state and ground textures, and the water
	// surface publishes the simulation outputs it reads into the user material.
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_WATER_SIMULATION			= PE_TEXT("WaterSimulation");
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_WATER_SURFACE				= PE_TEXT("WaterSurface");
	// The material / shader instance publisher. Its bindings are Exposure driven rather than
	// statically claimed, but a value it does publish and then cannot use is still its defect.
	PE_CONSTEXPR_STATIC const CHAR* SHADER_BINDING_SCOPE_MATERIAL					= PE_TEXT("Material");

	// Variant the engine internal shaders ship with when the caller has no preference.
	PE_CONSTEXPR_STATIC const CHAR* ENGINE_SHADER_DEFAULT_VARIANT_NAME				= PE_TEXT("Base");

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
	// pipeline state it is bound for cannot disagree. A resource the set holds no value for binds
	// the engine default when the plan resolved it as exposed, reports it when this scope is its
	// single claimant, and otherwise leaves the previous view in place.
	extern void	BindShaderParameterResources(IRCommandList* InCommandList, const EShaderParameterSet& InParameters
		, const EShaderBindingPlan& InBindingPlan, const ERHIShaderStage InStage, const EName& InScopeName);

	/*
	* Publishing: the one place that makes a parameter set reachable by a program.
	*
	* Two shapes, because a publisher owns either both halves or only one:
	*
	*   PublishShaderParameters  the publisher owns the set's constant buffers and its resources
	*                            (a material, an instance). Resolves the descriptor heap index of
	*                            every index driven resource, uploads the blocks, then binds both
	*                            through the plan of each stage InStageMask selects.
	*   BindShaderResources      the publisher owns resources only (the sky's cube, the lighting
	*                            pass's targets, the skeletal bones). Binds them, and refuses a
	*                            set that publishes an index driven resource: an index is a
	*                            constant buffer value, and a publisher with no blocks has
	*                            nowhere to carry it.
	*
	* Ordering is the contract, not a convention. The index of an index driven resource travels as
	* a member of the constant buffer the resource named, so it has to be inside the bytes that
	* upload. That is why resolving it cannot be a separate step a caller could order after the
	* upload, and why these exist as one call each rather than as three.
	*
	* InStageMask is one bit per EShaderStageType (ENGINE_SHADER_STAGE_MASK_GRAPHICS / _COMPUTE).
	* A stage the program does not carry, or that the mask leaves out, is skipped.
	*
	* InScopeName is the module identity of the publisher. A binding's plan entry carries the
	* three-state claim resolution (see ShaderBindingPlan.h): a claimed binding is reported as
	* missing only when this scope is its single claimant, an exposed binding falls back to the
	* engine default resource and stays silent, and an unclaimed binding was already reported at
	* plan build. A module that owns no resource bindings - the global and view blocks are
	* constant buffers only - therefore needs no claim table at all.
	*
	* FALSE means an index could not be produced or published - the target cannot express the
	* heap path, the device refused the allocation, or the set publishes an index driven resource
	* it cannot carry an index for. The caller may treat that as a draw worth skipping. A program
	* with nothing index driven publishes nothing and answers TRUE.
	*/
	PE_NODISCARD extern BOOL32	PublishShaderParameters(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice
		, EShaderParameterSet& InOutParameters, RShaderParameterBuffers& InOutBuffers
		, const EShaderProgram& InProgram, const UINT32 InStageMask, const EName& InScopeName);

	/*
	* The two halves of PublishShaderParameters, split so the renderer can publish during the
	* serial Prepare phase and bind during a parallel Record phase.
	*
	* PublishShaderParameterValues writes the bindless indices into the set, re-evaluates the
	* CPU mirrors and uploads the dirty buffers. It takes no command list, so it is legal
	* exactly where FFrameRenderData is allowed to run: on the render thread, once per frame.
	*
	* BindPublishedShaderParameters replays the blocks and resources of an already published
	* set. It records commands and touches no value, which is what a worker list needs.
	*/
	PE_NODISCARD extern BOOL32	PublishShaderParameterValues(IRRHIDevice* InRenderDevice
		, EShaderParameterSet& InOutParameters, RShaderParameterBuffers& InOutBuffers
		, const EShaderProgram& InProgram, const EName& InScopeName);
	extern void	BindPublishedShaderParameters(IRCommandList* InCommandList
		, const EShaderParameterSet& InParameters, RShaderParameterBuffers& InOutBuffers
		, const EShaderProgram& InProgram, const UINT32 InStageMask, const EName& InScopeName);
	// The result is advisory: a failure has already been logged by name where it was detected, so
	// a caller that cannot act on it may ignore it. Only a caller that is able to skip its draw
	// has a reason to look.
	extern BOOL32	BindShaderResources(IRCommandList* InCommandList
		, const EShaderParameterSet& InParameters, const EShaderProgram& InProgram, const UINT32 InStageMask
		, const EName& InScopeName);
	/*
	* The same, with a second set whose register bound resources take precedence over the base
	* set's. A pass instance inherits buffer values through its parent chain, but resource
	* overrides are read from one set only, so a per draw texture (the water simulation outputs
	* a surface reads) cannot reach a material's pass instance any other way. Bindless indices
	* are still checked in the base set, which is where their constant buffers live.
	*/
	extern BOOL32	BindShaderResources(IRCommandList* InCommandList
		, const EShaderParameterSet& InBaseParameters, const EShaderParameterSet& InOverrideParameters
		, const EShaderProgram& InProgram, const UINT32 InStageMask, const EName& InScopeName);
	// The same for a publisher that already holds one stage's plan rather than the program - the
	// debug wireframe's per primitive instance buffers arrive that way.
	extern BOOL32	BindShaderResources(IRCommandList* InCommandList
		, const EShaderParameterSet& InParameters, const EShaderBindingPlan& InBindingPlan, const ERHIShaderStage InStage
		, const EName& InScopeName);

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
