#pragma once

#include <CoreMinimal.h>
#include "PrimitiveSceneProxy.h"
#include <Renderer/Water/RWaterSimulation.h>

namespace PigeonEngine
{

	/*
	* The render thread proxy of one water simulation body.
	*
	* It submits no draw and it is never collected into a view: it exists in the scene's
	* primitive list only so the renderer's simulation stage can find it. The actual GPU
	* work lives in RWaterSimulation, which this proxy owns.
	*/
	class RWaterSimulationSceneProxy : public RPrimitiveSceneProxy
	{
	public:
		virtual BOOL32	IsRenderValid() const override;
		virtual BOOL32	ShouldCollectInView(const RViewProxy& InView) const override;
		virtual BOOL32	ShouldDrawInPass(const FRenderPassDesc& InPassDesc) const override;
		virtual void	ApplyRuntimePayload(const FPrimitiveRenderSnapshot& InSnapshot) override;
		virtual BOOL32	RecordCompute(IRCommandList* InCommandList, const FRenderPassDesc& InPassDesc) const override;

	public:
		PE_NODISCARD const RWaterSimulation&	GetSimulation() const noexcept { return Simulation; }
		PE_NODISCARD RWaterSimulation&			GetSimulation() noexcept { return Simulation; }

	private:
		// RecordCompute is const by contract; the simulation's GPU state is per frame mutable
		// state, the same way a parameter block is on the primitive base.
		mutable RWaterSimulation Simulation;

		RENDER_PROXY_CLASS_BODY(RWaterSimulationSceneProxy)

	};

}	// namespace PigeonEngine
