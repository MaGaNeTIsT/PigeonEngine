#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"
#include <RenderResource.h>
#include <RenderDevice/DeviceD3D11.h>
#include <Shader/ShaderEngineParameters.h>
#include <Shader/ShaderParameter.h>
#include <Shader/ShaderParameterBuffers.h>
#include <Renderer/ScenePass.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveRenderData.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveComponent.h>

namespace PigeonEngine
{

	class EShaderInstance;
	class EShaderProgram;
	class EShaderAsset;
	class IRCommandList;

	/*
	* One material slot resolved for one pass: the instance the primitive reported, the
	* program it resolved to, the pipeline state that program runs with, and the constant
	* buffers of the material's values.
	*
	* The slot is built on demand, because a pass only needs the material of the passes it
	* actually runs, and rebuilt when the reported asset changes. Its pipeline state comes from
	* the frame's cache, so the root signature it was created with is the one the binding plan
	* of the same program reports.
	*/
	struct RShaderPassSlot
	{
		UINT32						SlotIndex		= 0u;
		EName						PassName;
		const EShaderInstance*		Material		= nullptr;
		// The object the material answered with when this slot was built. A reload publishes a new
		// asset object for the same material, and everything below was derived from the old one -
		// the program, the pipeline state built from its reflection, the buffers laid out from its
		// binding plan - so the slot is only usable while this still matches the instance's asset.
		const EShaderAsset*			MaterialAsset	= nullptr;
		const EShaderProgram*		Program			= nullptr;
		IRRHIPipelineState*			PipelineState	= nullptr;
		RShaderParameterBuffers		MaterialBuffers;
	};

	/*
	* What every primitive proxy shares: the engine blocks it binds, the pass slots it resolves
	* its materials into, and the draw sequence itself.
	*
	* A subclass supplies what only it knows - which geometry it has, which streams that
	* geometry lives in, how its assets resolve to a material - through the hooks below. The
	* sequence around those hooks is here, once, so a new primitive cannot get the binding
	* order wrong: pipeline state first, then the engine blocks, then the material, then the
	* geometry, then the draw.
	*
	* No root index and no register is written down in this class. Every block is bound at the
	* root the current program's binding plan reports, which is also why the engine blocks are
	* bound per draw rather than once per frame: switching a pipeline state switches its root
	* signature, and that voids every root binding made before it.
	*/
	class RPrimitiveSceneProxy : public RBaseSceneProxy
	{
	public:
		virtual BOOL32	IsRenderValid()const;
		BOOL32			IsSceneProxyMovable()const;
		BOOL32			IsSceneProxyCastShadow()const;
		BOOL32			IsSceneProxyReceiveShadow()const;
	public:
		void			SetPrimitiveSettings(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow);
		void			UpdatePrimitiveMatrices(const ERenderPrimitiveMatrices& InMatrices);
	public:
		// Render thread: install a whole reported snapshot. The create path calls this; the
		// update path calls the two halves below, because a snapshot that only carries one
		// channel must not rebuild the other.
		void			SetupProxy(const FPrimitiveRenderSnapshot& InSnapshot);
		// Render thread: install what the game thread reported. The assets one rebuilds the
		// pass slots, the values one only rewrites the numbers, which is the whole reason the
		// two are separate calls.
		virtual void	ApplyAssets(const FPrimitiveRenderAssetReport& InAssets);
		virtual void	ApplyValues(const FShaderValueReport& InValues);
		// Runtime payload that is neither an asset nor a shader value, such as a skinned
		// primitive's bone matrices. The default ignores it, which is what a primitive without
		// such state does.
		virtual void	ApplyRuntimePayload(const FPrimitiveRenderSnapshot& InSnapshot);
		// The component this proxy was reported by. The scene maps primitives by it, so it can
		// never be the proxy's own object identity: the proxy is built on the render thread and
		// its identity is unrelated to the component's.
		PE_NODISCARD const ObjectIdentityType&	GetPrimitiveIdentity()const { return PrimitiveIdentity; }
	public:
		// Records this primitive into the pass currently being recorded. The parameter-less
		// pair is what the renderer calls today; the pass taking one is where it is heading.
		void			BindRenderResource()const;
		void			Draw();
		void			Draw(IRCommandList* InCommandList, const FRenderPassDesc& InPassDesc);
		// A draw that cannot run is otherwise silent: the object stops appearing and nothing in the
		// frame says why. This reports one line per (primitive, pass, reason), so an object that is
		// found not to be drawing costs one line rather than one per frame.
		//
		// The reason belongs to whoever knows it, so a hook that fails reports its own: a proxy
		// whose BindGeometry fails says which asset is missing rather than leaving the caller to
		// guess from a boolean.
		void			ReportSkippedDraw(const EName& InPassName, const EString& InReason)const;
		// A primitive that has compute work of its own overrides this. The default submits
		// nothing, which is what a graphics-only primitive does.
		virtual BOOL32	RecordCompute(IRCommandList* InCommandList, const FRenderPassDesc& InPassDesc)const;
	protected:
		// The per draw block, published from the reported values and bound at the root the
		// program's plan reports.
		BOOL32			EnsureDrawParameterBlock(const EShaderProgram& InProgram);
		void			UpdateDrawParameters();
		// Writes one value report into the draw block and uploads it. Kept apart from
		// ApplyValues so the stored report can be replayed without being stored again.
		void			WriteDrawValues(const FShaderValueReport& InValues);
		// Publishes the per draw block from this proxy's own local to world matrix. The normal
		// source of the block is the reported value report; this is the bridge for a proxy whose
		// own update path still drives the transform, so a moving primitive cannot end up drawn
		// from a stale block.
		void			PublishDrawParametersFromTransform();
		void			BindEngineParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram)const;
		// The material half of a bind: the values are pushed into the slot's buffers, the
		// buffers are bound at the roots the plan reports, and the resource values (textures,
		// buffers) are bound through the same plan.
		BOOL32			BindMaterial(IRCommandList* InCommandList, const RShaderPassSlot& InSlot)const;
	protected:
		// Pass slots. Built on demand per (slot, pass), invalidated when the assets change.
		PE_NODISCARD const RShaderPassSlot*	FindPassSlot(const UINT32 InSlotIndex, const EName& InPassName)const;
		const RShaderPassSlot*				EnsurePassSlot(const UINT32 InSlotIndex, const EName& InPassName, const FRenderPassDesc& InPassDesc);
		// Drops one built slot, which releases the material buffers it owns. Used when the material's
		// asset object was replaced under it, and it is the single-slot form of ReleasePassSlots.
		void								DropPassSlot(const UINT32 InSlotIndex, const EName& InPassName);
		void								ReleasePassSlots();
	protected:
		// What a subclass supplies. All defaults describe a primitive with nothing to draw,
		// so a partially migrated subclass is inert rather than wrong.
		//
		// The material a slot with no reported instance falls back to. Null means "no
		// fallback", and the slot is then skipped.
		PE_NODISCARD virtual const EShaderInstance*	ResolveFallbackInstance(const UINT32 InSlotIndex)const;
		// The vertex data declarations the pass's pipeline state has to carry on top of the
		// ones the shader reflection gives. A stream whose DATA format differs from what the
		// shader declared belongs here, and the skinned index stream is the reason this hook
		// exists: it is UINT16 x 4 bytes while the reflection reports uint4.
		PE_NODISCARD virtual const RInputLayoutDesc*	GetPipelineStateInputLayouts(UINT32& OutCount)const;
		// The draw groups of this primitive: one per submesh for a mesh, one covering
		// everything for a procedural primitive.
		PE_NODISCARD virtual UINT32	GetDrawGroupNum()const;
		PE_NODISCARD virtual BOOL32	GetDrawGroup(const UINT32 InGroupIndex, UINT32& OutSlotIndex, UINT32& OutIndexCount, UINT32& OutStartIndex)const;
		// Binds the vertex and index data of one group. FALSE skips the draw.
		PE_NODISCARD virtual BOOL32	BindGeometry(IRCommandList* InCommandList, const RShaderPassSlot& InSlot)const;
		// Anything a subclass binds in addition to the shared set, after the geometry.
		virtual void	BindExtraResources(IRCommandList* InCommandList, const RShaderPassSlot& InSlot)const;
		// The draw itself. The default is one indexed draw of the group.
		PE_NODISCARD virtual BOOL32	DrawGeometry(IRCommandList* InCommandList, const RShaderPassSlot& InSlot, const UINT32 InIndexCount, const UINT32 InStartIndex)const;
	protected:
		BOOL32							IsMovable;
		BOOL32							IsCastShadow;
		BOOL32							IsReceiveShadow;
		// Identity of the reporting component. It is what the scene registers this proxy under
		// and looks it up by, so it has to come from the snapshot rather than from the proxy.
		ObjectIdentityType				PrimitiveIdentity;
		// The render side copy of what the component reported. Pointers stay game side asset
		// pointers: they are resolved, never owned, on this thread.
		FPrimitiveRenderAssetReport		Assets;
		// The last value report. A primitive without an authored material only resolves the
		// program to lay the block out with when it draws - the engine fallback - so a report
		// that arrived before that had no layout to land in. Keeping it is what lets the block
		// be published from it the moment the layout exists, instead of dropping the values
		// until the game side happens to report them again.
		FShaderValueReport				LastDrawValues;
		BOOL32							bHasDrawValues;
		// The per draw block of this primitive. Named at construction: the name is the block's
		// identity, so there is no unnamed state for a later edit to leave it in (an unnamed
		// block never initializes, uploads or binds).
		RShaderParameterBlock			DrawParameterBlock{ EName(SHADER_BLOCK_DRAW_PARAMS) };
		TArray<RShaderPassSlot>			PassSlots;

		RENDER_PROXY_CLASS_BODY(RPrimitiveSceneProxy)

	};

};
