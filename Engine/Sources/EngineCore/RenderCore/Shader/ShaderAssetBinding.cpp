#include <Shader/ShaderAssetBinding.h>

#include <Main/MainManager.h>
#include <Shader/ShaderEngineParameters.h>
#include <Shader/ShaderParameterBuffers.h>
#include <RenderResourceLifetime.h>
#include <RHI/IRCommandList.h>
#include <RHI/IRRHIDevice.h>
#include <Core/Config/EngineConfig.h>
#include <Editor/EditorLogManager.h>
#include <IO/FileHelper.h>
#include <ShaderAsset/ShaderCompileJob.h>
#include <ShaderAsset/ShaderInstance.h>

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

	BOOL32 FillGraphicsPipelineShaders(const EShaderProgram& InProgram, RRHIGraphicsPipelineDesc& OutDesc)
	{
		const RShaderStageResource* VertexStage = ResolveShaderStageResource(InProgram, EShaderStageType::SHADER_STAGE_VERTEX);
		if (!VertexStage)
		{
			return FALSE;
		}
		const RShaderStageResource* PixelStage = ResolveShaderStageResource(InProgram, EShaderStageType::SHADER_STAGE_PIXEL);
		if (!PixelStage)
		{
			return FALSE;
		}

		OutDesc.VertexShader		= VertexStage->GetRHIShader();
		OutDesc.PixelShader			= PixelStage->GetRHIShader();
		// The input layout belongs to the vertex stage: it was built from that stage's
		// reflection, and it is the same description the vertex shader object was
		// created with, so the PSO and the shader cannot disagree.
		OutDesc.InputLayouts		= VertexStage->GetInputLayouts().GetData();
		OutDesc.InputLayoutCount	= VertexStage->GetInputLayouts().Num<UINT32>();
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
	// feature that ran and produced nothing. Named once per (resource, stage), because naming it per
	// draw would bury the frame in the same line.
	static void ReportUnboundShaderResource(const EName& InName, const ERHIShaderStage InStage)
	{
		const EString ReportKey = InName.ToString() + EString(PE_TEXT("|")) + ToString(static_cast<UINT32>(InStage));
		TArray<EString>& ReportedUnboundResources = GetReportedUnboundResources();
		if (ReportedUnboundResources.Contains(ReportKey))
		{
			return;
		}
		ReportedUnboundResources.Add(ReportKey);
		PE_LOG_ERROR(EString(PE_TEXT("[ShaderAssetBinding] Stage ")) + ToString(static_cast<UINT32>(InStage))
			+ EString(PE_TEXT(" reads '")) + InName.ToString()
			+ EString(PE_TEXT("' but no value was published for it: the slot keeps whatever it held.")));
	}

	void BindShaderParameterResources(IRCommandList* InCommandList, const EShaderParameterSet& InParameters
		, const EShaderBindingPlan& InBindingPlan, const ERHIShaderStage InStage)
	{
		if (!InCommandList)
		{
			return;
		}
		for (UINT32 BindingIndex = 0u, BindingNum = InBindingPlan.GetResourceBindingNum(); BindingIndex < BindingNum; BindingIndex++)
		{
			const EShaderResourceBinding* Binding = InBindingPlan.GetResourceBindingByIndex(BindingIndex);
			if (!Binding)
			{
				continue;
			}
			// A sampler carries no value in this model - the renderer binds its table once per frame -
			// so only the resource kinds are the ones a caller has to publish.
			const BOOL32 bNeedsValue = ((Binding->Kind == EShaderBindingKindType::SHADER_BINDING_KIND_SRV)
				|| (Binding->Kind == EShaderBindingKindType::SHADER_BINDING_KIND_UAV)) ? TRUE : FALSE;
			const EShaderParameterValue* Value = InParameters.FindResourceOverride(Binding->Name);
			if (!Value)
			{
				if (bNeedsValue != FALSE)
				{
					ReportUnboundShaderResource(Binding->Name, InStage);
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
			IRRHIBuffer* ValueBuffer = ((Value->StructuredBuffer != nullptr)
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
						ReportUnboundShaderResource(Binding->Name, InStage);
					}
				}
				else if (ValueTexture != nullptr)
				{
					InCommandList->SetShaderResourceView(InStage, Slot, ValueTexture);
				}
				else
				{
					ReportUnboundShaderResource(Binding->Name, InStage);
				}
				break;
			case EShaderBindingKindType::SHADER_BINDING_KIND_UAV:
				if (bUseBuffer != FALSE)
				{
					if (ValueBuffer != nullptr)
					{
						InCommandList->SetUnorderedAccessView(InStage, Slot, ValueBuffer);
					}
					else
					{
						ReportUnboundShaderResource(Binding->Name, InStage);
					}
				}
				else if (ValueTexture != nullptr)
				{
					InCommandList->SetUnorderedAccessView(InStage, Slot, ValueTexture);
				}
				else
				{
					ReportUnboundShaderResource(Binding->Name, InStage);
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

} // namespace PigeonEngine
