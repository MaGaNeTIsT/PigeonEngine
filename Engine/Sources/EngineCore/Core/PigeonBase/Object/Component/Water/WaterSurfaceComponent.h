#pragma once

#include <CoreMinimal.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>

namespace PigeonEngine
{

	class PWaterSimulationComponent;

	/*
	* A water surface drawn from a static mesh and a water material.
	*
	* The surface is deliberately thin: the mesh supplies the geometry the material
	* displaces along world Y, and the material samples the simulation's height/wet and
	* state textures. The component only names the simulation body it reads and the two
	* surface side scalars the material may use.
	*
	* PWaterSurfaceComponent reports itself under the WaterSurface pass as well as the
	* primary pass. RWaterSurfaceSceneProxy only allows the WaterSurface pass, so the
	* base and shadow passes never try to draw the translucent material with their own
	* fixed function state.
	*/
	class PWaterSurfaceComponent : public PStaticMeshComponent
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PWaterSurfaceComponent)

	public:
		void	SetSimulation(PWaterSimulationComponent* InSimulation);
		PE_NODISCARD PWaterSimulationComponent*	GetSimulation() const noexcept { return Simulation; }
		void	SetWaterHeightScale(const FLOAT InHeightScale);
		void	SetWaterMaxHeight(const FLOAT InMaxHeight);
		PE_NODISCARD FLOAT	GetWaterHeightScale() const noexcept { return WaterHeightScale; }
		PE_NODISCARD FLOAT	GetWaterMaxHeight() const noexcept { return WaterMaxHeight; }

	public:
		virtual const EName&	GetRenderProxyTypeName() const override;
		virtual void			CollectRenderAssets(FPrimitiveRenderAssetReport& InOutReport) const override;
		virtual void			CollectRenderValues(FShaderValueReport& InOutValues) const override;

	protected:
		// The simulation's area, clock and readiness change without the surface component
		// changing, so the component refreshes its values channel in the render-state tick,
		// which runs for every added component whether or not the actor is tickable.
		virtual void			TickRenderImpl() override;

	private:
		PWaterSimulationComponent*	Simulation			= nullptr;
		FLOAT						WaterHeightScale	= 1.f;
		FLOAT						WaterMaxHeight		= 1000.f;

	};

}	// namespace PigeonEngine
