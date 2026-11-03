#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RHI/IRRHIResource.h>
#include <RHI/RHITypes.h>
#include <Renderer/RenderFilter.h>

namespace PigeonEngine
{

	class RViewProxy;
	class RPrimitiveSceneProxy;
	class EShaderInstance;
	class EShaderProgram;
	class EShaderAsset;
	class EShaderParameterSet;
	class IRRHIDevice;
	class IRRHIPipelineState;
	class RShaderParameterBuffers;
	struct FRenderPassDesc;
	struct FShaderInstanceRenderData;

	/*
	* One primitive's vertex and index binding for one pass.
	*
	* The input layout a pass uses comes from its program, so the binding is resolved per
	* (primitive, pass) rather than per primitive. It is cheap to replay: every field here is
	* a pointer or an integer the record path hands to the command list.
	*/
	struct FGeometryBinding
	{
		IRRHIBuffer*	VertexBuffers[RHI_MAX_VERTEX_BUFFERS]	= {};
		UINT32			VertexStrides[RHI_MAX_VERTEX_BUFFERS]	= {};
		UINT32			VertexOffsets[RHI_MAX_VERTEX_BUFFERS]	= {};
		UINT32			VertexBufferCount						= 0u;
		IRRHIBuffer*	IndexBuffer								= nullptr;
		ERHIIndexFormat	IndexFormat								= ERHIIndexFormat::RHI_INDEX_FORMAT_UINT16;
	};

	/*
	* The identity of one published shader instance.
	*
	* Program is part of the key from the start: the same instance compiled against two
	* programs has two reflected layouts and two binding plans, so the buffers and the
	* bindless index fields cannot be shared between them.
	*/
	struct FShaderInstanceKey
	{
		const EShaderInstance*	ShaderInstance	= nullptr;
		const EShaderProgram*	Program			= nullptr;

		PE_NODISCARD BOOL32 operator<(const FShaderInstanceKey& InOther) const;
		PE_NODISCARD BOOL32 operator==(const FShaderInstanceKey& InOther) const;
	};

	/*
	* L1: the published data of one view.
	*
	* The view proxy owns the values and the GPU buffers; this record is the frame handle
	* every pass asks for. ResolvedFrame is what makes EnsureView idempotent: the first pass
	* that needs the view publishes it, every later pass in the same frame reads the stamp.
	*/
	struct FViewRenderData
	{
		UINT64			ResolvedFrame	= 0ull;
		RViewProxy*		View			= nullptr;
		EViewKindType	ViewKind		= EViewKindType::VIEW_KIND_CAMERA;
	};

	/*
	* L2: the published data of one primitive.
	*
	* View independent by construction: a world transform, bounds and flags are facts about
	* the primitive, so one frame's publish serves every view, every pass and every future
	* compute or ray tracing consumer.
	*/
	struct FPrimitiveRenderData
	{
		UINT64					ResolvedFrame	= 0ull;
		RPrimitiveSceneProxy*	Primitive		= nullptr;
	};

	/*
	* L4: one primitive's resolved data for one pass.
	*
	* Keyed by (primitive, pass, base shader instance). The base instance is the view's
	* override when it selected one, otherwise the instance the primitive reported for the
	* slot's primary pass. The resolved instance is the pass specific one the material layer
	* derives from that base (a depth only instance for the shadow pass, for example). A null
	* base means the slot has no material of its own, and the engine fallback (or a pass
	* instance derived from it) answers for that slot.
	*/
	struct FPrimitivePassData
	{
		UINT64						ResolvedFrame				= 0ull;
		UINT32						SlotIndex					= 0u;
		const FRenderPassDesc*		PassDesc					= nullptr;
		EName						PassName;
		const EShaderProgram*		Program						= nullptr;
		IRRHIPipelineState*			PipelineState				= nullptr;
		// The instance actually bound, which is the base instance for a primary pass and the
		// pass specific instance derived from it (or from the fallback) otherwise.
		const EShaderInstance*		ResolvedShaderInstance		= nullptr;
		const EShaderAsset*			ShaderInstanceAsset			= nullptr;
		UINT32						ShaderInstanceRevision		= 0u;
		// The base selected for this slot: the view override when it selected one, otherwise
		// the instance the primitive reported for the slot. Null means the slot has no
		// material, and the fallback answers. The slot is reusable while this pair is
		// unchanged, which is what lets a later view answer from the cache.
		const EShaderInstance*		BaseShaderInstance			= nullptr;
		const EShaderAsset*			BaseShaderInstanceAsset		= nullptr;
		BOOL32						bShaderInstancePending		= FALSE;
		UINT32						PendingRetryFrame			= 0u;
		EString						DisplayName;
		// Resolved during Prepare by the primitive's geometry hook. The record path only
		// replays it, so a worker never walks a shader input layout or a mesh stream table.
		BOOL32						bGeometryBindingCached		= FALSE;
		const void*					CachedGeometryResource		= nullptr;
		FGeometryBinding			Geometry;
		FShaderInstanceRenderData*	ShaderInstanceData			= nullptr;
	};

	/*
	* L5: the published data of one shader instance under one program.
	*
	* This is the object that makes "publish once" real. A material used by a thousand
	* primitives, eight shadow views and a compute pass has one parameter buffer set, one
	* bindless index write and one upload per frame, because every consumer asks
	* FFrameRenderData for the same key.
	*/
	struct FShaderInstanceRenderData
	{
		UINT64						ResolvedFrame		= 0ull;
		FShaderInstanceKey			Key;
		UINT32						ResolvedRevision	= 0u;
		EShaderParameterSet*		Parameters			= nullptr;
		RShaderParameterBuffers*	ParameterBuffers	= nullptr;

		~FShaderInstanceRenderData();
	};

	/*
	* L6: one descriptor table pair a group of draws reads.
	*
	* The descriptor contents do not change during a frame, so every draw that resolves to
	* the same (program, shader instance, view) binds the same tables. The record path only
	* binds the base; it never copies a descriptor again.
	*
	* The cached set path is the descriptor layer's own work item. Until it lands, the
	* record path keeps using the per-draw set the command list opens, and BindingSet stays
	* null; the data model is already keyed for it.
	*/
	struct FBindingSet
	{
		UINT64	ResolvedFrame	= 0ull;
		UINT32	SRVTableBase	= 0u;
		UINT32	UAVTableBase	= 0u;
		UINT64	KeyHash			= 0ull;
	};

	/*
	* The frame's publish registry and the only place any of L1-L6 is resolved or uploaded.
	*
	* Every Ensure* is idempotent for one frame. It asserts unless the caller is inside the
	* render thread's Prepare phase, which is what keeps every worker record path read only.
	*/
	class FFrameRenderData final
	{
	public:
		FFrameRenderData();
		~FFrameRenderData();

		FFrameRenderData(const FFrameRenderData&) = delete;
		FFrameRenderData& operator=(const FFrameRenderData&) = delete;

	public:
		void	BeginFrame(UINT64 InFrameIndex);
		// Ends the Prepare phase. Every Ensure* from here to EndFrame asserts, which is what
		// makes a worker's accidental publish fail at its call site.
		void	EndPrepare();
		// Releases every published object. Called before the device goes away, so the GPU
		// buffers the shader instance data owns are released while their device is alive.
		void	ReleaseAll();
		void	EndFrame();

		PE_NODISCARD UINT64	GetFrameIndex() const { return FrameIndex; }
		PE_NODISCARD BOOL32	IsPreparing() const { return bPreparing; }

	public:
		PE_NODISCARD FViewRenderData*			EnsureView(RViewProxy& InView);
		PE_NODISCARD FPrimitiveRenderData*		EnsurePrimitive(RPrimitiveSceneProxy& InPrimitive);
		// InViewShaderInstanceOverride is the view's selection for the whole item, or null
		// when the primitive's own per slot report is authoritative.
		PE_NODISCARD FPrimitivePassData*		EnsurePrimitivePass(RPrimitiveSceneProxy& InPrimitive, const FRenderPassDesc& InPassDesc
			, const EShaderInstance* InViewShaderInstanceOverride, UINT32 InSlotIndex);
		PE_NODISCARD FShaderInstanceRenderData*	EnsureShaderInstance(const EShaderInstance& InShaderInstance, const EShaderProgram& InProgram);
		PE_NODISCARD FBindingSet*				EnsureBindingSet(const FPrimitivePassData& InPrimitivePassData, const FViewRenderData& InViewData);

	public:
		// The device the published buffers were created on. Set once when the renderer starts.
		void					SetRenderDevice(IRRHIDevice* InRenderDevice) { RenderDevice = InRenderDevice; }
		PE_NODISCARD IRRHIDevice*	GetRenderDevice() const { return RenderDevice; }

	private:
		PE_NODISCARD FShaderInstanceRenderData*	FindShaderInstance(const FShaderInstanceKey& InKey) const;
		void									ReleaseShaderInstanceData();

	private:
		UINT64				FrameIndex		= 0ull;
		// Monotonic publish counter. The device's frame index cycles with the frame-in-flight
		// count, which would make a two frame old stamp equal the current one; the publish
		// idempotence needs a value that never repeats.
		UINT64				FrameCounter	= 0ull;
		BOOL32				bPreparing		= FALSE;
		IRRHIDevice*		RenderDevice	= nullptr;
		TMap<ObjectIdentityType, TUniquePtr<FViewRenderData>>		Views;
		TMap<ObjectIdentityType, TUniquePtr<FPrimitiveRenderData>>	Primitives;
		TMap<FShaderInstanceKey, TUniquePtr<FShaderInstanceRenderData>>	ShaderInstances;
		TArray<TUniquePtr<FBindingSet>>								BindingSets;

	};

}	// namespace PigeonEngine
