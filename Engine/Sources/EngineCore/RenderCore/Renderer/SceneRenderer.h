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
#include <Renderer/RenderData.h>
#include <Renderer/RenderPass.h>
#include <Renderer/RenderRecordScheduler.h>

namespace PigeonEngine
{

	class EShaderProgram;
	class EShaderInstance;
	class RViewProxy;
	class RViewPrimitiveList;
	struct RViewPrimitiveBounds;
	class RLocalLightSceneProxy;
	class RViewShadowState;
	class IRCommandList;
	class FBasePass;
	class FShadowPass;
	class FWaterSurfacePass;

	/*
	* Where a renderer's final image goes.
	*
	* The window output owns the device back buffer and the editor overlay. An offscreen
	* output owns a colour target the renderer builds at its output view's size and leaves
	* shader readable, which is the image an editor panel samples. Everything before the
	* final copy is the same for both, and that is what lets a preview run the whole
	* pipeline instead of a hand written subset of it.
	*/
	enum RRendererOutputType : UINT8
	{
		RENDERER_OUTPUT_WINDOW = 0,
		RENDERER_OUTPUT_OFFSCREEN
	};

	struct FRendererOutputDescription
	{
		RRendererOutputType	Type			= RRendererOutputType::RENDERER_OUTPUT_WINDOW;
		// Offscreen only. FORMAT_UNKNOWN takes the format the final output pass row declares,
		// so a pipeline change to that row reaches the target instead of drifting from it.
		RFormatType			OffscreenFormat	= RFormatType::FORMAT_UNKNOWN;
	};

	class RSceneRenderer : public EManagerBase
	{
	public:
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
		virtual void	Initialize() override;
		virtual void	ShutDown() override;
	#if ENGINE_EDITOR_ONLY
		void			SetManageDebugWireframe(BOOL32 InManageDebugWireframe) { bManageDebugWireframe = InManageDebugWireframe; }
		void			SetRenderDebugWireframe(BOOL32 InRenderDebugWireframe) { bRenderDebugWireframe = InRenderDebugWireframe; }
	#endif
	public:
		RScene*			GetRenderScene();
		void			InitNewFrame();
		void			Render();
		void			ResizeRenderTargets(UINT32 InWidth, UINT32 InHeight);
		/*
		* Selects where the frame's final copy lands. The renderer owns an offscreen target:
		* it is built from the output view's viewport on the render thread and rebuilt when
		* that size changes. A window renderer never builds one.
		*/
		void			SetOutputDescription(const FRendererOutputDescription& InOutputDescription);
		PE_NODISCARD BOOL32	IsWindowOutput() const;
		/*
		* The last recorded offscreen image, or null while the renderer is a window renderer.
		* RenderThread only: the target is built and replaced there, and the editor layer knows
		* how to turn it into its own handle. A caller on another thread reads a cached handle.
		*/
		PE_NODISCARD IRRHITexture*	GetOutputColorTexture();
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
		// The frame's publish registry. Passes and proxies ask it for the data they consume;
		// only the Prepare phase may resolve or upload through it.
		PE_NODISCARD FFrameRenderData&		GetFrameRenderData() { return FrameData; }
		PE_NODISCARD const FFrameRenderData&	GetFrameRenderData() const { return FrameData; }
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
		void	BindEngineParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram, const RViewProxy* InView = nullptr);
		// Adds the per view shading resources a forward material program declares - the two light
		// lists, the environment irradiance, the split sum pair and the water refraction copy - to
		// InOutOverrides. The caller binds them with the material's own set in one call, so the
		// program's single descriptor table carries the material resources, the water simulation
		// outputs and these together. The deferred lighting pass publishes the same values for its
		// own program; a forward pass that shades has its own binding plan slots and has to bind
		// them again. The water surface pass is the caller.
		void	AddWaterLightingParameters(EShaderParameterSet& InOutOverrides, const RViewProxy* InView);
		// Binds the renderer's indirect lighting block for a forward program. Separate from the
		// resource set above because the block is a constant buffer, not a table entry.
		void	BindWaterIndirectLightingParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram);
	protected:
		// Builds the pass instances for this frame: one per (item pass, view). ShadowPass and
		// BasePass are the two item passes; every later pass keeps the same shape.
		void			BuildPassInstances(TArray<FPassViewInstance>& OutInstances);
		// Prepare: select each instance's items and publish the data they consume.
		void			PreparePassInstances(TArray<FPassViewInstance>& InOutInstances);
		// Prepare: turn the instances of one stage into record tasks.
		void			BuildStageTasks(const TArray<FPassViewInstance>& InInstances, ERenderPassStageType InStage
			, TArray<FRecordTask>& OutTasks) const;
		// Record: the stage sync list. It owns transitions, clears, the frame's GPU profiler
		// bracket on the first stage, and the engine's per frame GPU setup on the same list.
		void			RecordStageSync(ERenderPassStageType InStage, IRCommandList* InCommandList);
		// Record: binds one item pass instance's targets and viewport.
		void			BeginItemPass(FPassRecordContext& InContext) const;
		void			EndItemPass(FPassRecordContext& InContext) const;
		// Record: replays a range of prepared items.
		void			RecordItemRange(FPassRecordContext& InContext, const TArray<FPassItem>& InItems, UINT32 InBegin, UINT32 InEnd) const;
		// Record: the per view shading tail that runs after every item pass: sky, shadow mask,
		// lighting and forward, with each main view's viewport. A renderer that owns only an
		// offscreen view (the asset preview) records this and its own display copy, instead of
		// the window passes below.
		void			RecordViewTailPasses(IRCommandList* InCommandList);
		// Record: the window's engine driven tail - the per view shading above, then the final
		// output blit into the back buffer and the editor overlay.
		void			RecordTailPasses(IRCommandList* InCommandList);
		void			InitViews();
		// Builds the pipeline states of the passes the renderer drives itself (lighting and the
		// final blit). It is called once per frame and returns as soon as every state exists: a
		// pass program only has device objects after the render thread has run the creation
		// commands its asset load queued, and the first call happens before that thread exists,
		// so a one shot build would latch those passes as permanently unbuildable.
		void			EnsureEnginePassPipelineStates();
		// The offscreen colour target, built to the output view's viewport. Null while the
		// renderer is a window renderer or while the output view has no size yet.
		PE_NODISCARD IRRHITexture*	ResolveOffscreenOutputTarget();
		void			FinalOutputPass();
#if ENGINE_EDITOR_ONLY
		void			ImGuiPass();
#endif
	protected:
		void			RenderLighting(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures);
		void			RenderSky(const RViewProxy* InViewProxy);
		void			RenderForward(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures);
		void			RenderWaterSimulation(IRCommandList* InCommandList);
		void			RenderWaterSurface(IRCommandList* InCommandList, RViewProxy* InViewProxy, RSceneTextures* InSceneTextures);
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
		/*
		* The local lights that may keep a shadow this frame, ranked by how much of the view they
		* cover. A light that is outside the main camera, casts no shadow, or is of a type without a
		* shadow projection is not a candidate at all, and no tile or slot is spent on it. The two
		* output arrays are split by type because their caps are independent, and each is ordered by
		* coverage descending with the identity ascending as the tie break, so an unchanged scene
		* ranks the same way every frame.
		*/
		void			SelectLocalShadowCandidates(const RViewProxy& InView
			, TArray<RLocalLightSceneProxy*>& OutSpotCandidates, TArray<RLocalLightSceneProxy*>& OutPointCandidates) const;
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
		/*
		* Fills one view's primitive list.
		*
		* This is the only place that walks the scene's primitives for a view: the cull decides
		* visibility against the view's own frustum, and every pass that draws the view reads the
		* list that comes out. A shadow view adds the caster filter, because a primitive that does
		* not cast is not part of that view's work at all.
		*
		* A primitive whose bound cannot be computed is kept: a missing bound is not evidence of
		* invisibility, and a caster dropped on a guess is a shadow that silently disappears.
		*
		* OutBounds is optional. When it is given it accumulates the world space union of the
		* primitives the cull kept, clipped to the frustum's own world AABB so a large caster
		* contributes only the part that can matter here. A kept primitive without a bound marks
		* the union incomplete, because a fit built from it could clip that primitive.
		*/
		void			CollectViewPrimitiveList(const RViewProxy& InView, RViewPrimitiveList& OutPrimitiveList
			, RViewPrimitiveBounds* OutBounds = nullptr) const;
		/*
		* Computes the world space union of every primitive this frame can draw as a caster, and
		* whether that union covers all of them. A directional cascade's conservative box uses it
		* for the near side of its depth range: the camera slice bounds the receivers, and an
		* occluder between the light and a receiver shadows it from anywhere in front of it.
		*
		* Run once per frame, after the scene commands have been applied and before the shadow
		* views are built.
		*/
		void			CollectSceneCasterBounds();
	protected:
		// Debug only: compares the C++ element stride of each light list against the field list
		// the shader reflected, which is the only thing that can catch a layout that drifted
		// between Assets/ShaderSources/Components/C_Lighting.json and LightSceneProxy.h. A
		// mismatch is a silent wrong shading result rather than a failure, which is why the
		// check exists at all.
		void			ValidateLightBufferLayout();
	protected:
		void			InitRendererSettings();
		BOOL32			IsNeedStencil() const;
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
			PIPELINE_STATE_WATER_REFRACTION_COPY,			// (full) Linear scene colour copy for water refraction
			PIPELINE_STATE_COUNT
		};
	protected:
		RScene*						Scene;
		RFullScreenTriangle			FullScreenTriangle;
#if ENGINE_EDITOR_ONLY
		RDebugWireframePrimitiveManager* DebugWireframePrimitiveManager;
		BOOL32					bManageDebugWireframe = TRUE;
		BOOL32					bRenderDebugWireframe = TRUE;
#endif
	protected:
		RSamplerResource			Samplers[RSamplerType::SAMPLER_TYPE_COUNT];
		IRRHIPipelineState*			PipelineStates[RPipelineStateType::PIPELINE_STATE_COUNT];
		// Active command list for the frame currently being recorded. It is the recorder 0 list
		// the engine driven tail runs on; item passes receive their list through
		// FPassRecordContext and never read this member.
		IRCommandList*				CurrentCommandList;
		// The frame's data registry and the recorder scheduler. Both are owned by the renderer
		// and live for its lifetime.
		FFrameRenderData			FrameData;
		FRenderRecordScheduler		RecordScheduler;
		FPassStatePlan				PassStatePlan;
		TUniquePtr<FBasePass>		BasePassObject;
		TUniquePtr<FShadowPass>		ShadowPassObject;
		TUniquePtr<FWaterSurfacePass>	WaterSurfacePassObject;
		// This frame's prepared pass instances. The stage scheduler consumes the shadow and base
		// entries; the water surface pass is recorded on the tail list and reads its own entry
		// from here after Prepare has resolved its items.
		TArray<FPassViewInstance>	FramePassInstances;
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
		const EShaderInstance*		WaterRefractionCopyProgramInstance = nullptr;
		const EShaderInstance*		SceneLightingProgramInstance = nullptr;
		const EShaderInstance*		ShadowMaskProgramInstance = nullptr;
		const EShaderInstance*		EnvironmentSHProgramInstance = nullptr;
		const EShaderProgram*		FullScreenProgram;
		const EShaderProgram*		WaterRefractionCopyProgram;
		const EShaderProgram*		SceneLightingProgram;
		const EShaderProgram*		ShadowMaskProgram;
	protected:
		ObjectIdentityType			FinalOutputView;
		BOOL32						NeedStencil;
		// Where this renderer's final copy goes. The window renderer takes the back buffer;
		// an offscreen renderer takes the target built from the output view.
		FRendererOutputDescription	OutputDescription;
		// The display-referred copy an offscreen renderer hands to its owner, and the size it
		// was built for. Owned here because it is rebuilt on the render thread when the view
		// resizes, and released with the renderer.
		RRenderTexture2D			OffscreenOutput;
		Vector2Int					OffscreenOutputSize = Vector2Int::Zero();
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
		// This frame's caster union, computed before the shadow views are built. A directional
		// cascade uses it to reach every occluder between the light and its slice; completeness
		// says whether every caster could state a bound, which is what a projection fit may rely
		// on. Authored as a member rather than recomputed per cascade because it is a fact about
		// the frame, not about one light.
		EBoundAABB					SceneCasterBounds;
		BOOL32						bSceneCasterBoundsComplete = TRUE;
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
		// The prefiltered environment: a cube resource whose levels carry the GGX filtered
		// environment shape (mean one). The resource is created with six array slices and the
		// cube misc flag, so the read side is a TextureCube SRV on both back ends - the lighting
		// pass samples it by reflection direction and lets the hardware select the face. Only the
		// write side is an array: no D3D version has a cube UAV, so the bake fills one mip at a
		// time through a six slice 2D array UAV view.
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

		friend class FBasePass;
		friend class FShadowPass;
		friend class FWaterSurfacePass;

	};

	/*
	* The deferred GBuffer pass. It is one implementation of FRenderPass: it consumes a main
	* camera view's primitive list, writes the view's GBuffer, and records every prepared item
	* through RPrimitiveSceneProxy::RecordPassItem. Nothing about the shadow pass appears here.
	*/
	class FBasePass final : public FRenderPass
	{
	public:
		explicit FBasePass(RSceneRenderer* InRenderer);
	public:
		virtual ERenderPassStageType	GetStage() const override { return ERenderPassStageType::RENDER_PASS_STAGE_BASE; }
		virtual BOOL32					ShouldUseView(const RViewProxy& InView) const override;
		virtual void					BeginPass(FPassRecordContext& InContext) const override;
		virtual void					RecordItems(FPassRecordContext& InContext, const TArray<FPassItem>& InItems, UINT32 InBegin, UINT32 InEnd) const override;
	private:
		RSceneRenderer*	Renderer;
	};

	/*
	* The shadow depth pass. It is the same interface as FBasePass over a shadow view: the
	* view supplies the frustum and the caster list, the pass supplies the atlas tile and the
	* depth target. The only shadow specific code is the target setup and the item record loop.
	*/
	class FShadowPass final : public FRenderPass
	{
	public:
		explicit FShadowPass(RSceneRenderer* InRenderer);
	public:
		virtual ERenderPassStageType	GetStage() const override { return ERenderPassStageType::RENDER_PASS_STAGE_SHADOW; }
		virtual BOOL32					ShouldUseView(const RViewProxy& InView) const override;
		virtual void					BeginPass(FPassRecordContext& InContext) const override;
		virtual void					RecordItems(FPassRecordContext& InContext, const TArray<FPassItem>& InItems, UINT32 InBegin, UINT32 InEnd) const override;
	private:
		RSceneRenderer*	Renderer;
	};

	/*
	* The translucent water surface pass. It consumes the main camera view's primitive list
	* after lighting and forward, sorts back to front, and records every prepared item through
	* the primitive's own record sequence. It is currently recorded on the engine tail list,
	* which is why it is prepared with the other passes but never scheduled as a stage.
	*/
	class FWaterSurfacePass final : public FRenderPass
	{
	public:
		explicit FWaterSurfacePass(RSceneRenderer* InRenderer);
	public:
		virtual ERenderPassStageType	GetStage() const override { return ERenderPassStageType::RENDER_PASS_STAGE_TRANSLUCENT; }
		virtual BOOL32					ShouldUseView(const RViewProxy& InView) const override;
		virtual void					CollectItems(const FPassViewContext& InContext, const RViewPrimitiveList& InPrimitiveList, TArray<FPassItem>& OutItems) const override;
		virtual BOOL32					IsItemParallel() const override { return FALSE; }
		virtual void					BeginPass(FPassRecordContext& InContext) const override;
		virtual void					EndPass(FPassRecordContext& InContext) const override;
		virtual void					RecordItems(FPassRecordContext& InContext, const TArray<FPassItem>& InItems, UINT32 InBegin, UINT32 InEnd) const override;
	private:
		RSceneRenderer*	Renderer;
	};

};
