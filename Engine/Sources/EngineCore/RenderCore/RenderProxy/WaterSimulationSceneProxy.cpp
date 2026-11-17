#include "WaterSimulationSceneProxy.h"

#include <Main/MainManager.h>
#include <RenderProxy/PrimitiveSceneProxyFactory.h>
#include <Renderer/SceneRenderer.h>
#include <Shader/ShaderAssetBinding.h>

namespace PigeonEngine
{

	static const FShaderBindingClaim GWaterSimulationBindingClaims[] =
	{
		// The simulation stage owns every resource its passes read and write. The same state
		// texture name appears in several passes, but one scope publishes all of them.
		{ EName(PE_TEXT("VelocityHeightFoamTex")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("GroundTex")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("WaterMask")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("OutIntegrateHeight")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("OutIntegrateVelocity")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("OutVelocityHeightFoamTex")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("PreviousHeightWetTex")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("OutHeightWetTex")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("OutState")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("OutHeightWet")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
	};
	PE_REGISTER_SHADER_BINDING_CLAIMS(GWaterSimulationBindingClaims, EName(SHADER_BINDING_SCOPE_WATER_SIMULATION));

	static void RegisterClassTypes()
	{
		RegisterClassType<RWaterSimulationSceneProxy, RPrimitiveSceneProxy>();
		RPrimitiveSceneProxyFactory::Register(EName(PE_TEXT("WaterSimulationPrimitive"))
			, []() -> RPrimitiveSceneProxy* { return new RWaterSimulationSceneProxy(); });
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RWaterSimulationSceneProxy::RWaterSimulationSceneProxy()
	{
		SetPrimitiveCategoryMask(ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_WATER_SIMULATION);
	}

	RWaterSimulationSceneProxy::RWaterSimulationSceneProxy(const RWaterSimulationSceneProxy& Other)
		: RPrimitiveSceneProxy(Other)
	{
		// The simulation's GPU state is per proxy and is never copied; a copy starts empty.
		SetPrimitiveCategoryMask(ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_WATER_SIMULATION);
	}

	RWaterSimulationSceneProxy::~RWaterSimulationSceneProxy()
	{
	}

	BOOL32 RWaterSimulationSceneProxy::IsRenderValid() const
	{
		return TRUE;
	}

	BOOL32 RWaterSimulationSceneProxy::ShouldCollectInView(const RViewProxy& InView) const
	{
		(void)InView;
		return FALSE;
	}

	BOOL32 RWaterSimulationSceneProxy::ShouldDrawInPass(const FRenderPassDesc& InPassDesc) const
	{
		(void)InPassDesc;
		return FALSE;
	}

	void RWaterSimulationSceneProxy::ApplyRuntimePayload(const FPrimitiveRenderSnapshot& InSnapshot)
	{
		RPrimitiveSceneProxy::ApplyRuntimePayload(InSnapshot);
		if (InSnapshot.bHasWaterSimulation != FALSE)
		{
			Simulation.ApplyDescription(InSnapshot.WaterSimulation);
		}
	}

	BOOL32 RWaterSimulationSceneProxy::RecordCompute(IRCommandList* InCommandList, const FRenderPassDesc& InPassDesc) const
	{
		(void)InPassDesc;
		EMainManager* Manager = EMainManager::GetManagerSingleton();
		if ((!Manager) || (!InCommandList))
		{
			return FALSE;
		}
		RSceneRenderer* Renderer = Manager->GetActiveSceneRenderer();
		if (!Renderer)
		{
			return FALSE;
		}
		const EGameTimer* GameTimer = Manager->GetGameTimer();
		const FLOAT DeltaTime = ((GameTimer != nullptr) ? static_cast<FLOAT>(GameTimer->GetDeltaTime()) : 0.f);
		return Simulation.Record(InCommandList, DeltaTime, *Renderer);
	}

}	// namespace PigeonEngine
