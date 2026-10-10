#include "MeshComponent.h"
#include <RenderProxy/MeshSceneProxy.h>
#include <Renderer/ScenePass.h>
#include <ShaderAsset/ShaderInstance.h>

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		RegisterClassType<PMeshComponent, PPrimitiveComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PMeshComponent::PMeshComponent()
	{
		POBJ_DEBUGNAME_SET(this, "MeshComponent");
	}
	PMeshComponent::~PMeshComponent()
	{
	}

	const EShaderInstance* PMeshComponent::GetMaterialInstance(UINT32 InSlotIdx) const
	{
		if (InSlotIdx < MaterialSlots.Num<UINT32>())
		{
			return (MaterialSlots[InSlotIdx]);
		}
		return nullptr;
	}
	UINT32 PMeshComponent::GetMaterialSlotCount() const
	{
		return (MaterialSlots.Num<UINT32>());
	}
	void PMeshComponent::SetMaterialInstance(const EShaderInstance* InMaterialInstance)
	{
		if (MaterialSlots.Num<INT32>() <= 0)
		{
			MaterialSlots.Add(nullptr);
		}
		MaterialSlots[0] = InMaterialInstance;
		MarkMaterialAsDirty();
	}
	void PMeshComponent::SetMaterialInstance(UINT32 InSlotIdx, const EShaderInstance* InMaterialInstance)
	{
		if (InSlotIdx >= MaterialSlots.Num<UINT32>())
		{
			MaterialSlots.SetNum(InSlotIdx + 1);
		}
		MaterialSlots[InSlotIdx] = InMaterialInstance;
		MarkMaterialAsDirty();
	}
	void PMeshComponent::SetMaterialSlotsNum(UINT32 InNum)
	{
		const UINT32 OldNum = MaterialSlots.Num<UINT32>();
		MaterialSlots.SetNum(InNum);
		for (UINT32 i = OldNum; i < InNum; i++)
		{
			MaterialSlots[i] = nullptr;
		}
	}
	const EAssetBase* PMeshComponent::GetMeshAssetBase() const
	{
		return MeshAssetBase;
	}
	void PMeshComponent::SetMeshAssetBase(const EAssetBase* InMeshAsset)
	{
		MeshAssetBase = InMeshAsset;
	}
	void PMeshComponent::CollectRenderAssets(FPrimitiveRenderAssetReport& InOutReport) const
	{
		InOutReport.SetMeshAsset(MeshAssetBase);

		// A slot is reported under the primary pass, which is the pass a material's own shader
		// asset is assembled for. The render side resolves the other passes of the material
		// through the fallback path until materials carry a full pass table.
		const FRenderPassDesc& PrimaryPassDesc = FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_BASE);
		if (PrimaryPassDesc.PassName == nullptr)
		{
			return;
		}
		const EName PrimaryPassName(PrimaryPassDesc.PassName);
		for (UINT32 SlotIndex = 0u, SlotNum = MaterialSlots.Num<UINT32>(); SlotIndex < SlotNum; SlotIndex++)
		{
			if (MaterialSlots[SlotIndex])
			{
				InOutReport.SetMaterialInstance(SlotIndex, PrimaryPassName, MaterialSlots[SlotIndex]);
			}
		}
	}

	// Render proxy functions START
	void PMeshComponent::CreateRenderState()
	{
		PPrimitiveComponent::CreateRenderState();
		if (ShouldRender())
		{
			//TODO
		}
	}
	void PMeshComponent::DestroyRenderState()
	{
		//TODO
		PPrimitiveComponent::DestroyRenderState();
	}
	void PMeshComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			//TODO
		}
		PPrimitiveComponent::SendUpdateRenderState();
	}
	void PMeshComponent::MarkRenderTransformAsDirty()
	{
		//TODO
		PPrimitiveComponent::MarkRenderTransformAsDirty();
	}
	void PMeshComponent::MarkRenderStateAsDirty()
	{
		PPrimitiveComponent::MarkRenderStateAsDirty();
	}
	void PMeshComponent::CleanMarkRenderStateDirty()
	{
		//TODO
		PPrimitiveComponent::CleanMarkRenderStateDirty();
	}
	// Render proxy functions END

}
