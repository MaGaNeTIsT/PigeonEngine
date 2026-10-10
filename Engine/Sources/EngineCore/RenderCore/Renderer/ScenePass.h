#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderCommon.h>
#include <Pipeline/RPipelineStateDesc.h>
#include <RHI/RHITypes.h>

namespace PigeonEngine
{

	class EShaderProgram;
	class IRCommandList;

	/*
	* RenderCore/Renderer - the pass contract.
	*
	* A pass is described once, as data, and both sides read that one description: the game
	* side asks which passes a material has to be assembled for, and the render side asks for
	* the fixed function state and the render targets a pass runs with. Adding a pass is
	* therefore one enum value, one table row and, for a pass with its own shaders, one
	* program descriptor under Assets/ShaderSources/Programs.
	*
	* The enum is append only. A value ends up in saved material data and in the pass tables,
	* so renumbering would silently repoint a pass at another one's state.
	*/

	enum ERenderPassType : UINT8
	{
		// Graphics, per primitive. Runs a material's own program, so a material has to be
		// assembled for it.
		RENDER_PASS_TYPE_BASE = 0,			// BasePass, GBuffer MRT, cull back
		RENDER_PASS_TYPE_BASE_GRASS,		// BezierGrass, GBuffer MRT, cull none
		RENDER_PASS_TYPE_BASE_GRASS_WIRE,	// BezierGrass, GBuffer MRT, cull none, wireframe
		RENDER_PASS_TYPE_DEPTH,				// DepthOnly, depth only, no colour target
		// Graphics, per primitive, reserved: no program yet.
		RENDER_PASS_TYPE_SHADOW,			// shadow map array
		// Graphics, engine driven. The pass owns its program, so a material is not involved.
		RENDER_PASS_TYPE_FORWARD,			// reserved: forward geometry / editor primitives
		RENDER_PASS_TYPE_SKY,				// Sky, cull front, depth less equal
		RENDER_PASS_TYPE_LIGHTING,			// SceneLighting, full screen, additive
		RENDER_PASS_TYPE_FINAL_OUTPUT,		// Copy, full screen blit to the back buffer
		RENDER_PASS_TYPE_IMGUI,				// ImGui overlay
		// Compute. A pass with no per primitive work is recorded once per frame; a pass with
		// per primitive work is recorded per primitive that reports an instance for it.
		RENDER_PASS_TYPE_GRASS_CONSTRUCT,	// BezierGrassConstruct, per primitive
		RENDER_PASS_TYPE_SWE_INTEGRATE,		// SWEIntegrateHeight / SWEIntegrateVelocity, per primitive
		RENDER_PASS_TYPE_SWE_MODIFIER,		// SWEModifier / SWEModifierWave / SWEModifierWhirlpool, per primitive
		RENDER_PASS_TYPE_COLOR_GRADE,		// ColorGrade, full screen
		RENDER_PASS_TYPE_COUNT
	};

	// A pass that owns no pipeline state row in the renderer: its pipeline state comes from a
	// program through RPipelineStateCache, so the pass only describes fixed function state that
	// the program's own desc does not carry.
	PE_CONSTEXPR_STATIC UINT32 RENDER_PASS_INVALID_PIPELINE_SLOT = 0xffffffffu;

	// Stage bits, in EShaderStageType order. They match ENGINE_SHADER_STAGE_MASK_* in
	// Shader/ShaderAssetBinding.h, which is what the assembly layer consumes.
	PE_CONSTEXPR_STATIC UINT32 RENDER_PASS_STAGE_MASK_VERTEX	= (1u << 0u);
	PE_CONSTEXPR_STATIC UINT32 RENDER_PASS_STAGE_MASK_PIXEL		= (1u << 1u);
	PE_CONSTEXPR_STATIC UINT32 RENDER_PASS_STAGE_MASK_COMPUTE	= (1u << 2u);
	PE_CONSTEXPR_STATIC UINT32 RENDER_PASS_STAGE_MASK_GRAPHICS	= (RENDER_PASS_STAGE_MASK_VERTEX | RENDER_PASS_STAGE_MASK_PIXEL);

	struct FRenderPassDesc
	{
	public:
		// Identity. PassName is the key callers look a pass up by and the name a GPU marker
		// shows; ProgramName is the program descriptor under Programs/P_<Name>.json, or null
		// when the pass owns no program.
		ERenderPassType		Type			= ERenderPassType::RENDER_PASS_TYPE_BASE;
		const CHAR*			PassName		= nullptr;
		const CHAR*			ProgramName		= nullptr;
		BOOL32				bIsCompute		= FALSE;
		// FALSE for a pass the renderer runs as a full screen or engine driven pass: a
		// material is only assembled for a pass that runs per primitive geometry.
		BOOL32				bMaterialPass	= FALSE;
		UINT32				StageMask		= RENDER_PASS_STAGE_MASK_GRAPHICS;
		// Index into RSceneRenderer's fixed function pipeline state array, or
		// RENDER_PASS_INVALID_PIPELINE_SLOT when the state comes from a program instead.
		UINT32				PipelineStateSlot = RENDER_PASS_INVALID_PIPELINE_SLOT;
	public:
		// Graphics. A compute pass leaves these at their defaults.
		RRasterizerState	Rasterizer;
		RBlendState			Blend;
		RDepthState			Depth;
		RStencilState		Stencil;
		RPrimitiveTopologyType	Topology = RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		UINT32				RenderTargetCount = 0u;
		RFormatType			RenderTargetFormats[RHI_MAX_RENDER_TARGETS] = {};
		UINT32				DepthBits		= 0u;
		BOOL32				HasStencil		= FALSE;
		// TRUE when the pass covers its whole render target with one draw and therefore has no
		// vertex stream to bind.
		BOOL32				bIsFullScreen	= FALSE;
	public:
		// Compute. Mirrors SHADER_THREAD_GROUP_X / Y / Z of the program descriptor: the
		// compiler writes no numthreads into the reflection, so the dispatcher has to state
		// the same numbers the shader was compiled with.
		UINT32				ThreadGroupX	= 0u;
		UINT32				ThreadGroupY	= 0u;
		UINT32				ThreadGroupZ	= 1u;
	};

	class FRenderPassRegistry
	{
	public:
		PE_NODISCARD static UINT32	GetPassNum();
		PE_NODISCARD static const FRenderPassDesc&	GetDesc(const ERenderPassType InType);
		// Lookup by name for callers that carry a pass name rather than an enum value, such as
		// the material pass table. Null when no pass carries that name.
		PE_NODISCARD static const FRenderPassDesc*	FindDescByPassName(const EName& InPassName);
		// Fills the pass facing pipeline state description of a pass. The program is an
		// argument because the two cases differ: a pass that owns its shaders passes the one
		// the register names, a per primitive pass passes the material's.
		static void	FillPipelineStateDesc(const FRenderPassDesc& InPassDesc, const EShaderProgram* InProgram, RGraphicsPipelineStateDesc& OutDesc);
	private:
		// The table is built on first use rather than at static init time: its entries carry
		// engine types with constructors, and a function local static is what keeps that
		// ordering well defined.
		PE_NODISCARD static const FRenderPassDesc*	GetTable();
	};

} // namespace PigeonEngine
