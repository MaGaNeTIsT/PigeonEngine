#include "WaterSurfaceSceneProxy.h"

#include <Main/MainManager.h>
#include <RenderProxy/PrimitiveSceneProxyFactory.h>
#include <RenderProxy/ViewProxy.h>
#include <RenderProxy/WaterSimulationSceneProxy.h>
#include <Renderer/RenderScene.h>
#include <Renderer/SceneRenderer.h>
#include <Shader/ShaderAssetBinding.h>
#include <ShaderAsset/ShaderInstance.h>

namespace PigeonEngine
{

	static const FShaderBindingClaim GWaterSurfaceBindingClaims[] =
	{
		// The surface proxy binds the simulation outputs into every water material. They are
		// owned by this scope, not by the material that samples them.
		{ EName(PE_TEXT("WaterHeightWetTex")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("WaterStateTex")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("WaterGroundTex")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
	};
	PE_REGISTER_SHADER_BINDING_CLAIMS(GWaterSurfaceBindingClaims, EName(SHADER_BINDING_SCOPE_WATER_SURFACE));

	static void RegisterClassTypes()
	{
		RegisterClassType<RWaterSurfaceSceneProxy, RStaticMeshSceneProxy>();
		RPrimitiveSceneProxyFactory::Register(EName(PE_TEXT("WaterSurfacePrimitive"))
			, []() -> RPrimitiveSceneProxy* { return new RWaterSurfaceSceneProxy(); });
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RWaterSurfaceSceneProxy::RWaterSurfaceSceneProxy()
	{
		SetPrimitiveCategoryMask(ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_WATER);
	}

	RWaterSurfaceSceneProxy::RWaterSurfaceSceneProxy(const RWaterSurfaceSceneProxy& Other)
		: RStaticMeshSceneProxy(Other)
		, SimulationIdentity(Other.SimulationIdentity)
		, WaterProgram(Other.WaterProgram)
		, WaterSurfaceParameters(Other.WaterSurfaceParameters)
		, WaterHeightScale(Other.WaterHeightScale)
		, WaterMaxHeight(Other.WaterMaxHeight)
		, bWaterDrawValid(TRUE)
	{
		SetPrimitiveCategoryMask(ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_WATER);
	}

	RWaterSurfaceSceneProxy::~RWaterSurfaceSceneProxy()
	{
	}

	BOOL32 RWaterSurfaceSceneProxy::ShouldCollectInView(const RViewProxy& InView) const
	{
		return InView.IsMainRenderView();
	}

	BOOL32 RWaterSurfaceSceneProxy::ShouldDrawInPass(const FRenderPassDesc& InPassDesc) const
	{
		return ((InPassDesc.Type == ERenderPassType::RENDER_PASS_TYPE_WATER_SURFACE) ? TRUE : FALSE);
	}

	BOOL32 RWaterSurfaceSceneProxy::GetWorldBound(Vector3& OutMin, Vector3& OutMax) const
	{
		if (!RStaticMeshSceneProxy::GetWorldBound(OutMin, OutMax))
		{
			return FALSE;
		}
		// The material displaces along world Y; the bound has to contain both directions the
		// height delta can take from the reference plane.
		const FLOAT BoundOffset = WaterMaxHeight * WaterHeightScale;
		OutMin.y -= BoundOffset;
		OutMax.y += BoundOffset;
		return TRUE;
	}

	void RWaterSurfaceSceneProxy::ApplyAssets(const FPrimitiveRenderAssetReport& InAssets)
	{
		RStaticMeshSceneProxy::ApplyAssets(InAssets);

		const ObjectIdentityType* Identity = InAssets.FindNamedIdentity(EName(PE_TEXT("WaterSimulation")));
		SimulationIdentity = (Identity ? *Identity : 0ull);

		if (WaterProgram)
		{
			return;
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice)
		{
			return;
		}
		const EName WaterPassName(PE_TEXT("WaterSurface"));
		for (UINT32 SlotIndex = 0u, SlotNum = InAssets.GetMaterialSlotNum(); SlotIndex < SlotNum; SlotIndex++)
		{
			const EShaderInstance* Instance = InAssets.FindMaterialInstance(SlotIndex, WaterPassName);
			if (!Instance)
			{
				continue;
			}
			const EShaderProgram* Program = Instance->GetProgram();
			if (!Program)
			{
				continue;
			}
			WaterSurfaceParameters.ReleaseRenderResources();
			if (WaterSurfaceParameters.EnsureInitialized(RenderDevice, *Program))
			{
				WaterProgram = Program;
			}
			break;
		}
	}

	void RWaterSurfaceSceneProxy::ApplyValues(const FShaderValueReport& InValues)
	{
		RStaticMeshSceneProxy::ApplyValues(InValues);
		if (!WaterSurfaceParameters.IsInitialized())
		{
			return;
		}

		EShaderParameterSet& Parameters = WaterSurfaceParameters.GetParameters();
		const EName BlockName(WaterSurfaceParameters.GetBlockName());
		for (UINT32 Index = 0u, Num = InValues.GetValueNum(); Index < Num; Index++)
		{
			const FShaderValueReportEntry* Entry = InValues.GetValueByIndex(Index);
			if ((!Entry) || (Entry->BlockName != BlockName))
			{
				continue;
			}
			switch (Entry->Kind)
			{
			case EPrimitiveValueKindType::PRIMITIVE_VALUE_SCALAR:
				WriteShaderScalar(Parameters, BlockName, Entry->FieldName, Entry->Words.Float32[0u]);
				if (Entry->FieldName == EName(PE_TEXT("WaterMaxHeight")))
				{
					WaterMaxHeight = Entry->Words.Float32[0u];
				}
				else if (Entry->FieldName == EName(PE_TEXT("WaterHeightScale")))
				{
					WaterHeightScale = Entry->Words.Float32[0u];
				}
				break;
			case EPrimitiveValueKindType::PRIMITIVE_VALUE_VECTOR:
				WriteShaderVector4(Parameters, BlockName, Entry->FieldName
					, Vector4(Entry->Words.Float32[0u], Entry->Words.Float32[1u], Entry->Words.Float32[2u], Entry->Words.Float32[3u]));
				break;
			case EPrimitiveValueKindType::PRIMITIVE_VALUE_UINT:
				WriteShaderUIntArray(Parameters, BlockName, Entry->FieldName, &(Entry->Words.UInt32[0u]), 1u);
				break;
			case EPrimitiveValueKindType::PRIMITIVE_VALUE_UINT4:
				WriteShaderUIntArray(Parameters, BlockName, Entry->FieldName, &(Entry->Words.UInt32[0u]), 4u);
				break;
			default:
				break;
			}
		}

		// The simulation area, reference plane and clock belong to the simulation body rather
		// than to the surface component, so they are read here, during Prepare, and not in the
		// record path.
		const RWaterSimulationSceneProxy* SimulationProxy = ResolveSimulationProxy();
		if (SimulationProxy)
		{
			const RWaterSimulation& Simulation = SimulationProxy->GetSimulation();
			FLOAT AreaLocation[2] = { Simulation.GetAreaLocationX(), Simulation.GetAreaLocationZ() };
			FLOAT AreaSize[2] = { Simulation.GetAreaSizeX(), Simulation.GetAreaSizeZ() };
			WriteShaderFloatArray(Parameters, BlockName, EName(PE_TEXT("WaterAreaLoc")), AreaLocation, 2u);
			WriteShaderFloatArray(Parameters, BlockName, EName(PE_TEXT("WaterAreaSize")), AreaSize, 2u);
			WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaterReferenceY")), Simulation.GetReferenceY());
			WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaterTime")), Simulation.GetSimulationTime());
			// One simulation texel in UV units: the material takes the height gradient for its
			// normal over two of these, so the taps have to be the domain's own spacing.
			const FLOAT TexelSize[2] =
			{
				1.f / static_cast<FLOAT>(EMath::Max(1u, Simulation.GetResolutionX())),
				1.f / static_cast<FLOAT>(EMath::Max(1u, Simulation.GetResolutionZ()))
			};
			WriteShaderFloatArray(Parameters, BlockName, EName(PE_TEXT("WaterTexelSize")), TexelSize, 2u);
			WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaterSimulationValid")), ((Simulation.IsReady() != FALSE) ? 1.f : 0.f));
			// The renderer records the refraction copy immediately before this pass, so a ready
			// simulation means the material's refraction texture is this frame's scene colour.
			WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaterRefractionValid")), ((Simulation.IsReady() != FALSE) ? 1.f : 0.f));
		}
		else
		{
			WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaterSimulationValid")), 0.f);
			WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaterRefractionValid")), 0.f);
		}
		// The shading tunables. They are constants of the debug water material for now; the
		// component can expose them when a final water material owns the look.
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaterNormalStrength")), 1.f);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaterFlowStrength")), 1.f);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaterScatterDepth")), 150.f);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaterRefractionStrength")), 0.05f);

		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (RenderDevice)
		{
			WaterSurfaceParameters.Upload(RenderDevice);
		}
	}

	BOOL32 RWaterSurfaceSceneProxy::BindExtraResources(IRCommandList* InCommandList, const FPrimitivePassData& InData) const
	{
		bWaterDrawValid = FALSE;
		if ((!InCommandList) || (!InData.Program) || (!InData.ShaderInstanceData))
		{
			return FALSE;
		}
		const RWaterSimulationSceneProxy* SimulationProxy = ResolveSimulationProxy();
		if (!SimulationProxy)
		{
			ReportSkippedDraw(InData.PassName, EString(PE_TEXT("its water simulation body is not registered")));
			return FALSE;
		}
		const RWaterSimulation& Simulation = SimulationProxy->GetSimulation();
		if (!Simulation.IsReady())
		{
			ReportSkippedDraw(InData.PassName, EString(PE_TEXT("its water simulation body has no GPU resources yet")));
			return FALSE;
		}
		const EShaderParameterSet* BaseParameters = InData.ShaderInstanceData->Parameters;
		if (!BaseParameters)
		{
			ReportSkippedDraw(InData.PassName, EString(PE_TEXT("its material parameters are not published")));
			return FALSE;
		}

		EShaderParameterSet Overrides;
		Overrides.SetTexture(EName(), EName(PE_TEXT("WaterHeightWetTex")), Simulation.GetHeightWetTexture());
		Overrides.SetTexture(EName(), EName(PE_TEXT("WaterStateTex")), Simulation.GetStateTexture());
		Overrides.SetTexture(EName(), EName(PE_TEXT("WaterGroundTex")), Simulation.GetGroundTexture());
		if (!BindShaderResources(InCommandList, *BaseParameters, Overrides, *InData.Program
			, ENGINE_SHADER_STAGE_MASK_GRAPHICS, EName(SHADER_BINDING_SCOPE_WATER_SURFACE)))
		{
			ReportSkippedDraw(InData.PassName, EString(PE_TEXT("its water simulation textures could not be bound")));
			return FALSE;
		}
		WaterSurfaceParameters.Bind(InCommandList, *InData.Program);
		bWaterDrawValid = TRUE;
		return TRUE;
	}

	BOOL32 RWaterSurfaceSceneProxy::BindPassResources(IRCommandList* InCommandList
		, const FPassRecordContext& InContext, const FPrimitivePassData& InData) const
	{
		if ((!InCommandList) || (!InData.Program) || (!InData.ShaderInstanceData)
			|| (!InData.ShaderInstanceData->Parameters))
		{
			return FALSE;
		}
		RSceneRenderer* Renderer = EMainManager::GetManagerSingleton()->GetActiveSceneRenderer();
		if (!Renderer)
		{
			return FALSE;
		}

		// The water material shades with the DefaultLit domain, whose light lists and environment
		// live in binding plan slots this forward pass owns. The deferred lighting pass bound the
		// same values for its own program; leaving them to carry over reads whatever that plan
		// left at the water plan's slots, which is how a shaded water surface ends up white.
		//
		// The material's own set is the base and these are the overrides, so the simulation
		// outputs, the per view shading resources and the material's textures all land in the
		// program's single descriptor table in one bind.
		EShaderParameterSet Overrides;
		const RWaterSimulationSceneProxy* SimulationProxy = ResolveSimulationProxy();
		if (SimulationProxy)
		{
			const RWaterSimulation& Simulation = SimulationProxy->GetSimulation();
			if (Simulation.IsReady() != FALSE)
			{
				Overrides.SetTexture(EName(), EName(PE_TEXT("WaterHeightWetTex")), Simulation.GetHeightWetTexture());
				Overrides.SetTexture(EName(), EName(PE_TEXT("WaterStateTex")), Simulation.GetStateTexture());
				Overrides.SetTexture(EName(), EName(PE_TEXT("WaterGroundTex")), Simulation.GetGroundTexture());
			}
		}
		const RViewProxy* View = ((InContext.Instance != nullptr) ? InContext.Instance->View : nullptr);
		Renderer->AddWaterLightingParameters(Overrides, View);
		const BOOL32 bBound = BindShaderResources(InCommandList, *InData.ShaderInstanceData->Parameters
			, Overrides, *InData.Program, ENGINE_SHADER_STAGE_MASK_GRAPHICS
			, EName(SHADER_BINDING_SCOPE_WATER_SURFACE));
		if (bBound == FALSE)
		{
			return FALSE;
		}
		Renderer->BindWaterIndirectLightingParameters(InCommandList, *InData.Program);
		return TRUE;
	}

	BOOL32 RWaterSurfaceSceneProxy::DrawGeometry(IRCommandList* InCommandList, const FPrimitivePassData& InData
		, const UINT32 InIndexCount, const UINT32 InStartIndex) const
	{
		if (bWaterDrawValid == FALSE)
		{
			return FALSE;
		}
		return RStaticMeshSceneProxy::DrawGeometry(InCommandList, InData, InIndexCount, InStartIndex);
	}

	const RWaterSimulationSceneProxy* RWaterSurfaceSceneProxy::ResolveSimulationProxy() const
	{
		if (SimulationIdentity == 0ull)
		{
			return nullptr;
		}
		const RScene* RenderScene = GetRenderScene();
		if (!RenderScene)
		{
			return nullptr;
		}
		RPrimitiveSceneProxy* Proxy = RenderScene->FindPrimitiveProxy(SimulationIdentity);
		if ((!Proxy) || ((Proxy->GetPrimitiveCategoryMask()
			& static_cast<UINT64>(ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_WATER_SIMULATION)) == 0ull))
		{
			return nullptr;
		}
		return static_cast<const RWaterSimulationSceneProxy*>(Proxy);
	}

}	// namespace PigeonEngine
