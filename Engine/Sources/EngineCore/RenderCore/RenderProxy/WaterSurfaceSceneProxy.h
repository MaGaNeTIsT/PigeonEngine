#pragma once

#include <CoreMinimal.h>
#include "StaticMeshSceneProxy.h"
#include <Shader/ShaderEngineParameters.h>

namespace PigeonEngine
{

	class EShaderProgram;
	class RWaterSimulationSceneProxy;

	/*
	* A water surface drawn with a user material.
	*
	* The geometry is a normal static mesh; the material's vertex function displaces it along
	* world Y from the simulation's height/wet texture. This proxy adds the part a static mesh
	* does not have: it finds the water simulation body by identity, binds the simulation's
	* output textures next to the material's own resources, and restricts the primitive to the
	* WaterSurface pass.
	*/
	class RWaterSurfaceSceneProxy : public RStaticMeshSceneProxy
	{
	public:
		virtual BOOL32	ShouldCollectInView(const RViewProxy& InView) const override;
		virtual BOOL32	ShouldDrawInPass(const FRenderPassDesc& InPassDesc) const override;
		virtual BOOL32	GetWorldBound(Vector3& OutMin, Vector3& OutMax) const override;
		virtual void	ApplyAssets(const FPrimitiveRenderAssetReport& InAssets) override;
		virtual void	ApplyValues(const FShaderValueReport& InValues) override;

	protected:
		virtual BOOL32	BindExtraResources(IRCommandList* InCommandList, const FPrimitivePassData& InData) const override;
		virtual BOOL32	BindPassResources(IRCommandList* InCommandList, const FPassRecordContext& InContext
			, const FPrimitivePassData& InData) const override;
		virtual BOOL32	DrawGeometry(IRCommandList* InCommandList, const FPrimitivePassData& InData
			, const UINT32 InIndexCount, const UINT32 InStartIndex) const override;

	private:
		PE_NODISCARD const RWaterSimulationSceneProxy*	ResolveSimulationProxy() const;

	private:
		ObjectIdentityType		SimulationIdentity		= 0ull;
		const EShaderProgram*	WaterProgram			= nullptr;
		RShaderParameterBlock	WaterSurfaceParameters{ EName(PE_TEXT("WaterSurfaceParams")) };
		FLOAT					WaterHeightScale		= 1.f;
		FLOAT					WaterMaxHeight			= 1000.f;
		// BindExtraResources decides whether the simulation outputs could be bound; DrawGeometry
		// reads the answer on the same single threaded tail list.
		mutable BOOL32			bWaterDrawValid			= TRUE;

		RENDER_PROXY_CLASS_BODY(RWaterSurfaceSceneProxy)

	};

}	// namespace PigeonEngine
