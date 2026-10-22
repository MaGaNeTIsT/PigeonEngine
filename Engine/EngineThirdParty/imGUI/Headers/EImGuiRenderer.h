#pragma once

#include <CoreMinimal.h>
#include <RHI/RHIResources.h>
#include <Pipeline/RPipelineStateCache.h>
#include <Shader/ShaderEngineParameters.h>

#if _EDITOR_ONLY

struct ImDrawData;
struct ImTextureData;

namespace PigeonEngine
{

	class IRRHIDevice;
	class IRCommandList;
	class IRRHIPipelineState;
	class EShaderInstance;

	/*
	* EImGuiRenderer
	*
	* Backend-agnostic ImGui renderer. Owns the M_EngineImGui shader instance's PSO,
	* the dynamic per-frame VB/IB, the font atlas texture (created on first
	* Render via the RendererHasTextures WantCreate path), and drives the
	* ImDrawData replay through IRCommandList. Replaces the previous D3D11-
	* only imGUIManager::D3DRenderDrawData path.
	*
	* The shader is an assembly of the new shader system: the ImGui program, out of the
	* M_EngineImGui material and the UI's own vertex factory (Assets/ShaderSources/
	* ImGui/). Two things live here because a shader source cannot state them: the
	* pipeline's input layout -- the UI colour is packed into four bytes per vertex and
	* a factory JSON has no normalized eight bit format to say so -- and the pass's own
	* two blocks, its view projection and the identity draw transform. Those blocks are
	* published by this renderer exactly like a view proxy publishes the view block: the
	* UI reads the engine's ViewParams / DrawParams names, but the values are the pass's,
	* so the pass owns the buffers and binds them into its own pipeline state. The
	* pipeline state comes from this renderer's own cache, which pairs the declared
	* vertex data with the shader's input layout and generates the program's parameter
	* layout.
	*
	* Lifecycle:
	*   Initialize()  on first frame (lazy - depends on MainManager device).
	*   Render(cmd, ImGui::GetDrawData()) per frame, after BeginRenderPass.
	*   ShutDown()    teardown (release bindless slot + heap-owned resources).
	*
	* Threading: every method runs on the RenderThread. No internal locks.
	*/
	class EImGuiRenderer
	{
	public:
		EImGuiRenderer();
		~EImGuiRenderer();

	public:
		BOOL8	Initialize();
		void	ShutDown();
		BOOL8	IsInitialized()const { return bInitialized; }

		// Replays ImDrawData via the supplied command list. Caller is
		// responsible for BeginRenderPass / EndRenderPass.
		void	Render(IRCommandList* InCmd, ImDrawData* InDrawData);

		// Bindless index that maps to the font atlas SRV. Used by
		// EngineTextureToImgui (and any other code) as the default
		// "ImGui-side texture id".
		UINT32	GetFontBindlessIndex()const { return FontBindlessSRVIndex; }

		// Engine shutdown: releases the session scoped device objects - the font atlas texture and the
		// bindless index it was registered under - and tells ImGui the texture is gone, which is what
		// makes its re-create path work. It must not run while a recorded frame is still to be
		// replayed (see the comment on ShutDown), which is why a shader replacement does not call it.
		void	ReleaseFontTexture();

	public:
		EImGuiRenderer(const EImGuiRenderer&) = delete;
		EImGuiRenderer& operator=(const EImGuiRenderer&) = delete;

	private:
		BOOL8	LoadImGuiMaterial();
		// Drops this renderer's hold on the instance. Nothing is released: the instance belongs to the
		// process wide instance cache, which keeps it alive and re-points it when the shader asset is
		// rebuilt. What this renderer owns is what it derived from the instance's program - the two
		// blocks and the pipeline state - and its own teardown releases those.
		void	ReleaseImGuiMaterialInstance();
		void	SetIdentityDrawParams();
		BOOL8	BuildPipelineState();
		void	UpdateTexture(ImTextureData* Tex);
		BOOL8	EnsureBuffers(UINT32 InVertexNum, UINT32 InIndexNum);
		void	ReleaseDynamicBuffers();
		void	UpdateViewParams(ImDrawData* InDrawData);

	private:
		BOOL8							bInitialized			= FALSE;
		// The ImGui shader instance. Non-owning: it comes from the process wide instance cache, which
		// is also what makes a shader reload reach this renderer - the cache re-points the instance to
		// the rebuilt asset, and this renderer's next program read follows it.
		const EShaderInstance*			ImGuiMaterialInstance	= nullptr;
		// The pass's pipeline states. The cache owns what it creates -- it builds them from
		// a program plus the vertex data this renderer declares and releases them through
		// the device -- so the pointer below is non-owning and stays valid until the cache
		// is invalidated.
		RPipelineStateCache				Pipelines;
		IRRHIPipelineState*				PipelineState			= nullptr;

		// Dynamic VB/IB - recreated when ImDrawData outgrows them. Old
		// buffer goes through Device->DestroyResource so the deferred-delete
		// queue holds it until its fence retires.
		RRHIVertexBuffer				DynamicVB;
		RRHIIndexBuffer					DynamicIB;
		UINT32							VBVertexCapacity		= 0u;
		UINT32							IBIndexCapacity			= 0u;

		// Font atlas - kept as a raw RRHITexture2D wrapper (no on-disk asset
		// wrapping needed; the atlas is rebuilt from ImGui IO each session).
		RRHITexture2D					FontTextureResource;
		UINT32							FontBindlessSRVIndex	= 0xFFFFFFFFu;	// RHI_INVALID_BINDLESS_INDEX

		// Register the pixel stage reads the font atlas from, taken from that stage's
		// binding plan at load time. The plan is the authority on where a material
		// texture lands, so no slot is computed, or written down, here.
		UINT32							FontTextureSlot			= 0u;

		// Tracks the last bindless index written into the CB so we skip the
		// upload+rebind ping-pong when consecutive ImDrawCmds share a
		// texture (the common case - most UI uses the font atlas).
		UINT32							LastBoundTextureIndex	= 0xFFFFFFFFu;

		// The pass's own two blocks. Named with the engine's block names because that is what
		// the shader source declares, but owned by this renderer: it is the publisher of the UI's
		// projection and transform, so the values live in buffers it binds itself. The shader
		// instance's set is left to the material - a material block declared here would be bound
		// through the instance, and the render tier's blocks are bound through their own objects.
		RShaderParameterBlock			ViewBlock{ EName(SHADER_BLOCK_VIEW_PARAMS) };
		RShaderParameterBlock			DrawBlock{ EName(SHADER_BLOCK_DRAW_PARAMS) };
	};

};

#endif	// _EDITOR_ONLY
