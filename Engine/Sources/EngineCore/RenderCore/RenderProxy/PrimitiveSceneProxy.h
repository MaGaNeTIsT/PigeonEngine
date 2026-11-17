#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"
#include <RenderResource.h>
#include <RenderDevice/DeviceD3D11.h>
#include <Shader/ShaderEngineParameters.h>
#include <Shader/ShaderParameter.h>
#include <Shader/ShaderParameterBuffers.h>
#include <Renderer/RenderData.h>
#include <Renderer/RenderPass.h>
#include <Renderer/ScenePass.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveRenderData.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveComponent.h>

namespace PigeonEngine
{

	class EShaderInstance;
	class EShaderProgram;
	class EShaderAsset;
	class IRCommandList;
	class RViewProxy;

	/*
	* What every primitive proxy shares: the engine blocks it publishes, the pass slots it
	* resolves its shader instances into, and the record sequence itself.
	*
	* A subclass supplies what only it knows - which geometry it has, which streams that
	* geometry lives in, how its assets resolve to a shader instance - through the hooks
	* below. The sequence around those hooks is here, once, so a new primitive cannot get the
	* binding order wrong: pipeline state first, then the engine blocks, then the shader
	* instance, then the geometry, then the draw.
	*
	* Everything in FPrimitivePassData is resolved during the render thread's Prepare phase.
	* RecordPassItem only replays it, which is what lets several recorders run the same
	* primitive through different views without a lock.
	*/
	class RPrimitiveSceneProxy : public RBaseSceneProxy
	{
	public:
		virtual BOOL32	IsRenderValid() const;
		BOOL32			IsSceneProxyMovable() const;
		BOOL32			IsSceneProxyCastShadow() const;
		BOOL32			IsSceneProxyReceiveShadow() const;
		// World space bounds of the geometry this proxy draws, used to cull it against a view
		// frustum. FALSE means the proxy cannot state one; callers treat that as "may be
		// visible" rather than hiding it.
		virtual BOOL32	GetWorldBound(Vector3& OutMin, Vector3& OutMax) const;
		// The shader instance the primitive reported for a pass, without resolving a pass slot.
		PE_NODISCARD const EShaderInstance* FindReportedMaterialInstance(const UINT32 InSlotIndex, const EName& InPassName) const;
		// The instance the primitive reported for the primary pass of its first slot. A view
		// primitive list groups by it; the value is the same under every view.
		PE_NODISCARD const EShaderInstance* GetReportedPrimaryMaterial() const { return ReportedPrimaryMaterial; }
		// The base a pass specific instance derives from for one slot: the view's override
		// when it selected one, otherwise the instance the primitive reported for that slot's
		// primary pass. Null is a valid answer and means the pass data resolves the engine
		// fallback for the slot. The slot is explicit because the report is per slot: one
		// pointer at the item level would make slot 0 answer for every other slot.
		PE_NODISCARD const EShaderInstance* ResolveBaseShaderInstance(const UINT32 InSlotIndex
			, const EShaderInstance* InViewShaderInstanceOverride) const;
	public:
		// Filter inputs. A proxy states its category once; the central filter registry reads
		// it together with the view and pass masks. The virtuals below are the object's own
		// escape hatches for rules that are genuinely about this primitive.
		PE_NODISCARD UINT64	GetPrimitiveCategoryMask() const { return PrimitiveCategoryMask; }
		void				SetPrimitiveCategoryMask(const UINT64 InMask) { PrimitiveCategoryMask = InMask; }
		virtual BOOL32		ShouldCollectInView(const RViewProxy& InView) const;
		virtual BOOL32		ShouldDrawInPass(const FRenderPassDesc& InPassDesc) const;
	public:
		void			SetPrimitiveSettings(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow);
		void			UpdatePrimitiveMatrices(const ERenderPrimitiveMatrices& InMatrices);
	public:
		// Render thread: install a whole reported snapshot.
		void			SetupProxy(const FPrimitiveRenderSnapshot& InSnapshot);
		virtual void	ApplyAssets(const FPrimitiveRenderAssetReport& InAssets);
		virtual void	ApplyValues(const FShaderValueReport& InValues);
		virtual void	ApplyRuntimePayload(const FPrimitiveRenderSnapshot& InSnapshot);
		PE_NODISCARD const ObjectIdentityType&	GetPrimitiveIdentity() const { return PrimitiveIdentity; }
	public:
		// Prepare: publish the per primitive draw block once for this frame.
		void			EnsurePublishedDrawParameters() const;
		// Prepare: resolve and publish one pass's data. This is the only entry point the
		// frame data registry calls; it resolves the pass specific instance, the pipeline
		// state, the geometry binding and the shared shader instance data.
		// InViewShaderInstanceOverride is the view's selection for the whole item, or null
		// when the primitive's own per slot report is authoritative.
		FPrimitivePassData*	EnsurePublishedPassData(const UINT32 InSlotIndex, const EName& InPassName
			, const FRenderPassDesc& InPassDesc, const EShaderInstance* InViewShaderInstanceOverride, FFrameRenderData& InFrameData);
	public:
		// Record: replay one prepared item. The command list and the pass instance come from
		// the context; nothing here reads a renderer or manager global.
		void			RecordPassItem(FPassRecordContext& InContext, const FPassItem& InItem) const;
		// A draw that cannot run is otherwise silent. Report one line per (primitive, pass,
		// reason) so an object that stops appearing costs one line rather than one per frame.
		void			ReportSkippedDraw(const EName& InPassName, const EString& InReason) const;
		// A primitive that has compute work of its own overrides this. The default submits
		// nothing, which is what a graphics only primitive does.
		virtual BOOL32	RecordCompute(IRCommandList* InCommandList, const FRenderPassDesc& InPassDesc) const;
	public:
		// Public wrappers over the protected draw group hooks. The pass item builder walks
		// groups here; a subclass still overrides the hooks only.
		PE_NODISCARD UINT32	GetDrawGroupCount() const;
		PE_NODISCARD BOOL32	GetDrawGroupDesc(const UINT32 InGroupIndex, UINT32& OutSlotIndex, UINT32& OutIndexCount, UINT32& OutStartIndex) const;
	protected:
		// The per draw block, published from the reported values and bound at the root the
		// program's plan reports.
		BOOL32			EnsureDrawParameterBlock(const EShaderProgram& InProgram);
		void			UpdateDrawParameters() const;
		void			WriteDrawValues(const FShaderValueReport& InValues);
		void			PublishDrawParametersFromTransform();
		// The shared engine blocks (frame and view) are skipped when the caller knows they
		// are still bound from the draw before - same program, same view, same root signature.
		// The per draw block is bound either way; only it changes from primitive to primitive.
		void			BindEngineParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram
			, const RViewProxy* InView, const BOOL32 InBindSharedEngineBlocks = TRUE) const;
		// Binds the published shader instance's blocks and resources. The values were written
		// and uploaded during Prepare; this call only replays the binding plan.
		BOOL32			BindShaderInstanceData(IRCommandList* InCommandList, const FPassItem& InItem) const;
	protected:
		// Pass data. Built on demand per (slot, pass), invalidated when the assets change.
		PE_NODISCARD const FPrimitivePassData*	FindPrimitivePassData(const UINT32 InSlotIndex, const EName& InPassName) const;
		PE_NODISCARD BOOL32						IsPrimitivePassDataCurrent(const FPrimitivePassData& InData
			, const EShaderInstance* InBaseInstance, const BOOL32 InIsPrimaryPass) const;
		FPrimitivePassData*						EnsurePrimitivePassData(const UINT32 InSlotIndex, const EName& InPassName
			, const FRenderPassDesc& InPassDesc, const EShaderInstance* InViewShaderInstanceOverride);
		void									DropPassSlot(const UINT32 InSlotIndex, const EName& InPassName);
		void									ReleasePassSlots();
	protected:
		// What a subclass supplies. All defaults describe a primitive with nothing to draw,
		// so a partially migrated subclass is inert rather than wrong.
		PE_NODISCARD virtual const EShaderInstance*	ResolveFallbackInstance(const UINT32 InSlotIndex) const;
		PE_NODISCARD virtual const RInputLayoutDesc*	GetPipelineStateInputLayouts(UINT32& OutCount) const;
		PE_NODISCARD virtual UINT32	GetDrawGroupNum() const;
		PE_NODISCARD virtual BOOL32	GetDrawGroup(const UINT32 InGroupIndex, UINT32& OutSlotIndex, UINT32& OutIndexCount, UINT32& OutStartIndex) const;
		// Prepare: resolve the vertex and index binding this pass's program needs. No command
		// is recorded here; the result is replayed by BindGeometry at record time.
		virtual BOOL32	ResolveGeometryBinding(FPrimitivePassData& InOutData) const;
		// Record: replay the resolved geometry binding.
		PE_NODISCARD virtual BOOL32	BindGeometry(IRCommandList* InCommandList, const FPrimitivePassData& InData) const;
		// Anything a subclass binds in addition to the shared set, after the geometry.
		virtual BOOL32	BindExtraResources(IRCommandList* InCommandList, const FPrimitivePassData& InData) const;
		// Record: resources that belong to the pass or the view rather than to the material,
		// bound after the material and its extra resources and before the draw. A forward pass
		// that shades uses it for the light lists and the environment, whose binding plan slots
		// differ from the deferred lighting pass's. TRUE when it wrote a descriptor table of its
		// own, which tells the record path that the next draw cannot reuse the material bindings.
		virtual BOOL32	BindPassResources(IRCommandList* InCommandList, const FPassRecordContext& InContext
			, const FPrimitivePassData& InData) const;
		// The draw itself. The default is one indexed draw of the group.
		PE_NODISCARD virtual BOOL32	DrawGeometry(IRCommandList* InCommandList, const FPrimitivePassData& InData
			, const UINT32 InIndexCount, const UINT32 InStartIndex) const;
	protected:
		BOOL32							IsMovable;
		BOOL32							IsCastShadow;
		BOOL32							IsReceiveShadow;
		UINT64							PrimitiveCategoryMask;
		ObjectIdentityType				PrimitiveIdentity;
		FPrimitiveRenderAssetReport		Assets;
		const EShaderInstance*			ReportedPrimaryMaterial = nullptr;
		FShaderValueReport				LastDrawValues;
		BOOL32							bHasDrawValues;
		mutable RShaderParameterBlock	DrawParameterBlock{ EName(SHADER_BLOCK_DRAW_PARAMS) };
		// Heap allocated so a later slot insertion cannot move the data an already prepared
		// FPassItem points at.
		TArray<TUniquePtr<FPrimitivePassData>>	PassSlots;
		RENDER_PROXY_CLASS_BODY(RPrimitiveSceneProxy)

	};

}	// namespace PigeonEngine
