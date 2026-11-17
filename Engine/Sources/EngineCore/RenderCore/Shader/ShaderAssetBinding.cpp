#include <Shader/ShaderAssetBinding.h>

#include <Main/MainManager.h>
#include <Pipeline/RPipelineStateDesc.h>
#include <Shader/ShaderEngineParameters.h>
#include <Shader/ShaderParameterBuffers.h>
#include <RenderResourceLifetime.h>
#include <RHI/IRCommandList.h>
#include <RHI/IRRHIDevice.h>
#include <RHI/RRHIBindlessManager.h>
#include <Core/Config/EngineConfig.h>
#include <Config/EngineShaderConfig.h>
#include <Editor/EditorLogManager.h>
#include <IO/FileHelper.h>
#include <ShaderAsset/ShaderCompileJob.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderTarget.h>

namespace PigeonEngine
{

	const EShaderStageEntry* ResolveShaderStageEntry(const EShaderProgram& InProgram, const EShaderStageType InStage)
	{
		const EShaderStageEntry* Entry = InProgram.FindStageEntry(InStage);
		if ((!Entry) || (!Entry->bValid))
		{
			return nullptr;
		}
		return Entry;
	}

	const RShaderStageResource* ResolveShaderStageResource(const EShaderProgram& InProgram, const EShaderStageType InStage)
	{
		const EShaderStageEntry* Entry = ResolveShaderStageEntry(InProgram, InStage);
		if (!Entry)
		{
			return nullptr;
		}
		const EShaderStageAsset* StageAsset = Entry->StageAsset;
		if (!StageAsset)
		{
			return nullptr;
		}
		const RShaderStageResource* StageResource = StageAsset->GetShaderStageResource();
		if ((!StageResource) || (!StageResource->IsRenderResourceValid()))
		{
			return nullptr;
		}
		return StageResource;
	}

	BOOL32 IsShaderStageEntryUsable(const EShaderProgram& InProgram, const EShaderStageType InStage)
	{
		return ((ResolveShaderStageEntry(InProgram, InStage) != nullptr) ? TRUE : FALSE);
	}

	BOOL32 FillGraphicsPipelineShaders(const EShaderProgram& InProgram, UINT32 InStageMask, RRHIGraphicsPipelineDesc& OutDesc)
	{
		const RShaderStageResource* VertexStage = ResolveShaderStageResource(InProgram, EShaderStageType::SHADER_STAGE_VERTEX);
		if (!VertexStage)
		{
			return FALSE;
		}

		OutDesc.VertexShader		= VertexStage->GetRHIShader();
		// The input layout belongs to the vertex stage: it was built from that stage's
		// reflection, and it is the same description the vertex shader object was
		// created with, so the PSO and the shader cannot disagree.
		OutDesc.InputLayouts		= VertexStage->GetInputLayouts().GetData();
		OutDesc.InputLayoutCount	= VertexStage->GetInputLayouts().Num<UINT32>();

		// The pixel stage is built only when the pass asked for it. A depth only pass draws with
		// no pixel work, and a pipeline without a pixel shader is what lets the rasterizer skip
		// that stage instead of running one that writes nothing.
		const BOOL32 bNeedsPixelStage = (((InStageMask & RGraphicsPipelineStateDesc::STAGE_MASK_PIXEL) != 0u) ? TRUE : FALSE);
		if (bNeedsPixelStage == FALSE)
		{
			OutDesc.PixelShader = nullptr;
			return TRUE;
		}
		const RShaderStageResource* PixelStage = ResolveShaderStageResource(InProgram, EShaderStageType::SHADER_STAGE_PIXEL);
		if (!PixelStage)
		{
			return FALSE;
		}
		OutDesc.PixelShader = PixelStage->GetRHIShader();
		return TRUE;
	}

	BOOL32 FillComputePipelineShader(const EShaderProgram& InProgram, RRHIComputePipelineDesc& OutDesc)
	{
		const RShaderStageResource* ComputeStage = ResolveShaderStageResource(InProgram, EShaderStageType::SHADER_STAGE_COMPUTE);
		if (!ComputeStage)
		{
			return FALSE;
		}
		OutDesc.ComputeShader = ComputeStage->GetRHIShader();
		return TRUE;
	}

	EShaderInstance* CreateShaderInstance(const EShaderAsset* InAsset, const EString& InVariantName)
	{
		if (!InAsset)
		{
			return nullptr;
		}
		EShaderInstance* Instance = new EShaderInstance(InAsset, EName(InVariantName));
		if (!Instance->RefreshProgram())
		{
			// Released through the manager, not here: the instance's constant buffers are built
			// by a command that is still queued and reads its parameter mirrors, so freeing the
			// memory now would leave that command reading released memory.
			RRenderResourceManager::GetManagerSingleton()->RetireRelease([Instance]() -> void { delete Instance; });
			return nullptr;
		}
		return Instance;
	}

	EString GetShaderInstanceDisplayName(const EShaderInstance* InInstance)
	{
		if ((!InInstance) || (!InInstance->GetAsset()))
		{
			return EString(PE_TEXT("<none>"));
		}
		return EString(InInstance->GetAsset()->GetName().ToString());
	}

	RShaderParameterBuffers* ResolveShaderParameterBuffers(const EShaderInstance& InInstance)
	{
		return static_cast<RShaderParameterBuffers*>(
			RRenderResourceManager::GetManagerSingleton()->GetRegistry().Resolve(InInstance.GetParameterBuffersRef()));
	}

	// The skips already reported, keyed by owner and reason. Only ever grows, and holds one entry
	// per thing that was found not to be drawing - which is what keeps a per frame draw path from
	// repeating itself in the log for as long as the state lasts.
	static TArray<EString>& GetReportedSkippedDraws()
	{
		static TArray<EString> ReportedSkippedDraws;
		return ReportedSkippedDraws;
	}

	void ReportSkippedDraw(const EString& InOwner, const EString& InReason)
	{
		const EString ReportKey = InOwner + EString(PE_TEXT("|")) + InReason;
		TArray<EString>& ReportedSkippedDraws = GetReportedSkippedDraws();
		if (ReportedSkippedDraws.Contains(ReportKey))
		{
			return;
		}
		ReportedSkippedDraws.Add(ReportKey);
		PE_LOG_ERROR(EString(PE_TEXT("[SkippedDraw] ")) + InOwner + EString(PE_TEXT(" is not drawn: ")) + InReason);
	}

	// The index driven bindings whose missing value has already been reported. A draw retries
	// every frame, so without this the warning would repeat for as long as the material stays
	// broken; the key is the program and the binding, which is what a reader has to fix.
	static TArray<EString>& GetReportedUnpublishedIndices()
	{
		static TArray<EString> ReportedUnpublishedIndices;
		return ReportedUnpublishedIndices;
	}

	static void ReportUnpublishedBindlessIndex(const EShaderProgram& InProgram, const EShaderResourceBinding& InBinding)
	{
		const EString ReportKey = InProgram.GetShaderName().ToString() + EString(PE_TEXT("|")) + InBinding.Name.ToString();
		TArray<EString>& ReportedUnpublishedIndices = GetReportedUnpublishedIndices();
		if (ReportedUnpublishedIndices.Contains(ReportKey))
		{
			return;
		}
		ReportedUnpublishedIndices.Add(ReportKey);
		// The draw is refused rather than drawn against the default: an index driven binding with
		// no value has no defined descriptor to fall back to. The warning names the program
		// because the resource name alone does not say which material failed to publish it, and
		// it is a warning rather than a silent skip so a missing object is not mistaken for an
		// object that was never in the scene.
		PE_LOG_WARN(EString(PE_TEXT("[ShaderAssetBinding] The index driven resource '")) + InBinding.Name.ToString()
			+ EString(PE_TEXT("' of program '")) + InProgram.GetShaderName().ToString()
			+ EString(PE_TEXT("' has no published value; the draw is skipped instead of reading the null sentinel.")));
	}

	namespace
	{
		// The walk both bind entry points share. A valid InOnlyBlock restricts it to that block;
		// an invalid one binds whatever the table holds, minus the blocks the render tier
		// publishes itself (whose roots belong to their own block objects).
		void BindConstantBufferBindings(IRCommandList* InCommandList, RShaderParameterBuffers& InOutBuffers
			, const EShaderProgram& InProgram, const EShaderStageType InStage, const BOOL32 InIsCompute
			, const EName& InOnlyBlock)
		{
			const BOOL32 bOnlyOneBlock = (InOnlyBlock.IsValid() != FALSE);
			const EShaderStageEntry* Entry = ResolveShaderStageEntry(InProgram, InStage);
			if (!Entry)
			{
				return;
			}

			IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
			// Whether a 'RootConstant' block can travel through the root signature's 32 bit
			// constant space is a property of the back-end, so it is asked for rather than
			// assumed from which back-end is active.
			const BOOL32 bSupportRootConstants = ((RenderDevice != nullptr)
				&& (RenderDevice->SupportsRootConstants() != FALSE)) ? TRUE : FALSE;

			for (UINT32 BindingIndex = 0u, BindingNum = Entry->BindingPlan.GetConstantBufferBindingNum(); BindingIndex < BindingNum; BindingIndex++)
			{
				const EShaderConstantBufferBinding* Binding = Entry->BindingPlan.GetConstantBufferBindingByIndex(BindingIndex);
				if (!Binding)
				{
					continue;
				}
				if ((bOnlyOneBlock != FALSE) && (Binding->BlockName != InOnlyBlock))
				{
					continue;
				}
				if ((bOnlyOneBlock == FALSE) && (IsRenderTierShaderBlock(Binding->BlockName) != FALSE))
				{
					// Published by a block object of the render tier: binding here would fight
					// the object that owns the root, and the value this table holds for it is a
					// mirror nobody ever wrote.
					continue;
				}
				// A block the shader source marked 'RootConstant' is fed through the root
				// signature's 32 bit constant space instead of a buffer read. A back-end without
				// that space has no such root, so there the block is bound as a plain constant
				// buffer, which is what the same reflection describes on that back-end. The
				// binding's RootIndex is the root the block lands on either way: the constant
				// block's root for the root constant mode, the buffer's root for the view mode
				// (the layout generator keeps both up to date).
				if ((Binding->Kind == EShaderConstantBufferBindingKindType::SHADER_CONSTANT_BUFFER_BINDING_KIND_ROOT_CONSTANTS)
					&& (bSupportRootConstants != FALSE))
				{
					const BYTE* RootConstantData = nullptr;
					UINT32 Num32BitValues = 0u;
					if (InOutBuffers.GetRootConstantData(InStage, Binding->BlockName, RootConstantData, Num32BitValues))
					{
						if (InIsCompute)
						{
							InCommandList->SetComputeRoot32BitConstants(Binding->RootIndex, RootConstantData, Num32BitValues);
						}
						else
						{
							InCommandList->SetGraphicsRoot32BitConstants(Binding->RootIndex, RootConstantData, Num32BitValues);
						}
						continue;
					}
					// No usable CPU image: fall through to the buffer path so the block is still
					// bound by the platform that can.
				}
				IRRHIBuffer* RHIBuffer = InOutBuffers.GetResolvedBuffer(InStage, Binding->BlockName);
				if (!RHIBuffer)
				{
					continue;
				}
				// The buffer a block is bound from may have been laid out by another program that
				// declared the block (see RShaderParameterBuffers), so the size this reflection
				// reports is worth checking against it: a shorter buffer would have the shader
				// read past its end, which the back-end cannot detect for a root view.
				if ((Binding->ByteSize > 0u) && (RHIBuffer->GetByteSize() < static_cast<UINT64>(Binding->ByteSize)))
				{
					PE_LOG_WARN(EString(PE_TEXT("[ShaderAssetBinding] Block '")) + Binding->BlockName.ToString()
						+ EString(PE_TEXT("' is bound from a ")) + ToString(RHIBuffer->GetByteSize())
						+ EString(PE_TEXT(" byte buffer while this program reads ")) + ToString(Binding->ByteSize)
						+ EString(PE_TEXT(" bytes of it.")));
				}
				if (InIsCompute)
				{
					InCommandList->SetComputeRootConstantBuffer(Binding->RootIndex, RHIBuffer);
				}
				else
				{
					InCommandList->SetGraphicsRootConstantBuffer(Binding->RootIndex, RHIBuffer);
				}
			}
		}
	}

	void BindShaderParameterBuffers(IRCommandList* InCommandList, RShaderParameterBuffers& InOutBuffers
		, const EShaderProgram& InProgram, const EShaderStageType InStage, const BOOL32 InIsCompute)
	{
		if (!InCommandList)
		{
			return;
		}
		BindConstantBufferBindings(InCommandList, InOutBuffers, InProgram, InStage, InIsCompute, EName());
	}

	void BindShaderParameterBlock(IRCommandList* InCommandList, RShaderParameterBuffers& InOutBuffers
		, const EShaderProgram& InProgram, const EShaderStageType InStage, const BOOL32 InIsCompute
		, const EName& InBlockName)
	{
		if ((!InCommandList) || (!InBlockName.IsValid()))
		{
			return;
		}
		BindConstantBufferBindings(InCommandList, InOutBuffers, InProgram, InStage, InIsCompute, InBlockName);
	}

	// The (binding name, stage) pairs already reported as unbound, keyed as one string so a session
	// that never publishes a resource costs one line rather than one per draw. Only ever grows, and
	// it holds one entry per resource that is read but never bound, which is a handful in a session.
	static TArray<EString>& GetReportedUnboundResources()
	{
		static TArray<EString> ReportedUnboundResources;
		return ReportedUnboundResources;
	}

	// A plan entry only exists for a binding the compiled shader reads (the builder skips the culled
	// ones), so an entry with no value published for it means a draw reads a slot nobody wrote:
	// that reads as zeroes - or as whatever the previous draw left there - and looks exactly like a
	// feature that ran and produced nothing. Named once per (scope, resource, stage), because
	// naming it per draw would bury the frame in the same line.
	static void ReportUnboundShaderResource(const EName& InName, const ERHIShaderStage InStage, const EName& InScopeName)
	{
		const EString ReportKey = InScopeName.ToString() + EString(PE_TEXT("|")) + InName.ToString()
			+ EString(PE_TEXT("|")) + ToString(static_cast<UINT32>(InStage));
		TArray<EString>& ReportedUnboundResources = GetReportedUnboundResources();
		if (ReportedUnboundResources.Contains(ReportKey))
		{
			return;
		}
		ReportedUnboundResources.Add(ReportKey);
		PE_LOG_ERROR(EString(PE_TEXT("[ShaderAssetBinding] The scope '")) + InScopeName.ToString()
			+ EString(PE_TEXT("' published no usable value for '")) + InName.ToString()
			+ EString(PE_TEXT("' on stage ")) + ToString(static_cast<UINT32>(InStage))
			+ EString(PE_TEXT(": the slot keeps whatever it held.")));
	}

	// Runtime reporting belongs to the owner. A binding is reported only when this publisher is
	// the single module that claimed it: a shared claim cannot tell which scope was supposed to
	// feed a given draw, an exposed binding has its own default resource, and an unclaimed one
	// was already reported when the plan was built. Binding is unaffected by the answer.
	static BOOL32	ShouldReportUnboundBinding(const EShaderResourceBinding& InBinding, const EName& InScopeName)
	{
		return (((InBinding.ClaimState == EShaderBindingClaimStateType::SHADER_BINDING_CLAIM_STATE_CLAIMED)
			&& (InBinding.ClaimScopeNum == 1u)
			&& (InBinding.SoleClaimScope == InScopeName)) ? TRUE : FALSE);
	}

	static RDefaultBindingTextureType	TranslateDefaultTextureType(const EShaderTextureType InTextureType)
	{
		switch (InTextureType)
		{
		case EShaderTextureType::SHADER_TEXTURE_TYPE_CUBE:
		case EShaderTextureType::SHADER_TEXTURE_TYPE_CUBE_ARRAY:
			return RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_CUBE;
		case EShaderTextureType::SHADER_TEXTURE_TYPE_3D:
		case EShaderTextureType::SHADER_TEXTURE_TYPE_3D_ARRAY:
			return RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_3D;
		case EShaderTextureType::SHADER_TEXTURE_TYPE_2D_ARRAY:
			return RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_2D_ARRAY;
		default:
			// No engine shader declares a 1D or 1D array binding today; a 2D default keeps an
			// exposed binding defined if one ever appears.
			return RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_2D;
		}
	}

	// The object an exposed binding without a value falls back to. Null when the device is gone
	// or the back-end refused the default; the caller then leaves the slot untouched rather than
	// binding something undefined.
	static IRRHIResource*	ResolveDefaultBindingResource(IRRHIDevice* InRenderDevice, const EShaderResourceBinding& InBinding)
	{
		if (!InRenderDevice)
		{
			return nullptr;
		}
		if (InBinding.bIsTexture != FALSE)
		{
			// The engine defaults are sampled resources. A write target with no value has no
			// meaningful black equivalent, so it is left untouched rather than bound to a view
			// that does not exist.
			if (InBinding.Kind == EShaderBindingKindType::SHADER_BINDING_KIND_UAV)
			{
				return nullptr;
			}
			return InRenderDevice->GetDefaultTexture(TranslateDefaultTextureType(InBinding.TextureType));
		}
		const BOOL8 bIsStructured = ((InBinding.ResourceKind == EShaderResourceKindType::SHADER_RESOURCE_KIND_STRUCTURED_BUFFER) ? TRUE : FALSE);
		const BOOL8 bIsUnorderedAccess = ((InBinding.Kind == EShaderBindingKindType::SHADER_BINDING_KIND_UAV) ? TRUE : FALSE);
		return InRenderDevice->GetDefaultBuffer(bIsStructured, bIsUnorderedAccess);
	}

	static void	BindDefaultBindingResource(IRCommandList* InCommandList, const ERHIShaderStage InStage
		, const EShaderResourceBinding& InBinding)
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		IRRHIResource* DefaultResource = ResolveDefaultBindingResource(RenderDevice, InBinding);
		if (!DefaultResource)
		{
			return;
		}
		if (InBinding.bIsTexture != FALSE)
		{
			IRRHITexture* DefaultTexture = static_cast<IRRHITexture*>(DefaultResource);
			if (InBinding.Kind == EShaderBindingKindType::SHADER_BINDING_KIND_UAV)
			{
				InCommandList->SetUnorderedAccessView(InStage, InBinding.OffsetInTable, DefaultTexture);
			}
			else if (InBinding.Kind == EShaderBindingKindType::SHADER_BINDING_KIND_SRV)
			{
				InCommandList->SetShaderResourceView(InStage, InBinding.OffsetInTable, DefaultTexture);
			}
			return;
		}
		IRRHIBuffer* DefaultBuffer = static_cast<IRRHIBuffer*>(DefaultResource);
		if (InBinding.Kind == EShaderBindingKindType::SHADER_BINDING_KIND_UAV)
		{
			InCommandList->SetUnorderedAccessView(InStage, InBinding.OffsetInTable, DefaultBuffer);
		}
		else if (InBinding.Kind == EShaderBindingKindType::SHADER_BINDING_KIND_SRV)
		{
			InCommandList->SetShaderResourceView(InStage, InBinding.OffsetInTable, DefaultBuffer);
		}
	}

	// The name of the constant buffer member that carries a resource's heap index:
	// "<ResourceName>_BindlessIdx". The suffix is the token the compiler declared the field with
	// (EngineShaderConfig.h), which is what keeps the two sides from spelling the same name twice.
	static EName MakeBindlessIndexFieldName(const EName& InResourceName)
	{
		return EName(InResourceName.ToString() + EString(PE_TEXT("_")) + EString(PE_TEXT(MC_SHADER_BINDLESS_INDEX_SUFFIX)));
	}

	static BOOL32 WriteShaderBindlessIndices(EShaderParameterSet& InOutParameters, const EShaderProgram& InProgram
		, IRRHIDevice* InRenderDevice, const EName& InScopeName)
	{
		RRHIBindlessManager* BindlessManager = RRHIBindlessManager::GetManagerSingleton();
		BOOL32 bAllWritten = TRUE;
		// Whether the configured target can express the heap path is asked at most once per call,
		// and only after a program turned out to have an index driven resource: a target that cannot
		// express it reports Bindless = false everywhere, so those builds never pay for the check,
		// and the ones that do take the path get the one answer that catches an artifact left over
		// from another target.
		BOOL32 bTargetChecked = FALSE;

		for (UINT32 StageIndex = 0u, StageCount = InProgram.GetStageEntryNum(); StageIndex < StageCount; StageIndex++)
		{
			const EShaderStageEntry* StageEntry = InProgram.GetStageEntryByIndex(StageIndex);
			if ((!StageEntry) || (StageEntry->bValid == FALSE))
			{
				continue;
			}
			const EShaderBindingPlan& Plan = StageEntry->BindingPlan;
			for (UINT32 BindingIndex = 0u, BindingNum = Plan.GetResourceBindingNum(); BindingIndex < BindingNum; BindingIndex++)
			{
				const EShaderResourceBinding* Binding = Plan.GetResourceBindingByIndex(BindingIndex);
				if ((!Binding) || (Binding->bBindless == FALSE))
				{
					continue;
				}

				if (bTargetChecked == FALSE)
				{
					bTargetChecked = TRUE;
					EShaderTarget ActiveTarget;
					if ((ResolveActiveShaderTarget(ActiveTarget)) && (DoesShaderTargetSupportBindless(ActiveTarget) == FALSE))
					{
						PE_LOG_ERROR(EString(PE_TEXT("[ShaderAssetBinding] The program '")) + InProgram.GetShaderName().ToString()
							+ EString(PE_TEXT("' is index driven but the configured target cannot express the descriptor heap path; see the shader target tables.")));
						bAllWritten = FALSE;
					}
				}

				if (Binding->Kind == EShaderBindingKindType::SHADER_BINDING_KIND_SAMPLER)
				{
					// The sampler heap has no allocation path (the device registers resources and
					// buffers only), so an index driven sampler is refused here rather than given an
					// index that would name the wrong heap.
					PE_LOG_ERROR(EString(PE_TEXT("[ShaderAssetBinding] The sampler '")) + Binding->Name.ToString()
						+ EString(PE_TEXT("' declares the heap path, which this engine cannot allocate an index for yet.")));
					bAllWritten = FALSE;
					continue;
				}
				if (Binding->BindlessConstantBuffer.IsValid() == FALSE)
				{
					PE_LOG_ERROR(EString(PE_TEXT("[ShaderAssetBinding] The resource '")) + Binding->Name.ToString()
						+ EString(PE_TEXT("' is index driven but its reflection names no index block.")));
					bAllWritten = FALSE;
					continue;
				}

				// Which object is published for it. A resource value carries either a texture or a
				// structured buffer, and the index names whichever one the value holds.
				const EShaderParameterValue* Value = InOutParameters.FindResourceOverride(Binding->Name);
				IRRHIResource* Resource = nullptr;
				if (Value)
				{
					if (Value->Texture)
					{
						Resource = const_cast<IRRHITexture*>(Value->Texture);
					}
					else if (Value->StructuredBuffer != nullptr)
					{
						Resource = const_cast<RStructuredBuffer*>(Value->StructuredBuffer)->GetResolvedRHIBuffer();
					}
				}
				if (!Resource)
				{
					if (Binding->ClaimState == EShaderBindingClaimStateType::SHADER_BINDING_CLAIM_STATE_EXPOSURE)
					{
						// Exposed but not authored yet: a legal intermediate state, and the one
						// case an engine default exists for. The index then names a defined black
						// resource instead of whatever descriptor the heap held at slot 0.
						Resource = ResolveDefaultBindingResource(InRenderDevice, *Binding);
						if (!Resource)
						{
							PE_LOG_ERROR(EString(PE_TEXT("[ShaderAssetBinding] The exposed binding '")) + Binding->Name.ToString()
								+ EString(PE_TEXT("' of program '")) + InProgram.GetShaderName().ToString()
								+ EString(PE_TEXT("' has no value and the engine default resource could not be created.")));
							bAllWritten = FALSE;
						}
					}
					else if (ShouldReportUnboundBinding(*Binding, InScopeName) != FALSE)
					{
						// A claimed binding with one owner: this publisher is that owner and has
						// no value for it, which is a sampling defect rather than a state. The
						// index field keeps its default, which names the heap's null sentinel, so
						// the draw is refused instead of being drawn without the resource - and
						// both the skip and the reason are reported, because an object that
						// quietly stops appearing reads as a scene bug rather than as this one.
						ReportSkippedDraw(EString(PE_TEXT("the index driven resource '")) + Binding->Name.ToString()
							+ EString(PE_TEXT("' of program '")) + InProgram.GetShaderName().ToString() + EString(PE_TEXT("'")),
							EString(PE_TEXT("no value was published for it, so the draw is refused instead of reading the null sentinel")));
						ReportUnpublishedBindlessIndex(InProgram, *Binding);
						bAllWritten = FALSE;
					}
					// An unclaimed binding was reported at plan build; a shared claim cannot tell
					// which scope this draw was supposed to get its value from. Neither is this
					// publisher's to fail: an owner that publishes the index into its own set is
					// what the shared claim describes. If none does, the field keeps its default
					// and names the null sentinel, which is a null view rather than an
					// uninitialized descriptor.
					if (!Resource)
					{
						continue;
					}
				}

				const EName IndexFieldName = MakeBindlessIndexFieldName(Binding->Name);
				// A bindless resource stays in the plan even when the compiler culled it: the
				// layout's heap flags are driven from the plan, and the index write is what the
				// heap path would need. When the resource's index field is culled as well, nothing
				// in this stage reads it - the write below would land in SetUIntArray's culled
				// no-op, and the idempotence check would then fail on every bind. A depth only
				// shadow program is the common case: its whole material block is culled, so this
				// used to allocate an index and emit a log line per material bind, every frame.
				// Skipping here leaves the field at its default, which is what a stage that never
				// reads it wants.
				if (StageEntry->Reflection.FindParameterTarget(Binding->BindlessConstantBuffer, IndexFieldName)
					== EShaderParameterTargetType::SHADER_PARAMETER_TARGET_CULLED)
				{
					continue;
				}

				const ERHIBindlessIndexKind IndexKind = ((Binding->Kind == EShaderBindingKindType::SHADER_BINDING_KIND_UAV)
					? ERHIBindlessIndexKind::RHI_BINDLESS_INDEX_UAV
					: ERHIBindlessIndexKind::RHI_BINDLESS_INDEX_SRV);
				const UINT32 Index = BindlessManager->AcquireIndex(Resource, IndexKind);
				if (Index == RHI_INVALID_BINDLESS_INDEX)
				{
					PE_LOG_ERROR(EString(PE_TEXT("[ShaderAssetBinding] No descriptor heap index could be allocated for the resource '"))
						+ Binding->Name.ToString() + EString(PE_TEXT("'.")));
					bAllWritten = FALSE;
					continue;
				}

				// Idempotent on purpose: writing the same index every frame would mark the mirror
				// dirty every frame and upload a constant buffer whose content did not change.
				const EShaderParameterValue* Existing = InOutParameters.FindOverride(Binding->BindlessConstantBuffer, IndexFieldName);
				if ((Existing) && (Existing->RawValue[0u] == Index))
				{
					continue;
				}

				// The field is a uint in the reflection, so it is written as raw 32 bit words: the
				// float setter would store the bit pattern of the index instead of the index.
				if (!InOutParameters.SetUIntArray(Binding->BindlessConstantBuffer, IndexFieldName, &Index, 1u))
				{
					PE_LOG_ERROR(EString(PE_TEXT("[ShaderAssetBinding] The index of the resource '")) + Binding->Name.ToString()
						+ EString(PE_TEXT("' could not be written into its parameter set.")));
					bAllWritten = FALSE;
				}
				else
				{
					// Once per resource, because the write above is skipped while the index does not
					// move: this is the only place the number the shader will read is visible.
					PE_LOG_LOG(EString(PE_TEXT("[ShaderAssetBinding] '")) + InProgram.GetShaderName().ToString()
						+ EString(PE_TEXT("' indexes '")) + Binding->Name.ToString() + EString(PE_TEXT("' at heap index "))
						+ ToString(Index) + EString(PE_TEXT(" through ")) + Binding->BindlessConstantBuffer.ToString()
						+ EString(PE_TEXT(".")) + IndexFieldName.ToString() + EString(PE_TEXT(".")));
				}
			}
		}
		return bAllWritten;
	}

	void BindShaderParameterResources(IRCommandList* InCommandList, const EShaderParameterSet& InParameters
		, const EShaderParameterSet* InOverrideParameters, const EShaderBindingPlan& InBindingPlan
		, const ERHIShaderStage InStage, const EName& InScopeName)
	{
		if (!InCommandList)
		{
			return;
		}
		// Reporting is the owner's job. A stage's plan is the union of every component the program
		// compiled in, while a set carries one module's values, so only a binding this scope is the
		// single claimant of counts as this publisher's hole. A value that is present but unusable
		// is always reported: the publisher did attempt to provide it.
		const auto ReportUnboundResource = [InScopeName, InStage](const EName& InResourceName, const BOOL32 InIsOwned)->void
		{
			if (InIsOwned != FALSE)
			{
				ReportUnboundShaderResource(InResourceName, InStage, InScopeName);
			}
		};
		for (UINT32 BindingIndex = 0u, BindingNum = InBindingPlan.GetResourceBindingNum(); BindingIndex < BindingNum; BindingIndex++)
		{
			const EShaderResourceBinding* Binding = InBindingPlan.GetResourceBindingByIndex(BindingIndex);
			if (!Binding)
			{
				continue;
			}
			// An index driven resource is not bound at a slot at all: the shader reaches it through
			// the descriptor heap with the index its constant buffer carries, which is what
			// WriteShaderBindlessIndices filled in before the buffer was uploaded. Binding it here
			// as well would cost a descriptor copy and, worse, leave two paths that can disagree
			// about which resource a draw actually reads.
			if (Binding->bBindless != FALSE)
			{
				continue;
			}
			// A sampler carries no value in this model - the renderer binds its table once per frame -
			// so only the resource kinds are the ones a caller has to publish.
			const BOOL32 bNeedsValue = ((Binding->Kind == EShaderBindingKindType::SHADER_BINDING_KIND_SRV)
				|| (Binding->Kind == EShaderBindingKindType::SHADER_BINDING_KIND_UAV)) ? TRUE : FALSE;
			// A per draw resource (the water simulation outputs) outranks the material's own
			// value of the same name. The base set is still the owner of every binding the
			// override set does not carry.
			const EShaderParameterValue* Value = ((InOverrideParameters != nullptr)
				? InOverrideParameters->FindResourceOverride(Binding->Name) : nullptr);
			if (!Value)
			{
				Value = InParameters.FindResourceOverride(Binding->Name);
			}
			if (!Value)
			{
				if (Binding->ClaimState == EShaderBindingClaimStateType::SHADER_BINDING_CLAIM_STATE_EXPOSURE)
				{
					// The reflection says this declaration belongs to the editor side and it has
					// no value yet. That is legal, and the draw reads a defined black resource
					// instead of whatever the slot happened to hold.
					BindDefaultBindingResource(InCommandList, InStage, *Binding);
				}
				else if (bNeedsValue != FALSE)
				{
					ReportUnboundResource(Binding->Name, ShouldReportUnboundBinding(*Binding, InScopeName));
				}
				continue;
			}
			// Where the resource lands inside its table. Which table that is belongs to the
			// bound pipeline's layout, so the binding side only needs the offset; the plan
			// carries the table root as well, for callers that have to check it.
			const UINT32 Slot = Binding->OffsetInTable;
			// Which pointer a value carries is what decides the call. A value normally carries
			// exactly one, but a resource that was written twice (once as a texture, once as a
			// structured buffer) carries both, so the one matching the value's kind wins and the
			// other stays a fallback rather than being bound by accident.
			const BOOL32 bKindIsBuffer = (((Value->Kind == EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_BUFFER)
				|| (Value->Kind == EShaderParameterValueKindType::SHADER_PARAMETER_VALUE_KIND_UAV)) ? TRUE : FALSE);
			IRRHITexture* ValueTexture = const_cast<IRRHITexture*>(Value->Texture);
			// A structured buffer that was declared but never created binds nothing. Without this
			// test the wrapper would be handed to the command list as if it were the back-end
			// object, which is a null native resource at best - and on D3D12 a cast to the wrong
			// interface, because an uncreated buffer has no RD3D12Buffer behind it. A renderer
			// with no sky is the case that reaches here: its environment buffer is never built.
			IRRHIBuffer* ValueBuffer = (((Value->StructuredBuffer != nullptr) && (Value->StructuredBuffer->IsRenderResourceValid() != FALSE))
				? const_cast<RStructuredBuffer*>(Value->StructuredBuffer)->GetResolvedRHIBuffer() : nullptr);
			const BOOL32 bUseBuffer = (((bKindIsBuffer != FALSE) && (ValueBuffer != nullptr)) || (ValueTexture == nullptr)) ? TRUE : FALSE;
			switch (Binding->Kind)
			{
			case EShaderBindingKindType::SHADER_BINDING_KIND_SRV:
				if (bUseBuffer != FALSE)
				{
					if (ValueBuffer != nullptr)
					{
						InCommandList->SetShaderResourceView(InStage, Slot, ValueBuffer);
					}
					else
					{
						ReportUnboundResource(Binding->Name, TRUE);
					}
				}
				else if (ValueTexture != nullptr)
				{
					InCommandList->SetShaderResourceView(InStage, Slot, ValueTexture);
				}
				else
				{
					ReportUnboundResource(Binding->Name, TRUE);
				}
				break;
			case EShaderBindingKindType::SHADER_BINDING_KIND_UAV:
				if (Value->TextureUAV != nullptr)
				{
					// A subresource view wins over the resource: the binding is one mip of a
					// chain, and the resource's own UAV handle cannot name it.
					InCommandList->SetUnorderedAccessView(InStage, Slot, const_cast<IRRHITextureUAVView*>(Value->TextureUAV));
					break;
				}
				if (bUseBuffer != FALSE)
				{
					if (ValueBuffer != nullptr)
					{
						InCommandList->SetUnorderedAccessView(InStage, Slot, ValueBuffer);
					}
					else
					{
						ReportUnboundResource(Binding->Name, TRUE);
					}
				}
				else if (ValueTexture != nullptr)
				{
					InCommandList->SetUnorderedAccessView(InStage, Slot, ValueTexture);
				}
				else
				{
					ReportUnboundResource(Binding->Name, TRUE);
				}
				break;
			case EShaderBindingKindType::SHADER_BINDING_KIND_SAMPLER:
				// A sampler value carries a description, not an object, so there is nothing to
				// hand the command list here: the engine binds its renderer wide sampler set
				// once per frame (SceneRenderer) and has no per material sampler objects yet.
				// The day it grows them, this is the one place that has to change.
				break;
			default:
				break;
			}
		}
	}

	// ---------------------------------------------------------------------------
	// Publishing
	// ---------------------------------------------------------------------------

	static BOOL32 StageMaskContains(const UINT32 InStageMask, const EShaderStageType InStage)
	{
		return (((InStageMask & (1u << static_cast<UINT32>(InStage))) != 0u) ? TRUE : FALSE);
	}

	/*
	* The one structural mistake this layer can attribute: a set publishes an index driven resource
	* but carries no field for its index.
	*
	* That is what a resource-only publisher looks like - it has no constant buffer to put the index
	* in - and it is not an authored error, so it is named here rather than left as a silent read of
	* whatever the block happens to hold. A resource the set publishes no value for is not this
	* publisher's business and is skipped: every engine path feeds a subset of a stage's plan.
	*/
	static BOOL32 CheckBindlessIndicesPublished(const EShaderParameterSet& InParameters, const EShaderBindingPlan& InBindingPlan)
	{
		BOOL32 bAllPublished = TRUE;
		for (UINT32 BindingIndex = 0u, BindingNum = InBindingPlan.GetResourceBindingNum(); BindingIndex < BindingNum; BindingIndex++)
		{
			const EShaderResourceBinding* Binding = InBindingPlan.GetResourceBindingByIndex(BindingIndex);
			if ((!Binding) || (Binding->bBindless == FALSE))
			{
				continue;
			}
			if (!InParameters.FindResourceOverride(Binding->Name))
			{
				continue;
			}
			const EName IndexFieldName = MakeBindlessIndexFieldName(Binding->Name);
			if (InParameters.FindOverride(Binding->BindlessConstantBuffer, IndexFieldName))
			{
				continue;
			}
			PE_LOG_ERROR(EString(PE_TEXT("[ShaderAssetBinding] The set publishes the index driven resource '"))
				+ Binding->Name.ToString() + EString(PE_TEXT("' but carries no '")) + IndexFieldName.ToString()
				+ EString(PE_TEXT("'. A publisher without constant buffers cannot host it; publish through PublishShaderParameters.")));
			bAllPublished = FALSE;
		}
		return bAllPublished;
	}

	BOOL32 PublishShaderParameterValues(IRRHIDevice* InRenderDevice
		, EShaderParameterSet& InOutParameters, RShaderParameterBuffers& InOutBuffers
		, const EShaderProgram& InProgram, const EName& InScopeName)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}

		// The index of an index driven resource is a value of a constant buffer, so it has to be
		// inside the bytes that upload. Resolving it cannot be a separate step a caller could
		// order after the upload.
		if (!WriteShaderBindlessIndices(InOutParameters, InProgram, InRenderDevice, InScopeName))
		{
			return FALSE;
		}
		// Writing an index changes a block's content, so the block is re-evaluated here rather than
		// trusting a prepare that ran before the write. A set whose blocks were already evaluated
		// pays one revision compare; one that was not would otherwise upload last frame's bytes.
		if (!InOutParameters.PrepareBuffers())
		{
			return FALSE;
		}
		if (!InOutBuffers.UploadFromParameterSet(InRenderDevice, InOutParameters))
		{
			return FALSE;
		}
		return TRUE;
	}

	void BindPublishedShaderParameters(IRCommandList* InCommandList
		, const EShaderParameterSet& InParameters, RShaderParameterBuffers& InOutBuffers
		, const EShaderProgram& InProgram, const UINT32 InStageMask, const EName& InScopeName)
	{
		if (!InCommandList)
		{
			return;
		}
		for (UINT32 StageIndex = 0u, StageCount = InProgram.GetStageEntryNum(); StageIndex < StageCount; StageIndex++)
		{
			const EShaderStageEntry* StageEntry = InProgram.GetStageEntryByIndex(StageIndex);
			if ((!StageEntry) || (StageEntry->bValid == FALSE) || (StageMaskContains(InStageMask, StageEntry->Stage) == FALSE))
			{
				continue;
			}
			ERHIShaderStage RHIStage;
			if (TranslateShaderStageToRHI(StageEntry->Stage, RHIStage) == FALSE)
			{
				continue;
			}
			const BOOL32 bIsCompute = ((RHIStage == ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE) ? TRUE : FALSE);
			// Blocks first, then the register bound resources. Both read their root from the plan,
			// so neither a root index nor a literal slot is written down here.
			BindShaderParameterBuffers(InCommandList, InOutBuffers, InProgram, StageEntry->Stage, bIsCompute);
			BindShaderParameterResources(InCommandList, InParameters, nullptr, StageEntry->BindingPlan, RHIStage, InScopeName);
		}
	}

	BOOL32 PublishShaderParameters(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice
		, EShaderParameterSet& InOutParameters, RShaderParameterBuffers& InOutBuffers
		, const EShaderProgram& InProgram, const UINT32 InStageMask, const EName& InScopeName)
	{
		if ((!InCommandList) || (!InRenderDevice))
		{
			return FALSE;
		}
		if (!PublishShaderParameterValues(InRenderDevice, InOutParameters, InOutBuffers, InProgram, InScopeName))
		{
			return FALSE;
		}
		BindPublishedShaderParameters(InCommandList, InOutParameters, InOutBuffers, InProgram, InStageMask, InScopeName);
		return TRUE;
	}

	BOOL32 BindShaderResources(IRCommandList* InCommandList, const EShaderParameterSet& InParameters
		, const EShaderProgram& InProgram, const UINT32 InStageMask, const EName& InScopeName)
	{
		if (!InCommandList)
		{
			return FALSE;
		}
		BOOL32 bResult = TRUE;
		for (UINT32 StageIndex = 0u, StageCount = InProgram.GetStageEntryNum(); StageIndex < StageCount; StageIndex++)
		{
			const EShaderStageEntry* StageEntry = InProgram.GetStageEntryByIndex(StageIndex);
			if ((!StageEntry) || (StageEntry->bValid == FALSE) || (StageMaskContains(InStageMask, StageEntry->Stage) == FALSE))
			{
				continue;
			}
			ERHIShaderStage RHIStage;
			if (TranslateShaderStageToRHI(StageEntry->Stage, RHIStage) == FALSE)
			{
				continue;
			}
			if (CheckBindlessIndicesPublished(InParameters, StageEntry->BindingPlan) == FALSE)
			{
				bResult = FALSE;
				continue;
			}
			BindShaderParameterResources(InCommandList, InParameters, nullptr, StageEntry->BindingPlan, RHIStage, InScopeName);
		}
		return bResult;
	}

	BOOL32 BindShaderResources(IRCommandList* InCommandList, const EShaderParameterSet& InParameters
		, const EShaderParameterSet& InOverrideParameters, const EShaderProgram& InProgram
		, const UINT32 InStageMask, const EName& InScopeName)
	{
		if (!InCommandList)
		{
			return FALSE;
		}
		BOOL32 bResult = TRUE;
		for (UINT32 StageIndex = 0u, StageCount = InProgram.GetStageEntryNum(); StageIndex < StageCount; StageIndex++)
		{
			const EShaderStageEntry* StageEntry = InProgram.GetStageEntryByIndex(StageIndex);
			if ((!StageEntry) || (StageEntry->bValid == FALSE) || (StageMaskContains(InStageMask, StageEntry->Stage) == FALSE))
			{
				continue;
			}
			ERHIShaderStage RHIStage;
			if (TranslateShaderStageToRHI(StageEntry->Stage, RHIStage) == FALSE)
			{
				continue;
			}
			// The bindless index fields live in the base set's constant buffers; the override
			// set only carries the register bound resources this draw replaces.
			if (CheckBindlessIndicesPublished(InParameters, StageEntry->BindingPlan) == FALSE)
			{
				bResult = FALSE;
				continue;
			}
			BindShaderParameterResources(InCommandList, InParameters, &InOverrideParameters
				, StageEntry->BindingPlan, RHIStage, InScopeName);
		}
		return bResult;
	}

	BOOL32 BindShaderResources(IRCommandList* InCommandList, const EShaderParameterSet& InParameters
		, const EShaderBindingPlan& InBindingPlan, const ERHIShaderStage InStage, const EName& InScopeName)
	{
		if (!InCommandList)
		{
			return FALSE;
		}
		if (CheckBindlessIndicesPublished(InParameters, InBindingPlan) == FALSE)
		{
			return FALSE;
		}
		BindShaderParameterResources(InCommandList, InParameters, nullptr, InBindingPlan, InStage, InScopeName);
		return TRUE;
	}

} // namespace PigeonEngine
