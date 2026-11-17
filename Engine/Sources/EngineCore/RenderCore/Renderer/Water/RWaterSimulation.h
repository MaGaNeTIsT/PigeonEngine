#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderResource.h>
#include <Shader/ShaderEngineParameters.h>
#include <PigeonBase/Object/Component/Water/WaterSimulationTypes.h>

namespace PigeonEngine
{

	class EShaderInstance;
	class EShaderParameterSet;
	class EShaderProgram;
	class IRCommandList;
	class IRRHIDevice;
	class IRRHIPipelineState;
	class IRRHITextureUAVView;
	class RSceneRenderer;

	/*
	* The render thread half of one water simulation body.
	*
	* It owns the four GPU resource sets the SWE passes share: two packed state textures,
	* two height/wet textures, the ground texture and the white water mask. Every pass is a
	* full grid dispatch that reads one state texture as an SRV and writes the other as a
	* UAV; the two indices are swapped after each pass, so no dispatch reads and writes the
	* same resource.
	*
	* The simulation is not a drawable. RWaterSimulationSceneProxy owns one of these, and
	* RSceneRenderer runs it once per frame from the simulation stage.
	*/
	class RWaterSimulation
	{
	public:
		RWaterSimulation() noexcept;
		~RWaterSimulation() noexcept;

		RWaterSimulation(const RWaterSimulation&) = delete;
		RWaterSimulation& operator=(const RWaterSimulation&) = delete;

	public:
		// Installs a reported description. Returns TRUE when the resolution or the ground
		// changed and the textures have to be rebuilt on the next Record call.
		BOOL32	ApplyDescription(const FWaterSimulationDescription& InDescription);
		void	RequestReset() noexcept;

		// Records this frame's simulation steps. FALSE when the programs or the resources are
		// not ready yet; the caller keeps the previous frame's outputs in that case.
		BOOL32	Record(IRCommandList* InCommandList, FLOAT InFrameDeltaTime, RSceneRenderer& InRenderer);

	public:
		PE_NODISCARD BOOL32				IsReady() const noexcept { return bResourcesReady; }
		PE_NODISCARD const IRRHITexture*	GetStateTexture() const noexcept;
		PE_NODISCARD const IRRHITexture*	GetHeightWetTexture() const noexcept;
		PE_NODISCARD const IRRHITexture*	GetGroundTexture() const noexcept;
		PE_NODISCARD FLOAT			GetReferenceY() const noexcept { return Description.ReferenceY; }
		PE_NODISCARD FLOAT			GetAreaLocationX() const noexcept { return Description.AreaLocationX; }
		PE_NODISCARD FLOAT			GetAreaLocationZ() const noexcept { return Description.AreaLocationZ; }
		PE_NODISCARD FLOAT			GetAreaSizeX() const noexcept { return Description.AreaSizeX; }
		PE_NODISCARD FLOAT			GetAreaSizeZ() const noexcept { return Description.AreaSizeZ; }
		PE_NODISCARD UINT32			GetResolutionX() const noexcept { return Description.ResolutionX; }
		PE_NODISCARD UINT32			GetResolutionZ() const noexcept { return Description.ResolutionZ; }
		PE_NODISCARD FLOAT			GetSimulationTime() const noexcept { return SimulationTime; }

	private:
		enum EWaterSimulationPassType : UINT8
		{
			WATER_SIMULATION_PASS_INITIALIZE = 0,
			WATER_SIMULATION_PASS_INTEGRATE_HEIGHT,
			WATER_SIMULATION_PASS_INTEGRATE_VELOCITY,
			WATER_SIMULATION_PASS_WAVE,
			WATER_SIMULATION_PASS_MODIFIER,
			WATER_SIMULATION_PASS_WHIRLPOOL,
			WATER_SIMULATION_PASS_UPDATE_HEIGHT_WET,
			WATER_SIMULATION_PASS_COUNT
		};

		struct FWaterSimulationPass
		{
		public:
			const EShaderInstance*	Instance		= nullptr;
			const EShaderProgram*	Program			= nullptr;
			IRRHIPipelineState*		PipelineState	= nullptr;
			RShaderParameterBlock	Parameters{ EName(PE_TEXT("SWEParams")) };
		};

	private:
		BOOL32	EnsurePasses(IRRHIDevice* InRenderDevice, RSceneRenderer& InRenderer);
		BOOL32	EnsureResources(IRRHIDevice* InRenderDevice);
		BOOL32	EnsureUAVViews(IRRHIDevice* InRenderDevice);
		void	ReleaseResources(IRRHIDevice* InRenderDevice);
		void	WriteParameters(const UINT32 InPassIndex, const FWaterSimulationParameters& InParameters, IRRHIDevice* InRenderDevice);
		PE_NODISCARD FWaterSimulationParameters	BuildParameters(FLOAT InDeltaTime) const;
		BOOL32	DispatchPass(const UINT32 InPassIndex, IRCommandList* InCommandList, const FWaterSimulationParameters& InParameters
			, IRRHIDevice* InRenderDevice, EShaderParameterSet& InResources);

		BOOL32	RecordInitialize(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice);
		BOOL32	RecordIntegrateHeight(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice, FLOAT InDeltaTime);
		BOOL32	RecordIntegrateVelocity(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice, FLOAT InDeltaTime);
		BOOL32	RecordWave(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice, FLOAT InDeltaTime);
		BOOL32	RecordModifier(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice, FLOAT InDeltaTime, const FWaterInteractionEvent& InEvent);
		BOOL32	RecordWhirlpool(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice, FLOAT InDeltaTime, const FWaterInteractionEvent& InEvent);
		BOOL32	RecordUpdateHeightWet(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice, FLOAT InDeltaTime);

		void	ApplyInteractions(IRCommandList* InCommandList, IRRHIDevice* InRenderDevice, FLOAT InDeltaTime, BOOL32 InApplyPending);

	private:
		TUniquePtr<FWaterSimulationPass>	Passes[EWaterSimulationPassType::WATER_SIMULATION_PASS_COUNT];
		// State and height/wet are created through RRenderTexture2D, not RTexture2DResource:
		// the D3D11 backend returns a different wrapper for a UAV-flagged texture, and only
		// RRenderTexture2D's unwrap path handles it.
		RRenderTexture2D					StateTextures[2];
		RRenderTexture2D					HeightWetTextures[2];
		RTexture2DResource					GroundTexture;
		RTexture2DResource					WaterMaskTexture;
		IRRHITextureUAVView*				StateUAVViews[2] = {};
		IRRHITextureUAVView*				HeightWetUAVViews[2] = {};
		FWaterSimulationDescription			Description;
		TArray<FWaterInteractionEvent>		PendingInteractions;
		TArray<FWaterInteractionEvent>		ActiveWhirlpools;
		FLOAT								Accumulator = 0.f;
		FLOAT								SimulationTime = 0.f;
		UINT32								CurrentStateIndex = 0u;
		UINT32								CurrentHeightWetIndex = 0u;
		UINT32								LastGroundRevision = 0u;
		BOOL32								bResourcesReady = FALSE;
		BOOL32								bResetRequested = FALSE;
		BOOL32								bGroundDirty = FALSE;
	};

}	// namespace PigeonEngine
