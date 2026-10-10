#pragma once

#include <CoreMinimal.h>
#include "PrimitiveComponent.h"

namespace PigeonEngine
{

	class EShaderInstance;
	class RFluidWaterSceneProxy;

	class PFluidWaterComponent : public PPrimitiveComponent
	{

		CLASS_VIRTUAL_NOCOPY_BODY(PFluidWaterComponent)

	public:
		// The water is one component, one surface, so it holds a single instance rather than a
		// slot array: the primitive layer stores no shader asset of its own, and the mesh layer
		// stores slots, which do not fit here.
		PE_NODISCARD const EShaderInstance*	GetMaterialInstance()const;
		void								SetMaterialInstance(const EShaderInstance* InMaterialInstance);

	protected:
		const EShaderInstance*	MaterialAsset = nullptr;

	protected:
		// Render proxy functions START
	public:
		enum PFluidWaterUpdateState : UINT8
		{
			FLUID_WATER_UPDATE_STATE_NONE     = 0,
			FLUID_WATER_UPDATE_STATE_MATRIX   = (1 << 0),
			FLUID_WATER_UPDATE_STATE_MATERIAL = (1 << 1)
		};
	public:
		RFluidWaterSceneProxy*  SceneProxy;
	public:
		UINT8                   GetUpdateRenderState()const;
		RFluidWaterSceneProxy*  CreateSceneProxy();
		virtual void            CreateRenderState()override;
		virtual void            DestroyRenderState()override;
		virtual void            SendUpdateRenderState()override;
	protected:
		void                    MarkAsDirty(PFluidWaterUpdateState InState);
		virtual void            MarkRenderTransformAsDirty()override;
		virtual void            MarkRenderStateAsDirty()override;
		virtual void            CleanMarkRenderStateDirty()override;
	protected:
		UINT8                   UpdateState;
		// Render proxy functions END
	};

};
