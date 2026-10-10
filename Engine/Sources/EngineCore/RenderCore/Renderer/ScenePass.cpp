#include "ScenePass.h"

#include <ShaderAsset/ShaderProgram.h>

namespace PigeonEngine
{

	/*
	* The pass table. Every entry states what its pass runs with; the renderer builds its
	* fixed function pipeline states from it and a material pass is assembled per entry that
	* carries bMaterialPass.
	*
	* PipelineStateSlot values are RSceneRenderer::RPipelineStateType values. A pass that
	* builds its pipeline state from a program leaves the slot invalid on purpose, and the
	* renderer skips it when it fills the fixed function table.
	*/
	static const FRenderPassDesc* MakePassTable()
	{
		static const FRenderPassDesc PassTable[ERenderPassType::RENDER_PASS_TYPE_COUNT] =
		{
			// RENDER_PASS_TYPE_BASE - the deferred GBuffer pass of every mesh material.
			{
				ERenderPassType::RENDER_PASS_TYPE_BASE, "BasePass", "BasePass", FALSE, TRUE,
				RENDER_PASS_STAGE_MASK_GRAPHICS, 0u,
				RRasterizerState(RCullModeType::CULL_BACK, RFillModeType::FILL_SOLID),
				RBlendState(RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RColorWriteMaskType::COLOR_WRITE_MASK_ALL, FALSE),
				RDepthState(RComparisonFunctionType::COMPARISON_LESS_EQUAL, RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL, TRUE),
				RStencilState(0xffu, 0xffu,
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP), FALSE),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				7u,
				{ RFormatType::FORMAT_R16G16B16A16_FLOAT, RFormatType::FORMAT_R16G16B16A16_FLOAT,
					RFormatType::FORMAT_R16G16B16A16_UNORM, RFormatType::FORMAT_R8G8B8A8_UNORM,
					RFormatType::FORMAT_R16G16B16A16_UINT, RFormatType::FORMAT_R11G11B10_FLOAT,
					RFormatType::FORMAT_R8_UNORM },
				32u, FALSE, FALSE,
				0u, 0u, 1u
			},
			// RENDER_PASS_TYPE_BASE_GRASS - the grass blade pass: same targets, no culling.
			{
				ERenderPassType::RENDER_PASS_TYPE_BASE_GRASS, "BasePassGrass", "BezierGrass", FALSE, TRUE,
				RENDER_PASS_STAGE_MASK_GRAPHICS, 1u,
				RRasterizerState(RCullModeType::CULL_NONE, RFillModeType::FILL_SOLID),
				RBlendState(RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RColorWriteMaskType::COLOR_WRITE_MASK_ALL, FALSE),
				RDepthState(RComparisonFunctionType::COMPARISON_LESS_EQUAL, RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL, TRUE),
				RStencilState(0xffu, 0xffu,
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP), FALSE),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				7u,
				{ RFormatType::FORMAT_R16G16B16A16_FLOAT, RFormatType::FORMAT_R16G16B16A16_FLOAT,
					RFormatType::FORMAT_R16G16B16A16_UNORM, RFormatType::FORMAT_R8G8B8A8_UNORM,
					RFormatType::FORMAT_R16G16B16A16_UINT, RFormatType::FORMAT_R11G11B10_FLOAT,
					RFormatType::FORMAT_R8_UNORM },
				32u, FALSE, FALSE,
				0u, 0u, 1u
			},
			// RENDER_PASS_TYPE_BASE_GRASS_WIRE - the grass debugging raster.
			{
				ERenderPassType::RENDER_PASS_TYPE_BASE_GRASS_WIRE, "BasePassGrassWireframe", "BezierGrass", FALSE, TRUE,
				RENDER_PASS_STAGE_MASK_GRAPHICS, 2u,
				RRasterizerState(RCullModeType::CULL_NONE, RFillModeType::FILL_WIREFRAME),
				RBlendState(RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RColorWriteMaskType::COLOR_WRITE_MASK_ALL, FALSE),
				RDepthState(RComparisonFunctionType::COMPARISON_LESS_EQUAL, RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL, TRUE),
				RStencilState(0xffu, 0xffu,
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP), FALSE),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				7u,
				{ RFormatType::FORMAT_R16G16B16A16_FLOAT, RFormatType::FORMAT_R16G16B16A16_FLOAT,
					RFormatType::FORMAT_R16G16B16A16_UNORM, RFormatType::FORMAT_R8G8B8A8_UNORM,
					RFormatType::FORMAT_R16G16B16A16_UINT, RFormatType::FORMAT_R11G11B10_FLOAT,
					RFormatType::FORMAT_R8_UNORM },
				32u, FALSE, FALSE,
				0u, 0u, 1u
			},
			// RENDER_PASS_TYPE_DEPTH - depth only. No colour target, and the pipeline state is
			// built from the program, so the fixed function slot stays invalid.
			{
				ERenderPassType::RENDER_PASS_TYPE_DEPTH, "DepthOnly", "DepthOnly", FALSE, TRUE,
				RENDER_PASS_STAGE_MASK_GRAPHICS, RENDER_PASS_INVALID_PIPELINE_SLOT,
				RRasterizerState(RCullModeType::CULL_BACK, RFillModeType::FILL_SOLID),
				RBlendState(RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RColorWriteMaskType::COLOR_WRITE_MASK_ALL, FALSE),
				RDepthState(RComparisonFunctionType::COMPARISON_LESS_EQUAL, RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL, TRUE),
				RStencilState(0xffu, 0xffu,
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP), FALSE),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				0u, {}, 32u, FALSE, FALSE,
				0u, 0u, 1u
			},
			// RENDER_PASS_TYPE_SHADOW - reserved. Described so the table and the enum are
			// complete; nothing records it yet.
			{
				ERenderPassType::RENDER_PASS_TYPE_SHADOW, "ShadowDepth", "DepthOnly", FALSE, TRUE,
				RENDER_PASS_STAGE_MASK_GRAPHICS, RENDER_PASS_INVALID_PIPELINE_SLOT,
				RRasterizerState(RCullModeType::CULL_FRONT, RFillModeType::FILL_SOLID),
				RBlendState(RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RColorWriteMaskType::COLOR_WRITE_MASK_ALL, FALSE),
				RDepthState(RComparisonFunctionType::COMPARISON_LESS_EQUAL, RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL, TRUE),
				RStencilState(0xffu, 0xffu,
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP), FALSE),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				0u, {}, 32u, FALSE, FALSE,
				0u, 0u, 1u
			},
			// RENDER_PASS_TYPE_FORWARD - reserved for forward rendered geometry. Only its fixed
			// function state is used today, by the editor and debug primitive paths.
			{
				ERenderPassType::RENDER_PASS_TYPE_FORWARD, "Forward", nullptr, FALSE, FALSE,
				RENDER_PASS_STAGE_MASK_GRAPHICS, 5u,
				RRasterizerState(RCullModeType::CULL_BACK, RFillModeType::FILL_SOLID),
				RBlendState(RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RColorWriteMaskType::COLOR_WRITE_MASK_ALL, FALSE),
				RDepthState(RComparisonFunctionType::COMPARISON_LESS_EQUAL, RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL, TRUE),
				RStencilState(0xffu, 0xffu,
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP), FALSE),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				1u, { RFormatType::FORMAT_R16G16B16A16_FLOAT }, 32u, FALSE, FALSE,
				0u, 0u, 1u
			},
			// RENDER_PASS_TYPE_SKY - the sky, drawn as a full screen quad. No culling: the quad
			// covers the viewport, so there is no winding for it to be right or wrong about, and
			// the vertex stage places it on the far plane so the depth test keeps it behind the
			// geometry. It draws into the base pass's MRT, which is still bound when the sky
			// runs, so it declares the same six targets: the pipeline state a per primitive pass
			// builds has to match the render targets the command list has bound.
			{
				ERenderPassType::RENDER_PASS_TYPE_SKY, "Sky", "Sky", FALSE, FALSE,
				RENDER_PASS_STAGE_MASK_GRAPHICS, 4u,
				RRasterizerState(RCullModeType::CULL_NONE, RFillModeType::FILL_SOLID),
				RBlendState(RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RColorWriteMaskType::COLOR_WRITE_MASK_ALL, FALSE),
				RDepthState(RComparisonFunctionType::COMPARISON_LESS_EQUAL, RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL, TRUE),
				RStencilState(0xffu, 0xffu,
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP), FALSE),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				7u,
				{ RFormatType::FORMAT_R16G16B16A16_FLOAT, RFormatType::FORMAT_R16G16B16A16_FLOAT,
					RFormatType::FORMAT_R16G16B16A16_UNORM, RFormatType::FORMAT_R8G8B8A8_UNORM,
					RFormatType::FORMAT_R16G16B16A16_UINT, RFormatType::FORMAT_R11G11B10_FLOAT,
					RFormatType::FORMAT_R8_UNORM },
				32u, FALSE, FALSE,
				0u, 0u, 1u
			},
			// RENDER_PASS_TYPE_LIGHTING - full screen additive lighting over the GBuffer.
			// Additive on the colour channel (ONE / ONE), because the pass writes the lighting
			// term on top of the base pass colour the target already holds; the alpha channel
			// keeps its destination value, which is where the scene colour reserves its flags.
			// No culling: the pass is one full screen quad, which has no outside to cull, and
			// its winding follows the vertex factory's clip space mapping rather than anything
			// the pass states - a cull mode here would silently drop the whole pass the day that
			// mapping changes.
			{
				ERenderPassType::RENDER_PASS_TYPE_LIGHTING, "SceneLighting", "SceneLighting", FALSE, FALSE,
				RENDER_PASS_STAGE_MASK_GRAPHICS, 3u,
				RRasterizerState(RCullModeType::CULL_NONE, RFillModeType::FILL_SOLID),
				RBlendState(RBlendOptionType::BLEND_ONE, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RColorWriteMaskType::COLOR_WRITE_MASK_ALL, TRUE),
				RDepthState(RComparisonFunctionType::COMPARISON_ALWAYS, RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL, FALSE),
				RStencilState(0xffu, 0xffu,
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP), FALSE),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				1u, { RFormatType::FORMAT_R16G16B16A16_FLOAT }, 0u, FALSE, TRUE,
				0u, 0u, 1u
			},
			// RENDER_PASS_TYPE_FINAL_OUTPUT - full screen blit of the scene colour to the back buffer.
			// No culling for the same reason the lighting pass states none: the blit is one full
			// screen quad, whose winding belongs to the vertex factory rather than to the pass.
			{
				ERenderPassType::RENDER_PASS_TYPE_FINAL_OUTPUT, "FinalOutput", "Copy", FALSE, FALSE,
				RENDER_PASS_STAGE_MASK_GRAPHICS, 6u,
				RRasterizerState(RCullModeType::CULL_NONE, RFillModeType::FILL_SOLID),
				RBlendState(RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
					RColorWriteMaskType::COLOR_WRITE_MASK_ALL, FALSE),
				RDepthState(RComparisonFunctionType::COMPARISON_ALWAYS, RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL, FALSE),
				RStencilState(0xffu, 0xffu,
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP), FALSE),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				1u, { RFormatType::FORMAT_R8G8B8A8_UNORM }, 0u, FALSE, TRUE,
				0u, 0u, 1u
			},
			// RENDER_PASS_TYPE_IMGUI - the editor overlay. Its pipeline state carries the
			// program, so the fixed function slot stays invalid; the renderer describes the
			// depth behaviour and the editor renderer builds the state.
			{
				ERenderPassType::RENDER_PASS_TYPE_IMGUI, "ImGui", "ImGui", FALSE, FALSE,
				RENDER_PASS_STAGE_MASK_GRAPHICS, RENDER_PASS_INVALID_PIPELINE_SLOT,
				RRasterizerState(RCullModeType::CULL_BACK, RFillModeType::FILL_SOLID),
				RBlendState(RBlendOptionType::BLEND_SRC_ALPHA, RBlendOptionType::BLEND_INV_SRC_ALPHA, RBlendOperationType::BLEND_OP_ADD,
					RBlendOptionType::BLEND_ONE, RBlendOptionType::BLEND_INV_SRC_ALPHA, RBlendOperationType::BLEND_OP_ADD,
					RColorWriteMaskType::COLOR_WRITE_MASK_ALL, TRUE),
				RDepthState(RComparisonFunctionType::COMPARISON_ALWAYS, RDepthWriteMaskType::DEPTH_WRITE_MASK_ZERO, FALSE),
				RStencilState(0xffu, 0xffu,
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
					RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP), FALSE),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				1u, { RFormatType::FORMAT_R8G8B8A8_UNORM }, 0u, FALSE, FALSE,
				0u, 0u, 1u
			},
			// RENDER_PASS_TYPE_GRASS_CONSTRUCT - the grass instance construction compute pass.
			{
				ERenderPassType::RENDER_PASS_TYPE_GRASS_CONSTRUCT, "GrassConstruct", "BezierGrassConstruct",
				TRUE, FALSE, RENDER_PASS_STAGE_MASK_COMPUTE, RENDER_PASS_INVALID_PIPELINE_SLOT,
				RRasterizerState(), RBlendState(), RDepthState(), RStencilState(),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_UNDEFINED,
				0u, {}, 0u, FALSE, FALSE,
				8u, 8u, 1u
			},
			// RENDER_PASS_TYPE_SWE_INTEGRATE - reserved, the shallow water integration passes.
			{
				ERenderPassType::RENDER_PASS_TYPE_SWE_INTEGRATE, "SweIntegrate", "SWEIntegrateHeight",
				TRUE, FALSE, RENDER_PASS_STAGE_MASK_COMPUTE, RENDER_PASS_INVALID_PIPELINE_SLOT,
				RRasterizerState(), RBlendState(), RDepthState(), RStencilState(),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_UNDEFINED,
				0u, {}, 0u, FALSE, FALSE,
				8u, 8u, 1u
			},
			// RENDER_PASS_TYPE_SWE_MODIFIER - reserved, the shallow water modifier passes.
			{
				ERenderPassType::RENDER_PASS_TYPE_SWE_MODIFIER, "SweModifier", "SWEModifier",
				TRUE, FALSE, RENDER_PASS_STAGE_MASK_COMPUTE, RENDER_PASS_INVALID_PIPELINE_SLOT,
				RRasterizerState(), RBlendState(), RDepthState(), RStencilState(),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_UNDEFINED,
				0u, {}, 0u, FALSE, FALSE,
				8u, 8u, 1u
			},
			// RENDER_PASS_TYPE_COLOR_GRADE - the full screen colour grading compute pass.
			{
				ERenderPassType::RENDER_PASS_TYPE_COLOR_GRADE, "ColorGrade", "ColorGrade",
				TRUE, FALSE, RENDER_PASS_STAGE_MASK_COMPUTE, RENDER_PASS_INVALID_PIPELINE_SLOT,
				RRasterizerState(), RBlendState(), RDepthState(), RStencilState(),
				RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_UNDEFINED,
				0u, {}, 0u, FALSE, FALSE,
				8u, 8u, 1u
			},
		};
		return PassTable;
	}

	UINT32 FRenderPassRegistry::GetPassNum()
	{
		return static_cast<UINT32>(ERenderPassType::RENDER_PASS_TYPE_COUNT);
	}

	const FRenderPassDesc& FRenderPassRegistry::GetDesc(const ERenderPassType InType)
	{
		const FRenderPassDesc* Table = GetTable();
		const UINT32 Index = static_cast<UINT32>(InType);
		if (Index >= GetPassNum())
		{
			// Only reachable with a value cast in from outside the enum. Answering with the
			// first entry keeps a caller's bind from reading past the table.
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Render pass type is out of range."));
			return Table[0u];
		}
		return Table[Index];
	}

	const FRenderPassDesc* FRenderPassRegistry::FindDescByPassName(const EName& InPassName)
	{
		const FRenderPassDesc* Table = GetTable();
		for (UINT32 Index = 0u, Num = GetPassNum(); Index < Num; Index++)
		{
			if ((Table[Index].PassName != nullptr) && (EName(Table[Index].PassName) == InPassName))
			{
				return &(Table[Index]);
			}
		}
		return nullptr;
	}

	void FRenderPassRegistry::FillPipelineStateDesc(const FRenderPassDesc& InPassDesc, const EShaderProgram* InProgram, RGraphicsPipelineStateDesc& OutDesc)
	{
		OutDesc.Program				= InProgram;
		OutDesc.Rasterizer			= InPassDesc.Rasterizer;
		OutDesc.Blend				= InPassDesc.Blend;
		OutDesc.Depth				= InPassDesc.Depth;
		OutDesc.Stencil				= InPassDesc.Stencil;
		OutDesc.Topology			= InPassDesc.Topology;
		OutDesc.RenderTargetCount	= InPassDesc.RenderTargetCount;
		for (UINT32 Index = 0u; Index < RHI_MAX_RENDER_TARGETS; Index++)
		{
			OutDesc.RenderTargetFormats[Index] = InPassDesc.RenderTargetFormats[Index];
		}
		OutDesc.DepthBits			= InPassDesc.DepthBits;
		OutDesc.HasStencil			= InPassDesc.HasStencil;
	}

	const FRenderPassDesc* FRenderPassRegistry::GetTable()
	{
		return MakePassTable();
	}

} // namespace PigeonEngine
