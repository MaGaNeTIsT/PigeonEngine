#pragma once

#include <CoreMinimal.h>
#include "RenderScene.h"
#include <RenderResource.h>
#include <RHI/IRRHIDevice.h>
#include <Pipeline/RPipelineStateCache.h>
#include <Shader/ShaderEngineParameters.h>
#include <Shader/ShaderParameter.h>
#include <RenderProxy/RenderSingletonObject.h>
#include <RenderProxy/LightSceneProxy.h>

namespace PigeonEngine
{

	class EShaderProgram;
	class EShaderInstance;
	class RViewProxy;
	class IRCommandList;

	class RSceneRenderer : public EManagerBase
	{
	public:
		using RVisibilityMapType		= TMap<ObjectIdentityType, BOOL32>;
		using RViewSceneTextureType		= TMap<ObjectIdentityType, RSceneTextures*>;
		using RShadowMapType			= TMap<ObjectIdentityType, RShadowTexture*>;
		using RViewShadowMapType		= TMap<ObjectIdentityType, RShadowMapType>;
	public:
		/*
		* The two gpu light lists, the cpu staging they are filled from, and the counts that
		* go with them.
		*
		* One set for the frame rather than one per view: a light list is a global fact, and
		* what a view owns is a cascade's projection - which is exactly why a light element
		* carries a shadow map index and no matrix. Two views therefore share the lists, and
		* the counts published in GlobalParams address the same buffers for both.
		*
		* The buffers are sized once at the maximum and only the used prefix is uploaded.
		* Sizing to fit each frame would mean recreating the resource, and with it the shader
		* resource view that was built at creation, every time a light was added or removed.
		*/
		class RLightLists
		{
		public:
			// Builds both buffers at their maximum capacity. Idempotent: a second call after
			// the device is up is what a device reset needs, and it is a no-op otherwise.
			void	EnsureBuffers(IRRHIDevice* InRenderDevice);
			// Pushes the used prefix of both staging arrays. The counts are what the shading
			// loop reads, so a shorter upload is a shorter loop rather than a wrong one.
			void	Upload(IRRHIDevice* InRenderDevice);
		public:
			RStructuredBuffer				DirectionalLightBuffer;
			RStructuredBuffer				LocalLightBuffer;
			TArray<EDirectionalLightsGPU>	DirectionalLightStaging;
			TArray<ELocalLightsGPU>			LocalLightStaging;
			UINT32							DirectionalLightCount = 0u;
			UINT32							LocalLightCount = 0u;
		};
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
	public:
		RScene*			GetRenderScene();
		void			InitNewFrame();
		void			Render();
		void			ResizeRenderTargets(UINT32 InWidth, UINT32 InHeight);
		/*
		* Render thread: a new asset object was published for InShaderName, so everything derived
		* from the previous one is stale. Called from a command the load thread recorded right after
		* the swap, and it runs before the frame that will use the new object - which is what makes
		* this thread the one that drops what it owns: pipeline states, the engine pass states, the
		* parameter blocks that latched a layout, and the programs the modules below it cached.
		*
		* Dropping is all it does. Every one of those is rebuilt from the current asset by its next
		* use, so there is nothing to restore here.
		*/
		void			OnShaderAssetReplaced(const EName& InShaderName);
	public:
		// The pipeline states of the frame. A proxy asks for one instead of creating its
		// own: the cache is what pairs a program's reflection generated root signature with
		// the pipeline state, and a pipeline created outside it would carry the wrong roots.
		PE_NODISCARD RPipelineStateCache&	GetPipelineStateCache() { return PipelineStateCache; }
		// The view the frame is currently recording. ViewParams belongs to the view, not to
		// the primitive, so a primitive draw publishes and binds it through this handle.
		PE_NODISCARD RViewProxy*			GetCurrentViewProxy() const { return CurrentViewProxy; }
	public:
		// Builds the GlobalParams mirrors from the first program that declares the block.
		BOOL32	EnsureGlobalParameterBlock(IRRHIDevice* InRenderDevice, const EShaderProgram& InProgram);
		// Refreshes the per frame values and pushes them into the gpu buffers. A no-op until
		// the block has a layout. The light counts are published here because they are what
		// addresses the two light lists, and the lists are a frame wide fact: every view binds
		// the same buffers and reads the same counts.
		void	UpdateGlobalParameters();
		// Binds the per frame block at the root the program's plan reports.
		void	BindGlobalParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram) const;
		// Binds the blocks every draw shares - the per frame block and the current view's block -
		// at the roots the given program's plan reports. Callers run it after their own pipeline
		// state on purpose: switching a signature voids every root binding, so a pass that bound
		// its blocks before its state would lose them.
		void	BindEngineParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram);
	protected:
		void			InitViews();
		// Builds the pipeline states of the passes the renderer drives itself (lighting and the
		// final blit). It is called once per frame and returns as soon as every state exists: a
		// pass program only has device objects after the render thread has run the creation
		// commands its asset load queued, and the first call happens before that thread exists,
		// so a one shot build would latch those passes as permanently unbuildable.
		void			EnsureEnginePassPipelineStates();
		void			BasePass();
		void			FinalOutputPass();
#if _EDITOR_ONLY
		void			ImGuiPass();
#endif
	protected:
		void			RenderBasePass(RSceneTextures* InSceneTextures);
		void			RenderLighting(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures);
		void			RenderSky(const RViewProxy* InViewProxy);
		void			RenderForward(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures);
	protected:
		// Puts a texture into a state a pixel shader may sample, unless it already is in one.
		// A resource a previous pass wrote as a render target has to be transitioned before a
		// later pass binds its view, or the two permissions would be asked for at once.
		void			TransitionToShaderReadable(IRRHITexture* InTexture);
		// Depth buffer counterpart: sampling a depth buffer is a shader read on top of the
		// depth read permission, which is what the combined state says.
		void			TransitionDepthToShaderReadable(IRRHITexture* InTexture);
	protected:
		// Gathers the frame's lights, fills the two gpu lists and publishes their counts. Frame
		// level and not per view: the lists are shared, and a count is only meaningful together
		// with the list it addresses, so both are produced by this one call.
		void			InitLights();
		// The per view half of what used to be one function: each shadow domain of each light
		// gets its own culling pass, because a cascade's frustum belongs to the view whose
		// frustum it was derived from.
		void			InitDirectionalLightShadowDomains(RViewProxy* InViewProxy);
		void			ProcessOcclusionCull(RViewProxy* InViewProxy);
		void			InitRenderPasses(RViewProxy* InViewProxy);
		// The proxy of one view, by identity. A view is reached through its identity rather than
		// an index into the list, because that list belongs to the scene and is not promised to
		// keep its order between frames.
		RViewProxy*		FindViewProxy(const ObjectIdentityType InViewIdentity) noexcept;
	protected:
		// Fills the gpu lists from the scene's light proxies and returns nothing: the counts it
		// produces are the ones the frame publishes. Kept separate from InitLights so the
		// gather and the publish can be read apart.
		void			GatherDirectionalLights();
		void			GatherLocalLights();
	protected:
		// Projects the sky's environment cube into the SH buffer, with a compute dispatch. Runs in
		// the frame the cube is first seen and in any frame it changes - and only then, because
		// what the coefficients hold is the environment's shape and not its brightness, so a
		// change to the sky's luminance or tint does not come through here.
		//
		// Recorded before the base pass, which is where the lighting pass that reads the
		// coefficients runs, so a bake lands in the same frame it is asked for.
		void			RenderEnvironmentSH();
		// Resolves the bake's pipeline state from its program on first use. FALSE while either is
		// missing, which is the state the frames before the shader device objects exist are in.
		BOOL32			EnsureEnvironmentSHPipelineState();
		// The sky light the frame's environment comes from. One environment per frame: the
		// coefficients are baked once and every view shades against them, so a second sky light
		// would have nowhere to put its own. Null while the scene has no sky with a usable cube.
		RSkyLightSceneProxy*	FindPrimarySkyLight() noexcept;
		// Refreshes the environment's brightness and tint and binds them where the pass that shades
		// reads them, at the root that program's plan reports. Run after the pass's pipeline state,
		// for the same reason every other block is: switching a signature voids root bindings.
		//
		// These two values are why the bake does not rerun when the sky's brightness changes - the
		// coefficients hold the environment's shape and the scale is applied here - and they are the
		// same pair the sky pass multiplies its cube by.
		void			BindIndirectLightingParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram);
	protected:
		void			InitShadowTextures(RViewProxy* InViewProxy);
		void			OctreeCull(const EFrustum& InViewFrustum, RVisibilityMapType& InOutVisibilityMap);
		void			PrimitiveCull(const EFrustum& InViewFrustum, RVisibilityMapType& InOutVisibilityMap);
	protected:
		// Debug only: compares the C++ element stride of each light list against the field list
		// the shader reflected, which is the only thing that can catch a layout that drifted
		// between Assets/ShaderSources/Components/C_Lighting.json and LightSceneProxy.h. A
		// mismatch is a silent wrong shading result rather than a failure, which is why the
		// check exists at all.
		void			ValidateLightBufferLayout();
	protected:
		void			InitRendererSettings();
		BOOL32			IsNeedStencil()const;
	public:
		// The renderer wide sampler table, in the register order the shaders declare it:
		// s0..s3 are the system layer's samplers (Components/C_EngineSystem.json) and s4..s6 the
		// generated material sampler group (SamplerMaterial2D / Cube / 3D, stated by
		// EShaderJobBuilder::BuildGeneratedComponents). Every program compiles against this one
		// table, so an entry's position here *is* its register, and a sampler a shader declares
		// by one of these names samples through exactly this state.
		enum RSamplerType : UINT8
		{
			SAMPLER_TYPE_LINEAR_WRAP	= 0,	// s0, SamplerLinearWrap
			SAMPLER_TYPE_LINEAR_CLAMP,			// s1, SamplerLinearClamp
			SAMPLER_TYPE_POINT_CLAMP,			// s2, SamplerPointClamp
			SAMPLER_TYPE_SHADOW,				// s3, SamplerShadow
			SAMPLER_TYPE_MATERIAL_2D,			// s4, SamplerMaterial2D
			SAMPLER_TYPE_MATERIAL_CUBE,			// s5, SamplerMaterialCube
			SAMPLER_TYPE_MATERIAL_3D,			// s6, SamplerMaterial3D
			SAMPLER_TYPE_COUNT
		};
		// Pre-built pipeline states. Each one bundles (raster + blend + depth-
		// stencil + render-target formats + topology) for a specific render
		// pass. PSOs marked "(env)" leave VS/PS empty; the mesh proxy that
		// runs in that pass binds its own shaders. PSOs marked "(full)"
		// include shaders too because the pass is fixed-function (full-screen
		// lighting / final blit).
		enum RPipelineStateType : UINT8
		{
			PIPELINE_STATE_BASE_PASS_MRT			= 0,	// (env) Static / skeletal mesh into 4x GBuffer + DSV
			PIPELINE_STATE_BASE_PASS_GRASS_SOLID,			// (env) BezierGrass solid (cull NONE)
			PIPELINE_STATE_BASE_PASS_GRASS_WIREFRAME,		// (env) BezierGrass wireframe
			PIPELINE_STATE_LIGHTING,						// (full) Full-screen lighting
			PIPELINE_STATE_SKY,								// (env) Sky (cull FRONT, depth LESS_EQUAL)
			PIPELINE_STATE_FORWARD,							// (env) Forward / debug primitives
			PIPELINE_STATE_FINAL_OUTPUT,					// (full) Full-screen blit to back buffer
			PIPELINE_STATE_COUNT
		};
	protected:
		RScene*						Scene;
		RFullScreenTriangle			FullScreenTriangle;
#if _EDITOR_ONLY
		RDebugWireframePrimitiveManager* DebugWireframePrimitiveManager;
#endif
	protected:
		RSamplerResource			Samplers[RSamplerType::SAMPLER_TYPE_COUNT];
		IRRHIPipelineState*			PipelineStates[RPipelineStateType::PIPELINE_STATE_COUNT];
		// Active command list for the frame currently being recorded.
		// Acquired in Render() and submitted at the end; sub-passes share it.
		IRCommandList*				CurrentCommandList;
		// Pipeline states of the frame, created on demand from a program and its layout.
		RPipelineStateCache			PipelineStateCache;
		// TRUE once every engine driven pass state exists, so the per frame retry in
		// EnsureEnginePassPipelineStates costs one branch. See its comment for why a retry is
		// needed at all.
		BOOL32						bEnginePassStatesReady = FALSE;
		// The GlobalParams block, published once per frame and read by every pass. Named at
		// construction: the name is the block's identity, so there is no unnamed state for a
		// later edit to leave it in (an unnamed block never initializes, uploads or binds).
		RShaderParameterBlock		GlobalParameterBlock{ EName(SHADER_BLOCK_GLOBAL_PARAMS) };
		// The view the frame is recording, set by BasePass for the duration of a view.
		RViewProxy*					CurrentViewProxy;
		// The engine shader programs the fixed-function passes run with. One program
		// carries both stages, so the full screen blit and the deferred lighting pass
		// share M_EngineFullScreenTriangle's vertex shader.
		//
		// The instances are kept next to the programs on purpose: an instance is the stable handle
		// across a shader reload (the process wide cache re-points it), so re-reading its program is
		// how a replaced shader reaches these, and reading is all the refresh may do - it runs on
		// the render thread, where a resolution that missed the cache would load.
		const EShaderInstance*		FullScreenProgramInstance = nullptr;
		const EShaderInstance*		SceneLightingProgramInstance = nullptr;
		const EShaderInstance*		EnvironmentSHProgramInstance = nullptr;
		const EShaderProgram*		FullScreenProgram;
		const EShaderProgram*		SceneLightingProgram;
	protected:
		ObjectIdentityType			FinalOutputView;
		BOOL32						NeedStencil;
		RViewSceneTextureType		ViewSceneTextures;
		RViewShadowMapType			ViewShadowMaps;
		// The frame's two light lists. Global by design: a light is not a property of a view.
		RLightLists					LightLists;
	protected:
		// The environment's diffuse representation: nine SH radiance coefficients the bake writes
		// with a compute dispatch and the lighting pass reads as irradiance. One buffer for the
		// frame, like the light lists, and for the same reason - the environment is one field that
		// every view is inside of.
		RStructuredBuffer		EnvironmentSHBuffer;
		IRRHIPipelineState*		EnvironmentSHPipelineState;
		const EShaderProgram*	EnvironmentSHProgram;
		// The cube the buffer currently holds coefficients of, compared by texture identity. That
		// one comparison is what makes "the sky light was recreated", "the asset was swapped" and
		// "nothing moved" a single test, and it is what keeps the dispatch off the frame path in
		// the frames where nothing did.
		const IRRHITexture*		EnvironmentSHBakedSource;
		// The environment's brightness and tint, for the pass that shades. A block of the
		// renderer's own rather than the sky's: the sky publishes its pair for its own draw, and
		// this one travels with the coefficients the renderer bakes and binds.
		RShaderParameterBlock	IndirectLightingParameterBlock{ EName(SHADER_BLOCK_INDIRECT_LIGHTING_PARAMS) };
		// Whether the pair above has been reported once. It answers "what is the indirect term
		// multiplied by" for a session where the environment looks black, which is otherwise the
		// hardest kind of question to answer from a picture.
		BOOL32					bIndirectLightingParamsReported;
	public:
		RSceneRenderer();
		virtual ~RSceneRenderer();
	public:
		RSceneRenderer(const RSceneRenderer&) = delete;
		RSceneRenderer& operator=(const RSceneRenderer&) = delete;
	};

};
