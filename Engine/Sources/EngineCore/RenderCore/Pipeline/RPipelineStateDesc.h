#pragma once

#include <CoreMinimal.h>
#include <RenderCommon.h>
#include <RHI/RHITypes.h>

namespace PigeonEngine
{

	class EShaderProgram;

	/*
	* RenderCore/Pipeline - the data a pass fills in to get a pipeline state.
	*
	* A pass describes what it wants to draw with; it never names a root signature, a root
	* index or a register. Both come from the shader program's own reflection through the
	* layout generator (see RPipelineLayout.h), so a pass that swaps its program in cannot
	* leave a stale binding behind.
	*
	* The fixed-function half mirrors RRHIGraphicsPipelineDesc on purpose: this desc is the
	* pass-facing one, and RPipelineStateCache is the single place that turns it into the
	* back-end desc plus the root signature the program's layout resolved to.
	*/
	struct RGraphicsPipelineStateDesc
	{
	public:
		// Stage bits the pipeline builds shaders from. They mirror RENDER_PASS_STAGE_MASK_* in
		// Renderer/ScenePass.h and ENGINE_SHADER_STAGE_MASK_* in Shader/ShaderAssetBinding.h, which
		// use the same order (vertex, pixel, compute). A pass that draws geometry but needs no pixel
		// stage - a depth only shadow pass - clears the pixel bit and the pipeline is built without
		// a pixel shader, which is what lets the rasterizer skip that stage entirely.
		static constexpr UINT32	STAGE_MASK_VERTEX		= (1u << 0u);
		static constexpr UINT32	STAGE_MASK_PIXEL		= (1u << 1u);
		static constexpr UINT32	STAGE_MASK_GRAPHICS		= (STAGE_MASK_VERTEX | STAGE_MASK_PIXEL);
	public:
		// Program the pipeline runs. Required: it carries the shaders, the input layout and
		// the parameter layout the root signature is generated from.
		const EShaderProgram*	Program					= nullptr;
		UINT32					StageMask				= STAGE_MASK_GRAPHICS;
		RRasterizerState		Rasterizer{};
		RBlendDesc				Blend{};
		RDepthState				Depth{};
		RStencilState			Stencil{};
		RPrimitiveTopologyType	Topology				= RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		UINT32					RenderTargetCount		= 0u;
		RFormatType				RenderTargetFormats[RHI_MAX_RENDER_TARGETS]{};
		// DSV width in bits, 0 meaning "no depth-stencil view". Same convention as the
		// resource side, so a pass can pass a texture's own depth bits straight through.
		UINT32					DepthBits				= 0u;
		BOOL32					HasStencil				= FALSE;
		UINT32					SampleCount				= 1u;
		UINT32					SampleQuality			= 0u;
		// Optional declaration of the vertex data this pipeline will be bound to. Null / 0 uses the
		// program's own layout, which is the engine's default and matches how the meshes are packed
		// (every attribute in its own stream slot, all of them 32 bit float).
		//
		// These are DATA descriptions, not a copy of the shader's wish list: a packed color stream is
		// declared as (scalar 1 byte, 4 components, UNORM) here even though the shader reads a float4.
		// Only the elements whose real layout differs have to be named - an element this array does
		// not mention keeps the one the shader declares.
		//
		// A declaration describes the bytes inside a vertex, not where the vertex comes from: the
		// stream slot and the per-vertex / per-instance classification always come from the shader,
		// because that is what the draw's SetVertexBuffers calls bind by. A declared
		// AlignedByteOffset therefore has to agree with the stride / offset the caller passes to
		// SetVertexBuffers for that slot.
		//
		// The cache pairs each declared element with the shader input it feeds, reports the ones it
		// cannot honor (a scalar class the API would not accept, a format no API format exists for)
		// and refuses to create the pipeline for those; see Pipeline/RInputLayoutCompatibility.h.
		const RInputLayoutDesc*	InputLayouts			= nullptr;
		UINT32					InputLayoutCount		= 0u;

	public:
		RGraphicsPipelineStateDesc() noexcept
			: Program{ nullptr }
			, StageMask{ STAGE_MASK_GRAPHICS }
			, Rasterizer{}
			, Blend{}
			, Depth{}
			, Stencil{}
			, Topology{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST }
			, RenderTargetCount{ 0u }
			, RenderTargetFormats{}
			, DepthBits{ 0u }
			, HasStencil{ FALSE }
			, SampleCount{ 1u }
			, SampleQuality{ 0u }
			, InputLayouts{ nullptr }
			, InputLayoutCount{ 0u }
		{
			for (UINT32 i = 0u; i < RHI_MAX_RENDER_TARGETS; i++)
			{
				RenderTargetFormats[i] = RFormatType::FORMAT_UNKNOWN;
			}
		}
	};

};	// PigeonEngine
