#include "WaterSurfaceComponent.h"

#include <PigeonBase/Object/Component/Water/WaterSimulationComponent.h>
#include <Renderer/ScenePass.h>
#include <ShaderAsset/ShaderInstance.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PWaterSurfaceComponent, PStaticMeshComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PWaterSurfaceComponent::PWaterSurfaceComponent()
	{
		POBJ_DEBUGNAME_SET(this, "WaterSurfaceComponent");
		SetCastShadow(FALSE);
	}

	PWaterSurfaceComponent::~PWaterSurfaceComponent()
	{
	}

	void PWaterSurfaceComponent::TickRenderImpl()
	{
		// The proxy reads the simulation's area, clock and readiness every frame, so the
		// values channel is refreshed here. Marking before the base runs is what lets the
		// base send this frame's update instead of the next frame's.
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
		PStaticMeshComponent::TickRenderImpl();
	}

	void PWaterSurfaceComponent::SetSimulation(PWaterSimulationComponent* InSimulation)
	{
		Simulation = InSimulation;
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_ASSETS | PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSurfaceComponent::SetWaterHeightScale(const FLOAT InHeightScale)
	{
		WaterHeightScale = EMath::Max(InHeightScale, 0.f);
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PWaterSurfaceComponent::SetWaterMaxHeight(const FLOAT InMaxHeight)
	{
		WaterMaxHeight = EMath::Max(InMaxHeight, 0.f);
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	const EName& PWaterSurfaceComponent::GetRenderProxyTypeName() const
	{
		// Has to match the name RWaterSurfaceSceneProxy registers itself with.
		static const EName ProxyTypeName(PE_TEXT("WaterSurfacePrimitive"));
		return ProxyTypeName;
	}

	void PWaterSurfaceComponent::CollectRenderAssets(FPrimitiveRenderAssetReport& InOutReport) const
	{
		PStaticMeshComponent::CollectRenderAssets(InOutReport);

		// The base reports every slot under the primary pass, which keeps the material
		// assembled for the editor and the fallback path. The water pass needs the same
		// instances under its own name, because the pass descriptor names its program.
		const FRenderPassDesc& WaterPassDescription = FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_WATER_SURFACE);
		if (WaterPassDescription.PassName != nullptr)
		{
			const EName WaterPassName(WaterPassDescription.PassName);
			for (UINT32 SlotIndex = 0u, SlotNum = GetMaterialSlotCount(); SlotIndex < SlotNum; SlotIndex++)
			{
				const EShaderInstance* Instance = GetMaterialInstance(SlotIndex);
				if (Instance)
				{
					InOutReport.SetMaterialInstance(SlotIndex, WaterPassName, Instance);
				}
			}
		}
		if (Simulation)
		{
			InOutReport.SetNamedIdentity(EName(PE_TEXT("WaterSimulation")), Simulation->GetUniqueID());
		}
	}

	void PWaterSurfaceComponent::CollectRenderValues(FShaderValueReport& InOutValues) const
	{
		PStaticMeshComponent::CollectRenderValues(InOutValues);

		const EName BlockName(PE_TEXT("WaterSurfaceParams"));
		InOutValues.SetScalar(BlockName, EName(PE_TEXT("WaterHeightScale")), WaterHeightScale);
		InOutValues.SetScalar(BlockName, EName(PE_TEXT("WaterMaxHeight")), WaterMaxHeight);
		InOutValues.SetScalar(BlockName, EName(PE_TEXT("WaterEnable")), ((Simulation != nullptr) ? 1.f : 0.f));
	}

}	// namespace PigeonEngine
