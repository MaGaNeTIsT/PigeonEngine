#include "PrimitiveSceneProxy.h"
#include <Renderer/RenderScene.h>
#include <Renderer/SceneRenderer.h>
#include <Pipeline/RPipelineStateCache.h>
#include <RenderProxy/ViewProxy.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderProgram.h>
#include <Shader/ShaderAssetBinding.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <RHI/IRCommandList.h>
#include <RHI/IRRHIDevice.h>
#include <Main/MainManager.h>
#include <Config/EngineConfig.h>
#include <Development/Profiler.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RPrimitiveSceneProxy, RBaseSceneProxy>();
	}

	// The pass a shader instance is authored for. A different pass derives its own instance
	// from the base instance the view selected, so this name is the input of every derivation.
	static const EName& GetPrimaryPassName()
	{
		static const EName PrimaryPassName =
			((FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_BASE).PassName != nullptr)
				? EName(FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_BASE).PassName)
				: EName(PE_TEXT("BasePass")));
		return PrimaryPassName;
	}

	// The frame the renderer is recording. A pending pass slot retries its instance once per
	// frame, and the device's own counter is the frame boundary without any state to reset.
	static UINT32 GetRenderFrameIndex()
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		return ((RenderDevice != nullptr) ? RenderDevice->GetCurrentFrameIndex() : 0u);
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RPrimitiveSceneProxy::RPrimitiveSceneProxy()
		: IsMovable(FALSE), IsCastShadow(FALSE), IsReceiveShadow(FALSE), PrimitiveCategoryMask(0ull), bHasDrawValues(FALSE)
	{
	}
	RPrimitiveSceneProxy::RPrimitiveSceneProxy(const RPrimitiveSceneProxy& Other)
		: RBaseSceneProxy(Other), IsMovable(Other.IsMovable), IsCastShadow(Other.IsCastShadow)
		, IsReceiveShadow(Other.IsReceiveShadow), PrimitiveCategoryMask(Other.PrimitiveCategoryMask)
		, Assets(Other.Assets), ReportedPrimaryMaterial(Other.ReportedPrimaryMaterial), bHasDrawValues(FALSE)
		, DrawParameterBlock(Other.DrawParameterBlock)
	{
		// The pass slots own render tier buffers and the draw block owns a constant buffer of
		// its own, so neither is copied: a copy rebuilds them from the reported assets the
		// first time it draws. The block's name travels with the copy, so the rebuilt buffers
		// land on the same block the prototype published.
	}
	RPrimitiveSceneProxy::~RPrimitiveSceneProxy()
	{
	}
	BOOL32 RPrimitiveSceneProxy::IsRenderValid() const
	{
		return FALSE;
	}
	BOOL32 RPrimitiveSceneProxy::IsSceneProxyMovable() const
	{
		return IsMovable;
	}
	BOOL32 RPrimitiveSceneProxy::IsSceneProxyCastShadow() const
	{
		return IsCastShadow;
	}
	BOOL32 RPrimitiveSceneProxy::IsSceneProxyReceiveShadow() const
	{
		return IsReceiveShadow;
	}
	BOOL32 RPrimitiveSceneProxy::ShouldCollectInView(const RViewProxy& InView) const
	{
		(void)InView;
		return TRUE;
	}
	BOOL32 RPrimitiveSceneProxy::ShouldDrawInPass(const FRenderPassDesc& InPassDesc) const
	{
		(void)InPassDesc;
		return TRUE;
	}
	BOOL32 RPrimitiveSceneProxy::GetWorldBound(Vector3& OutMin, Vector3& OutMax) const
	{
		// A proxy without geometry of a known extent - a procedural primitive, a debug draw -
		// states no bound. The cull keeps it rather than dropping it on a guess.
		(void)OutMin;
		(void)OutMax;
		return FALSE;
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
		(void)InSnapshot;
	}
	void RPrimitiveSceneProxy::ApplyAssets(const FPrimitiveRenderAssetReport& InAssets)
	{
		Assets = InAssets;

		// The pass slots were resolved from the previous report, so they are gone.
		ReleasePassSlots();

		// The primary instance is a view independent fact of the primitive: a view primitive
		// list groups by it, and every view sees the same value.
		ReportedPrimaryMaterial = Assets.FindMaterialInstance(0u, GetPrimaryPassName());

		// The per draw block takes its layout from the program that will read it, so the first
		// reported instance that resolves to a program supplies it. A report with no instance
		// at all leaves the block uninitialized rather than latching an empty layout.
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
	const EShaderInstance* RPrimitiveSceneProxy::FindReportedMaterialInstance(const UINT32 InSlotIndex, const EName& InPassName) const
	{
		return Assets.FindMaterialInstance(InSlotIndex, InPassName);
	}
	const EShaderInstance* RPrimitiveSceneProxy::ResolveBaseShaderInstance(const UINT32 InSlotIndex
		, const EShaderInstance* InViewShaderInstanceOverride) const
	{
		// The view's selection is a whole item choice and outranks the primitive's report. The
		// report is read per slot on purpose: one item level pointer cannot stand for a mesh
		// whose submeshes have different materials.
		if (InViewShaderInstanceOverride)
		{
			return InViewShaderInstanceOverride;
		}
		return Assets.FindMaterialInstance(InSlotIndex, GetPrimaryPassName());
	}
	void RPrimitiveSceneProxy::ApplyValues(const FShaderValueReport& InValues)
	{
		LastDrawValues = InValues;
		bHasDrawValues = ((InValues.IsEmpty() != FALSE) ? FALSE : TRUE);

		if (!DrawParameterBlock.IsInitialized())
		{
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
				// another one belongs to whoever owns that block.
				continue;
			}
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
		// never reached a buffer; this publishes them for the first time.
		UpdateDrawParameters();
		PublishDrawParametersFromTransform();
		if (bHasDrawValues != FALSE)
		{
			WriteDrawValues(LastDrawValues);
		}
		return TRUE;
	}
	void RPrimitiveSceneProxy::UpdateDrawParameters() const
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		const_cast<RShaderParameterBlock&>(DrawParameterBlock).Upload(RenderDevice);
	}
	void RPrimitiveSceneProxy::EnsurePublishedDrawParameters() const
	{
		UpdateDrawParameters();
	}
	void RPrimitiveSceneProxy::PublishDrawParametersFromTransform()
	{
		if (!DrawParameterBlock.IsInitialized())
		{
			return;
		}

		EShaderParameterSet& Parameters = DrawParameterBlock.GetParameters();
		const Matrix4x4& LocalToWorld = GetLocalToWorldMatrix();
		const Matrix4x4 InverseWorld(LocalToWorld.Inverse());

		// The same convention the component side uses: written as the CPU holds them, never
		// transposed, because the shader side matrix is the transpose of these.
		const EName BlockName(SHADER_BLOCK_DRAW_PARAMS);
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_WORLD_MATRIX), LocalToWorld);
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_WORLD_INV_MATRIX), InverseWorld);
		WriteShaderMatrix(Parameters, BlockName, EName(SHADER_FIELD_WORLD_INV_TRANSPOSE_MATRIX), InverseWorld.Transpose());

		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		DrawParameterBlock.Upload(RenderDevice);
	}
	void RPrimitiveSceneProxy::BindEngineParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram
		, const RViewProxy* InView, const BOOL32 InBindSharedEngineBlocks) const
	{
		if (!InCommandList)
		{
			return;
		}
		// Everything here runs after SetPipelineState on purpose: a pipeline state brings its
		// own root signature, and switching a root signature voids every root binding made
		// before it.
		RSceneRenderer* Renderer = EMainManager::GetManagerSingleton()->GetSceneRenderer();
		if ((Renderer) && (InBindSharedEngineBlocks != FALSE))
		{
			Renderer->BindGlobalParameters(InCommandList, InProgram);
			if (InView)
			{
				InView->BindParameters(InCommandList, InProgram);
			}
		}
		DrawParameterBlock.Bind(InCommandList, InProgram);
	}

	// ---------------------------------------------------------------------------
	// Pass data
	// ---------------------------------------------------------------------------

	const FPrimitivePassData* RPrimitiveSceneProxy::FindPrimitivePassData(const UINT32 InSlotIndex, const EName& InPassName) const
	{
		for (UINT32 Index = 0u, Num = PassSlots.Num<UINT32>(); Index < Num; Index++)
		{
			const FPrimitivePassData* Data = PassSlots[Index].get();
			if ((Data) && (Data->SlotIndex == InSlotIndex) && (Data->PassName == InPassName))
			{
				return Data;
			}
		}
		return nullptr;
	}
	BOOL32 RPrimitiveSceneProxy::IsPrimitivePassDataCurrent(const FPrimitivePassData& InData
		, const EShaderInstance* InBaseInstance, const BOOL32 InIsPrimaryPass) const
	{
		if (!InData.ResolvedShaderInstance)
		{
			return FALSE;
		}
		// The instance moves the moment a reload is published; the revision moves when a trait
		// or the resolved program changes. Either makes the pipeline state, the binding plan
		// and the buffers this slot built stale.
		if ((InData.ShaderInstanceAsset != InData.ResolvedShaderInstance->GetAsset())
			|| (InData.ShaderInstanceRevision != InData.ResolvedShaderInstance->GetRevision()))
		{
			return FALSE;
		}
		// The base belongs to the slot, and a view override belongs to the item. Either one
		// changing selects a different instance, so the resolved instance is stale even for a
		// primary pass.
		if (InBaseInstance)
		{
			if ((InData.BaseShaderInstance != InBaseInstance)
				|| (InData.BaseShaderInstanceAsset != InBaseInstance->GetAsset()))
			{
				return FALSE;
			}
		}
		else if (InData.BaseShaderInstance != nullptr)
		{
			return FALSE;
		}
		if (InIsPrimaryPass != FALSE)
		{
			return TRUE;
		}
		// A pending slot exists only to keep the object drawn while its pass instance is
		// assembled. It answers no cache hit, so the next draw rebuilds it.
		if (InData.bShaderInstancePending != FALSE)
		{
			return FALSE;
		}
		return TRUE;
	}
	FPrimitivePassData* RPrimitiveSceneProxy::EnsurePrimitivePassData(const UINT32 InSlotIndex, const EName& InPassName
		, const FRenderPassDesc& InPassDesc, const EShaderInstance* InViewShaderInstanceOverride)
	{
		// A shader instance is assembled once, for the pass it authored. A different pass - the
		// depth only shadow pass is the first one - derives its own instance from the base one,
		// keeping the material and swapping only the program.
		const EName& PrimaryPassName = GetPrimaryPassName();
		const BOOL32 bIsPrimaryPass = ((InPassName == PrimaryPassName) ? TRUE : FALSE);
		REngineShaderInstanceCache* InstanceCache = REngineShaderInstanceCache::GetManagerSingleton();

		// The base is per slot. The view override, when it selected one, answers for the whole
		// item; otherwise the slot's own reported primary is the base, which is what keeps slot
		// 0's material from being derived into every other slot. Null means "no material in
		// this slot", and the fallback below answers for it.
		const EShaderInstance* BaseInstance = ResolveBaseShaderInstance(InSlotIndex, InViewShaderInstanceOverride);

		if (FPrimitivePassData* Existing = const_cast<FPrimitivePassData*>(FindPrimitivePassData(InSlotIndex, InPassName)))
		{
			// A pending slot keeps the object drawn with the fallback, and its retry is once per
			// frame: a slow assembly must not rebuild this slot for every view and every caster.
			if ((Existing->bShaderInstancePending != FALSE) && (Existing->PendingRetryFrame == GetRenderFrameIndex()))
			{
				return Existing;
			}
			if (IsPrimitivePassDataCurrent(*Existing, BaseInstance, bIsPrimaryPass) != FALSE)
			{
				return Existing;
			}
			DropPassSlot(InSlotIndex, InPassName);
		}

		// The primary pass runs the base instance itself: a view override when there is one,
		// otherwise the slot's own report. A non-primary pass uses an instance the slot
		// reported for that pass; a view override replaces that report, so the derivation
		// below runs from the override instead.
		const EShaderInstance* Instance = nullptr;
		if (bIsPrimaryPass != FALSE)
		{
			Instance = BaseInstance;
		}
		else if (InViewShaderInstanceOverride == nullptr)
		{
			Instance = Assets.FindMaterialInstance(InSlotIndex, InPassName);
		}
		BOOL32 bInstancePending = FALSE;
		if ((!Instance) && (bIsPrimaryPass == FALSE) && (BaseInstance))
		{
			Instance = InstanceCache->RequirePassInstance(*BaseInstance, InPassName);
			if (!Instance)
			{
				bInstancePending = TRUE;
			}
		}
		if (!Instance)
		{
			// A slot with no reported instance falls back to this primitive's engine material, so
			// geometry is never dropped merely because nothing was authored. The fallback is a
			// primary pass instance, so a non primary pass derives from it on the same terms.
			const EShaderInstance* Fallback = ResolveFallbackInstance(InSlotIndex);
			if ((Fallback) && (bIsPrimaryPass == FALSE))
			{
				Instance = InstanceCache->RequirePassInstance(*Fallback, InPassName);
			}
			else
			{
				Instance = Fallback;
			}
		}
		if (!Instance)
		{
			return nullptr;
		}
		const EShaderProgram* Program = Instance->GetProgram();
		if (!Program)
		{
			ReportSkippedDraw(InPassName, EString(PE_TEXT("its material instance resolved to no usable program")));
			return nullptr;
		}

		RSceneRenderer* Renderer = EMainManager::GetManagerSingleton()->GetSceneRenderer();
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if ((!Renderer) || (!RenderDevice))
		{
			return nullptr;
		}

		FPrimitivePassData NewDataValue;
		FPrimitivePassData& NewData = NewDataValue;
		NewData.SlotIndex					= InSlotIndex;
		NewData.PassDesc					= &InPassDesc;
		NewData.PassName					= InPassName;
		NewData.ResolvedShaderInstance		= Instance;
		NewData.DisplayName					= GetShaderInstanceDisplayName(Instance);
		// The object the program below was resolved from, not the one the instance is bound to
		// right now: a rebind on the load thread could land between the two reads.
		NewData.ShaderInstanceAsset			= Instance->GetResolvedAsset();
		NewData.Program						= Program;
		NewData.ShaderInstanceRevision		= Instance->GetRevision();
		NewData.BaseShaderInstance			= BaseInstance;
		NewData.BaseShaderInstanceAsset		= ((BaseInstance != nullptr) ? BaseInstance->GetAsset() : nullptr);
		NewData.bShaderInstancePending		= bInstancePending;
		NewData.PendingRetryFrame			= ((bInstancePending != FALSE) ? GetRenderFrameIndex() : 0u);

		// The per draw block takes its layout from the program this draw runs, which is the
		// only place a primitive without an authored material can supply one.
		EnsureDrawParameterBlock(*Program);

		// The pipeline state carries the pass's fixed function state, this program's shaders
		// and the roots generated from this program's own reflection.
		RGraphicsPipelineStateDesc Desc;
		FRenderPassRegistry::FillPipelineStateDesc(InPassDesc, Program, Desc);
		RPipelineStateCache::ApplyMaterialRenderTraits(Instance->GetTraits(), Desc);
		UINT32 InputLayoutNum = 0u;
		Desc.InputLayouts = GetPipelineStateInputLayouts(InputLayoutNum);
		Desc.InputLayoutCount = InputLayoutNum;
		NewData.PipelineState = Renderer->GetPipelineStateCache().GetOrCreateGraphics(Desc);
		if (!NewData.PipelineState)
		{
			ReportSkippedDraw(InPassName, EString(PE_TEXT("its pipeline state could not be built for program '"))
				+ Program->GetShaderName().ToString() + EString(PE_TEXT("'")));
			return nullptr;
		}

		// Geometry is resolved here, in Prepare, so the record path never walks an input layout
		// or a mesh stream table. A primitive without geometry leaves the binding empty.
		ResolveGeometryBinding(NewData);

		PassSlots.Add(EMemory::MakeUnique<FPrimitivePassData>(NewDataValue));
		return PassSlots[PassSlots.Num<UINT32>() - 1u].get();
	}
	FPrimitivePassData* RPrimitiveSceneProxy::EnsurePublishedPassData(const UINT32 InSlotIndex, const EName& InPassName
		, const FRenderPassDesc& InPassDesc, const EShaderInstance* InViewShaderInstanceOverride, FFrameRenderData& InFrameData)
	{
		FPrimitivePassData* Data = EnsurePrimitivePassData(InSlotIndex, InPassName, InPassDesc, InViewShaderInstanceOverride);
		if ((!Data) || (!Data->ResolvedShaderInstance) || (!Data->Program))
		{
			return nullptr;
		}
		if (Data->ResolvedFrame != InFrameData.GetFrameIndex())
		{
			Data->ShaderInstanceData = InFrameData.EnsureShaderInstance(*Data->ResolvedShaderInstance, *Data->Program);
			Data->ResolvedFrame = InFrameData.GetFrameIndex();
		}
		return ((Data->ShaderInstanceData) ? Data : nullptr);
	}
	void RPrimitiveSceneProxy::DropPassSlot(const UINT32 InSlotIndex, const EName& InPassName)
	{
		for (UINT32 Index = 0u, Num = PassSlots.Num<UINT32>(); Index < Num; Index++)
		{
			const FPrimitivePassData* Data = PassSlots[Index].get();
			if ((!Data) || (Data->SlotIndex != InSlotIndex) || (Data->PassName != InPassName))
			{
				continue;
			}
			PassSlots.RemoveAt(Index);
			return;
		}
	}
	void RPrimitiveSceneProxy::ReleasePassSlots()
	{
		PassSlots.Empty();
	}

	// ---------------------------------------------------------------------------
	// Record
	// ---------------------------------------------------------------------------

	BOOL32 RPrimitiveSceneProxy::BindShaderInstanceData(IRCommandList* InCommandList, const FPassItem& InItem) const
	{
		if ((!InCommandList) || (!InItem.PrimitivePassData) || (!InItem.ShaderInstanceData))
		{
			return FALSE;
		}
		FShaderInstanceRenderData* InstanceData = InItem.ShaderInstanceData;
		if ((!InstanceData->Parameters) || (!InstanceData->ParameterBuffers))
		{
			return FALSE;
		}
		BindPublishedShaderParameters(InCommandList, *InstanceData->Parameters, *InstanceData->ParameterBuffers
			, *InItem.PrimitivePassData->Program, ENGINE_SHADER_STAGE_MASK_GRAPHICS, EName(SHADER_BINDING_SCOPE_MATERIAL));
		return TRUE;
	}
	void RPrimitiveSceneProxy::ReportSkippedDraw(const EName& InPassName, const EString& InReason) const
	{
		// The component's identity rather than this proxy's address: a proxy is rebuilt from a
		// snapshot, and the identity is what a reader can match against the scene.
		PigeonEngine::ReportSkippedDraw(EString(PE_TEXT("primitive "))
			+ ToString(static_cast<UINT64>(GetPrimitiveIdentity()))
			+ EString(PE_TEXT(" pass '")) + InPassName.ToString() + EString(PE_TEXT("'")), InReason);
	}
	void RPrimitiveSceneProxy::RecordPassItem(FPassRecordContext& InContext, const FPassItem& InItem) const
	{
		IRCommandList* CommandList = InContext.CommandList;
		const FPrimitivePassData* Data = InItem.PrimitivePassData;
		const FPassViewInstance* Instance = InContext.Instance;
		if ((!CommandList) || (!Data) || (!Instance))
		{
			return;
		}
		if ((!Data->PipelineState) || (!Data->Program))
		{
			return;
		}

		PE_GPU_EVENT_SCOPE_IF(CommandList, Data->DisplayName, EEngineSettings::ENGINE_RHI_ENABLE_DRAW_MARKERS);

		const BOOL32 bPipelineAlreadyBound = ((CommandList->GetBoundPipelineState() == Data->PipelineState) ? TRUE : FALSE);
		if (bPipelineAlreadyBound == FALSE)
		{
			CommandList->SetPipelineState(Data->PipelineState);
		}

		const BOOL32 bShaderInstanceBound = ((bPipelineAlreadyBound != FALSE)
			&& (InContext.StateCache.ShaderInstance == Data->ResolvedShaderInstance)
			&& (InContext.StateCache.PipelineState == Data->PipelineState)) ? TRUE : FALSE;
		if (bShaderInstanceBound == FALSE)
		{
			InContext.StateCache.ShaderInstance = nullptr;
			InContext.StateCache.PipelineState = nullptr;
			if (!BindShaderInstanceData(CommandList, InItem))
			{
				ReportSkippedDraw(Data->PassName, EString(PE_TEXT("its material parameters could not be bound")));
				return;
			}
			InContext.StateCache.ShaderInstance = Data->ResolvedShaderInstance;
			InContext.StateCache.PipelineState = Data->PipelineState;
		}

		const BOOL32 bSharedEngineBound = ((bPipelineAlreadyBound != FALSE)
			&& (InContext.StateCache.Program == Data->Program)
			&& (InContext.StateCache.View == Instance->View)) ? TRUE : FALSE;
		BindEngineParameters(CommandList, *Data->Program, Instance->View, ((bSharedEngineBound != FALSE) ? FALSE : TRUE));
		if (bSharedEngineBound == FALSE)
		{
			InContext.StateCache.Program = Data->Program;
			InContext.StateCache.View = Instance->View;
		}

		if (!BindGeometry(CommandList, *Data))
		{
			return;
		}
		if (BindExtraResources(CommandList, *Data) != FALSE)
		{
			// The extra binding wrote a descriptor table of its own, which may have opened a
			// fresh set. The material's descriptors are not in that set, so no following draw
			// may treat them as still bound.
			InContext.StateCache.ShaderInstance = nullptr;
			InContext.StateCache.PipelineState = nullptr;
		}
		if (!DrawGeometry(CommandList, *Data, InItem.IndexCount, InItem.StartIndex))
		{
			ReportSkippedDraw(Data->PassName, EString(PE_TEXT("its geometry could not be submitted")));
		}
	}
	BOOL32 RPrimitiveSceneProxy::RecordCompute(IRCommandList* InCommandList, const FRenderPassDesc& InPassDesc) const
	{
		(void)InCommandList;
		(void)InPassDesc;
		return FALSE;
	}

	// ---------------------------------------------------------------------------
	// Hooks: a primitive that overrides none of these draws nothing.
	// ---------------------------------------------------------------------------

	UINT32 RPrimitiveSceneProxy::GetDrawGroupCount() const
	{
		return GetDrawGroupNum();
	}
	BOOL32 RPrimitiveSceneProxy::GetDrawGroupDesc(const UINT32 InGroupIndex, UINT32& OutSlotIndex, UINT32& OutIndexCount, UINT32& OutStartIndex) const
	{
		return GetDrawGroup(InGroupIndex, OutSlotIndex, OutIndexCount, OutStartIndex);
	}
	const EShaderInstance* RPrimitiveSceneProxy::ResolveFallbackInstance(const UINT32 InSlotIndex) const
	{
		(void)InSlotIndex;
		return nullptr;
	}
	const RInputLayoutDesc* RPrimitiveSceneProxy::GetPipelineStateInputLayouts(UINT32& OutCount) const
	{
		OutCount = 0u;
		return nullptr;
	}
	UINT32 RPrimitiveSceneProxy::GetDrawGroupNum() const
	{
		return 0u;
	}
	BOOL32 RPrimitiveSceneProxy::GetDrawGroup(const UINT32 InGroupIndex, UINT32& OutSlotIndex, UINT32& OutIndexCount, UINT32& OutStartIndex) const
	{
		(void)InGroupIndex;
		OutSlotIndex = 0u;
		OutIndexCount = 0u;
		OutStartIndex = 0u;
		return FALSE;
	}
	BOOL32 RPrimitiveSceneProxy::ResolveGeometryBinding(FPrimitivePassData& InOutData) const
	{
		(void)InOutData;
		return FALSE;
	}
	BOOL32 RPrimitiveSceneProxy::BindGeometry(IRCommandList* InCommandList, const FPrimitivePassData& InData) const
	{
		(void)InCommandList;
		(void)InData;
		return FALSE;
	}
	BOOL32 RPrimitiveSceneProxy::BindExtraResources(IRCommandList* InCommandList, const FPrimitivePassData& InData) const
	{
		(void)InCommandList;
		(void)InData;
		return FALSE;
	}
	BOOL32 RPrimitiveSceneProxy::DrawGeometry(IRCommandList* InCommandList, const FPrimitivePassData& InData
		, const UINT32 InIndexCount, const UINT32 InStartIndex) const
	{
		(void)InData;
		if (!InCommandList)
		{
			return FALSE;
		}
		// One indexed draw. BaseVertex stays 0: the import path writes absolute vertex indices
		// into the merged index buffer, so the start index alone picks the right slice.
		InCommandList->DrawIndexed(InIndexCount, 1u, InStartIndex, 0, 0u);
		return TRUE;
	}

}	// namespace PigeonEngine
