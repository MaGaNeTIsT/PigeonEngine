#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderCommon.h>
#include <Renderer/RenderData.h>
#include <RHI/IRRHIResource.h>
#include <RHI/RHITypes.h>

namespace PigeonEngine
{

	class IRCommandList;
	class IRRHIPipelineState;
	class RViewProxy;
	class RPrimitiveSceneProxy;
	class EShaderInstance;
	class EShaderProgram;
	class RViewPrimitiveList;
	class FFrameRenderData;
	class FRenderPass;
	struct RViewPrimitiveItem;
	struct FViewRenderData;
	struct FPrimitivePassData;
	struct FShaderInstanceRenderData;
	struct FBindingSet;
	struct FRenderPassDesc;
	struct FRenderFilterContext;

	/*
	* Execution stages. A stage is both an ordering unit and a state barrier: every pass
	* instance inside one stage is independent of the others, and the renderer records one
	* state sync list before the stage's recorder lists. Two passes with an ordering
	* dependency must therefore live in different stages - that is the only rule for adding
	* a stage.
	*/
	enum ERenderPassStageType : UINT8
	{
		RENDER_PASS_STAGE_SHADOW = 0,
		RENDER_PASS_STAGE_BASE,
		RENDER_PASS_STAGE_SKY,
		RENDER_PASS_STAGE_SHADOW_MASK,
		RENDER_PASS_STAGE_LIGHTING,
		RENDER_PASS_STAGE_FORWARD,
		RENDER_PASS_STAGE_FINAL,
		RENDER_PASS_STAGE_OVERLAY,
		// Appended after the existing stages so no persistent pass data moves. The
		// simulation stage is executed first by RSceneRenderer::Render; the translucent
		// stage is currently recorded on the engine tail list after forward.
		RENDER_PASS_STAGE_SIMULATION,
		RENDER_PASS_STAGE_TRANSLUCENT,
		RENDER_PASS_STAGE_COUNT
	};

	/*
	* The targets one pass instance writes. The renderer builds this in Prepare; the record
	* path only replays it. Clear colors are values, not operations: the stage sync list owns
	* the clear, so every recorder binds the same target with LOAD.
	*/
	struct FPassTargets
	{
		IRRHITexture*	RenderTargets[RHI_MAX_RENDER_TARGETS]	= {};
		UINT32			RenderTargetCount						= 0u;
		IRRHITexture*	DepthTarget								= nullptr;
		Color4			ClearColors[RHI_MAX_RENDER_TARGETS]		= {};
		FLOAT			ClearDepth								= 1.0f;
		UINT8			ClearStencil							= 0u;
		EViewport		Viewport;
		ERect			Scissor;
	};

	/*
	* One unit of work a pass asks a recorder to perform.
	*
	* The same shape serves a graphics draw, a per primitive compute dispatch and a future
	* ray tracing dispatch: the pass interprets the fields it cares about, the scheduler only
	* partitions the array. Nothing here names a draw.
	*/
	struct FPassItem
	{
		RPrimitiveSceneProxy*		Primitive				= nullptr;
		FPrimitiveRenderData*		PrimitiveData			= nullptr;
		FPrimitivePassData*			PrimitivePassData		= nullptr;
		FShaderInstanceRenderData*	ShaderInstanceData		= nullptr;
		FBindingSet*				BindingSet				= nullptr;
		UINT32						SlotIndex				= 0u;
		UINT32						IndexCount				= 0u;
		UINT32						StartIndex				= 0u;
		UINT64						ItemFlags				= 0u;
	};

	/*
	* A pass instance over one view: the view's data, the targets it writes and the items
	* this pass selected from the view's primitive list. The renderer owns the array; the
	* scheduler partitions each instance's items into record tasks.
	*/
	struct FPassViewInstance
	{
		const FRenderPass*			Pass		= nullptr;
		RViewProxy*					View		= nullptr;
		FViewRenderData*			ViewData	= nullptr;
		FPassTargets				Targets;
		TArray<FPassItem>			Items;
		UINT32						StageOrder	= 0u;
	};

	/*
	* Prepare-time context for one pass instance. CollectItems runs here and may ask the
	* frame data to resolve L1/L4/L5; the pass desc carries the fixed function state.
	*/
	struct FPassViewContext
	{
		RViewProxy*				View		= nullptr;
		FViewRenderData*		ViewData	= nullptr;
		FFrameRenderData*		FrameData	= nullptr;
		const FRenderPassDesc*	PassDesc	= nullptr;
		FPassTargets			Targets;
	};

	/*
	* The last bound state of one recorder's command list. A draw that runs with the same
	* program, shader instance and pipeline state as the draw before it can skip the rebind.
	* This is list state, not frame state: every recorder owns its own copy, and the scheduler
	* resets it when the list starts a new pass instance.
	*/
	struct FPrimitiveDrawStateCache
	{
		IRCommandList*			CommandList		= nullptr;
		IRRHIPipelineState*		PipelineState	= nullptr;
		const EShaderInstance*	ShaderInstance	= nullptr;
		const EShaderProgram*	Program			= nullptr;
		const RViewProxy*		View			= nullptr;
	};

	/*
	* Record-time context. The command list and the pass instance are explicit: no recorder
	* ever asks a manager for "the active list", and no worker touches renderer members.
	*/
	struct FPassRecordContext
	{
		IRCommandList*				CommandList		= nullptr;
		UINT32						RecorderIndex	= 0u;
		UINT64						FrameIndex		= 0ull;
		FFrameRenderData*			FrameData		= nullptr;
		// The task's pass instance. Record is read only, so the context carries it as const.
		const FPassViewInstance*	Instance		= nullptr;
		FPrimitiveDrawStateCache	StateCache;
	};

	/*
	* One resource transition or clear a stage owns. The before state is read when the stage's
	* sync list is recorded, so a plan states only the destination.
	*/
	struct FRHITransition
	{
		IRRHIResource*		Resource	= nullptr;
		ERHIResourceState	After		= ERHIResourceState::RHI_RESOURCE_STATE_COMMON;
	};

	struct FRHIClear
	{
		IRRHIResource*	Resource		= nullptr;
		UINT8			ClearFlags		= 0u;
		Color4			ClearColor		= Color4(0.f, 0.f, 0.f, 0.f);
		FLOAT			ClearDepth		= 1.0f;
		UINT8			ClearStencil	= 0u;
	};

	struct FPassStageState
	{
		ERenderPassStageType	Stage = ERenderPassStageType::RENDER_PASS_STAGE_SHADOW;
		TArray<FRHITransition>	Transitions;
		TArray<FRHIClear>		Clears;
	};

	/*
	* The frame's state plan. Passes declare what their stage writes; the renderer turns each
	* stage's list into one sync command list recorded on the render thread with IMMEDIATE
	* state tracking. Recorder lists then run with EXTERNAL state and never transition.
	*/
	class FPassStatePlan final
	{
	public:
		void	Reset();
		void	AddTransition(ERenderPassStageType InStage, IRRHIResource* InResource, ERHIResourceState InAfter);
		void	AddClear(ERenderPassStageType InStage, const FRHIClear& InClear);
		PE_NODISCARD const TArray<FPassStageState>& GetStages() const { return Stages; }
	private:
		PE_NODISCARD FPassStageState*	FindStage(ERenderPassStageType InStage);
		TArray<FPassStageState>			Stages;
	};

	/*
	* A pass is one stage, one view predicate, one item collection and one record loop.
	*
	* BasePass and ShadowPass are two implementations of this interface; so are the engine
	* driven full screen passes and a future compute pass. The interface deliberately has no
	* "draw" in its name: RecordItems is whatever the pass records, and CollectItems is the
	* filter from a view's primitive list into the pass's work items.
	*/
	class FRenderPass
	{
	public:
		explicit FRenderPass(const FRenderPassDesc& InPassDesc);
		virtual ~FRenderPass() = default;

		FRenderPass(const FRenderPass&) = delete;
		FRenderPass& operator=(const FRenderPass&) = delete;

	public:
		PE_NODISCARD virtual ERenderPassStageType	GetStage() const = 0;
		PE_NODISCARD const FRenderPassDesc&	GetPassDesc() const;
		// Whether this pass consumes the given view. ShadowPass answers only shadow views;
		// BasePass answers only a main camera view.
		PE_NODISCARD virtual BOOL32					ShouldUseView(const RViewProxy& InView) const = 0;

		// Prepare: select the pass's items from the view's collected primitive list. The
		// default implementation applies the central filter and resolves the item data.
		virtual void	CollectItems(const FPassViewContext& InContext, const RViewPrimitiveList& InViewList, TArray<FPassItem>& OutItems) const;
		// Prepare: resolve or upload anything the selected items need beyond the default.
		virtual void	PrepareItems(FFrameRenderData& InFrameData, TArray<FPassItem>& InOutItems) const;
		// Prepare: declare this stage's transitions and clears.
		virtual void	DeclareState(FPassStatePlan& InOutPlan) const;

		// Record: whether the item range may be split across recorders.
		PE_NODISCARD virtual BOOL32					IsItemParallel() const { return TRUE; }
		virtual void	BeginPass(FPassRecordContext& InContext) const;
		virtual void	RecordItems(FPassRecordContext& InContext, const TArray<FPassItem>& InItems, UINT32 InBegin, UINT32 InEnd) const = 0;
		virtual void	EndPass(FPassRecordContext& InContext) const;

	public:
		// An additional, pass-local predicate. The central registry runs first; this is for
		// one-off experiments that should not add a new pass subclass or a new global rule.
		void	SetItemPredicate(TFunction<BOOL32(const FRenderFilterContext&)> InPredicate) { ItemPredicate = InPredicate; }

	protected:
		PE_NODISCARD virtual BOOL32	ShouldDrawItem(const FRenderFilterContext& InContext) const;

	protected:
		const FRenderPassDesc*							PassDesc = nullptr;
		TFunction<BOOL32(const FRenderFilterContext&)>	ItemPredicate;

	};

}	// namespace PigeonEngine
