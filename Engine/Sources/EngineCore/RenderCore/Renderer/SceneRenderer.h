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
	class RLocalLightSceneProxy;
	class RViewShadowState;
	class IRCommandList;

	class RSceneRenderer : public EManagerBase
	{
	public:
		using RVisibilityMapType		= TMap<ObjectIdentityType, BOOL32>;
		using RViewSceneTextureType		= TMap<ObjectIdentityType, RSceneTextures*>;
		/*
		* Where a light's shadow views landed in one view's slot table.
		*
		* Keyed by the light proxy's identity because that is what the light list is built from,
		* and a range rather than a count because a directional light's cascades and a point
		* light's six faces are consecutive: the receiver reaches the others from the first slot.
		* FirstSlot zero means the frame allocated no shadow view for the light, which is the
		* "casts no shadow" address the slot table's sentinel occupies.
		*/
		struct FShadowLightAddress
		{
			UINT32	FirstSlot	= 0u;
			UINT32	ViewNum		= 0u;
		};
		using RShadowLightAddressMapType	= TMap<ObjectIdentityType, FShadowLightAddress>;
		using RViewShadowStateMapType		= TMap<ObjectIdentityType, RViewShadowState*>;
		// One address table per main view. A light's first slot is an index into that view's own
		// slot table, so two views can place the same light at different addresses: their atlases
		// allocate independently, and their local light candidates are ranked by their own screen
		// coverage. The table is what the light list is encoded from, per view.
		using RViewShadowAddressMapType		= TMap<ObjectIdentityType, RShadowLightAddressMapType>;
		/*
		* One view's pair of light list buffers.
		*
		* Every main view owns a pair, because the shadow address a light element carries is an
		* index into that view's slot table: the same light sits at different slots in two views,
		* so the two views cannot share the bytes. It has to be a buffer per view rather than one
		* buffer rewritten between views, because the device records an upload into its own upload
		* command list and executes all of it before the frame's list - two uploads into one buffer
		* would leave only the last one visible to every view.
		*
		* A pair is a few kilobytes, sized once at the maximum, and only the used prefix is written.
		* Sizing to fit each frame would mean recreating the resource, and with it the shader
		* resource view that was built at creation, every time a light was added or removed.
		*/
		struct RViewLightBuffers
		{
			// Builds both buffers at their maximum capacity. Idempotent: a second call after the
			// device is up is what a device reset needs, and it is a no-op otherwise.
			void	EnsureBuffers(IRRHIDevice* InRenderDevice);
			RStructuredBuffer	DirectionalLightBuffer;
			RStructuredBuffer	LocalLightBuffer;
		};
		using RViewLightBuffersMapType		= TMap<ObjectIdentityType, RViewLightBuffers*>;
	public:
		/*
		* The cpu staging the two light lists are filled from, and the counts that go with them.
		*
		* The staging is frame wide: a light list is a fact about the scene, and what a view owns
		* is a cascade's projection - which is exactly why a light element carries a shadow map
		* index and no matrix. What the *encoding* depends on is the view, because that index
		* addresses the view's slot table, so the staging is filled once per frame and then encoded
		* into each view's own buffers.
		*/
		class RLightLists
		{
		public:
			// Reserves both staging arrays at their maximum capacity. Idempotent.
			void	ReserveStaging();
			// Encodes every staged element's shadow address from InShadowAddresses, writes the used
			// prefix into the given view's buffers and uploads. An empty table encodes every light
			// as "casts no shadow", which is what a view whose shadow data is not usable receives:
			// the receiver then skips the shadow term without needing a per view shader flag.
			void	UploadForView(IRRHIDevice* InRenderDevice, RViewLightBuffers& InOutBuffers
				, const RShadowLightAddressMapType& InShadowAddresses);
		public:
			TArray<EDirectionalLightsGPU>	DirectionalLightStaging;
			TArray<ELocalLightsGPU>			LocalLightStaging;
			// The proxy identity of each staged element, in staging order. The gather fills them
			// beside the elements because UploadForView has to look up an address per element,
			// and the staging element itself carries no identity.
			TArray<ObjectIdentityType>		DirectionalLightIdentities;
			TArray<ObjectIdentityType>		LocalLightIdentities;
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
		/*
		* Resolves every directional light's cascade shadow into the view's mask target, before the
		* lighting pass that samples it. Every main view has its own state, its own atlas and its own
		* mask, so the pass runs once per view and never reads another view's data.
		*
		* It is skipped for a view whose shadow data is not usable this frame. That view's light list
		* is then encoded with no shadow addresses, so its lighting skips the shadow term entirely
		* rather than reading a mask that was never written.
		*
		* The mask is cleared to one, so a channel no light claims reads as "fully lit" rather than
		* as a shadow, and the target is left in a shader readable state for the lighting pass.
		*/
		void			RenderDirectionalShadowMask(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures);
		/*
		* Whether this view has a directional shadow mask the lighting pass may trust. It is what
		* RenderDirectionalShadowMask gates on and what decides whether the view's light list carries
		* shadow addresses at all, so a view that cannot produce a mask is shaded unshadowed instead
		* of reading a target that was never written.
		*/
		PE_NODISCARD BOOL32	IsDirectionalShadowMaskAvailable(const ObjectIdentityType InViewIdentity);
		/*
		* Encodes the frame's light staging into every main view's own buffers and uploads them.
		* Called once per frame after the gather, before the frame records: a slot index is an index
		* into one view's slot table, so each view needs its own bytes, and the device executes
		* uploads before the frame's command list - which is why the per view split is buffers and
		* not a rewrite between views.
		*/
		void			UploadLightListsForViews();
		// The buffer pair of a view, created on first use. Ownership stays with the renderer.
		RViewLightBuffers*	EnsureViewLightBuffers(const ObjectIdentityType InViewIdentity);
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
		/*
		* Builds one view's shadow views for this frame.
		*
		* The order is the whole design in one place: rank the local lights that are visible in the
		* main camera, ask every surviving light for the views it needs, hand those requests to the
		* atlas, build a view per request and cull the casters against it, then write the slot table
		* the shading side indexes. Nothing here knows a light type - the requests do, through the
		* kind they carry.
		*/
		void			InitShadowViews(RViewProxy* InViewProxy);
		// Records the depth atlas of every view that owns shadow views, before the base pass that
		// samples it. One pass per view, one viewport per allocated tile, and one caster list per
		// shadow view.
		void			RenderShadowViews();
		/*
		* The local lights that may keep a shadow this frame, ranked by how much of the view they
		* cover. A light that is outside the main camera, casts no shadow, or is of a type without a
		* shadow projection is not a candidate at all, and no tile or slot is spent on it. The two
		* output arrays are split by type because their caps are independent, and each is ordered by
		* coverage descending with the identity ascending as the tie break, so an unchanged scene
		* ranks the same way every frame.
		*/
		void			SelectLocalShadowCandidates(const RViewProxy& InView
			, TArray<RLocalLightSceneProxy*>& OutSpotCandidates, TArray<RLocalLightSceneProxy*>& OutPointCandidates)const;
		// The shadow state of a view, created on first use. Ownership stays with the renderer.
		RViewShadowState*	EnsureViewShadowState(const ObjectIdentityType InViewIdentity);
		// Releases the states of views the scene no longer has. A view that merely stopped being the
		// main one keeps its atlas: the atlas is sized by options, not by the view's resolution, so
		// recreating it for the next frame that shades that view would cost a full resource rebuild.
		void			PruneViewShadowStates();
		// Releases every atlas and slot buffer. Called on shutdown, before the device goes.
		void			ReleaseViewShadowStates();
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
		// Specular IBL: the prefiltered environment cube and the DFG lookup table.
		//
		// The prefilter runs once per (cube, resolution exponent) pair, after the SH bake and
		// before the lighting pass that samples its output. The LUT is independent of the
		// environment and is generated once.
		void			RenderEnvironmentPrefilter();
		BOOL32			EnsureEnvironmentPrefilterPipelineState();
		// Creates or recreates the prefiltered cube and its per-mip UAV views when the exponent
		// changed. The texture is returned to the caller through the members; FALSE means the
		// device refused a resource and the pass must be skipped.
		BOOL32			EnsurePrefilteredEnvironmentResources(IRRHIDevice* InRenderDevice, const UINT32 InResolutionExponent);
		void			RenderEnvironmentBRDFLUT();
		BOOL32			EnsureEnvironmentBRDFPipelineState();
		BOOL32			EnsureBRDFLUTResources(IRRHIDevice* InRenderDevice);
		// Releases the prefiltered cube, the LUT, and every subresource view they own. Called on
		// shutdown and whenever the prefiltered cube is recreated.
		void			ReleaseSpecularIBLResources();
	protected:
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
			PIPELINE_STATE_DIRECTIONAL_SHADOW_MASK,			// (full) Full-screen directional shadow mask
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
		const EShaderInstance*		ShadowMaskProgramInstance = nullptr;
		const EShaderInstance*		EnvironmentSHProgramInstance = nullptr;
		const EShaderProgram*		FullScreenProgram;
		const EShaderProgram*		SceneLightingProgram;
		const EShaderProgram*		ShadowMaskProgram;
	protected:
		ObjectIdentityType			FinalOutputView;
		BOOL32						NeedStencil;
		RViewSceneTextureType		ViewSceneTextures;
		// Per main view shadow state: the depth atlas, the slot table, and the shadow views that
		// feed them. Keyed by view identity because a cascade belongs to the view it was derived
		// from; a view that is not the main one has no entry.
		RViewShadowStateMapType		ViewShadowStates;
		// Where this frame's lights landed in each view's slot table, filled by InitShadowViews
		// and read when that view's light list is encoded. One table per view because a slot index
		// only means something against the slot table it came from: a rear view mirror view has its
		// own atlas, its own candidates and therefore its own addresses for the same lights.
		RViewShadowAddressMapType	ViewShadowAddresses;
		// TRUE once the budget's first drop has been reported. A frame that has to drop shadow
		// views would otherwise log every frame, which is noise rather than a report.
		BOOL32						bShadowDropReported = FALSE;
		// The frame's light staging. Global by design: a light is not a property of a view. What a
		// view owns is the encoding - the shadow addresses - and that lives in ViewLightBuffers.
		RLightLists					LightLists;
		// Each main view's own light list buffers, created on first use and freed with the view.
		RViewLightBuffersMapType	ViewLightBuffers;
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
		// The directional shadow mask's block: the reciprocal of the mask's pixel size, and
		// nothing else. Every other value the resolver needs is per cascade and arrives in the
		// slot table, which is why this block stays this small - a second copy here would be a
		// second source of truth for a value the slot already carries.
		RShaderParameterBlock	ShadowMaskParameterBlock{ EName(SHADER_BLOCK_SHADOW_MASK_PARAMS) };
		// Whether the pair above has been reported once. It answers "what is the indirect term
		// multiplied by" for a session where the environment looks black, which is otherwise the
		// hardest kind of question to answer from a picture.
		BOOL32					bIndirectLightingParamsReported;
	protected:
		// ---- Specular IBL ----
		// The prefiltered environment: a six slice 2D array whose levels carry the GGX filtered
		// environment shape (mean one). It is deliberately not a cube resource: D3D11 has no
		// cube UAV dimension, while a 2D array works on both back ends with the same shader.
		// The lighting pass maps a reflection direction to (face, uv) itself.
		RRHITexture2D			PrefilteredEnvironmentCube;
		TArray<IRRHITextureUAVView*>	PrefilteredEnvironmentUAVViews;
		UINT32					PrefilteredEnvironmentMipCount;
		UINT32					PrefilteredEnvironmentResolutionExponent;
		// What the current chain was built from. Comparing these is what keeps the bake off the
		// frame path: a different cube, a different mean, or a different exponent rebuilds it.
		const IRRHITexture*		EnvironmentPrefilterBakedSource;
		FLOAT					EnvironmentPrefilterBakedMean;
		// The DFG table: one 2D texture, independent of the environment, generated once.
		RRHITexture2D			EnvironmentBRDFLUT;
		IRRHITextureUAVView*	EnvironmentBRDFLUTUAVView;
		BOOL32					bEnvironmentBRDFLUTBaked;
		IRRHIPipelineState*		EnvironmentPrefilterPipelineState;
		IRRHIPipelineState*		EnvironmentBRDFPipelineState;
		const EShaderProgram*	EnvironmentPrefilterProgram;
		const EShaderProgram*	EnvironmentBRDFProgram;
		const EShaderInstance*	EnvironmentPrefilterProgramInstance;
		const EShaderInstance*	EnvironmentBRDFProgramInstance;
		RShaderParameterBlock	EnvironmentPrefilterParameterBlock{ EName(PE_TEXT("PrefilterParams")) };
		RShaderParameterBlock	EnvironmentBRDFParameterBlock{ EName(PE_TEXT("BRDFLUTParams")) };
	public:
		RSceneRenderer();
		virtual ~RSceneRenderer();
	public:
		RSceneRenderer(const RSceneRenderer&) = delete;
		RSceneRenderer& operator=(const RSceneRenderer&) = delete;
	};

};
