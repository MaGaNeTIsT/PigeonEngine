#include "RWaterSimulation.h"

#include <cmath>

#include <Editor/EditorLogManager.h>
#include <Main/MainManager.h>
#include <Renderer/ScenePass.h>
#include <Renderer/SceneRenderer.h>
#include <RHI/IRCommandList.h>
#include <RHI/IRRHIDevice.h>
#include <Pipeline/RPipelineStateCache.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <Shader/ShaderAssetBinding.h>
#include <ShaderAsset/ShaderInstance.h>
#include <ShaderAsset/ShaderParameterSet.h>
#include <ShaderAsset/ShaderProgram.h>
#include <ShaderAsset/ShaderJobBuilder.h>

namespace PigeonEngine
{


	static const CHAR* const GWaterSimulationPassShaderNames[] =
	{
		PE_TEXT("CS_SWEInitialize"),
		PE_TEXT("CS_SWEIntegrateHeight"),
		PE_TEXT("CS_SWEIntegrateVelocity"),
		PE_TEXT("CS_SWEModifierWave"),
		PE_TEXT("CS_SWEModifier"),
		PE_TEXT("CS_SWEModifierWhirlpool"),
		PE_TEXT("CS_SWEUpdateHeightWet")
	};

	static const CHAR* const GWaterSimulationPassProgramNames[] =
	{
		PE_TEXT("SWEInitialize"),
		PE_TEXT("SWEIntegrateHeight"),
		PE_TEXT("SWEIntegrateVelocity"),
		PE_TEXT("SWEModifierWave"),
		PE_TEXT("SWEModifier"),
		PE_TEXT("SWEModifierWhirlpool"),
		PE_TEXT("SWEUpdateHeightWet")
	};

	static void TransitionTexture(IRCommandList* InCommandList, IRRHITexture* InTexture, const ERHIResourceState InState)
	{
		if ((!InCommandList) || (!InTexture))
		{
			return;
		}
		const ERHIResourceState CurrentState = InTexture->GetCurrentState();
		if (CurrentState != InState)
		{
			InCommandList->ResourceBarrier(InTexture, CurrentState, InState);
		}
	}

	static BOOL32 CreateFlatGroundTexture(IRRHIDevice* InRenderDevice, RTexture2DResource& OutTexture)
	{
		FLOAT Zero = 0.f;
		RRHISubresourceData InitialData(reinterpret_cast<const UINT8*>(&Zero), static_cast<UINT64>(sizeof(FLOAT))
			, static_cast<UINT32>(sizeof(FLOAT)), static_cast<UINT32>(sizeof(FLOAT)));
		const RTextureDesc Description(1u, 1u, static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE)
			, RFormatType::FORMAT_R32_FLOAT);
		return ((OutTexture.InitFromRHI(InRenderDevice, Description, &InitialData) != FALSE) ? TRUE : FALSE);
	}

	static BOOL32 CreateGroundHeightMapTexture(IRRHIDevice* InRenderDevice, const UINT32 InResolutionX
		, const UINT32 InResolutionZ, const TArray<FLOAT>& InRelativeDepths, RTexture2DResource& OutTexture)
	{
		if (InRelativeDepths.Num<UINT32>() != (InResolutionX * InResolutionZ))
		{
			return FALSE;
		}
		RRHISubresourceData InitialData(reinterpret_cast<const UINT8*>(InRelativeDepths.GetData())
			, static_cast<UINT64>(InRelativeDepths.Num<UINT32>()) * sizeof(FLOAT)
			, InResolutionX * static_cast<UINT32>(sizeof(FLOAT))
			, InResolutionX * InResolutionZ * static_cast<UINT32>(sizeof(FLOAT)));
		const RTextureDesc Description(InResolutionX, InResolutionZ, static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE)
			, RFormatType::FORMAT_R32_FLOAT);
		return ((OutTexture.InitFromRHI(InRenderDevice, Description, &InitialData) != FALSE) ? TRUE : FALSE);
	}

	static BOOL32 CreateWaterMaskTexture(IRRHIDevice* InRenderDevice, RTexture2DResource& OutTexture)
	{
		FLOAT One = 1.f;
		RRHISubresourceData InitialData(reinterpret_cast<const UINT8*>(&One), static_cast<UINT64>(sizeof(FLOAT))
			, static_cast<UINT32>(sizeof(FLOAT)), static_cast<UINT32>(sizeof(FLOAT)));
		const RTextureDesc Description(1u, 1u, static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE)
			, RFormatType::FORMAT_R32_FLOAT);
		return ((OutTexture.InitFromRHI(InRenderDevice, Description, &InitialData) != FALSE) ? TRUE : FALSE);
	}


	RWaterSimulation::RWaterSimulation() noexcept
	{
		for (UINT32 Index = 0u; Index < static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_COUNT); Index++)
		{
			Passes[Index] = EMemory::MakeUnique<FWaterSimulationPass>();
		}
	}

	RWaterSimulation::~RWaterSimulation() noexcept
	{
		EMainManager* Manager = EMainManager::GetManagerSingleton();
		IRRHIDevice* RenderDevice = ((Manager != nullptr) ? Manager->GetRenderDevice() : nullptr);
		if (RenderDevice)
		{
			ReleaseResources(RenderDevice);
		}
	}

	BOOL32 RWaterSimulation::ApplyDescription(const FWaterSimulationDescription& InDescription)
	{
		const BOOL32 bResolutionChanged = ((Description.ResolutionX != InDescription.ResolutionX)
			|| (Description.ResolutionZ != InDescription.ResolutionZ)) ? TRUE : FALSE;
		const BOOL32 bGroundChanged = ((Description.GroundType != InDescription.GroundType)
			|| (Description.GroundRevision != InDescription.GroundRevision)) ? TRUE : FALSE;

		Description = InDescription;
		Description.ResolutionX			= EMath::Max(1u, Description.ResolutionX);
		Description.ResolutionZ			= EMath::Max(1u, Description.ResolutionZ);
		Description.Interactions.Empty();
		Description.bResetSimulation	= FALSE;

		if (InDescription.bResetSimulation != FALSE)
		{
			bResetRequested = TRUE;
		}
		if ((bResolutionChanged != FALSE) || (bGroundChanged != FALSE))
		{
			bGroundDirty = TRUE;
		}
		LastGroundRevision = Description.GroundRevision;

		for (UINT32 Index = 0u, Num = InDescription.Interactions.Num<UINT32>(); Index < Num; Index++)
		{
			const FWaterInteractionEvent& Event = InDescription.Interactions[Index];
			if ((Event.Type == EWaterInteractionType::WATER_INTERACTION_WHIRLPOOL) && (Event.Duration > 0.f))
			{
				ActiveWhirlpools.Add(Event);
			}
			else
			{
				PendingInteractions.Add(Event);
			}
		}
		return ((bResolutionChanged != FALSE) || (bGroundChanged != FALSE)) ? TRUE : FALSE;
	}

	void RWaterSimulation::RequestReset() noexcept
	{
		bResetRequested = TRUE;
	}

	BOOL32 RWaterSimulation::Record(IRCommandList* InCommandList, FLOAT InFrameDeltaTime, RSceneRenderer& InRenderer)
	{
		if (!InCommandList)
		{
			return FALSE;
		}
		EMainManager* Manager = EMainManager::GetManagerSingleton();
		IRRHIDevice* RenderDevice = ((Manager != nullptr) ? Manager->GetRenderDevice() : nullptr);
		if (!RenderDevice)
		{
			return FALSE;
		}
		if (!EnsurePasses(RenderDevice, InRenderer))
		{
			return FALSE;
		}
		if ((bResourcesReady == FALSE) || (bGroundDirty != FALSE))
		{
			if (!EnsureResources(RenderDevice))
			{
				return FALSE;
			}
		}

		TransitionTexture(InCommandList, GroundTexture.GetResolvedRHITexture(), ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		TransitionTexture(InCommandList, WaterMaskTexture.GetResolvedRHITexture(), ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);

		if (bResetRequested != FALSE)
		{
			if (!RecordInitialize(InCommandList, RenderDevice))
			{
				return FALSE;
			}
			bResetRequested = FALSE;
		}

		FLOAT DeltaTime = InFrameDeltaTime * Description.TimeScale;
		if (DeltaTime < 0.f)
		{
			DeltaTime = 0.f;
		}
		Accumulator += DeltaTime;
		const FLOAT FixedTimeStep = EMath::Max(Description.FixedTimeStep, 0.001f);
		const UINT32 MaxSubsteps = EMath::Max(1u, Description.MaxSubsteps);
		// A long frame must not turn into an unbounded catch-up queue (or an out of range
		// float to integer conversion below). The simulation slows down instead.
		const FLOAT MaxAccumulator = FixedTimeStep * static_cast<FLOAT>(MaxSubsteps);
		if (Accumulator > MaxAccumulator)
		{
			Accumulator = MaxAccumulator;
		}
		UINT32 StepCount = static_cast<UINT32>(Accumulator / FixedTimeStep);
		StepCount = EMath::Min(StepCount, MaxSubsteps);
		Accumulator -= static_cast<FLOAT>(StepCount) * FixedTimeStep;
		if (Accumulator < 0.f)
		{
			Accumulator = 0.f;
		}

		BOOL32 bRecorded = TRUE;
		for (UINT32 StepIndex = 0u; StepIndex < StepCount; StepIndex++)
		{
			if (!RecordIntegrateHeight(InCommandList, RenderDevice, FixedTimeStep))
			{
				bRecorded = FALSE;
				break;
			}
			if (!RecordIntegrateVelocity(InCommandList, RenderDevice, FixedTimeStep))
			{
				bRecorded = FALSE;
				break;
			}
			if (Description.bEnableWave != FALSE)
			{
				RecordWave(InCommandList, RenderDevice, FixedTimeStep);
			}
			ApplyInteractions(InCommandList, RenderDevice, FixedTimeStep, ((StepIndex == 0u) ? TRUE : FALSE));
			if (!RecordUpdateHeightWet(InCommandList, RenderDevice, FixedTimeStep))
			{
				bRecorded = FALSE;
				break;
			}
			SimulationTime += FixedTimeStep;
		}

		// The outputs are the surface pass's inputs. Even a step that failed has to leave
		// them in the readable state; the next frame retries the simulation.
		TransitionTexture(InCommandList, StateTextures[CurrentStateIndex].GetResolvedRHITexture()
			, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		TransitionTexture(InCommandList, HeightWetTextures[CurrentHeightWetIndex].GetResolvedRHITexture()
			, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		return bRecorded;
	}

	const IRRHITexture* RWaterSimulation::GetStateTexture() const noexcept
	{
		return StateTextures[CurrentStateIndex].GetResolvedRHITexture();
	}

	const IRRHITexture* RWaterSimulation::GetHeightWetTexture() const noexcept
	{
		return HeightWetTextures[CurrentHeightWetIndex].GetResolvedRHITexture();
	}

	const IRRHITexture* RWaterSimulation::GetGroundTexture() const noexcept
	{
		return GroundTexture.GetResolvedRHITexture();
	}

	BOOL32 RWaterSimulation::EnsurePasses(IRRHIDevice* InRenderDevice, RSceneRenderer& InRenderer)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}
		REngineShaderInstanceCache* InstanceCache = REngineShaderInstanceCache::GetManagerSingleton();
		if (!InstanceCache)
		{
			return FALSE;
		}

		const UINT32 PassCount = static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_COUNT);
		for (UINT32 Index = 0u; Index < PassCount; Index++)
		{
			FWaterSimulationPass& Pass = *(Passes[Index]);
			EShaderAssemblyRequest Request;
			Request.ShaderName	= EName(GWaterSimulationPassShaderNames[Index]);
			Request.ProgramName	= EName(GWaterSimulationPassProgramNames[Index]);
			Request.StageMask	= ENGINE_SHADER_STAGE_MASK_COMPUTE;

			const EShaderInstance* Instance = InstanceCache->RequireInstance(Request);
			if (!Instance)
			{
				if (Index == static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_WAVE))
				{
					Pass.Instance = nullptr;
					Pass.Program = nullptr;
					Pass.PipelineState = nullptr;
					continue;
				}
				return FALSE;
			}
			const EShaderProgram* Program = Instance->GetProgram();
			if (!Program)
			{
				return FALSE;
			}
			if ((Pass.Instance != Instance) || (Pass.Program != Program))
			{
				Pass.Instance = Instance;
				Pass.Program = Program;
				Pass.PipelineState = nullptr;
				Pass.Parameters.ReleaseRenderResources();
			}
			if (!Pass.PipelineState)
			{
				Pass.PipelineState = InRenderer.GetPipelineStateCache().GetOrCreateCompute(*Program);
				if (!Pass.PipelineState)
				{
					return FALSE;
				}
			}
			if (!Pass.Parameters.EnsureInitialized(InRenderDevice, *Program))
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	BOOL32 RWaterSimulation::EnsureResources(IRRHIDevice* InRenderDevice)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}
		ReleaseResources(InRenderDevice);

		const UINT32 ResolutionX = EMath::Max(1u, Description.ResolutionX);
		const UINT32 ResolutionZ = EMath::Max(1u, Description.ResolutionZ);
		const UINT8 StateBindFlags = static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE)
			| static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS);
		const RTextureDesc StateDescription(ResolutionX, ResolutionZ, StateBindFlags, RFormatType::FORMAT_R16G16B16A16_FLOAT);
		const RTextureDesc HeightWetDescription(ResolutionX, ResolutionZ, StateBindFlags, RFormatType::FORMAT_R16G16_FLOAT);

		for (UINT32 Index = 0u; Index < 2u; Index++)
		{
			if (!StateTextures[Index].InitFromRHI(InRenderDevice, StateDescription))
			{
				ReleaseResources(InRenderDevice);
				return FALSE;
			}
			if (!HeightWetTextures[Index].InitFromRHI(InRenderDevice, HeightWetDescription))
			{
				ReleaseResources(InRenderDevice);
				return FALSE;
			}
		}

		if ((Description.GroundType == EWaterGroundType::WATER_GROUND_HEIGHT_MAP) && (Description.GroundRelativeDepths))
		{
			if (!CreateGroundHeightMapTexture(InRenderDevice, ResolutionX, ResolutionZ, *(Description.GroundRelativeDepths), GroundTexture))
			{
				PE_LOG_ERROR(EString(PE_TEXT("[WaterSimulation] The ground height map size does not match the resolution; using a flat ground.")));
				if (!CreateFlatGroundTexture(InRenderDevice, GroundTexture))
				{
					ReleaseResources(InRenderDevice);
					return FALSE;
				}
			}
		}
		else
		{
			if (!CreateFlatGroundTexture(InRenderDevice, GroundTexture))
			{
				ReleaseResources(InRenderDevice);
				return FALSE;
			}
		}
		if (!CreateWaterMaskTexture(InRenderDevice, WaterMaskTexture))
		{
			ReleaseResources(InRenderDevice);
			return FALSE;
		}
		if (!EnsureUAVViews(InRenderDevice))
		{
			ReleaseResources(InRenderDevice);
			return FALSE;
		}

		CurrentStateIndex = 0u;
		CurrentHeightWetIndex = 0u;
		Accumulator = 0.f;
		SimulationTime = 0.f;
		bResourcesReady = TRUE;
		bGroundDirty = FALSE;
		bResetRequested = TRUE;
		LastGroundRevision = Description.GroundRevision;
		return TRUE;
	}

	BOOL32 RWaterSimulation::EnsureUAVViews(IRRHIDevice* InRenderDevice)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}
		RRHITextureUAVDesc UAVDescription;
		for (UINT32 Index = 0u; Index < 2u; Index++)
		{
			IRRHITexture* StateTexture = StateTextures[Index].GetResolvedRHITexture();
			if ((!StateTexture) || (!InRenderDevice->CreateTextureUAVView(StateTexture, UAVDescription, &StateUAVViews[Index])))
			{
				return FALSE;
			}
			IRRHITexture* HeightWetTexture = HeightWetTextures[Index].GetResolvedRHITexture();
			if ((!HeightWetTexture) || (!InRenderDevice->CreateTextureUAVView(HeightWetTexture, UAVDescription, &HeightWetUAVViews[Index])))
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	void RWaterSimulation::ReleaseResources(IRRHIDevice* InRenderDevice)
	{
		if (InRenderDevice)
		{
			for (UINT32 Index = 0u; Index < 2u; Index++)
			{
				if (StateUAVViews[Index])
				{
					InRenderDevice->DestroyTextureUAVView(StateUAVViews[Index]);
					StateUAVViews[Index] = nullptr;
				}
				if (HeightWetUAVViews[Index])
				{
					InRenderDevice->DestroyTextureUAVView(HeightWetUAVViews[Index]);
					HeightWetUAVViews[Index] = nullptr;
				}
			}
		}
		for (UINT32 Index = 0u; Index < 2u; Index++)
		{
			StateTextures[Index].ReleaseRenderResource();
			HeightWetTextures[Index].ReleaseRenderResource();
		}
		GroundTexture.ReleaseRenderResource();
		WaterMaskTexture.ReleaseRenderResource();
		bResourcesReady = FALSE;
	}

	FWaterSimulationParameters RWaterSimulation::BuildParameters(FLOAT InDeltaTime) const
	{
		FWaterSimulationParameters Parameters = Description.Parameters;
		const FLOAT ResolutionX = static_cast<FLOAT>(EMath::Max(1u, Description.ResolutionX));
		const FLOAT ResolutionZ = static_cast<FLOAT>(EMath::Max(1u, Description.ResolutionZ));
		Parameters.ResolutionX			= ResolutionX;
		Parameters.ResolutionZ			= ResolutionZ;
		Parameters.InverseResolutionX	= 1.f / ResolutionX;
		Parameters.InverseResolutionZ	= 1.f / ResolutionZ;
		Parameters.AreaLocationX		= Description.AreaLocationX;
		Parameters.AreaLocationZ		= Description.AreaLocationZ;
		Parameters.AreaSizeX			= EMath::Max(Description.AreaSizeX, 0.0001f);
		Parameters.AreaSizeZ			= EMath::Max(Description.AreaSizeZ, 0.0001f);
		Parameters.SimulationDeltaTime	= InDeltaTime;
		Parameters.CaptureY				= Description.ReferenceY;
		Parameters.InitialDepth			= Description.InitialDepth;
		Parameters.PixelAnchor			= Vector2Int(0, 0);
		Parameters.PixelSize			= Vector2Int(static_cast<INT32>(Description.ResolutionX), static_cast<INT32>(Description.ResolutionZ));
		Parameters.WaveActiveTime		= SimulationTime;
		Parameters.WhirlpoolTime		= SimulationTime;
		return Parameters;
	}

	BOOL32 RWaterSimulation::DispatchPass(const UINT32 InPassIndex, IRCommandList* InCommandList
		, const FWaterSimulationParameters& InParameters, IRRHIDevice* InRenderDevice, EShaderParameterSet& InResources)
	{
		if ((!InCommandList) || (!InRenderDevice)
			|| (InPassIndex >= static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_COUNT)))
		{
			return FALSE;
		}
		FWaterSimulationPass& Pass = *(Passes[InPassIndex]);
		if ((!Pass.Program) || (!Pass.PipelineState))
		{
			return FALSE;
		}
		WriteParameters(InPassIndex, InParameters, InRenderDevice);
		InCommandList->SetPipelineState(Pass.PipelineState);
		Pass.Parameters.Bind(InCommandList, *Pass.Program);
		if (BindShaderResources(InCommandList, InResources, *Pass.Program, ENGINE_SHADER_STAGE_MASK_COMPUTE
			, EName(SHADER_BINDING_SCOPE_WATER_SIMULATION)) == FALSE)
		{
			return FALSE;
		}
		const FRenderPassDesc& PassDescription = FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_SWE_INTEGRATE);
		const UINT32 GroupSizeX = EMath::Max(1u, PassDescription.ThreadGroupX);
		const UINT32 GroupSizeZ = EMath::Max(1u, PassDescription.ThreadGroupY);
		const UINT32 ResolutionX = EMath::Max(1u, Description.ResolutionX);
		const UINT32 ResolutionZ = EMath::Max(1u, Description.ResolutionZ);
		InCommandList->Dispatch((ResolutionX + GroupSizeX - 1u) / GroupSizeX
			, (ResolutionZ + GroupSizeZ - 1u) / GroupSizeZ, 1u);
		return TRUE;
	}

	void RWaterSimulation::WriteParameters(const UINT32 InPassIndex, const FWaterSimulationParameters& InParameters
		, IRRHIDevice* InRenderDevice)
	{
		FWaterSimulationPass& Pass = *(Passes[InPassIndex]);
		EShaderParameterSet& Parameters = Pass.Parameters.GetParameters();
		const EName BlockName(Pass.Parameters.GetBlockName());

		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("InverseResolutionX")), InParameters.InverseResolutionX);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("InverseResolutionZ")), InParameters.InverseResolutionZ);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("ResolutionX")), InParameters.ResolutionX);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("ResolutionZ")), InParameters.ResolutionZ);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("AreaLocationX")), InParameters.AreaLocationX);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("AreaLocationZ")), InParameters.AreaLocationZ);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("AreaSizeX")), InParameters.AreaSizeX);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("AreaSizeZ")), InParameters.AreaSizeZ);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("SimulationDeltaTime")), InParameters.SimulationDeltaTime);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("CaptureY")), InParameters.CaptureY);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("InitialDepth")), InParameters.InitialDepth);
		Parameters.SetIntArray(BlockName, EName(PE_TEXT("PixelAnchor")), &(InParameters.PixelAnchor.x), 2u);
		Parameters.SetIntArray(BlockName, EName(PE_TEXT("PixelSize")), &(InParameters.PixelSize.x), 2u);

		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("FoamFadeSpeed")), InParameters.FoamFadeSpeed);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("FoamShallowDepth")), InParameters.FoamShallowDepth);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("FoamShallowGenerate")), InParameters.FoamShallowGenerate);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("FoamWaveGenerate")), InParameters.FoamWaveGenerate);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("FluidTransportMax")), InParameters.FluidTransportMax);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("FoamAdvection")), InParameters.FoamAdvection);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("EvaporationDepth")), InParameters.EvaporationDepth);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("FluxEnergy")), InParameters.FluxEnergy);

		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("Gravity")), InParameters.Gravity);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("VelocityClamp")), InParameters.VelocityClamp);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("ScaleY")), InParameters.ScaleY);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("Friction")), InParameters.Friction);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("Damping")), InParameters.Damping);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("AccelerationClamp")), InParameters.AccelerationClamp);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OvershootingBlend")), InParameters.OvershootingBlend);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OvershootingLimit")), InParameters.OvershootingLimit);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("MinWater")), InParameters.MinWater);

		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("SourceVelocityX")), InParameters.SourceVelocityX);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("SourceVelocityZ")), InParameters.SourceVelocityZ);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("SourceVolume")), InParameters.SourceVolume);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("SourceMode")), InParameters.SourceMode);
		WriteShaderFloatArray(Parameters, BlockName, EName(PE_TEXT("ModifierLocation")), &(InParameters.ModifierLocation.x), 2u);
		WriteShaderFloatArray(Parameters, BlockName, EName(PE_TEXT("ModifierSize")), &(InParameters.ModifierSize.x), 2u);
		WriteShaderFloatArray(Parameters, BlockName, EName(PE_TEXT("ModifierForward")), &(InParameters.ModifierForward.x), 3u);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("YOffset")), InParameters.YOffset);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("Hardness")), InParameters.Hardness);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("Intensity")), InParameters.Intensity);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("Radius")), InParameters.Radius);

		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("ModifierRadius")), InParameters.ModifierRadius);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("ModifierRadial")), InParameters.ModifierRadial);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("ModifierSuction")), InParameters.ModifierSuction);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WhirlpoolTranslucency")), InParameters.WhirlpoolTranslucency);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WhirlpoolTime")), InParameters.WhirlpoolTime);

		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveVelocityScale")), InParameters.WaveVelocityScale);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveCalming")), InParameters.WaveCalming);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveSurfaceHeight")), InParameters.WaveSurfaceHeight);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveBorderXNegative")), InParameters.WaveBorderXNegative);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveBorderZNegative")), InParameters.WaveBorderZNegative);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveBorderXPositive")), InParameters.WaveBorderXPositive);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveBorderZPositive")), InParameters.WaveBorderZPositive);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveLength")), InParameters.WaveLength);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveSpeed")), InParameters.WaveSpeed);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveAmplitude")), InParameters.WaveAmplitude);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveSteepness")), InParameters.WaveSteepness);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveActiveTime")), InParameters.WaveActiveTime);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("WaveDirectionAngle")), InParameters.WaveDirectionAngle);

		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineStateWetHeightOffset")), InParameters.OutlineStateWetHeightOffset);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineNoWaterBlend")), InParameters.OutlineNoWaterBlend);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineThickness")), InParameters.OutlineThickness);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineSurfaceSmoothing")), InParameters.OutlineSurfaceSmoothing);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineFoamThickness")), InParameters.OutlineFoamThickness);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineFluidThickness")), InParameters.OutlineFluidThickness);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineDryingOffset")), InParameters.OutlineDryingOffset);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineDryingSpeed")), InParameters.OutlineDryingSpeed);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineBorderBlend")), InParameters.OutlineBorderBlend);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineWetnessLimit")), InParameters.OutlineWetnessLimit);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineMinWater")), InParameters.OutlineMinWater);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineFallingSpeed")), InParameters.OutlineFallingSpeed);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineMinOutline")), InParameters.OutlineMinOutline);
		WriteShaderScalar(Parameters, BlockName, EName(PE_TEXT("OutlineAverage")), InParameters.OutlineAverage);

		Pass.Parameters.Upload(InRenderDevice);
	}

	BOOL32 RWaterSimulation::RecordInitialize(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice)
	{
		IRRHITexture* StateTexture = StateTextures[0].GetResolvedRHITexture();
		IRRHITexture* HeightWetTexture = HeightWetTextures[0].GetResolvedRHITexture();
		if ((!StateTexture) || (!HeightWetTexture) || (!StateUAVViews[0]) || (!HeightWetUAVViews[0]))
		{
			return FALSE;
		}
		TransitionTexture(InCommandList, StateTexture, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
		TransitionTexture(InCommandList, HeightWetTexture, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

		FWaterSimulationParameters Parameters = BuildParameters(0.f);
		Parameters.InitialDepth = Description.InitialDepth;
		EShaderParameterSet Resources;
		Resources.SetTexture(EName(), EName(PE_TEXT("GroundTex")), GroundTexture.GetResolvedRHITexture());
		Resources.SetTextureUAV(EName(), EName(PE_TEXT("OutState")), StateUAVViews[0]);
		Resources.SetTextureUAV(EName(), EName(PE_TEXT("OutHeightWet")), HeightWetUAVViews[0]);
		const BOOL32 bDispatched = DispatchPass(static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_INITIALIZE)
			, InCommandList, Parameters, InRenderDevice, Resources);

		TransitionTexture(InCommandList, StateTexture, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		TransitionTexture(InCommandList, HeightWetTexture, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		if (bDispatched != FALSE)
		{
			CurrentStateIndex = 0u;
			CurrentHeightWetIndex = 0u;
		}
		return bDispatched;
	}

	BOOL32 RWaterSimulation::RecordIntegrateHeight(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice, FLOAT InDeltaTime)
	{
		const UINT32 InputIndex = CurrentStateIndex;
		const UINT32 OutputIndex = 1u - InputIndex;
		IRRHITexture* InputTexture = StateTextures[InputIndex].GetResolvedRHITexture();
		IRRHITexture* OutputTexture = StateTextures[OutputIndex].GetResolvedRHITexture();
		if ((!InputTexture) || (!OutputTexture) || (!StateUAVViews[OutputIndex]))
		{
			return FALSE;
		}
		TransitionTexture(InCommandList, InputTexture, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		TransitionTexture(InCommandList, OutputTexture, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

		const FWaterSimulationParameters Parameters = BuildParameters(InDeltaTime);
		EShaderParameterSet Resources;
		Resources.SetTexture(EName(), EName(PE_TEXT("VelocityHeightFoamTex")), InputTexture);
		Resources.SetTexture(EName(), EName(PE_TEXT("GroundTex")), GroundTexture.GetResolvedRHITexture());
		Resources.SetTextureUAV(EName(), EName(PE_TEXT("OutIntegrateHeight")), StateUAVViews[OutputIndex]);
		if (!DispatchPass(static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_INTEGRATE_HEIGHT)
			, InCommandList, Parameters, InRenderDevice, Resources))
		{
			return FALSE;
		}
		CurrentStateIndex = OutputIndex;
		return TRUE;
	}

	BOOL32 RWaterSimulation::RecordIntegrateVelocity(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice, FLOAT InDeltaTime)
	{
		const UINT32 InputIndex = CurrentStateIndex;
		const UINT32 OutputIndex = 1u - InputIndex;
		IRRHITexture* InputTexture = StateTextures[InputIndex].GetResolvedRHITexture();
		IRRHITexture* OutputTexture = StateTextures[OutputIndex].GetResolvedRHITexture();
		if ((!InputTexture) || (!OutputTexture) || (!StateUAVViews[OutputIndex]))
		{
			return FALSE;
		}
		TransitionTexture(InCommandList, InputTexture, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		TransitionTexture(InCommandList, OutputTexture, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

		const FWaterSimulationParameters Parameters = BuildParameters(InDeltaTime);
		EShaderParameterSet Resources;
		Resources.SetTexture(EName(), EName(PE_TEXT("VelocityHeightFoamTex")), InputTexture);
		Resources.SetTexture(EName(), EName(PE_TEXT("GroundTex")), GroundTexture.GetResolvedRHITexture());
		Resources.SetTexture(EName(), EName(PE_TEXT("WaterMask")), WaterMaskTexture.GetResolvedRHITexture());
		Resources.SetTextureUAV(EName(), EName(PE_TEXT("OutIntegrateVelocity")), StateUAVViews[OutputIndex]);
		if (!DispatchPass(static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_INTEGRATE_VELOCITY)
			, InCommandList, Parameters, InRenderDevice, Resources))
		{
			return FALSE;
		}
		CurrentStateIndex = OutputIndex;
		return TRUE;
	}

	BOOL32 RWaterSimulation::RecordWave(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice, FLOAT InDeltaTime)
	{
		FWaterSimulationPass& Pass = *(Passes[static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_WAVE)]);
		if ((!Pass.Program) || (!Pass.PipelineState))
		{
			return FALSE;
		}
		const UINT32 InputIndex = CurrentStateIndex;
		const UINT32 OutputIndex = 1u - InputIndex;
		IRRHITexture* InputTexture = StateTextures[InputIndex].GetResolvedRHITexture();
		IRRHITexture* OutputTexture = StateTextures[OutputIndex].GetResolvedRHITexture();
		if ((!InputTexture) || (!OutputTexture) || (!StateUAVViews[OutputIndex]))
		{
			return FALSE;
		}
		TransitionTexture(InCommandList, InputTexture, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		TransitionTexture(InCommandList, OutputTexture, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

		const FWaterSimulationParameters Parameters = BuildParameters(InDeltaTime);
		EShaderParameterSet Resources;
		Resources.SetTexture(EName(), EName(PE_TEXT("VelocityHeightFoamTex")), InputTexture);
		Resources.SetTextureUAV(EName(), EName(PE_TEXT("OutVelocityHeightFoamTex")), StateUAVViews[OutputIndex]);
		if (!DispatchPass(static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_WAVE)
			, InCommandList, Parameters, InRenderDevice, Resources))
		{
			return FALSE;
		}
		CurrentStateIndex = OutputIndex;
		return TRUE;
	}

	BOOL32 RWaterSimulation::RecordModifier(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice
		, FLOAT InDeltaTime, const FWaterInteractionEvent& InEvent)
	{
		const UINT32 InputIndex = CurrentStateIndex;
		const UINT32 OutputIndex = 1u - InputIndex;
		IRRHITexture* InputTexture = StateTextures[InputIndex].GetResolvedRHITexture();
		IRRHITexture* OutputTexture = StateTextures[OutputIndex].GetResolvedRHITexture();
		if ((!InputTexture) || (!OutputTexture) || (!StateUAVViews[OutputIndex]))
		{
			return FALSE;
		}
		TransitionTexture(InCommandList, InputTexture, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		TransitionTexture(InCommandList, OutputTexture, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

		FWaterSimulationParameters Parameters = BuildParameters(InDeltaTime);
		Parameters.SourceVelocityX	= InEvent.Velocity.x;
		Parameters.SourceVelocityZ	= InEvent.Velocity.y;
		Parameters.SourceVolume		= InEvent.Volume;
		Parameters.SourceMode		= InEvent.Mode;
		Parameters.ModifierLocation	= Vector2(InEvent.WorldLocation.x, InEvent.WorldLocation.z);
		Parameters.ModifierSize		= InEvent.Size;
		Parameters.ModifierForward	= Vector3(std::cos(InEvent.ForwardAngle), 0.f, std::sin(InEvent.ForwardAngle));
		Parameters.YOffset			= InEvent.YOffset;
		Parameters.Hardness			= InEvent.Hardness;
		Parameters.Intensity		= InEvent.Intensity;
		Parameters.Radius			= InEvent.Radius;

		EShaderParameterSet Resources;
		Resources.SetTexture(EName(), EName(PE_TEXT("VelocityHeightFoamTex")), InputTexture);
		Resources.SetTexture(EName(), EName(PE_TEXT("GroundTex")), GroundTexture.GetResolvedRHITexture());
		Resources.SetTexture(EName(), EName(PE_TEXT("WaterMask")), WaterMaskTexture.GetResolvedRHITexture());
		Resources.SetTextureUAV(EName(), EName(PE_TEXT("OutVelocityHeightFoamTex")), StateUAVViews[OutputIndex]);
		if (!DispatchPass(static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_MODIFIER)
			, InCommandList, Parameters, InRenderDevice, Resources))
		{
			return FALSE;
		}
		CurrentStateIndex = OutputIndex;
		return TRUE;
	}

	BOOL32 RWaterSimulation::RecordWhirlpool(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice
		, FLOAT InDeltaTime, const FWaterInteractionEvent& InEvent)
	{
		const UINT32 InputIndex = CurrentStateIndex;
		const UINT32 OutputIndex = 1u - InputIndex;
		IRRHITexture* InputTexture = StateTextures[InputIndex].GetResolvedRHITexture();
		IRRHITexture* OutputTexture = StateTextures[OutputIndex].GetResolvedRHITexture();
		if ((!InputTexture) || (!OutputTexture) || (!StateUAVViews[OutputIndex]))
		{
			return FALSE;
		}
		TransitionTexture(InCommandList, InputTexture, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		TransitionTexture(InCommandList, OutputTexture, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

		FWaterSimulationParameters Parameters = BuildParameters(InDeltaTime);
		Parameters.ModifierLocation		= Vector2(InEvent.WorldLocation.x, InEvent.WorldLocation.z);
		Parameters.ModifierRadius		= InEvent.Radius;
		Parameters.ModifierRadial		= InEvent.Radial;
		Parameters.ModifierSuction		= InEvent.Suction;
		Parameters.WhirlpoolTranslucency = InEvent.Translucency;
		Parameters.WhirlpoolTime		= SimulationTime;

		EShaderParameterSet Resources;
		Resources.SetTexture(EName(), EName(PE_TEXT("VelocityHeightFoamTex")), InputTexture);
		Resources.SetTexture(EName(), EName(PE_TEXT("GroundTex")), GroundTexture.GetResolvedRHITexture());
		Resources.SetTexture(EName(), EName(PE_TEXT("WaterMask")), WaterMaskTexture.GetResolvedRHITexture());
		Resources.SetTextureUAV(EName(), EName(PE_TEXT("OutVelocityHeightFoamTex")), StateUAVViews[OutputIndex]);
		if (!DispatchPass(static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_WHIRLPOOL)
			, InCommandList, Parameters, InRenderDevice, Resources))
		{
			return FALSE;
		}
		CurrentStateIndex = OutputIndex;
		return TRUE;
	}

	BOOL32 RWaterSimulation::RecordUpdateHeightWet(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice, FLOAT InDeltaTime)
	{
		const UINT32 StateIndex = CurrentStateIndex;
		const UINT32 PreviousHeightWetIndex = CurrentHeightWetIndex;
		const UINT32 NextHeightWetIndex = 1u - PreviousHeightWetIndex;
		IRRHITexture* StateTexture = StateTextures[StateIndex].GetResolvedRHITexture();
		IRRHITexture* PreviousHeightWetTexture = HeightWetTextures[PreviousHeightWetIndex].GetResolvedRHITexture();
		IRRHITexture* NextHeightWetTexture = HeightWetTextures[NextHeightWetIndex].GetResolvedRHITexture();
		if ((!StateTexture) || (!PreviousHeightWetTexture) || (!NextHeightWetTexture) || (!HeightWetUAVViews[NextHeightWetIndex]))
		{
			return FALSE;
		}
		TransitionTexture(InCommandList, StateTexture, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		TransitionTexture(InCommandList, PreviousHeightWetTexture, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
		TransitionTexture(InCommandList, NextHeightWetTexture, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);

		const FWaterSimulationParameters Parameters = BuildParameters(InDeltaTime);
		EShaderParameterSet Resources;
		Resources.SetTexture(EName(), EName(PE_TEXT("PreviousHeightWetTex")), PreviousHeightWetTexture);
		Resources.SetTexture(EName(), EName(PE_TEXT("VelocityHeightFoamTex")), StateTexture);
		Resources.SetTexture(EName(), EName(PE_TEXT("GroundTex")), GroundTexture.GetResolvedRHITexture());
		Resources.SetTextureUAV(EName(), EName(PE_TEXT("OutHeightWetTex")), HeightWetUAVViews[NextHeightWetIndex]);
		if (!DispatchPass(static_cast<UINT32>(EWaterSimulationPassType::WATER_SIMULATION_PASS_UPDATE_HEIGHT_WET)
			, InCommandList, Parameters, InRenderDevice, Resources))
		{
			return FALSE;
		}
		CurrentHeightWetIndex = NextHeightWetIndex;
		return TRUE;
	}

	void RWaterSimulation::ApplyInteractions(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice
		, FLOAT InDeltaTime, BOOL32 InApplyPending)
	{
		const UINT32 MaxDispatches = 8u;
		UINT32 AppliedNum = 0u;
		if (InApplyPending != FALSE)
		{
			for (UINT32 Index = 0u, Num = PendingInteractions.Num<UINT32>(); (Index < Num) && (AppliedNum < MaxDispatches); Index++)
			{
				const FWaterInteractionEvent& Event = PendingInteractions[Index];
				if (Event.Type == EWaterInteractionType::WATER_INTERACTION_IMPULSE)
				{
					if (RecordModifier(InCommandList, InRenderDevice, InDeltaTime, Event))
					{
						AppliedNum++;
					}
				}
				else if (Event.Type == EWaterInteractionType::WATER_INTERACTION_WHIRLPOOL)
				{
					if (RecordWhirlpool(InCommandList, InRenderDevice, InDeltaTime, Event))
					{
						AppliedNum++;
					}
				}
			}
			PendingInteractions.Empty();
		}

		for (UINT32 Index = 0u; (Index < ActiveWhirlpools.Num<UINT32>()) && (AppliedNum < MaxDispatches);)
		{
			FWaterInteractionEvent& Event = ActiveWhirlpools[Index];
			if (RecordWhirlpool(InCommandList, InRenderDevice, InDeltaTime, Event))
			{
				AppliedNum++;
			}
			Event.Duration -= InDeltaTime;
			if (Event.Duration <= 0.f)
			{
				ActiveWhirlpools.RemoveAt(Index);
				continue;
			}
			Index++;
		}
	}

}	// namespace PigeonEngine
