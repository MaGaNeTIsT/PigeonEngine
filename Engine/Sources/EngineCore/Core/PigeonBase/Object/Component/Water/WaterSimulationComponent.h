#pragma once

#include <CoreMinimal.h>
#include <PigeonBase/Object/Component/Primitive/PrimitiveComponent.h>
#include <PigeonBase/Object/Component/Water/WaterSimulationTypes.h>

namespace PigeonEngine
{

	/*
	* One water simulation body in the world.
	*
	* The component is the game thread authoring surface: it owns the simulation plane in
	* world XZ, the reference plane's world Y, the resolution, the ground and the tunables.
	* It does not tick the simulation; it reports a description to RWaterSimulationSceneProxy,
	* which runs the SWE passes on the render thread.
	*
	* The component is a primitive so it travels through the generic snapshot / proxy factory
	* path, but it never draws: its proxy is skipped by view collection and by every pass.
	*
	* The component's world position is the centre of the simulation plane and its world Y is
	* the reference plane. AreaLocationX/Z is derived as centre minus half the area size.
	*/
	class PWaterSimulationComponent : public PPrimitiveComponent
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PWaterSimulationComponent)

	public:
		// Simulation domain.
		void	SetResolution(const UINT32 InResolutionX, const UINT32 InResolutionZ);
		void	SetAreaSize(const FLOAT InAreaSizeX, const FLOAT InAreaSizeZ);
		void	SetInitialDepth(const FLOAT InInitialDepth);
		PE_NODISCARD UINT32	GetResolutionX() const noexcept { return ResolutionX; }
		PE_NODISCARD UINT32	GetResolutionZ() const noexcept { return ResolutionZ; }
		PE_NODISCARD FLOAT	GetAreaSizeX() const noexcept { return AreaSizeX; }
		PE_NODISCARD FLOAT	GetAreaSizeZ() const noexcept { return AreaSizeZ; }
		PE_NODISCARD FLOAT	GetInitialDepth() const noexcept { return InitialDepth; }

	public:
		// Ground.
		void	SetGroundFlat();
		void	SetGroundHeightMap(const TArray<FLOAT>& InRelativeDepths);
		PE_NODISCARD EWaterGroundType	GetGroundType() const noexcept { return GroundType; }

	public:
		// Tunables and time step.
		void	SetSimulationParameters(const FWaterSimulationParameters& InParameters);
		PE_NODISCARD const FWaterSimulationParameters&	GetSimulationParameters() const noexcept { return SimulationParameters; }
		void	SetWaveEnabled(const BOOL32 InIsWaveEnabled);
		void	SetTimeStep(const FLOAT InFixedTimeStep, const UINT32 InMaxSubsteps, const FLOAT InTimeScale);

	public:
		// Interactions. AddImpulse is one shot; AddWhirlpool keeps running for Duration seconds.
		void	AddImpulse(const FWaterImpulseDescription& InImpulse);
		void	AddWhirlpool(const FWaterWhirlpoolDescription& InWhirlpool);
		void	ResetSimulation();

	public:
		virtual const EName&	GetRenderProxyTypeName() const override;
		virtual BOOL32			BuildRenderSnapshot(FPrimitiveRenderSnapshot& OutSnapshot) const override;
		virtual void			CreateRenderState() override;
		virtual void			DestroyRenderState() override;
		virtual void			SendUpdateRenderState() override;

	private:
		UINT32					ResolutionX			= 256u;
		UINT32					ResolutionZ			= 256u;
		FLOAT					AreaSizeX			= 1000.f;
		FLOAT					AreaSizeZ			= 1000.f;
		FLOAT					InitialDepth		= 10.f;
		EWaterGroundType		GroundType			= EWaterGroundType::WATER_GROUND_FLAT;
		TSharedPtr<TArray<FLOAT>>	GroundRelativeDepths;
		UINT32					GroundRevision		= 0u;
		BOOL32					bEnableWave			= FALSE;
		FLOAT					FixedTimeStep		= 1.f / 60.f;
		UINT32					MaxSubsteps			= 3u;
		FLOAT					TimeScale			= 1.f;
		FWaterSimulationParameters	SimulationParameters;
		// The snapshot consumes these: BuildRenderSnapshot is const, so the members that the
		// hand-off clears are mutable rather than const_cast.
		mutable TArray<FWaterInteractionEvent>	PendingInteractions;
		mutable BOOL32			bResetRequested		= FALSE;

	};

}	// namespace PigeonEngine
