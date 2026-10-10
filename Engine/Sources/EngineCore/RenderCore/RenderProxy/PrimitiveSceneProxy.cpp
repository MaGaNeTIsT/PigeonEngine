#include "PrimitiveSceneProxy.h"
#include <Renderer/RenderScene.h>
#include <Renderer/SceneRenderer.h>
#include <RenderProxy/ViewProxy.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderProgram.h>
#include <Shader/ShaderAssetBinding.h>
#include <RHI/IRCommandList.h>
#include <RHI/IRRHIDevice.h>
#include <Main/MainManager.h>
#include <Development/Profiler.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RPrimitiveSceneProxy, RBaseSceneProxy>();
	}

	void RPrimitiveSceneProxy::ReportSkippedDraw(const EName& InPassName, const EString& InReason)const
	{
		// The component's identity rather than this proxy's address: a proxy is rebuilt from a
		// snapshot, and the identity is what a reader can match against the scene. The report
		// itself is the draw path's shared one, which is what deduplicates it by owner and reason.
		PigeonEngine::ReportSkippedDraw(EString(PE_TEXT("primitive "))
			+ ToString(static_cast<UINT64>(GetPrimitiveIdentity()))
			+ EString(PE_TEXT(" pass '")) + InPassName.ToString() + EString(PE_TEXT("'")), InReason);
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

RPrimitiveSceneProxy::RPrimitiveSceneProxy()
	: IsMovable(FALSE), IsCastShadow(FALSE), IsReceiveShadow(FALSE), bHasDrawValues(FALSE)
{
}
RPrimitiveSceneProxy::RPrimitiveSceneProxy(const RPrimitiveSceneProxy& Other)
	: RBaseSceneProxy(Other), IsMovable(Other.IsMovable), IsCastShadow(Other.IsCastShadow)
	, IsReceiveShadow(Other.IsReceiveShadow), Assets(Other.Assets), bHasDrawValues(FALSE)
	, DrawParameterBlock(Other.DrawParameterBlock)
{
	// The pass slots own render tier buffers and the draw block owns a constant buffer of
	// its own, so neither is copied: a copy rebuilds them from the reported assets the
	// first time it draws. The block's name travels with the copy, so the rebuilt buffers
	// land on the same block the prototype published. The value report is per component, so
	// a prototype's report is not the copy's to publish.
}
	RPrimitiveSceneProxy::~RPrimitiveSceneProxy()
	{
	}
	BOOL32 RPrimitiveSceneProxy::IsRenderValid()const
	{
		return FALSE;
	}
	BOOL32 RPrimitiveSceneProxy::IsSceneProxyMovable()const
	{
		return IsMovable;
	}
	BOOL32 RPrimitiveSceneProxy::IsSceneProxyCastShadow()const
	{
		return IsCastShadow;
	}
	BOOL32 RPrimitiveSceneProxy::IsSceneProxyReceiveShadow()const
	{
		return IsReceiveShadow;
	}
	void RPrimitiveSceneProxy::SetPrimitiveSettings(const BOOL32 InIsMovable, const BOOL32 InIsCastShadow, const BOOL32 InIsReceiveShadow)
	{
		IsMovable		= InIsMovable;
		IsCastShadow	= InIsCastShadow;
		IsReceiveShadow	= InIsReceiveShadow;
	}
	void RPrimitiveSceneProxy::UpdatePrimitiveMatrices(const ERenderPrimitiveMatrices& InMatrices)
	{
		SetProxyWorldTransform(InMatrices.WorldLocation, InMatrices.WorldRotation, InMatrices.WorldScaling);
	}

	// ---------------------------------------------------------------------------
	// Reported state
	// ---------------------------------------------------------------------------

	void RPrimitiveSceneProxy::SetupProxy(const FPrimitiveRenderSnapshot& InSnapshot)
	{
		PrimitiveIdentity = InSnapshot.Identity;
		SetPrimitiveSettings(InSnapshot.bMovable, InSnapshot.bCastShadow, InSnapshot.bReceiveShadow);
		SetProxyWorldTransform(InSnapshot.WorldLocation, InSnapshot.WorldRotation, InSnapshot.WorldScaling);
		// Assets first: the per draw block takes its layout from the program the reported
		// material resolves to, and the values below cannot be stored before that layout
		// exists.
		ApplyAssets(InSnapshot.Assets);
		ApplyValues(InSnapshot.Values);
		ApplyRuntimePayload(InSnapshot);
	}
	void RPrimitiveSceneProxy::ApplyRuntimePayload(const FPrimitiveRenderSnapshot& InSnapshot)
	{
		// A primitive with no runtime payload of its own has nothing to do here.
		(void)InSnapshot;
	}
	void RPrimitiveSceneProxy::ApplyAssets(const FPrimitiveRenderAssetReport& InAssets)
	{
		Assets = InAssets;

		// The pass slots were resolved from the previous report, so they are gone.
		ReleasePassSlots();

		// The per draw block takes its layout from the program that will read it, so the first
		// reported instance that resolves to a program supplies it. A report with no instance
		// at all leaves the block uninitialized rather than latching an empty layout, and a
		// later report can still supply one.
		for (UINT32 SlotIndex = 0u, SlotNum = Assets.GetMaterialSlotNum(); SlotIndex < SlotNum; SlotIndex++)
		{
			const FPrimitiveMaterialSlotReport* SlotReport = Assets.GetMaterialSlot(SlotIndex);
			if (!SlotReport)
			{
				continue;
			}
			for (UINT32 PassIndex = 0u, PassNum = SlotReport->Passes.Num<UINT32>(); PassIndex < PassNum; PassIndex++)
			{
				const EShaderInstance* Instance = SlotReport->Passes[PassIndex].Instance;
				const EShaderProgram* Program = (Instance ? Instance->GetProgram() : nullptr);
				if (Program)
				{
					EnsureDrawParameterBlock(*Program);
					break;
				}
			}
			if (DrawParameterBlock.IsInitialized())
			{
				break;
			}
		}
	}
	void RPrimitiveSceneProxy::ApplyValues(const FShaderValueReport& InValues)
	{
		// The report is kept whether or not the block can take it yet: a primitive that draws
		// through the engine fallback only learns the layout at draw time, and this is what the
		// block is published from then.
		LastDrawValues = InValues;
		bHasDrawValues = ((InValues.IsEmpty() != FALSE) ? FALSE : TRUE);

		if (!DrawParameterBlock.IsInitialized())
		{
			// A set with no mirrors cannot hold a value, and the layout comes from a program
			// the primitive has not reported yet. Nothing is lost: the report above is what
			// the block is published from as soon as that layout exists.
			return;
		}

		WriteDrawValues(InValues);
	}
	void RPrimitiveSceneProxy::WriteDrawValues(const FShaderValueReport& InValues)
	{
		EShaderParameterSet& Parameters = DrawParameterBlock.GetParameters();
		const EName& DrawBlockName = DrawParameterBlock.GetBlockName();
		for (UINT32 Index = 0u, Num = InValues.GetValueNum(); Index < Num; Index++)
		{
			const FShaderValueReportEntry* Entry = InValues.GetValueByIndex(Index);
			if (!Entry)
			{
				continue;
			}
			if (Entry->BlockName != DrawBlockName)
			{
				// The draw block is the only block this proxy publishes, so a value addressed to
				// another one belongs to whoever owns that block (a proxy that grows a block of
				// its own gives it an RShaderParameterBlock and reports through that). Writing it
				// here would land in an override no mirror can take, which is why it is dropped.
				continue;
			}
			// The kind decides the setter; the names are the ones the shader source declares,
			// so a name this program does not have becomes a write that binds nothing.
			switch (Entry->Kind)
			{
			case EPrimitiveValueKindType::PRIMITIVE_VALUE_SCALAR:
				WriteShaderScalar(Parameters, Entry->BlockName, Entry->FieldName, Entry->Words.Float32[0u]);
				break;
			case EPrimitiveValueKindType::PRIMITIVE_VALUE_VECTOR:
				WriteShaderVector4(Parameters, Entry->BlockName, Entry->FieldName
					, Vector4(Entry->Words.Float32[0u], Entry->Words.Float32[1u], Entry->Words.Float32[2u], Entry->Words.Float32[3u]));
				break;
			case EPrimitiveValueKindType::PRIMITIVE_VALUE_MATRIX:
				// Already row major and not transposed by the reporter, which is the upload
				// convention the shader library expects.
				Parameters.SetFloatArray(Entry->BlockName, Entry->FieldName, &(Entry->Words.Float32[0u]), PRIMITIVE_VALUE_WORD_MAX);
				break;
			case EPrimitiveValueKindType::PRIMITIVE_VALUE_UINT:
				WriteShaderUIntArray(Parameters, Entry->BlockName, Entry->FieldName, &(Entry->Words.UInt32[0u]), 1u);
				break;
			case EPrimitiveValueKindType::PRIMITIVE_VALUE_UINT4:
				WriteShaderUIntArray(Parameters, Entry->BlockName, Entry->FieldName, &(Entry->Words.UInt32[0u]), 4u);
				break;
			default:
				break;
			}
		}

		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		DrawParameterBlock.Upload(RenderDevice);
	}

	// ---------------------------------------------------------------------------
	// Engine blocks
	// ---------------------------------------------------------------------------

	BOOL32 RPrimitiveSceneProxy::EnsureDrawParameterBlock(const EShaderProgram& InProgram)
	{
		if (DrawParameterBlock.IsInitialized())
		{
			return TRUE;
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!DrawParameterBlock.EnsureInitialized(RenderDevice, InProgram))
		{
			return FALSE;
		}
		// The block has just acquired a layout, so the values recorded while it had none have
		// never reached a buffer; this publishes them for the first time. The transform is the
		// default, and a report that did arrive is written on top of it, because a value the
		// game side authored outranks the transform it reported alongside.
		UpdateDrawParameters();
		PublishDrawParametersFromTransform();
		if (bHasDrawValues != FALSE)
		{
			WriteDrawValues(LastDrawValues);
		}
		return TRUE;
	}
	void RPrimitiveSceneProxy::UpdateDrawParameters()
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		DrawParameterBlock.Upload(RenderDevice);
	}
	void RPrimitiveSceneProxy::PublishDrawParametersFromTransform()
	{
		if (!DrawParameterBlock.IsInitialized())
		{
			// No program has declared the block yet, so there is no layout to write into. The
			// next update publishes from the value report, which is the normal source.
			return;
		}

		EShaderParameterSet& Parameters = DrawParameterBlock.GetParameters();
		const Matrix4x4& LocalToWorld = GetLocalToWorldMatrix();
		const Matrix4x4 InverseWorld(LocalToWorld.Inverse());

		// The same convention the component side uses: written as the CPU holds them, never
		// transposed, because the shader side matrix is the transpose of these. The inverse
		// transpose carries a transpose in its value, which is what the normal transform is.
		const EName BlockName(SHADER_BLOCK_DRAW_PARAMS);
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_WORLD_MATRIX), LocalToWorld);
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_WORLD_INV_MATRIX), InverseWorld);
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_WORLD_INV_TRANSPOSE_MATRIX), InverseWorld.Transpose());

		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		DrawParameterBlock.Upload(RenderDevice);
	}
	void RPrimitiveSceneProxy::BindEngineParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram)const
	{
		if (!InCommandList)
		{
			return;
		}
		// Everything here runs after SetPipelineState on purpose: a pipeline state brings its
		// own root signature, and switching a root signature voids every root binding made
		// before it. Each block looks its root up in the program's binding plan, so none of
		// these calls carries a root index of its own. The shared blocks are the renderer's, so
		// the renderer binds them and this draw adds its own block on top.
		RSceneRenderer* Renderer = EMainManager::GetManagerSingleton()->GetSceneRenderer();
		if (Renderer)
		{
			Renderer->BindEngineParameters(InCommandList, InProgram);
		}
		DrawParameterBlock.Bind(InCommandList, InProgram);
	}
	BOOL32 RPrimitiveSceneProxy::BindMaterial(IRCommandList* InCommandList, const RShaderPassSlot& InSlot)const
	{
		if ((!InCommandList) || (!InSlot.Material) || (!InSlot.Program))
		{
			return FALSE;
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice)
		{
			return FALSE;
		}

		// The values are re-evaluated on the asset side first, then the dirty mirrors are
		// pushed into this slot's gpu buffers, which is what the bind reads.
		RShaderParameterBuffers& ParameterBuffers = const_cast<RShaderParameterBuffers&>(InSlot.MaterialBuffers);
		InSlot.Material->Prepare();
		ParameterBuffers.UploadFromParameterSet(RenderDevice, InSlot.Material->GetParameters());

		BindShaderParameterBuffers(InCommandList, ParameterBuffers
			, *InSlot.Program, EShaderStageType::SHADER_STAGE_VERTEX, FALSE);
		BindShaderParameterBuffers(InCommandList, ParameterBuffers
			, *InSlot.Program, EShaderStageType::SHADER_STAGE_PIXEL, FALSE);

		const EShaderStageEntry* VertexEntry = ResolveShaderStageEntry(*InSlot.Program, EShaderStageType::SHADER_STAGE_VERTEX);
		if (VertexEntry)
		{
			BindShaderParameterResources(InCommandList, InSlot.Material->GetParameters()
				, VertexEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_VERTEX);
		}
		const EShaderStageEntry* PixelEntry = ResolveShaderStageEntry(*InSlot.Program, EShaderStageType::SHADER_STAGE_PIXEL);
		if (PixelEntry)
		{
			BindShaderParameterResources(InCommandList, InSlot.Material->GetParameters()
				, PixelEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_PIXEL);
		}
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// Pass slots
	// ---------------------------------------------------------------------------

	const RShaderPassSlot* RPrimitiveSceneProxy::FindPassSlot(const UINT32 InSlotIndex, const EName& InPassName)const
	{
		for (UINT32 Index = 0u, Num = PassSlots.Num<UINT32>(); Index < Num; Index++)
		{
			if ((PassSlots[Index].SlotIndex == InSlotIndex) && (PassSlots[Index].PassName == InPassName))
			{
				return &(PassSlots[Index]);
			}
		}
		return nullptr;
	}
	const RShaderPassSlot* RPrimitiveSceneProxy::EnsurePassSlot(const UINT32 InSlotIndex, const EName& InPassName, const FRenderPassDesc& InPassDesc)
	{
		if (const RShaderPassSlot* Existing = FindPassSlot(InSlotIndex, InPassName))
		{
			// Everything a slot holds was derived from the material's asset object: the program,
			// the pipeline state built from that program's reflection, the binding plan inside it,
			// the constant buffers laid out from it. A reload publishes a NEW object for the same
			// material, so a slot built from the previous one is dropped and rebuilt below. The
			// instance is the stable handle, and it is what reports which object is current.
			if (Existing->MaterialAsset == Existing->Material->GetAsset())
			{
				return Existing;
			}
			DropPassSlot(InSlotIndex, InPassName);
		}

		// A slot with no reported instance falls back to this primitive's engine material, so
		// geometry is never dropped merely because nothing was authored.
		const EShaderInstance* Instance = Assets.FindMaterialInstance(InSlotIndex, InPassName);
		if (!Instance)
		{
			Instance = ResolveFallbackInstance(InSlotIndex);
		}
		if (!Instance)
		{
			// Quiet: this is the "nothing assigned yet, and the fallback is not assembled yet"
			// state, which is what every primitive looks like for the first frames of a load.
			return nullptr;
		}
		const EShaderProgram* Program = Instance->GetProgram();
		if (!Program)
		{
			// Not quiet, because the instance exists: the asset has no usable program for this
			// variant, which is what a replaced stage or a failed assembly leaves behind, and the
			// object it belongs to would otherwise just stop appearing.
			ReportSkippedDraw(InPassName, EString(PE_TEXT("its material instance resolved to no usable program")));
			return nullptr;
		}

		RSceneRenderer* Renderer = EMainManager::GetManagerSingleton()->GetSceneRenderer();
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if ((!Renderer) || (!RenderDevice))
		{
			return nullptr;
		}

		RShaderPassSlot NewSlot;
		NewSlot.SlotIndex	= InSlotIndex;
		NewSlot.PassName	= InPassName;
		NewSlot.Material	= Instance;
		// The object the program below was resolved from, not the one the instance is bound to right
		// now: a rebind on the load thread could land between the two reads, and a slot that mixed
		// the two would look current while holding the replaced program.
		NewSlot.MaterialAsset = Instance->GetResolvedAsset();
		NewSlot.Program		= Program;

		// The per draw block takes its layout from the program this draw runs, which is the
		// only place a primitive without an authored material can supply one: its slot resolves
		// to the engine fallback here, and a block that never got a layout binds nothing, so
		// the draw would run with the world matrix its statement defaults to.
		EnsureDrawParameterBlock(*Program);

		// The pipeline state carries the pass's fixed function state, this program's shaders
		// and the roots generated from this program's own reflection. The extra input layout
		// declarations are the data side of the vertex streams, which is what a stream whose
		// real format differs from what the shader declared needs.
		RGraphicsPipelineStateDesc Desc;
		FRenderPassRegistry::FillPipelineStateDesc(InPassDesc, Program, Desc);
		UINT32 InputLayoutNum = 0u;
		Desc.InputLayouts = GetPipelineStateInputLayouts(InputLayoutNum);
		Desc.InputLayoutCount = InputLayoutNum;
		NewSlot.PipelineState = Renderer->GetPipelineStateCache().GetOrCreateGraphics(Desc);
		if (!NewSlot.PipelineState)
		{
			// The other way a program with a missing stage shows up: the pipeline state cannot be
			// built, so the primitive stops appearing and nothing else in the frame says so. It can
			// be a single frame while a device object is still queued, which the report survives --
			// one line per primitive and reason, not one per frame.
			ReportSkippedDraw(InPassName, EString(PE_TEXT("its pipeline state could not be built for program '"))
				+ Program->GetShaderName().ToString() + EString(PE_TEXT("'")));
			return nullptr;
		}

		// The values stay on the asset side; these are the buffers they are uploaded into.
		Instance->Prepare();
		NewSlot.MaterialBuffers.CreateFromParameterSet(RenderDevice, Instance->GetParameters());

		PassSlots.Add(NewSlot);
		// The returned pointer stays valid until the next EnsurePassSlot call, which is all a
		// caller needs: the slot is consumed by the draw it was resolved for.
		return &(PassSlots[PassSlots.Num<UINT32>() - 1u]);
	}
	void RPrimitiveSceneProxy::DropPassSlot(const UINT32 InSlotIndex, const EName& InPassName)
	{
		for (UINT32 Index = 0u, Num = PassSlots.Num<UINT32>(); Index < Num; Index++)
		{
			if ((PassSlots[Index].SlotIndex != InSlotIndex) || (PassSlots[Index].PassName != InPassName))
			{
				continue;
			}
			// RemoveAt destroys the element it removes, and that is what releases the material
			// buffers the slot owned - they were laid out from the program being replaced.
			//
			// The per draw block goes with it: its layout was read from the same program's
			// reflection, and the rebuild that follows this call re-initializes it against the
			// current one.
			DrawParameterBlock.ReleaseRenderResources();
			PassSlots.RemoveAt(Index);
			return;
		}
	}
	void RPrimitiveSceneProxy::ReleasePassSlots()
	{
		// The slots own render tier constant buffers, so emptying the array releases them on
		// this thread, which is the only thread that may touch them.
		PassSlots.Empty();
	}

	// ---------------------------------------------------------------------------
	// Draw
	// ---------------------------------------------------------------------------

	void RPrimitiveSceneProxy::BindRenderResource()const
	{
		// Everything a draw binds is bound inside Draw, right after the pipeline state that
		// would otherwise void it. The hook stays because the renderer calls it first.
	}
	void RPrimitiveSceneProxy::Draw()
	{
		IRCommandList* CmdList = EMainManager::GetManagerSingleton()->GetActiveCommandList();
		if (!CmdList)
		{
			return;
		}
		Draw(CmdList, FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_BASE));
	}
	void RPrimitiveSceneProxy::Draw(IRCommandList* InCommandList, const FRenderPassDesc& InPassDesc)
	{
		if ((!InCommandList) || (InPassDesc.PassName == nullptr))
		{
			return;
		}
		const EName PassName(InPassDesc.PassName);
		for (UINT32 GroupIndex = 0u, GroupNum = GetDrawGroupNum(); GroupIndex < GroupNum; GroupIndex++)
		{
			UINT32 SlotIndex = 0u;
			UINT32 IndexCount = 0u;
			UINT32 StartIndex = 0u;
			if (!GetDrawGroup(GroupIndex, SlotIndex, IndexCount, StartIndex))
			{
				// Quiet on purpose: a group that cannot be read is the "its geometry is not loaded
				// yet" state, which every primitive passes through once on the frame its asset
				// lands. Reporting it would bury the reasons that mean something.
				continue;
			}
			const RShaderPassSlot* Slot = EnsurePassSlot(SlotIndex, PassName, InPassDesc);
			if ((!Slot) || (!Slot->PipelineState) || (!Slot->Program))
			{
				if (Slot)
				{
					// EnsurePassSlot reports the reasons it can see; a slot that exists but is
					// incomplete is the one it hands back, so this is where that one is reported.
					ReportSkippedDraw(PassName, EString(PE_TEXT("its pass slot has no program or no pipeline state")));
				}
				continue;
			}

			const EString EventName = GetShaderInstanceDisplayName(Slot->Material);
			PE_GPU_EVENT_SCOPE(InCommandList, EventName);

			InCommandList->SetPipelineState(Slot->PipelineState);
			if (!BindMaterial(InCommandList, *Slot))
			{
				ReportSkippedDraw(PassName, EString(PE_TEXT("its material parameters could not be bound")));
				continue;
			}
			BindEngineParameters(InCommandList, *Slot->Program);
			if (!BindGeometry(InCommandList, *Slot))
			{
				// Reported by the hook rather than here: that is where the reason is known, and
				// the mesh proxy names the stage or the mesh resource it could not bind.
				continue;
			}
			BindExtraResources(InCommandList, *Slot);
			if (!DrawGeometry(InCommandList, *Slot, IndexCount, StartIndex))
			{
				ReportSkippedDraw(PassName, EString(PE_TEXT("its geometry could not be submitted")));
				continue;
			}
		}
	}
	BOOL32 RPrimitiveSceneProxy::RecordCompute(IRCommandList* InCommandList, const FRenderPassDesc& InPassDesc)const
	{
		// A graphics only primitive submits no compute work.
		(void)InCommandList;
		(void)InPassDesc;
		return FALSE;
	}

	// ---------------------------------------------------------------------------
	// Hooks: a primitive that overrides none of these draws nothing.
	// ---------------------------------------------------------------------------

	const EShaderInstance* RPrimitiveSceneProxy::ResolveFallbackInstance(const UINT32 InSlotIndex)const
	{
		(void)InSlotIndex;
		return nullptr;
	}
	const RInputLayoutDesc* RPrimitiveSceneProxy::GetPipelineStateInputLayouts(UINT32& OutCount)const
	{
		OutCount = 0u;
		return nullptr;
	}
	UINT32 RPrimitiveSceneProxy::GetDrawGroupNum()const
	{
		return 0u;
	}
	BOOL32 RPrimitiveSceneProxy::GetDrawGroup(const UINT32 InGroupIndex, UINT32& OutSlotIndex, UINT32& OutIndexCount, UINT32& OutStartIndex)const
	{
		(void)InGroupIndex;
		OutSlotIndex = 0u;
		OutIndexCount = 0u;
		OutStartIndex = 0u;
		return FALSE;
	}
	BOOL32 RPrimitiveSceneProxy::BindGeometry(IRCommandList* InCommandList, const RShaderPassSlot& InSlot)const
	{
		(void)InCommandList;
		(void)InSlot;
		return FALSE;
	}
	void RPrimitiveSceneProxy::BindExtraResources(IRCommandList* InCommandList, const RShaderPassSlot& InSlot)const
	{
		(void)InCommandList;
		(void)InSlot;
	}
	BOOL32 RPrimitiveSceneProxy::DrawGeometry(IRCommandList* InCommandList, const RShaderPassSlot& InSlot, const UINT32 InIndexCount, const UINT32 InStartIndex)const
	{
		(void)InSlot;
		if (!InCommandList)
		{
			return FALSE;
		}
		// One indexed draw of the group. BaseVertex MUST stay 0: the mesh import path writes
		// absolute vertex indices into the merged index buffer, so the start index alone
		// picks the right slice and a base vertex would double offset it.
		InCommandList->DrawIndexed(InIndexCount, 1u, InStartIndex, 0, 0u);
		return TRUE;
	}

};
