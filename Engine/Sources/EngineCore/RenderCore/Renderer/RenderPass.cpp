#include "RenderPass.h"

#include <Main/MainManager.h>
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/ViewProxy.h>
#include <Renderer/RenderData.h>
#include <Renderer/RenderFilter.h>
#include <Renderer/ScenePass.h>
#include <Renderer/SceneRenderer.h>
#include <Renderer/ViewPrimitiveList.h>

namespace PigeonEngine
{

	void FPassStatePlan::Reset()
	{
		Stages.SetNum(static_cast<UINT32>(ERenderPassStageType::RENDER_PASS_STAGE_COUNT));
		for (UINT32 StageIndex = 0u, StageNum = Stages.Num<UINT32>(); StageIndex < StageNum; StageIndex++)
		{
			Stages[StageIndex].Stage = static_cast<ERenderPassStageType>(StageIndex);
			Stages[StageIndex].Transitions.SetNum(0);
			Stages[StageIndex].Clears.SetNum(0);
		}
	}

	void FPassStatePlan::AddTransition(const ERenderPassStageType InStage, IRRHIResource* InResource, const ERHIResourceState InAfter)
	{
		FPassStageState* Stage = FindStage(InStage);
		if ((!Stage) || (!InResource))
		{
			return;
		}
		FRHITransition Transition;
		Transition.Resource	= InResource;
		Transition.After	= InAfter;
		Stage->Transitions.Add(Transition);
	}

	void FPassStatePlan::AddClear(const ERenderPassStageType InStage, const FRHIClear& InClear)
	{
		FPassStageState* Stage = FindStage(InStage);
		if ((!Stage) || (!InClear.Resource))
		{
			return;
		}
		Stage->Clears.Add(InClear);
	}

	FPassStageState* FPassStatePlan::FindStage(const ERenderPassStageType InStage)
	{
		const UINT32 StageIndex = static_cast<UINT32>(InStage);
		if (StageIndex >= Stages.Num<UINT32>())
		{
			return nullptr;
		}
		return &(Stages[StageIndex]);
	}

	FRenderPass::FRenderPass(const FRenderPassDesc& InPassDesc)
		: PassDesc(&InPassDesc)
	{
	}

	const FRenderPassDesc& FRenderPass::GetPassDesc() const
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("RenderPass: a pass has no description.")), (PassDesc != nullptr));
		return *PassDesc;
	}

	void FRenderPass::CollectItems(const FPassViewContext& InContext, const RViewPrimitiveList& InViewList, TArray<FPassItem>& OutItems) const
	{
		if (((!InContext.View) || (!InContext.ViewData) || (!InContext.FrameData)) || (ShouldUseView(*InContext.View) == FALSE))
		{
			return;
		}

		const UINT64 FrameIndex = InContext.FrameData->GetFrameIndex();
		for (UINT32 ItemIndex = 0u, ItemNum = InViewList.Num(); ItemIndex < ItemNum; ItemIndex++)
		{
			const RViewPrimitiveItem& ViewItem = InViewList[ItemIndex];
			if (!ViewItem.Primitive)
			{
				continue;
			}

			FRenderFilterContext FilterContext;
			FilterContext.View		= InContext.View;
			FilterContext.PassDesc	= PassDesc;
			FilterContext.Primitive	= ViewItem.Primitive;
			FilterContext.FrameIndex = FrameIndex;
			if (ShouldDrawItem(FilterContext) == FALSE)
			{
				continue;
			}

			// The item carries a view selection only when the view overrode the primitive's
			// material. Null is the common case: the pass data then reads the slot's own
			// reported instance, so one slot's material never answers for another slot.
			const EShaderInstance* ViewShaderInstanceOverride = ViewItem.ShaderInstanceOverride;

			FPrimitiveRenderData* PrimitiveData = InContext.FrameData->EnsurePrimitive(*ViewItem.Primitive);
			if (!PrimitiveData)
			{
				continue;
			}

			const UINT32 GroupNum = ViewItem.Primitive->GetDrawGroupCount();
			for (UINT32 GroupIndex = 0u; GroupIndex < GroupNum; GroupIndex++)
			{
				UINT32 SlotIndex = 0u;
				UINT32 IndexCount = 0u;
				UINT32 StartIndex = 0u;
				if (ViewItem.Primitive->GetDrawGroupDesc(GroupIndex, SlotIndex, IndexCount, StartIndex) == FALSE)
				{
					continue;
				}

				FPrimitivePassData* PassData = InContext.FrameData->EnsurePrimitivePass(*ViewItem.Primitive
					, GetPassDesc(), ViewShaderInstanceOverride, SlotIndex);
				if ((!PassData) || (!PassData->Program) || (!PassData->PipelineState))
				{
					continue;
				}

				// The view and global blocks are published before the first item that binds them.
				// Both calls are Prepare phase operations; the stamps in FFrameRenderData make
				// them idempotent across every item and every pass that shares the view.
				IRRHIDevice* RenderDevice = InContext.FrameData->GetRenderDevice();
				RSceneRenderer* Renderer = EMainManager::GetManagerSingleton()->GetSceneRenderer();
				if (RenderDevice)
				{
					if (Renderer)
					{
						Renderer->EnsureGlobalParameterBlock(RenderDevice, *PassData->Program);
					}
					if (InContext.View->EnsureParameterBlock(RenderDevice, *PassData->Program) == FALSE)
					{
						continue;
					}
				}

				FPassItem PassItem;
				PassItem.Primitive			= ViewItem.Primitive;
				PassItem.PrimitiveData		= PrimitiveData;
				PassItem.PrimitivePassData	= PassData;
				PassItem.ShaderInstanceData	= PassData->ShaderInstanceData;
				PassItem.BindingSet			= InContext.FrameData->EnsureBindingSet(*PassData, *InContext.ViewData);
				PassItem.SlotIndex			= SlotIndex;
				PassItem.IndexCount			= IndexCount;
				PassItem.StartIndex			= StartIndex;
				OutItems.Add(PassItem);
			}
		}
	}

	void FRenderPass::PrepareItems(FFrameRenderData& InFrameData, TArray<FPassItem>& InOutItems) const
	{
		// The default item path publishes everything it needs while collecting: the view and
		// primitive stamps in FFrameRenderData make those calls idempotent, and the shared
		// shader instance data is published by EnsurePrimitivePass.
		(void)InFrameData;
		(void)InOutItems;
	}

	void FRenderPass::DeclareState(FPassStatePlan& InOutPlan) const
	{
		(void)InOutPlan;
	}

	void FRenderPass::BeginPass(FPassRecordContext& InContext) const
	{
		(void)InContext;
	}

	void FRenderPass::EndPass(FPassRecordContext& InContext) const
	{
		(void)InContext;
	}

	BOOL32 FRenderPass::ShouldDrawItem(const FRenderFilterContext& InContext) const
	{
		if (FRenderFilterRegistry::ShouldDraw(InContext) == FALSE)
		{
			return FALSE;
		}
		if (ItemPredicate)
		{
			return ItemPredicate(InContext);
		}
		return TRUE;
	}

}	// namespace PigeonEngine
